/* Full project, engine, UI, SMC queues and pickup logic. Hardware I/O and clip
 * field storage alone are substitutes; clip navigation/turn code is extracted. */
#define main simulator_main
void smc_ui_cancel(void);
void smc_ui_clip_show(int n, const char *name, const char *value);
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
static uint8_t led_vel[128];
int plat_ble_smc_send(const uint8_t *msg, uint8_t len) {
    if (msg && len >= 3 && (msg[0] & 0xF0) == 0x90 && msg[1] < 128)
        led_vel[msg[1]] = msg[2];
    return 1;
}
#ifndef SMC_SLOT_SEND_IMPLEMENTED
/* Existing one-peer fixtures record peer zero. Dual-peer tests override this. */
int plat_ble_smc_send_slot(unsigned slot, const uint8_t *m, uint8_t n)
{ return slot == 0 ? plat_ble_smc_send(m, n) : 1; }
#endif

#ifndef SMC_SOURCE_FILE
#define SMC_SOURCE_FILE "app/smc_mixer.c"
#endif
#include SMC_SOURCE_FILE
#include "ble/ble_midi_central.c"
static blec_t receiver;
static int route;
/* All scenario controls are DAW. Legacy bit/noise variants must not change mapping. */
static int hci_send(uint8_t t,const uint8_t *p,uint16_t n,void *ctx)
{ (void)t;(void)p;(void)n;(void)ctx;return 0; }
static void hci_midi(const uint8_t *p,uint8_t n,void *ctx)
{ (void)ctx;smc_ble_midi(p,n); }
static void receive_ready(void)
{ receiver.conn=1;receiver.linked=1;receiver.midi_handle=6;receiver.state=BLEC_CONNECTED; }
static void hci_input(const uint8_t *m,unsigned n,unsigned tail)
{
    uint8_t b[32]={1,0,0,0,0,0,4,0,0x1b,6,0,0x80,0x81};
    b[2]=n+9-tail;b[4]=n+5;memcpy(b+13,m,n);
    blec_rx(&receiver,2,b,n+13-tail,now_ms);
    if(tail) {
        uint8_t end[16]={1,0x10,0,0};end[2]=tail;
        memcpy(end+4,b+n+13-tail,tail);
        blec_rx(&receiver,2,end,tail+4,now_ms);
    }
}
static void smc_input(const uint8_t *m,unsigned n)
{
    static unsigned fragments;
    if(route>=3) hci_input(m,n,(fragments++)%5);
    else smc_ble_midi(m,n);
}
static void message(int st, int a, int b) {
    uint8_t msg[3] = {st,a,b};
    now_ms += 100;
    if(route==2) {
        unsigned before=smc_host_w;
        assert(!smc_midi_host(0x0B | 0xBFu<<8 | (uint32_t)msg[1]<<16 | (uint32_t)msg[2]<<24));
        assert(smc_host_w==before);
    }
    smc_input(msg,3);
    smc_poll(); smc_frame();
}
static void press(int note) { message(0x90,note,127); message(0x80,note,0); }
static void turn(int n, int delta) { message(0xB0,16+n,delta>0?delta:64-delta); }
static void fader(int n, int v) { message(0xE0+n,0,v); }
static void view(int v) {
    /* 0x60/0x61 select mixer banks, while SELECT owns view navigation. */
    smc_view(v, 0);
    assert(ui.view == v);
}
static void edit(pref_t r, int n) {
    const x0x_param_t *d=pref_desc(r); assert(d && d->max);
    int old=pref_get(r), delta=old==d->max?-1:1;
    turn(n,delta); assert(pref_get(r)!=old);
}
/* A fast spin on strip 5 must not accelerate strip 1, or the panel's first knob. */
static void knob_accel_isolated(void) {
    pref_t slow, fast;
    const x0x_param_t *ds, *df;
    int before;
    uint8_t panel_run;
    smc_view(V_HOME, 0);
    slow = smc_knob_target(0);
    fast = smc_knob_target(4);
    ds = pref_desc(slow);
    df = pref_desc(fast);
    assert(ds && df && ds->max >= 24 && df->max >= 24);
    pref_set(slow, 40);
    pref_set(fast, 40);
    ui.enc_run[EN_K1] = 9;
    ui.enc_t[EN_K1] = now_ms;
    panel_run = ui.enc_run[EN_K1];
    for (int i = 0; i < 6; i++) {
        uint8_t msg[3] = {0xB0, 20, 1};
        now_ms += 10;
        smc_ble_midi(msg, 3);
        smc_poll();
    }
    before = pref_get(slow);
    now_ms += 10;
    {
        uint8_t msg[3] = {0xB0, 16, 1};
        smc_ble_midi(msg, 3);
        smc_poll();
    }
    assert(pref_get(slow) == before + 1);
    assert(ui.enc_run[EN_K1] == panel_run);
}
/* Panel curve stays x8. SMC quick spins use a shorter curve. Same translation unit. */
static int accel_at(int is_smc, uint32_t gap, int s, int range, uint8_t run)
{
    int got;
    accel_smc = (uint8_t)is_smc;
    ui.enc_t[EN_K1] = now_ms;
    ui.enc_run[EN_K1] = run;
    now_ms += gap;
    got = (int)accel(EN_K1, s, range);
    accel_smc = 0;
    return got;
}
static void knob_cc(int n, int value, uint32_t gap)
{
    uint8_t msg[3] = {0xB0, (uint8_t)(16 + n), (uint8_t)value};
    now_ms += gap;
    smc_ble_midi(msg, 3);
    smc_poll();
}
/* Count `count` same-byte turns. The first message after a strip reset sees a
 * long gap, so it is the one-step warmup; later gaps are `gap`. */
