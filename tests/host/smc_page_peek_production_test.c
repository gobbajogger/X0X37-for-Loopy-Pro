/* SPDX-License-Identifier: GPL-3.0-only */
/* Test the actual extracted SMC production popup functions, not a replica. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BOARD_SMK37 0
#define NKIT 2
#define NPARTS 5
#define O_NONE 0
#define O_ASK 2
#define V_HOME 1
#define V_MIX 2
#define V_FX 3
#define V_SEQ 4
#define V_PART 5
#define R_ENG 1
#define T_MIX 1
#define T_FX 2
#define T_MST 3
#define MX_LEVEL 1
#define MX_CUT 2
#define MX_PAN 3
#define MX_REV 4
#define MX_DLY 5
#define FX_RV_LEVEL 1
#define FX_DL_LEVEL 2
#define FX_RV_DECAY 3
#define FX_DL_FDBK 4
#define MST_DJF 1
#define MST_RESO 2
#include "app/smc_page_peek.h"
typedef struct {int kind,a,b,c;} pref_t;
static struct {int view,prev_view,mixsel,help,overlay;} ui;
static struct {int n;} pg;
static struct {int locked;} smc;
static uint8_t pages[10];
static uint32_t ticks;
static int dirty_hashes;
static uint32_t blit_hash[4];
static smc_page_peek_t smc_peek;
static int project_busy,performance,clip_grid,drum_mode,overlay_count,cancel_count;
static int cur_page(void) {return pages[ui.view];}
static uint8_t *page_of(int view) {return &pages[view];}
static void build_pages(void) {
    pg.n=ui.view==V_MIX?4:ui.view==V_FX?2:3;
    if (cur_page()>=pg.n) pages[ui.view]=0;
    dirty_hashes++;
}
static int part_at(int n) {const int order[5]={0,1,4,2,3};return order[n];}
static uint32_t plat_ms(void) {return ticks;}
static int ui_project_busy(void) {return project_busy;}
static int perf_testing(void) {return performance;}
static int smk_clip_mode_on(void) {return clip_grid;}
static int smc_drum_mix(void) {return drum_mode?-1+2:-1;}
static int smc_page_now(void) {return cur_page();}
static int smc_same(pref_t a,pref_t b) {return a.kind==b.kind && a.a==b.a && a.b==b.b && a.c==b.c;}
static pref_t smc_home_target(int n) {
    if (n<NPARTS) return (pref_t){R_ENG,T_MIX,part_at(n),MX_LEVEL};
    if (n==5) return (pref_t){R_ENG,T_FX,0,FX_RV_LEVEL};
    if (n==6) return (pref_t){R_ENG,T_FX,0,FX_DL_LEVEL};
    return (pref_t){R_ENG,T_MST,0,MST_DJF};
}
static void smc_ui_cancel(void) {cancel_count++;}
static void smc_ui_begin(int n,int knob) {(void)n;(void)knob;overlay_count++;}
static void smc_peek_cancel(void) {smc_page_peek_cancel(&smc_peek);}
#include <smc_page_peek_production.inc>

static int count;
#define CHECK(x) do {count++;if(!(x)) {fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static void reset(int view,int page,int selected,int previous) {
    memset(&smc_peek,0,sizeof smc_peek);
    memset(&ui,0,sizeof ui);
    memset(&smc,0,sizeof smc);
    memset(pages,0,sizeof pages);
    ui.view=view;ui.mixsel=selected;ui.prev_view=previous;
    pages[view]=(uint8_t)page;
    ticks=100;project_busy=performance=clip_grid=drum_mode=0;
    overlay_count=cancel_count=dirty_hashes=0;
    build_pages();
}
int main(void) {
    reset(V_SEQ,2,3,V_HOME);
    smc_show(0,0,smc_home_target(0));
    CHECK(smc_peek.on&&ui.view==V_MIX&&cur_page()==0&&ui.mixsel==0);
    CHECK(smc_peek.saved.view==V_SEQ&&smc_peek.saved.page==2&&smc_peek.saved.mixsel==3);
    CHECK(ui.prev_view==V_HOME&&overlay_count==0&&dirty_hashes==2);
    ticks+=999;smc_show(2,0,smc_home_target(2));
    CHECK(smc_peek.on&&ui.mixsel==4&&smc_peek.saved.mixsel==3);
    ticks+=1199;smc_peek_tick();CHECK(smc_peek.on);
    ticks++;smc_peek_tick();
    CHECK(!smc_peek.on&&ui.view==V_SEQ&&cur_page()==2&&ui.mixsel==3&&ui.prev_view==V_HOME);

    reset(V_SEQ,1,1,V_HOME);smc_show(5,0,smc_home_target(5));
    CHECK(ui.view==V_FX&&cur_page()==0);
    ticks+=20;smc_show(6,0,smc_home_target(6));
    CHECK(ui.view==V_FX&&cur_page()==1);
    ticks+=20;smc_show(7,0,smc_home_target(7));
    CHECK(ui.view==V_MIX&&cur_page()==1+NKIT);
    CHECK(smc_peek.saved.view==V_SEQ&&smc_peek.saved.page==1);
    smc_peek_finish();CHECK(ui.view==V_SEQ&&cur_page()==1&&!smc_peek.on);

    reset(V_HOME,0,2,V_PART);
    smc_show(2,1,(pref_t){R_ENG,T_MIX,part_at(2),MX_CUT});
    CHECK(ui.view==V_MIX&&cur_page()==0&&ui.mixsel==4);
    smc_peek_finish();CHECK(ui.view==V_HOME&&ui.mixsel==2);

    reset(V_SEQ,0,2,V_HOME);
    smc_show(0,1,(pref_t){R_ENG,T_MIX,0,MX_CUT});
    CHECK(!smc_peek.on&&overlay_count==1&&ui.view==V_SEQ);

    reset(V_SEQ,0,2,V_HOME);ui.help=1;
    smc_show(0,0,smc_home_target(0));CHECK(!smc_peek.on&&ui.view==V_SEQ);
    ui.help=0;ui.overlay=O_ASK;
    smc_show(0,0,smc_home_target(0));CHECK(!smc_peek.on&&ui.view==V_SEQ);
    ui.overlay=O_NONE;smc.locked=1;
    smc_show(0,0,smc_home_target(0));CHECK(!smc_peek.on&&ui.view==V_SEQ);
    smc.locked=0;clip_grid=1;
    smc_show(0,0,smc_home_target(0));CHECK(!smc_peek.on&&ui.view==V_SEQ);
    clip_grid=0;performance=1;
    smc_show(0,0,smc_home_target(0));CHECK(!smc_peek.on&&ui.view==V_SEQ);
    performance=0;drum_mode=1;
    smc_show(0,0,smc_home_target(0));CHECK(!smc_peek.on&&ui.view==V_SEQ);

    reset(V_SEQ,2,3,V_HOME);smc_show(0,0,smc_home_target(0));
    ui.view=V_PART;pages[V_PART]=1;smc_peek_tick();
    CHECK(!smc_peek.on&&ui.view==V_PART&&cur_page()==1);
    ticks+=2000;smc_peek_tick();CHECK(ui.view==V_PART);

    reset(V_SEQ,2,3,V_HOME);smc_show(0,0,smc_home_target(0));
    ui.help=1;smc_peek_tick();CHECK(!smc_peek.on&&ui.view==V_SEQ&&ui.mixsel==3);

    reset(V_SEQ,2,3,V_HOME);smc_show(0,0,smc_home_target(0));
    project_busy=1;smc_peek_tick();
    CHECK(!smc_peek.on&&ui.view==V_MIX);

    reset(V_MIX,0,1,V_HOME);smc_show(1,0,smc_home_target(1));
    CHECK(!smc_peek.on&&ui.view==V_MIX&&overlay_count==0);
    puts("PASS: extracted native SMC mapped-page routing and restore checks");
    printf("assertions=%d, failures=0\n",count);
    return 0;
}
