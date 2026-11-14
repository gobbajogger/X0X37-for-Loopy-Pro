/* SPDX-License-Identifier: GPL-3.0-only */
/* Compile the exact production REC feeder, Free Play owner gate and input()
 * in a deterministic stubbed host. NOT a whole-app/audio qualification. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "rec_outer_keymap.h"
#define BOARD_SMK37 1
#define X0X_HOST 1
#define X0X_DEV_PADCAL 0
#define NBASS 2
#define B_HOME 1
#define B_SAVE 2
#define V_SEQ 1
#define V_PART 2
#define V_HOME 3
#define O_ASK 4
#define PART_303A 0
#define PART_303B 1
#define PART_909 2
#define PART_808 3
#define PART_BREAK 4
struct {
    int rec, overlay, part, view, oct;
    uint32_t btn;
    uint8_t freeplay[2];
} ui;
struct {uint8_t rec_ext_mask; int playing;} seq;
static uint64_t physical_keys;
static int project_busy, perf_active, clip_active, smc_active;
static unsigned calls_in, calls_out, misses, duplicate, central_events;
static int active_note[37], note_pitch[37], last_pitch[37];
static unsigned checks, failures, owner_before, owner_after, body_count;
#define T(expr) do { checks++; if (!(expr)) { \
  fprintf(stderr,"FAIL %u:%d: %s\n",checks,__LINE__,#expr); failures++; } } while(0)
static int perf_testing(void) {return perf_active;}
/* is_303/bidx/kbd_on and part_is_303 are extracted from ui.c below. */
static int smc_ui_active(void) {return smc_active;}
uint8_t smk_clip_mode_on(void) {return (uint8_t)clip_active;}
static uint64_t plat_physical_keys(void) {return physical_keys;}
static unsigned live_token(unsigned physical);
int ui_live_key_owner(void);
static void bass_kbd_physical(int p, int pitch)
{
    if (p<0 || p>=37 || (p>=5 && p<=31)) central_events++;
    if (active_note[p]) duplicate++;
    active_note[p]=1;note_pitch[p]=last_pitch[p]=pitch;calls_in++;
}
static int bass_kbd_release(int identity)
{
    for (int p=0;p<37;p++) if (active_note[p] && (int)live_token((unsigned)p)==identity) {
        active_note[p]=0;calls_out++;return 1;
    }
    misses++;return 0;
}
static void live_boundary(int step)
{
    if (step) owner_before=(unsigned)ui_live_key_owner();
    else owner_after=(unsigned)ui_live_key_owner();
}
static void input_body(void) {body_count++;}
/* EXACT_PRODUCTION_FUNCTIONS */
static unsigned active_count(void)
{
    unsigned n=0;for(int p=0;p<37;p++)n+=(unsigned)active_note[p];return n;
}
static void scan(void) {input();}
static void clean(void)
{
    ui.rec=0; ui.overlay=0; ui.view=V_SEQ; ui.part=PART_303A;
    ui.oct=0; ui.btn=0;ui.freeplay[0]=ui.freeplay[1]=0;
    project_busy=perf_active=clip_active=smc_active=0;
    physical_keys=0;scan();
    T(active_count()==0);T(seq.rec_ext_mask==0);
}
static void begin_rec(int part,int oct,int playing)
{
    clean();ui.part=part;ui.oct=oct;seq.playing=playing;
    ui.rec=1;scan();T(ui_live_key_owner()==0);
    T(seq.rec_ext_mask==3);
}
static void key(unsigned p,int down)
{
    uint64_t bit=UINT64_C(1)<<p;
    if(down)physical_keys|=bit;else physical_keys&=~bit;
    scan();
}
static int is_outer(unsigned p) {return p<5u || p>=32u;}
static void all_keys_and_octaves(void)
{
    for(int b=0;b<2;b++)for(int oct=-2;oct<=3;oct++)for(unsigned p=0;p<37;p++) {
        begin_rec(b,oct,(int)(p&1u));
        unsigned before=calls_in,release=calls_out;
        key(p,1);key(p,1);
        if(is_outer(p)) {
            T(calls_in==before+1);T(active_note[p]);
            T(last_pitch[p]==36+(int)p+12*oct);
            T(live_token(p)>=28 && live_token(p)<=37);
        } else {T(calls_in==before);T(!active_note[p]);}
        key(p,0);
        T(calls_out==release+(unsigned)is_outer(p));
        T(active_count()==0);
        T(owner_before==0 && owner_after==0);
    }
}
static void owner_and_freeplay(void)
{
    clean();T(ui_live_key_owner()==0); /* neither Free Play nor EDIT */
    ui.freeplay[0]=1;T(ui_live_key_owner()==1);
    ui.view=V_PART;T(ui_live_key_owner()==1);
    ui.part=PART_303B;ui.freeplay[1]=1;T(ui_live_key_owner()==2);
    ui.part=PART_909;T(ui_live_key_owner()==0);
    ui.part=PART_303A;
    ui.overlay=O_ASK;T(ui_live_key_owner()==0);ui.overlay=0;
    ui.btn=1u<<B_HOME;T(ui_live_key_owner()==0);ui.btn=0;
    ui.btn=1u<<B_SAVE;T(ui_live_key_owner()==0);ui.btn=0;
    project_busy=1;T(ui_live_key_owner()==0);project_busy=0;
    perf_active=1;T(ui_live_key_owner()==0);perf_active=0;
    clip_active=1;T(ui_live_key_owner()==0);clip_active=0;
    smc_active=1;T(ui_live_key_owner()==0);smc_active=0;
    ui.view=V_HOME;T(ui_live_key_owner()==0);
    ui.view=V_SEQ;T(ui_live_key_owner()==1);
    ui.rec=1;T(ui_live_key_owner()==0);
    scan();ui.rec=0;physical_keys=UINT64_C(1)<<36;
    unsigned prior=calls_in;scan();T(calls_in==prior);
    physical_keys=0;scan();T(active_count()==0);
}
static void suppress_at_entry_and_switches(void)
{
    clean();physical_keys=(UINT64_C(1)<<0)|(UINT64_C(1)<<32);
    scan();ui.rec=1;scan();T(active_count()==0);
    key(0,0);key(32,0);T(active_count()==0);
    unsigned before=calls_in;
    key(0,1);key(32,1);T(calls_in==before+2);T(active_count()==2);
    key(0,0);T(active_count()==1 && active_note[32]);
    ui.part=PART_909;scan();T(active_count()==0);
    unsigned after=calls_in;
    key(36,1);scan();T(calls_in==after);
    ui.part=PART_303B;scan();T(active_count()==0); /* 36 already held */
    key(36,0);key(36,1);T(active_count()==1);
    ui.part=PART_BREAK;scan();T(active_count()==0);
    clean();begin_rec(PART_303A,1,1);
    key(36,1);T(active_count()==1);
    ui.overlay=O_ASK;scan();T(active_count()==0);
    ui.overlay=0;scan();T(active_count()==0);
    key(36,0);key(36,1);T(active_count()==1);
    project_busy=1;scan();T(active_count()==0);project_busy=0;
    key(36,0);scan();key(36,1);T(active_count()==1);
    perf_active=1;scan();T(active_count()==0);perf_active=0;
    key(36,0);scan();key(36,1);T(active_count()==1);
    clip_active=1;scan();T(active_count()==0);clip_active=0;
    key(36,0);scan();key(36,1);T(active_count()==1);
    ui.rec=0;scan();T(active_count()==0);
    T(seq.rec_ext_mask==0);T(ui_live_key_owner()==0);
    clean();
}
static void sustained_pairs_and_central_noise(void)
{
    begin_rec(PART_303A,0,0);
    unsigned before=calls_in;
    key(0,1);key(36,1);T(calls_in==before+2);T(active_count()==2);
    for(unsigned p=5;p<=31;p++) {key(p,1);key(p,0);}
    T(calls_in==before+2);T(active_count()==2);
    key(36,0);T(active_count()==1 && active_note[0]);
    key(0,0);T(active_count()==0);
    ui.rec=0;scan();T(active_count()==0);
    T(misses==0);T(duplicate==0);T(central_events==0);
}
int main(void)
{
    all_keys_and_octaves(); owner_and_freeplay();
    suppress_at_entry_and_switches(); sustained_pairs_and_central_noise();
    T(calls_in==calls_out);
    T(body_count>500u);
    printf("rec37-extracted-owner checks=%u on=%u off=%u errors=%u owner-freeplay=covered\n",checks,calls_in,calls_out,failures);
    return failures?1:0;
}
