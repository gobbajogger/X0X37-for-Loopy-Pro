/* SPDX-License-Identifier: GPL-3.0-only
 * Browser bridge to the existing unchanged native host firmware simulator.
 * The host HAL, not the hardware main_fm1 entry point, is used here.
 */
#define main x0x_script_main
#include "host/x0x_host.c"
#undef main
#define CK_FN
#include "firmware/src/app/smk_clip_keys.h"
#include "firmware/src/app/smc_protocol.h"
#include "firmware/src/app/smk_apc40.h"

/* Same non-clip host substitutions as tests/host/smc_drum_mutes_test.c.
 * MIDI messages enter the production queue directly: this is NOT BLE emulation. */
uint8_t smk_clip_mode_on(void) { return 0; }
int smk_ck_smc_turn(int n, int delta) { (void)n; (void)delta; return 0; }
int smk_ck_smc_nav(uint8_t note) { (void)note; return 0; }
static uint32_t feedback_count;
int plat_ble_smc_send(const uint8_t *msg, uint8_t len) { (void)msg; (void)len; feedback_count++; return 1; }
#include "firmware/src/app/smc_mixer.c"
static void sim_run(uint32_t ms) { smc_poll(); run_ms(ms); smc_frame(); }
void sim_init(void) { load_force = 0; boot(); proj.set.bt |= PROJ_BT_SMC_CENTRAL; sim_run(32); }
int sim_smc_link(int up, int user) {
    if(up<0||up>1||user<0||user>1)return -1;
    proj.set.bt=(uint8_t)((proj.set.bt|PROJ_BT_SMC_CENTRAL)&~PROJ_BT_SMC_USER);
    if(user)proj.set.bt|=PROJ_BT_SMC_USER;
    smc_ble_link(up);sim_run(20);return 0;
}
int sim_smc_message(int status,int a,int b) {
    if(status<0x80||status>0xef||a<0||a>127||b<0||b>127)return -1;
    uint8_t msg[3]={(uint8_t)status,(uint8_t)a,(uint8_t)b};
    smc_ble_midi(msg,3);sim_run(20);return 0;
}
int sim_smc_mmc(int cmd) {
    if(cmd<1||cmd>43)return -1;
    uint8_t msg[6]={0xf0,0x7f,0x7f,0x06,(uint8_t)cmd,0xf7};
    smc_ble_midi(msg,6);sim_run(20);return 0;
}
void sim_smc_state(uint32_t *out) {
    out[0]=smc.up;out[1]=smc.picked;out[2]=smc.seen;out[3]=smc.solo;
    out[4]=smc_rx_diag();out[5]=feedback_count;
    for(int i=0;i<8;i++)out[6+i]=smc.value[i];
    for(int i=0;i<128;i++)out[14+i]=smc.lit[i];
}
int sim_input(int kind, int id, int value) {
    switch(kind) {
    case 0: if(id<0 || id>=NB || value<0 || value>1) return -1;
        if(value) held_btn |= 1u<<id; else held_btn &= ~(1u<<id); break;
    case 1: if(id<0 || id>=NKEYS || value<0 || value>127) return -1;
        if(value) {held_keys |= 1u<<id; key_velocity[id]=(uint8_t)value;} else held_keys &= ~(1u<<id); break;
    case 2: if(id<0 || id>=NE || value < -64 || value>64) return -1;
        enc_acc[id] += value; break;
    case 3: if(value<0 || value>4096) return -1; master=(uint32_t)value; break;
    case 4: if(value<0 || value>3) return -1; arrows_held=(uint32_t)value; break;
    case 5:
        held_btn=held_keys=arrows_held=0; memset(enc_acc,0,sizeof enc_acc);
        /* A browser blur/cancel releases temporary controller holds too. Use
         * production release routing, preserving the active link and preset.
         * Drop pending input first so an older queued press cannot re-latch. */
        smc_project_drop();
        for(uint8_t note=8;note<16;note++) smc_release(note);
        break;
    default:return -1;
    }
    sim_run(20); return 0;
}
int sim_advance(int ms) { if(ms<0 || ms>1000) return -1; sim_run((uint32_t)ms); return 0; }
void sim_state(uint32_t *out) {
    out[0]=now_ms; out[1]=seq.playing; out[2]=ui.view; out[3]=ui.part;
    out[4]=ui.rec; out[5]=lit_btn; out[6]=lit_keys; out[7]=dim_keys;
    out[8]=(uint32_t)(seq.bpm*10.0f+0.5f); out[9]=midi_out_count; out[10]=note_ons;
    out[11]=held_btn; out[12]=held_keys; out[13]=ui.overlay;
}
void sim_rgb(uint8_t *out) {
    for(unsigned i=0;i<240u*240u;i++) { uint16_t p=fb[i]; p=(uint16_t)((p>>8)|(p<<8));
        out[3*i]=(uint8_t)(((p>>11)&31)*255/31);out[3*i+1]=(uint8_t)(((p>>5)&63)*255/63);out[3*i+2]=(uint8_t)((p&31)*255/31); }
}
int sim_notes(int note,int scale,int root,int snap,int chord,int inv,uint8_t *out) {
    if(note<48||note>84||scale<0||scale>=24||root<0||root>11||snap<0||snap>2||chord< -1||chord>=30||inv<0||inv>3)return -1;
    return (int)ck_notes((uint32_t)note,ck_mask((uint32_t)scale,(uint32_t)root),(uint32_t)snap,chord,(uint32_t)inv,out);
}
int sim_pad(int cols,int rows,int col,int row,int view,int physical) {
    if(cols<1||cols>8||rows<1||rows>5||col<0||col>7||row<0||row>4||view<0||view>2||physical<0||physical>15)return -1;
    return smk_apc40_note_for_pad_view(cols,rows,col,row,view,smk_apc40_logical_pad_for_physical(physical));
}
int sim_relative(int value) {return value>=0&&value<128 ? smc_relative(value) : -999;}
