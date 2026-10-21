/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual SMC queue and UI dispatch; hardware I/O uses the existing host fixture. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
#define CK_FN __attribute__((unused))
#include "app/smk_clip_keys.h"
static struct { uint8_t pg; } ck;
static uint8_t clip_grid_mode, fields[CK_NE];
static uint32_t ck_get(uint32_t f) { return fields[f]; }
static void ck_set(uint32_t f, uint32_t v) { fields[f] = v; }
static const char *ck_val_txt(uint32_t f, uint32_t v, char *s) { (void)f; sprintf(s,"%u",v); return s; }
#include "clip_smc.inc"
uint8_t smk_clip_mode_on(void) { return clip_grid_mode; }
int plat_ble_smc_send(const uint8_t *msg, uint8_t len) { (void)msg; (void)len; return 1; }
#ifndef SMC_SOURCE_FILE
#define SMC_SOURCE_FILE "app/smc_mixer.c"
#endif
#include SMC_SOURCE_FILE
#include "ble/smc_host_ingress.h"
static int queue_smc(void *ctx, const uint8_t *p, uint8_t n)
{
    uint32_t before=smc_enqueued;
    (void)ctx; smc_ble_midi(p,n);
    return smc_enqueued != before;
}
static void incoming(smc_host_ingress_t *s,const uint8_t *p,unsigned n)
{
    uint8_t packet[16]={0x80,0x81};
    assert(n+2<=sizeof packet);memcpy(packet+2,p,n);
    uint8_t host_w=smc_host_w;
    assert(smc_hi_notification(s,packet,n+2,queue_smc,0)==SMC_HI_OK);
    memset(packet,0xDD,sizeof packet); /* queue must own bytes before UI poll */
    assert(smc_host_w==host_w); /* peripheral/USB SMC ingress untouched */
    smc_poll();smc_frame();
}
int main(void)
{
    smc_host_ingress_t s={0};
    clip_grid_mode=0;boot();
    proj.set.bt=PROJ_BT_SMC_CENTRAL;smc_ble_link(1);smc_poll();smc_hi_link(&s,1);
    const uint8_t select[]={0x90,24,127};incoming(&s,select,sizeof select);
    assert(ui.view==V_PART && ui.part==0);
    pref_t r=smc_knob_target(0);const x0x_param_t *d=pref_desc(r);assert(d&&d->max);
    int old=pref_get(r);uint8_t knob[]={0xB0,16,old==d->max?0x41:1};
    incoming(&s,knob,sizeof knob);assert(pref_get(r)!=old);
    const uint8_t fader_zero[]={0xE0,0,0};incoming(&s,fader_zero,sizeof fader_zero);
    assert(!strncmp(ui.msg,"PICK F1",7)||!strncmp(ui.msg,"SMC F1",6));
    /* Partial SysEx cannot contaminate the next link's actual SELECT/encoder. */
    const uint8_t partial[]={0x80,0x81,0xF0,0x7F};
    assert(smc_hi_notification(&s,partial,sizeof partial,queue_smc,0)==SMC_HI_OK);
    smc_hi_link(&s,0);smc_ble_link(0);smc_poll();
    smc_hi_link(&s,1);smc_ble_link(1);smc_poll();
    incoming(&s,select,sizeof select);assert(ui.view==V_PART&&ui.part==0);
    /* Stock six-byte MMC still reaches the real User-preset mapping. */
    proj.set.bt=PROJ_BT_SMC_CENTRAL|PROJ_BT_SMC_USER;
    const uint8_t mmc[]={0xF0,0x7F,0x7F,6,8,0xF7}; /* command is selected from the production mapping below */
    uint8_t command[6];memcpy(command,mmc,6);
    unsigned cmd;for(cmd=1;cmd<=43&&smc_mmc_note(cmd)!=25;cmd++);assert(cmd<=43);
    command[4]=(uint8_t)cmd;incoming(&s,command,sizeof command);
    assert(ui.view==V_PART&&ui.part==1);
    /* Count real queue saturation; callback success is not fabricated. */
    smc_qr=smc_qw;unsigned before=s.dropped;
    const uint8_t note_packet[]={0x80,0x81,0x90,24,127};
    for(unsigned i=0;i<SMC_Q+1;i++)
        assert(smc_hi_notification(&s,note_packet,sizeof note_packet,queue_smc,0)==SMC_HI_OK);
    assert(s.dropped==before+1);smc_poll();
    puts("PASS vendor parser -> copied SMC queue -> SELECT/knob/fader/MMC UI dispatch; no peripheral queue");
    return 0;
}