static int knob_burst(pref_t r, int n, int value, int count, uint32_t gap)
{
    int before;
    smc.knob_i = 0;
    before = pref_get(r);
    knob_cc(n, value, 80);
    for (int i = 1; i < count; i++)
        knob_cc(n, value, gap);
    return pref_get(r) - before;
}
static void knob_quick_spin_slower(void)
{
    pref_t pan, tempo, length, rate, transpose;
    const x0x_param_t *dpan, *drate;
    int travel, saved_tempo, saved_len, saved_rate, saved_tr, part;
    uint32_t frames;
    int32_t buf[512];
    /* Panel numbers stay. A running spin, one detent. */
    assert(accel_at(0, 8, 1, 127, 8) == 8);
    assert(accel_at(0, 20, 1, 127, 8) == 5);
    assert(accel_at(0, 30, 1, 127, 8) == 3);
    assert(accel_at(0, 50, 1, 127, 8) == 1);
    assert(accel_at(0, 8, 1, 48, 8) == 3);
    assert(accel_at(0, 8, 1, 255, 8) == 16);
    assert(accel_at(0, 8, 20, 127, 8) == 160);
    assert(accel_at(0, 8, -1, 127, 8) == -8);
    assert(accel_at(0, 8, 1, 127, 2) == 1);          /* still warming up */
    assert(accel_at(0, 8, 5, 20, 8) == 5);           /* short range: no multiply */
    /* SMC: slow click unchanged, quick end much smaller, both signs. */
    assert(accel_at(1, 8, 1, 127, 2) == 1);
    assert(accel_at(1, 80, 1, 127, 8) == 1);
    assert(accel_at(1, 80, -1, 127, 8) == -1);
    assert(accel_at(1, 8, 1, 127, 8) == 3);
    assert(accel_at(1, 20, 1, 127, 8) == 2);
    assert(accel_at(1, 30, 1, 127, 8) == 2);
    assert(accel_at(1, 8, 1, 48, 8) == 2);           /* shorter pot */
    assert(accel_at(1, 8, 1, 255, 8) == 3);          /* wide: not the panel's x16 */
    assert(accel_at(1, 8, 20, 127, 8) == 26);        /* 3*3 + 17, not 20*8 */
    assert(accel_at(1, 8, -20, 127, 8) == -26);
    assert(accel_at(1, 8, 63, 127, 8) == 69);        /* flick, not 63*8 */
    assert(accel_at(1, 8, -63, 127, 8) == -69);
    assert(accel_at(1, 900, 20, 127, 8) == 20);      /* a slow bundle stays 1:1 */
    assert(accel_at(1, 8, 20, 20, 8) == 20);         /* short range still unmultiplied */
    assert(accel_smc == 0);

    smc_view(V_HOME, 0);
    pan = smc_knob_target(0);
    dpan = pref_desc(pan);
    assert(dpan && !dpan->names && dpan->max >= 100 && dpan->max <= 150);
    pref_set(pan, 40);
    assert(knob_burst(pan, 0, 1, 1, 80) == 1);
    pref_set(pan, 40);
    assert(knob_burst(pan, 0, 0x41, 1, 80) == -1);
    /* 12 messages: one slow anchor, two more warmup steps, then 9 at x3 = 30.
     * The old curve on those 9 was x8, so 75. */
    pref_set(pan, 40);
    travel = knob_burst(pan, 0, 1, 12, 8);
    assert(travel == 30);
    assert(travel > 12 && travel < 75);
    pref_set(pan, 80);
    assert(knob_burst(pan, 0, 0x41, 12, 8) == -30);
    /* Anchor with one detent, then a 63-detent flick 10 ms later: +69, not 63*8.
     * 10 + 1 + 69 = 80, still short of the top, so this did not slam the end. */
    pref_set(pan, 10);
    smc.knob_i = 0;
    knob_cc(0, 1, 80);
    knob_cc(0, 0x3F, 10);
    assert(pref_get(pan) == 80);
    pref_set(pan, 100);
    smc.knob_i = 0;
    knob_cc(0, 0x41, 80);
    knob_cc(0, 0x7F, 10);
    assert(pref_get(pan) == 30);
    /* Clamp. A fast turn into either end stops. It does not wrap. */
    pref_set(pan, dpan->max);
    smc.knob_i = 0;
    knob_cc(0, 0x3F, 8);
    assert(pref_get(pan) == dpan->max);
    knob_cc(0, 1, 8);
    assert(pref_get(pan) == dpan->max);
    pref_set(pan, 0);
    smc.knob_i = 0;
    knob_cc(0, 0x7F, 8);
    assert(pref_get(pan) == 0);
    knob_cc(0, 0x41, 8);
    assert(pref_get(pan) == 0);

    /* Wide tempo on GLOBAL, and a shorter 303 transpose. Named rate stays one step. */
    smc_view(V_GLO, 0);
    tempo = smc_knob_target(0);
    assert(pref_desc(tempo) && pref_desc(tempo)->max > 150);
    saved_tempo = pref_get(tempo);
    pref_set(tempo, 40);
    travel = knob_burst(tempo, 0, 1, 12, 8);
    assert(travel == 30);
    assert(travel < 3 + 9 * 16);                     /* old wide curve */
    pref_set(tempo, saved_tempo);

    part = ui.part;
    smc_select_part(PART_303A);
    smc_view(V_SEQ, 0);
    length = smc_knob_target(0);
    rate = smc_knob_target(1);
    transpose = smc_knob_target(3);
    assert(pref_desc(length) && pref_desc(length)->max > 24 && pref_desc(length)->max < 100);
    drate = pref_desc(rate);
    assert(drate && drate->names && drate->max < 24);
    assert(pref_desc(transpose) && pref_desc(transpose)->max > 24 && pref_desc(transpose)->max < 100);
    saved_len = pref_get(length);
    saved_rate = pref_get(rate);
    saved_tr = pref_get(transpose);
    pref_set(length, 10);
    travel = knob_burst(length, 0, 1, 12, 8);
    assert(travel == 21);                            /* 3 + 9*2, not the old x3 */
    pref_set(transpose, 10);
    assert(knob_burst(transpose, 3, 1, 12, 8) == 21);
    pref_set(rate, 0);
    smc.knob_i = 0;
    knob_cc(1, 0x3F, 8);                             /* a flick on a switch is still one position */
    assert(pref_get(rate) == 1);
    pref_set(rate, drate->max);
    smc.knob_i = 0;
    knob_cc(1, 0x3F, 8);
    knob_cc(1, 1, 8);
    assert(pref_get(rate) == drate->max);            /* stops; does not wrap to the first name */
    pref_set(rate, 0);
    smc.knob_i = 0;
    knob_cc(1, 0x7F, 8);
    assert(pref_get(rate) == 0);
    pref_set(length, saved_len);
    pref_set(rate, saved_rate);
    pref_set(transpose, saved_tr);

    /* Playing: the spin stays on the main loop. The render still completes. */
    smc_view(V_HOME, 0);
    pan = smc_knob_target(0);
    pref_set(pan, 40);
    seq.playing = 1;
    frames = eng_prof_frames;
    for (int i = 0; i < 512; i++) buf[i] = 0x7fffffff;
    engine_render(buf, 256);
    assert(eng_prof_frames == frames + 256);
    assert(buf[0] != 0x7fffffff);
    travel = knob_burst(pan, 0, 1, 8, 8);
    assert(travel == 18);                            /* 3 + 5*3; still the short curve */
    for (int i = 0; i < 512; i++) buf[i] = 0x7fffffff;
    engine_render(buf, 256);
    assert(seq.playing);
    assert(eng_prof_frames == frames + 512);
    assert(buf[0] != 0x7fffffff);
    seq.playing = 0;
    smc_select_part(part);
    smc_view(V_HOME, 0);
    assert(accel_smc == 0);
}
/* Panel part change must move the lit Record strip even if smc.rec_part is stale. */
static void record_lamp_follows_part(void) {
    memset(led_vel, 0, sizeof led_vel);
    smc_view(V_HOME, 0);
    ui.rec = 1;
    ui.part = 0;
    smc.rec_part = 0;
    smc_frame();
    assert(led_vel[0] == 127);
    assert(led_vel[4] == 0);
    ui.part = 2;                       /* panel SELECT / part change; no SMC Record press */
    for (int i = 0; i < 128; i++) smc.known[i] = 0;
    smc_frame();
    assert(led_vel[0] == 0);
    assert(led_vel[4] == 127);
    ui.rec = 0;
    for (int i = 0; i < 128; i++) smc.known[i] = 0;
    smc_frame();
    assert(led_vel[4] == 0);
}
/* Nav lamps on strips 6–8 must follow their own shortcut, not one shared page formula. */
static void nav_strip_lamps(void) {
    memset(led_vel, 0, sizeof led_vel);
    for (int i = 0; i < 128; i++) smc.known[i] = 0;
    smc_view(V_FX, 0);
    smc_frame();
    assert(led_vel[29] == 127);        /* SELECT 6 = reverb */
    assert(led_vel[30] == 0);          /* SELECT 7 stays dark on reverb */
    assert(led_vel[13] == 0);          /* S6 is MIX, not FX */
    smc_view(V_FX, 1);
    for (int i = 0; i < 128; i++) smc.known[i] = 0;
    smc_frame();
    assert(led_vel[29] == 0);
    assert(led_vel[30] == 127);
    smc_view(V_MIX, 3);
    for (int i = 0; i < 128; i++) smc.known[i] = 0;
    smc_frame();
    assert(led_vel[31] == 127);        /* SELECT 8 = master MIX page */
    assert(led_vel[7] == 0);           /* R8 only on HOME */
    assert(led_vel[13] == 127);        /* S6 on any MIX page */
    smc_view(V_HOME, 0);
    for (int i = 0; i < 128; i++) smc.known[i] = 0;
    smc_frame();
    assert(led_vel[7] == 127);
    assert(led_vel[31] == 0);
}
/* Help is only a card: faders and knobs keep working, and the card stays up. */
static void help_keeps_controls(void) {
    pref_t r;
    int after; /* retired bit never changes DAW format */
    uint8_t at, msg[3];
    smc_view(V_HOME, 0);
    r = smc_target(0);
    pref_set(r, 64);
    smc.picked = smc.seen = 0;
    smc_frame();
    at = smc_scaled(r);
    ui.help = 1;
    assert(smc_same(smc_target(0), r));
    msg[0] = 0xE0; msg[1] = 0; msg[2] = 0;
    smc_ble_midi(msg, 3); smc_poll();
    msg[2] = at;
    smc_ble_midi(msg, 3); smc_poll();
    assert(ui.help && (smc.picked & 1));
    msg[2] = (uint8_t)(at < 120 ? at + 8 : at - 8);
    smc_ble_midi(msg, 3); smc_poll();
    assert(ui.help && pref_get(r) != 64);
    r = smc_knob_target(0);
    after = pref_get(r);
    msg[0] = 0xB0; msg[1] = 16; msg[2] = 1;
    smc_ble_midi(msg, 3); smc_poll();
    assert(ui.help && pref_get(r) != after);
    msg[0] = 0x90; msg[1] = 6; msg[2] = 127; /* R7 closes help; inject before HCI routes are armed */
    smc_ble_midi(msg, 3); smc_poll();
    msg[2] = 0; smc_ble_midi(msg, 3); smc_poll(); /* release Home so it is not still held */
    assert(!ui.help);
}
static void scenario(void) {
    static const int expected_strip[5] = {0,1,4,3,2};
    clip_grid_mode=0; ck.pg=0; memset(fields,0,sizeof fields); memset(&smc,0,sizeof smc);
    memset(&ui,0,sizeof ui); memset(led_vel,0,sizeof led_vel);
    boot(); proj.set.bt=PROJ_BT_SMC_CENTRAL | ((route==1 || route==2 || route==4)?PROJ_BT_SMC_USER:0); smc_ble_link(1); smc_poll();
    knob_accel_isolated();
    knob_quick_spin_slower();
    record_lamp_follows_part();
    nav_strip_lamps();
    help_keeps_controls();
    if(route>=3) {
        uint8_t addr[6]={0};blec_init(&receiver,hci_send,hci_midi,0,addr);receive_ready();
        const uint8_t partial[]={0xF0,0x7F,0x7F};hci_input(partial,3,0);
        assert(receiver.in_sysex);
        const uint8_t down[]={5,4,0,1,0,0x13};
        blec_rx(&receiver,4,down,sizeof down,now_ms);smc_ble_link(0);smc_poll();
        receive_ready();smc_ble_link(1);smc_poll();
        /* The first fader, knob and ordinary SELECT after interrupted SysEx reach UI. */
        smc_select_part(PART_303A); smc_view(V_HOME,0);
        fader(0,0);assert(smc_ui_active());
        press(24); assert(ui.part==PART_909 && ui.view==V_PART && !smc.fx_down);
        press(24+PART_303B); assert(ui.part==PART_303B && ui.view==V_PART && !smc.fx_down);
        edit(smc_knob_target(0),0);smc_view(V_HOME,0);
    }
    if(getenv("SMC_PICKUP_ONLY")) goto pickup;
    /* Starting at HOME, SELECT must expose each part's sound. */
    for(int part=0;part<NPARTS;part++) {
        smc_select_part((part + 1) % NPARTS);
        smc_view(V_HOME,0);
        press(24 + expected_strip[part]);
        assert(ui.part == part && ui.view == V_PART && !smc.fx_down);
        /* The same actual input is an assigned effect only in FX context.
         * DAW Note buttons own press/release; stock User MMC has toggle-only edges. */
        smc_view(V_FX,0);
        message(0x90,24+expected_strip[part],127);
        assert(ui.part == part && ui.view == V_FX && (smc.fx_down & (1u << part)));
        message(0x80,24+expected_strip[part],0);
        assert(ui.part == part && ui.view == V_FX && !(smc.fx_down & (1u << part)));
        smc_view(V_PART, 0);
        for(int p=0;p<pg.n;p++) {
            assert(smc_page_now()==p);
            for(int n=0;n<4;n++) { pref_t r=smc_knob_target(n); if(r.kind!=R_NONE && !pref_read_only(r)) edit(r,n); } /* the 303 STATUS row is read-only */
            for(int n=4;n<8;n++) assert(smc_knob_target(n).kind==R_NONE ||
                !smc_same(smc_knob_target(n),smc_target(n)));
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
        smc_select_part(kit); smc_view(V_PART, 0);
        for(int t=0;t<NDRUM;t++) {
            assert(ui.sel[kit]==t); assert(pg.r[0][0].b==t);
            if(t+1<NDRUM) press(0x2F);
        }
        press(0x2F); assert(ui.sel[kit]==NDRUM-1);
        view(V_MIX);
        while(smc_page_now()>kit+1) press(0x62);
        while(smc_page_now()<kit+1) press(0x63);
        press(0x2E); assert(ui.sel[kit]==0);
        for(int t=0;t<NDRUM;t++) {
            if(t==8) press(0x2F);
            /* Already-selected SELECT now intentionally returns to main MIX. */
            if (ui.sel[kit] != t) press(24+t%8);
            assert(ui.sel[kit]==t);
            pref_t k=smc_knob_target(t%8); assert(k.b==t);
            edit(k,t%8);                  /* every strip in both drum knob banks */
            for(int n=0;n<8;n++) assert(smc_knob_target(n).kind == R_NONE || !smc_same(smc_knob_target(n),smc_target(n)));
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
    smc_view(V_HOME,0);
    for(int n=0;n<8;n++) { assert(smc_knob_target(n).kind == R_NONE || !smc_same(smc_knob_target(n),smc_target(n))); edit(smc_knob_target(n),n); }
    /* Establish a physical sample below the new target; routing must no
     * longer discard a valid crossing from the previous visible page. */
    fader(0,0);
    pref_t r=smc_target(0); pref_set(r,64); smc_frame();
    fader(0,1); assert(!(smc.picked&1) && smc_ui_active());
    int old=pref_get(r); smc_frame(); assert(pref_get(r)==old);
    fader(0,127); assert((smc.picked&1) && smc_ui_active());
    fader(0,100); assert(smc.picked&1);
    pref_set(r,20); smc_frame(); assert(!(smc.picked&1) && (smc.seen&1));
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
    smc_select_part(PART_303A); smc_view(V_PART, 0);
    view(V_SEQ); view(V_GEN); view(V_FX); view(V_MIX); view(V_SONG); view(V_HOME);
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
    ui.help=1; press(24); assert(ui.help && ui.part==PART_909 && ui.view==V_PART && !smc.fx_down);
    old=pref_get(smc_knob_target(0)); turn(0,1);
    assert(ui.help && pref_get(smc_knob_target(0))!=old); /* card stays; knob still edits */
    press(6); assert(!ui.help);
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
    for(route=0;route<5;route++) { printf("Route %d\n",route); scenario(); }
    /* Rejection observability without rewriting input presets or dispatch rules. */
    const uint8_t m[3]={0xE0,0,100};
    proj.set.bt=PROJ_BT_SMC_CENTRAL|PROJ_BT_SMC_USER;
    smc_ble_midi(m,3);smc_poll();assert((smc_rx_diag()>>24)==0); /* DAW remains admitted with legacy bit */
    smc_ble_link(0);smc_poll();smc_ble_midi(m,3);smc_poll();assert((smc_rx_diag()>>24)==2);
    smc_ble_link(1);smc_poll();proj.set.bt=0;
    smc_ble_midi(m,3);smc_poll();assert((smc_rx_diag()>>24)==3);
    unsigned drops=(smc_rx_diag()>>8)&255;
    /* Faders and encoders are held in the overflow mailbox, never dropped. */
    for(unsigned i=0;i<SMC_Q+1;i++) smc_ble_midi(m,3);
    assert(((smc_rx_diag()>>8)&255)==drops);smc_poll();
    /* Other messages are still counted when the ring is full. */
    const uint8_t note[3]={0x90,0x70,127};
    for(unsigned i=0;i<SMC_Q+1;i++) smc_ble_midi(note,3);
    assert(((smc_rx_diag()>>8)&255)==((drops+1)&255));smc_poll();
    return 0;
}
