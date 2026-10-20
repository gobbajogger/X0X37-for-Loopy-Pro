/* Full project, engine, UI, SMC queues and pickup logic. Hardware I/O and clip
 * field storage alone are substitutes; clip navigation/turn code is extracted. */
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
static int route;
static void message(int st, int a, int b) {
    uint8_t msg[3] = {st,a,b};
    now_ms += 100;
    if(route && (st & 0xF0)==0xE0) { msg[0]=0xBF; msg[1]=40+(st&7); }
    if(route && (st & 0xF0)==0xB0) msg[0]=0xBF;
    if(route && (st==0x90 || st==0x80)) {
        if(st==0x80) return;            /* stock MMC buttons have no release */
        int cmd; for(cmd=1;cmd<=43 && smc_mmc_note(cmd)!=a;cmd++);
        assert(cmd<=43);
        if(route==2) smc_mmc_host(cmd,0);
        else { uint8_t mmc[6]={0xF0,0x7F,0x7F,6,cmd,0xF7}; smc_ble_midi(mmc,6); }
    } else if(route==2)
        assert(smc_midi_host(0x0B | (uint32_t)msg[0]<<8 | (uint32_t)msg[1]<<16 | (uint32_t)msg[2]<<24));
    else smc_ble_midi(msg,3);
    smc_poll(); smc_frame();
}
static void press(int note) { message(0x90,note,127); message(0x80,note,0); }
static void turn(int n, int delta) { message(0xB0,16+n,delta>0?delta:64-delta); }
static void fader(int n, int v) { message(0xE0+n,0,v); }
static void view(int v) {
    for(int i=0;ui.view!=v && i<8;i++) press(0x61);
    assert(ui.view==v);
}
static void edit(pref_t r, int n) {
    const x0x_param_t *d=pref_desc(r); assert(d && d->max);
    int old=pref_get(r), delta=old==d->max?-1:1;
    turn(n,delta); assert(pref_get(r)!=old);
}
static void scenario(void) {
    clip_grid_mode=0; ck.pg=0; memset(fields,0,sizeof fields); memset(&smc,0,sizeof smc);
    memset(&ui,0,sizeof ui); boot(); proj.set.bt=PROJ_BT_SMC_CENTRAL | (route?PROJ_BT_SMC_USER:0); smc_ble_link(1); smc_poll();
    if(getenv("SMC_PICKUP_ONLY")) goto pickup;
    /* Starting at HOME, SELECT must expose each part's sound. */
    for(int part=0;part<NPARTS;part++) {
        smc_view(V_HOME,0); press(24+part); assert(ui.part==part && ui.view==V_PART);
        for(int p=0;p<pg.n;p++) {
            assert(cur_page()==p);
            for(int n=0;n<4;n++) { pref_t r=smc_knob_target(n); if(r.kind!=R_NONE) edit(r,n); }
            for(int n=4;n<8;n++) assert(smc_knob_target(n).kind==R_NONE);
            if(p+1<pg.n) press(0x63);
        }
        /* Open deep list and reach every non-header row by SMC Up/Down. */
        press(5); assert(ui.overlay==O_LIST);
        for(int n=0;n<6 && n<ui.list_n;n++) {
            pref_t row=list_rows[ui.list_top+n];
            if(row.kind==R_HDR || row.kind==R_ACT) {
                int selected=ui.list_sel; turn(n,1);
                assert(ui.overlay==O_LIST && ui.list_sel==selected);
            }
        }
        int previous=-1;
        for(int i=0;i<LIST_MAX && ui.list_sel!=previous;i++) {
            previous=ui.list_sel; pref_t r=list_rows[previous];
            const x0x_param_t *d=pref_desc(r);
            if(d && d->max && r.kind!=R_ACT && r.kind!=R_HDR) {
                int old=pref_get(r); press(old==d->max?0x62:0x63); assert(pref_get(r)!=old);
            }
            press(0x61);
        }
        assert(ui.list_sel==ui.list_n-1); press(6); assert(ui.overlay==O_NONE);
    }
    /* Both kits: all eleven tracks, sound pages then mix pan and sends. */
    for(int kit=0;kit<2;kit++) {
        view(V_HOME); press(24+kit);
        for(int t=0;t<NDRUM;t++) {
            assert(ui.sel[kit]==t); assert(pg.r[0][0].b==t);
            if(t+1<NDRUM) press(0x2F);
        }
        press(0x2F); assert(ui.sel[kit]==NDRUM-1);
        view(V_MIX);
        while(cur_page()>kit+1) press(0x62);
        while(cur_page()<kit+1) press(0x63);
        press(0x2E); assert(ui.sel[kit]==0);
        for(int t=0;t<NDRUM;t++) {
            if(t==8) press(0x2F);
            press(24+t%8); assert(ui.sel[kit]==t);
            for(int n=1;n<4;n++) { pref_t r=smc_knob_target(n); assert(r.b==t); edit(r,n); }
            assert(smc_knob_target(0).kind==R_NONE);
            pref_t r=smc_target(t%8); assert(r.b==t);
            fader(t%8,smc_scaled(r)); fader(t%8,70); assert(smc.picked & (1u<<(t%8)));

        }
        for(int n=NDRUM-8;n<8;n++) {
            assert(smc_target(n).kind==R_NONE);
            int selected=ui.sel[kit], selected_part=ui.part;
            press(24+n); fader(n,127);
            assert(ui.sel[kit]==selected && ui.part==selected_part && drum_mix_page()==kit);
        }
        press(0x2E); press(0x2F); assert(ui.sel[kit]==8);
    }
pickup: ;
    /* Real notice/frame: feedback must not rearm pickup after our own write. */
    smc_view(V_HOME,0); pref_t r=smc_target(0); pref_set(r,64); smc_frame();
    fader(0,0); assert(!(smc.picked&1) && !strncmp(ui.msg,"PICK F1",7));
    int old=pref_get(r); smc_frame(); assert(pref_get(r)==old);
    fader(0,127); assert((smc.picked&1) && !strncmp(ui.msg,"SMC F1",6));
    fader(0,100); assert(smc.picked&1);
    pref_set(r,20); smc_frame(); assert(!(smc.picked&1) && !(smc.seen&1));
    fader(0,110); assert(!(smc.picked&1));
    fader(0,0); assert(smc.picked&1);
    /* Waiting must not allocate motion; crossing must record through production motion_touch. */
    seq.playing=1; ui.rec=1; pref_set(r,64); smc_frame();
    int lanes=motion_count(proj.arr.lane); fader(0,0);
    assert(motion_count(proj.arr.lane)==lanes);
    fader(0,127); assert(motion_count(proj.arr.lane)==lanes+1);
    int mp=engine_motion_part(r.a,r.b);
    assert(motion_find(proj.arr.lane,seq.ppat[mp],mp,r.a,r.b,r.c)>=0);
    seq.playing=0; ui.rec=0;
    smc_ble_link(0); smc_poll(); assert(!smc.picked && !smc.seen && !smc.solo);
    smc_ble_link(1); smc_poll(); assert(!smc.picked && !smc.seen);
    if(getenv("SMC_PICKUP_ONLY")) return;
    /* View navigation reaches all ordinary views and Global's actions. */
    press(26); view(V_SEQ); view(V_GEN); view(V_FX); view(V_MIX); view(V_SONG); view(V_HOME);
    press(14); assert(ui.view==V_GLO && ui.overlay==O_LIST); press(6); assert(ui.overlay==O_NONE);
    /* Confirmation guards edits, consumes input, and has explicit cancel/confirm. */
    ask(ACT_SONG_CLR,0,"TEST",0); old=pref_get(r);
    fader(0,127); turn(0,1); press(24); assert(ui.overlay==O_ASK && pref_get(r)==old);
    press(6); assert(ui.overlay==O_NONE);
    ask(ACT_SONG_CLR,0,"TEST",0); press(5); assert(ui.overlay==O_NONE);
    int saved_test=perf.test, saved_phase=perf.phase;
    perf.test=0; perf.phase=0; old=pref_get(r);
    fader(0,127); turn(0,1); press(24);
    assert(pref_get(r)==old && smc_qr==smc_qw && smc_host_r==smc_host_w);
    perf.test=saved_test; perf.phase=saved_phase;
    ui.help=1; int part=ui.part; press(24); fader(0,127); turn(0,1);
    assert(ui.part==part && pref_get(r)==old); press(6); assert(!ui.help);
    /* Clip grid hides ordinary parameters; all three editors and nine ARP fields reachable. */
    clip_grid_mode=1; ck.pg=0; old=pref_get(pg.r[cur_page()][0]); turn(0,1);
    assert(pref_get(pg.r[cur_page()][0])==old);
    for(int p=1;p<=3;p++) {
        press(0x61); assert((ck.pg&3)==p);
        int count=p==3?9:4;
        for(int i=0;i<count;i++) {
            int f=CK_PAGE_FIELDS[p-1][i]; assert((ck.pg>>4)==f);
            press(0x63); assert(fields[f]==1); press(0x62); assert(fields[f]==0);
            press(0x2F);
        }
    }
    press(0x61); assert(!(ck.pg&3));
    press(0x61); press(5); assert(ui.view==V_GLO && ui.overlay==O_LIST);
    int clip_field=fields[CK_E_ON]; turn(0,1); assert(fields[CK_E_ON]==clip_field);
    press(6); assert(ui.overlay==O_NONE && (ck.pg&3)==1);
    press(6); assert(!(ck.pg&3));
    puts("PASS: complete SMC navigation, edits, pickup/frame, confirmation/help and clip paths");
}
int main(void) {
    for(route=0;route<3;route++) { printf("Route %d\n",route); scenario(); }
    return 0;
}
