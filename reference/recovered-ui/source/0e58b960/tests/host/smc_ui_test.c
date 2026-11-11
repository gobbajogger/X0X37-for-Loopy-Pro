/* SPDX-License-Identifier: GPL-3.0-only */
/* Real production UI, framebuffer, engine, SMC queues and clip editor routing.
 * Hardware, clip field storage and the clip grid painter alone are substitutes.
 * The assertions protect the end user's restore/navigation/pickup contract. */
#define BOARD_SMK37 1
#define X0X_DEV_PADCAL 1
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
#define CK_FN __attribute__((unused))
#include "app/smk_clip_keys.h"
static struct { uint8_t pg; } ck;
static uint8_t clip_grid_mode, fields[CK_NE], clip_waiting, clip_dirty;
static unsigned clip_forces;
static uint32_t ck_get(uint32_t f) { return fields[f]; }
static void ck_set(uint32_t f, uint32_t v) { fields[f] = (uint8_t)v; }
static const char *ck_val_txt(uint32_t f, uint32_t v, char *s)
{ (void)f; sprintf(s, "%u", (unsigned)v); return s; }
#include "clip_smc.inc"
uint8_t smk_clip_mode_on(void) { return clip_grid_mode; }
uint8_t smk_clip_transport_button(uint8_t b, uint8_t p) { (void)b; (void)p; return 0; }
uint8_t smk_clip_history_button(uint8_t b, uint8_t p, uint8_t h) { (void)b; (void)p; (void)h; return 0; }
void smk_play_note_button(uint8_t p, uint8_t h) { (void)p; (void)h; }
uint8_t smk_ck_flash_busy(void) { return 0; }
uint8_t smk_ck_ext(void) { return 0; }
uint8_t smk_ck_knobs_owned(void) { return 0; }
uint8_t smk_ck_panel(uint32_t *p, uint32_t f) { (void)p; (void)f; return 0; }
uint8_t smk_clip_lcd_draw(uint32_t w, uint32_t h)
{
    if (!clip_dirty) return 0;
    clip_dirty = 0;
    cv_begin(w, h, C_BLACK);
    cv_band(0, h);
    cv_rect(12, 8, 212, 92, RGB(30, 60, 90));
    return 1;
}
void smk_clip_lcd_force(void) { clip_forces++; clip_dirty = 1; }
uint8_t smk_clip_wait(void) { return clip_waiting; }
uint8_t smk_apc_transport_led_state(uint8_t n) { (void)n; return 0; }
uint8_t smk_padcal_on(void) { return 0; }
void smk_padcal_enter(void) {}
uint8_t smk_padcal_draw(uint32_t w, uint32_t h) { (void)w; (void)h; return 0; }
#ifndef SMC_UI_SEND_IMPLEMENTED
int plat_ble_smc_send(const uint8_t *m, uint8_t n) { (void)m; (void)n; return 1; }
#endif
#ifndef SMC_SLOT_SEND_IMPLEMENTED
/* Existing one-peer fixtures record peer zero. Dual-peer tests override this. */
int plat_ble_smc_send_slot(unsigned slot, const uint8_t *m, uint8_t n)
{ return slot == 0 ? plat_ble_smc_send(m, n) : 1; }
#endif

#ifndef SMC_SOURCE_FILE
#define SMC_SOURCE_FILE "app/smc_mixer.c"
#endif
#include SMC_SOURCE_FILE

static unsigned checks;
#define CHECK(x) do { checks++; if (!(x)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)
typedef struct {
    uint8_t view, part, prev_view, sel[NKIT], page[NVIEWS][NPARTS];
    uint8_t spage, pbank, mixsel, overlay, list_sel, list_top, list_n, help;
    uint8_t ask_act, ask_overlay, song_sel, clip, clip_page;
    uint16_t ask_arg;
    int8_t touched;
    uint32_t touch_until, msg_until;
    char msg[sizeof ui.msg], ask_q[sizeof ui.ask_q];
} screen_state_t;
static screen_state_t screen_state(void)
{
    screen_state_t s;
    memset(&s, 0, sizeof s);
    s.view = ui.view; s.part = ui.part; s.prev_view = ui.prev_view;
    memcpy(s.sel, ui.sel, sizeof s.sel); memcpy(s.page, ui.page, sizeof s.page);
    s.spage = ui.spage; s.pbank = ui.pbank; s.mixsel = ui.mixsel;
    s.overlay = ui.overlay; s.list_sel = ui.list_sel; s.list_top = ui.list_top;
    s.list_n = ui.list_n; s.help = ui.help; s.ask_act = ui.ask_act;
    s.ask_overlay = ui.ask_overlay; s.ask_arg = ui.ask_arg; s.song_sel = ui.song_sel;
    s.touched = ui.touched; s.touch_until = ui.touch_until;
    s.msg_until = ui.msg_until; memcpy(s.msg, ui.msg, sizeof s.msg);
    memcpy(s.ask_q, ui.ask_q, sizeof s.ask_q);
    s.clip = clip_grid_mode; s.clip_page = ck.pg;
    return s;
}
static void unchanged(screen_state_t before)
{
    screen_state_t after = screen_state();
    if (memcmp(&before, &after, sizeof before)) {
        const uint8_t *a = (const uint8_t *)&before, *b = (const uint8_t *)&after;
        for (unsigned i = 0; i < sizeof before; i++)
            if (a[i] != b[i]) fprintf(stderr, "state byte %u: %u -> %u\n", i, a[i], b[i]);
    }
    CHECK(!memcmp(&before, &after, sizeof before));
}
static void shot(const char *name)
{
    const char *dir = getenv("SMC_UI_SHOTS"); char path[1024];
    if (!dir) return;
    snprintf(path, sizeof path, "%s/%s.png", dir, name); png_write(path);
}
static void setup(int view, int part)
{
    held_btn = held_keys = 0;
    memset(enc_acc, 0, sizeof enc_acc);
    memset(&ui, 0, sizeof ui);
    smc_slot = 0;
    memset(smc_states, 0, sizeof smc_states);
    memset(smc_inputs, 0, sizeof smc_inputs);
    smc_ui_cancel();
    clip_grid_mode = clip_waiting = ck.pg = 0; clip_dirty = 1;
    memset(fields, 0, sizeof fields);
    boot();
    now_ms = 1000;
    proj.set.autosave_off = 1;
    proj.set.bt = PROJ_BT_SMC_CENTRAL;
    smc_ble_link(1); smc_poll();
    ui.part = (uint8_t)part; set_view(view); build_pages();
    ui.touched = -1; ui.msg_until = 0;
    ui_frame();
}
static void message(int st, int a, int b)
{
    uint8_t msg[3] = {(uint8_t)st, (uint8_t)a, (uint8_t)b};
    smc_ble_midi(msg, 3); smc_poll();
}
static void fader(int n, int value) { message(0xE0 + n, 0, value); }
static void knob(int n, int delta) { message(0xB0, 16 + n, delta > 0 ? delta : 64 - delta); }
static void expire(void) { now_ms += 1200; ui_frame(); CHECK(!smc_ui_active()); }
static uint16_t before_fb[240 * 240], active_fb[240 * 240];
static unsigned changed_rows(int lo, int hi, const uint16_t *a, const uint16_t *b)
{
    unsigned n = 0;
    for (int y = lo; y < hi; y++) n += memcmp(a + 240 * y, b + 240 * y, 480) != 0;
    return n;
}

static void test_restore(void)
{
    const int views[] = {V_HOME, V_PART, V_GEN, V_FX, V_MIX, V_GLO, V_SONG, V_SEQ};
    for (unsigned i = 0; i < sizeof views / sizeof *views; i++) {
        setup(views[i], PART_303A);
        ui.mixsel = 3; ui.spage = 1; ui.pbank = 1; ui.sel[0] = 10; ui.sel[1] = 8;
        ui_frame(); memcpy(before_fb, fb, sizeof fb);
        screen_state_t before = screen_state();
        pref_t target = smc_target(0);
        int value = pref_get(target);
        fader(0, 0);
        CHECK(smc_ui_active()); CHECK(pref_get(target) == value); unchanged(before);
        ui_frame(); CHECK(smc_ui_active()); unchanged(before);
        CHECK(changed_rows(0, 32, before_fb, fb) > 0);
        CHECK(changed_rows(40, 150, before_fb, fb) > 20);
        CHECK(changed_rows(160, 240, before_fb, fb) > 0);
        if (!i) shot("01-main-pickup");
        expire(); unchanged(before);
        CHECK(!memcmp(before_fb, fb, sizeof fb));
        if (!i) shot("02-home-restored");
    }
}
static void test_activity(void)
{
    setup(V_HOME, PART_303A);
    pref_t r = smc_target(0); pref_set(r, 64); smc_frame();
    screen_state_t before = screen_state();
    fader(0, 0); CHECK(smc_ui_active()); CHECK(!(smc.picked & 1));
    ui_frame(); memcpy(before_fb, fb, sizeof fb);
    now_ms += 1199; ui_frame(); CHECK(smc_ui_active());
    fader(2, 0); CHECK(smc_ui_active()); unchanged(before);
    now_ms += 1; ui_frame(); CHECK(smc_ui_active());
    now_ms += 1198; ui_frame(); CHECK(smc_ui_active());
    now_ms += 1; ui_frame(); CHECK(!smc_ui_active()); unchanged(before);
    fader(0, 1); ui_frame(); memcpy(active_fb, fb, sizeof fb);
    pref_set(r, 23); ui_frame(); CHECK(memcmp(active_fb, fb, sizeof fb));
    fader(0, smc_scaled(r)); CHECK(smc.picked & 1);
    ui_frame(); shot("03-picked-fader");
    knob(6, 1); CHECK(smc_ui_active()); ui_frame(); shot("04-delay-knob");
    now_ms = UINT32_MAX - 600u;
    fader(0, 0); CHECK(smc_ui_active());
    now_ms += 1199; CHECK(smc_ui_active()); now_ms++; CHECK(!smc_ui_active());
    puts("PASS activity: pickup, live values, repeated controls, exact boundary and wrap");
}
static void test_existing_feedback(void)
{
    setup(V_FX, PART_303A);
    ui.touched = 0; ui.touch_until = now_ms + 5000;
    strcpy(ui.msg, "PRIOR FEEDBACK"); ui.msg_until = now_ms + 10000;
    ui_frame(); memcpy(before_fb, fb, sizeof fb);
    screen_state_t before = screen_state();
    fader(0, 0); CHECK(smc_ui_active()); unchanged(before);
    ui_frame(); unchanged(before); expire(); unchanged(before);
    CHECK(!memcmp(before_fb, fb, sizeof fb));
    puts("PASS feedback: existing header message and local knob readout survive temporary UI");
}
static void test_navigation(void)
{
    setup(V_PART, PART_303A); fader(0, 0); CHECK(smc_ui_active());
    held_btn = 1u << B_FX; input(); CHECK(!smc_ui_active());
    held_btn = 0; input(); CHECK(ui.view == V_FX);
    int view = ui.view; now_ms += 1500; ui_frame(); CHECK(ui.view == view && !smc_ui_active());
    fader(0, 1); CHECK(smc_ui_active()); enc_acc[EN_SELECT] = 1; input(); CHECK(!smc_ui_active());
    int page = cur_page(); now_ms += 1500; ui_frame(); CHECK(cur_page() == page);
    fader(0, 2); CHECK(smc_ui_active()); held_keys = 1; input(); CHECK(!smc_ui_active());
    held_keys = 0; input();
    fader(0, 3); CHECK(smc_ui_active());
    message(0x90, 0x2F, 127); /* SMC SEQ navigation */
    CHECK(!smc_ui_active());
    puts("PASS navigation: physical buttons, encoder, key and SMC navigation cancel");
}
static void test_overlays(void)
{
    setup(V_SEQ, PART_303A);
    fader(0, 0); CHECK(smc_ui_active());
    ask(ACT_SONG_CLR, 3, "TEST CONFIRM", "KEEP STATE");
    screen_state_t saved = screen_state();
    CHECK(!smc_ui_active()); fader(0, 127); knob(0, 1); CHECK(!smc_ui_active());
    now_ms += 1500; ui_frame(); unchanged(saved);
    setup(V_SEQ, PART_303A);
    message(0x90, 5, 127); CHECK(ui.overlay == O_LIST);
    ui.list_sel = ui.list_n > 3 ? 3 : 0; ui.list_top = 1;
    saved = screen_state(); fader(0, 0); unchanged(saved);
    now_ms += 1500; ui_frame(); unchanged(saved);
    fader(0, 1); CHECK(smc_ui_active());
    message(0x90, 0x61, 127); CHECK(ui.overlay == O_LIST && !smc_ui_active());
    saved = screen_state(); now_ms += 1500; ui_frame(); unchanged(saved);
    setup(V_SEQ, PART_303A); fader(0, 0); CHECK(smc_ui_active());
    ui.help = 1; held_btn = ui.btn = 1u << (ui.help - 1);
    CHECK(!smc_ui_active()); saved = screen_state();
    fader(0, 0); knob(0, 1); unchanged(saved);
    now_ms += 1500; ui_frame(); unchanged(saved);
    puts("PASS overlays: confirmation blocks input; menu/help state survives activity and timeout");
}
static void test_clip(void)
{
    for (int waiting = 0; waiting < 2; waiting++) {
        setup(V_SEQ, PART_303A); clip_grid_mode = 1; clip_waiting = (uint8_t)waiting;
        smk_clip_lcd_force(); ui_frame(); memcpy(before_fb, fb, sizeof fb);
        screen_state_t before = screen_state(); unsigned forces = clip_forces;
        fader(0, 0); CHECK(smc_ui_active()); ui_frame(); unchanged(before);
        CHECK(memcmp(before_fb, fb, sizeof fb));
        shot(waiting ? "06-clip-wait-smc" : "05-clip-grid-smc");
        expire(); unchanged(before); CHECK(!memcmp(before_fb, fb, sizeof fb));
        CHECK(clip_forces > forces);
    }
    setup(V_SEQ, PART_303A); clip_grid_mode = 1; ck.pg = 1;
    screen_state_t before = screen_state();
    knob(0, 1); CHECK(fields[CK_PAGE_FIELDS[0][0]] == 1); CHECK(smc_ui_active());
    ui_frame(); unchanged(before); shot("07-clip-field");
    expire(); unchanged(before);
    puts("PASS clip: grid/waiting framebuffer restores; actual extracted CK turn shows full field UI");
}
static void test_actions_and_reset(void)
{
    setup(V_HOME, PART_303A);
    strcpy(ui.msg, "PRIOR MESSAGE"); ui.msg_until = now_ms + 10000;
    screen_state_t before = screen_state();
    message(0x90, 16, 127); CHECK(smc_ui_active()); CHECK(part_muted(0));
    unchanged(before); ui_frame(); shot("08-mute-action");
    CHECK(!strcmp(ui.msg, "PRIOR MESSAGE"));
    now_ms += 5000; ui_frame(); CHECK(smc_ui_active()); unchanged(before);
    message(0x80, 16, 0); /* matched release starts the inactivity grace */
    now_ms += 1199; ui_frame(); CHECK(smc_ui_active()); unchanged(before);
    now_ms++; ui_frame(); CHECK(!smc_ui_active()); unchanged(before);
    message(0x90, 8, 127); CHECK(smc_ui_active()); CHECK(smc.solo & 1);
    ui_frame(); memcpy(before_fb, fb, sizeof fb);
    message(0x80, 8, 0); CHECK(!(smc.solo & 1));
    ui_frame(); CHECK(!smc_ui_active() || memcmp(before_fb, fb, sizeof fb));
    setup(V_SEQ, PART_909); ui.sel[0] = 8; build_pages();
    for (int strip = 3; strip < 8; strip++) {
        fader(strip, 127); CHECK(!smc_ui_active());
        for (int row = 0; row < 3; row++) {
            message(0x90, row * 8 + strip, 127); CHECK(!smc_ui_active());
        }
    }
    setup(V_PART, PART_303A); fader(0, 0); CHECK(smc_ui_active());
    ui_init(); CHECK(!smc_ui_active());
    fader(0, 1); CHECK(smc_ui_active());
    CHECK(ui_project_begin() == 0); CHECK(!smc_ui_active());
    fader(0, 0); CHECK(!smc_ui_active()); ui_project_end(0);
    fader(0, 2); CHECK(smc_ui_active()); smc_project_reset(); CHECK(!smc_ui_active());
    puts("PASS actions/reset: mute+Solo, inactive strips, init/project lock/reset");
}
static void test_navigation_replay(void)
{
    setup(V_HOME, PART_303A);
    fader(0, 0); CHECK(smc_ui_active());
    uint32_t until = smc_ui.until;
    now_ms += 500; fader(0, 0);
    CHECK(smc_ui.until == until); /* duplicate does not extend inactivity */
    held_btn = 1u << B_FX; input(); held_btn = 0; input();
    CHECK(!smc_ui_active());
    smc_frame(); CHECK(!smc_ui_active());
    fader(0, 0); CHECK(!smc_ui_active());
    for (int i = 0; i < 12; i++) {
        enc_acc[EN_SELECT] = 1; input(); smc_frame();
        fader(0, 0); CHECK(!smc_ui_active());
    }
    fader(0, 1); CHECK(smc_ui_active()); CHECK(!(smc.picked & 1));
    until = smc_ui.until;
    now_ms++; message(0xB0, 16, 0); message(0xB0, 16, 64);
    CHECK(smc_ui.until == until); /* zero relative packets are not activity */
    int target = smc_scaled(smc_target(0));
    fader(0, target); CHECK(smc.picked & 1); CHECK(smc_ui_active());
    until = smc_ui.until;
    now_ms++; fader(0, target); CHECK(smc_ui.until == until);
    expire(); fader(0, target); CHECK(!smc_ui_active());
    knob(0, 1); CHECK(smc_ui_active());
    setup(V_PART, PART_909); fader(0, 0); CHECK(smc_ui_active());
    held_btn = 1u << B_SEQ; input(); held_btn = 0; input();
    smc_frame(); CHECK(!smc_ui_active());
    fader(0, 0); CHECK(!smc_ui_active()); CHECK(!(smc.picked & 1));
    fader(0, 1); CHECK(smc_ui_active());
    setup(V_HOME, PART_303A); fader(0, 0); CHECK(smc_ui_active());
    ask(ACT_SONG_CLR, 0, "TEST", 0); smc_poll(); CHECK(!smc_ui_active());
    message(0x90, 6, 127); CHECK(ui.overlay == O_NONE);
    fader(0, 0); CHECK(!smc_ui_active());
    fader(0, 1); CHECK(smc_ui_active());
    puts("PASS provenance: rapid/idle navigation, duplicate/no-change/zero-delta silence, real movement and pickup");
}
static void check_distinct(void)
{
    for (int n = 0; n < 8; n++) {
        pref_t k = smc_knob_target(n);
        if (k.kind == R_NONE) continue;
        for (int i = 0; i < 8; i++) CHECK(!smc_same(k, smc_target(i)));
        for (int i = 0; i < n; i++) CHECK(!smc_same(k, smc_knob_target(i)));
    }
}
static void test_distinct_maps(void)
{
    setup(V_HOME, PART_303A);
    for (int n = 0; n < 8; n++) CHECK(smc_same(smc_target(n), smc_home_target(n)));
    for (int n = 0; n < NPARTS; n++) CHECK(smc_same(smc_knob_target(n), PR(R_ENG,T_MIX,n==2?4:n==4?2:n,MX_CUT)));
    CHECK(smc_same(smc_knob_target(5), PR(R_ENG,T_FX,0,FX_RV_DECAY)));
    CHECK(smc_same(smc_knob_target(6), PR(R_ENG,T_FX,0,FX_DL_FDBK)));
    CHECK(smc_same(smc_knob_target(7), PR(R_ENG,T_MST,0,MST_RESO)));
    knob(0, 1); ui_frame(); shot("09-part-pan");
    for (int kit = 0; kit < 2; kit++) for (int view = 0; view < 2; view++) {
        setup(view ? V_SEQ : V_MIX, kit);
        if (!view) *page_of(V_MIX) = kit + 1;
        for (int bank = 0; bank < 2; bank++) {
            ui.sel[kit] = bank * 8; build_pages(); check_distinct();
            for (int n = 0; n < 8; n++) {
                pref_t f = smc_target(n), k = smc_knob_target(n);
                if (bank && n >= 3) { CHECK(f.kind == R_NONE && k.kind == R_NONE); continue; }
                CHECK(smc_same(f, find_ref(kit ? T_808 : T_909, bank*8+n, "Level")));
                CHECK(smc_same(k, find_ref(kit ? T_808 : T_909, bank*8+n, "Pan")));
            }
            knob(0, 1); ui_frame();
            if (kit && bank && view) shot("10-808-seq-bank2-pan");
        }
    }
    const int views[] = {V_HOME,V_PART,V_GEN,V_FX,V_MIX,V_GLO,V_SONG,V_SEQ};
    for (unsigned v = 0; v < sizeof views/sizeof *views; v++) for (int part = 0; part < NPARTS; part++) {
        setup(views[v], part);
        for (int page = 0; page < pg.n; page++) {
            *page_of(ui.view) = page; build_pages(); check_distinct();
        }
    }
    setup(V_GLO, PART_303A); CHECK(smc_same(smc_knob_target(2), PR(R_ENG,T_MST,0,MST_RESO)));
    setup(V_MIX, PART_303A); *page_of(V_MIX) = 3; build_pages();
    CHECK(smc_same(smc_knob_target(3), PR(R_ENG,T_MST,0,MST_RESO)));
    /* Cross-strip duplicates must park, without substituting an unrelated field. */
    ui.overlay = O_LIST; ui.list_top = 0; ui.list_n = 6; put_s(list_title, "MASTER");
    for (int n = 0; n < 6; n++) list_rows[n] = smc_home_target((n + 3) % 8);
    check_distinct();
    for (int n = 0; n < 6; n++) CHECK(smc_knob_target(n).kind == R_NONE);
    fader(0, 0); /* Show parked encoder rows after another genuine knob on list. */
    list_rows[5] = PR(R_ENG,T_MST,0,MST_RESO); knob(5, 1); ui_frame(); shot("11-list-on-fader");
    int on_fader = pref_get(list_rows[0]); knob(0, 1); CHECK(pref_get(list_rows[0]) == on_fader);
    CHECK(smc_ui_active() && smc_ui.strip == 0); ui_frame(); shot("12-parked-knob");
    puts("PASS maps: exact separate dimensions, both kits/banks, all production pages and cross-strip list parking");
}

/* Sound-page knobs: matching names keep the same relative slots across drums. */
static int track_slot_of(int t, int v, const char *name)
{
    int page, k, slot = 0;
    setup(V_PART, t == T_909 ? PART_909 : PART_808);
    ui.sel[t == T_909 ? 0 : 1] = (uint8_t)v;
    build_pages();
    for (page = 0; page < pg.n; page++) {
        if (pg.group[page] != GR_TRACK) continue;
        for (k = 0; k < 4; k++) {
            pref_t r = pg.r[page][k];
            if (r.kind == R_NONE) continue;
            if (name_is(engine_param(r.a, r.b, r.c), name)) return slot;
            slot++;
        }
    }
    return -1;
}
static void test_track_knob_align(void)
{
    static const char *const names[] = {
        "Tune", "Tone", "Attack", "Sound", "Decay", "Snappy", "Tail", "P.Dpth", "Pitch",
    };
    int kit, v, i, j;
    for (kit = 0; kit < 2; kit++) {
        int t = kit ? T_808 : T_909;
        for (v = 0; v < NDRUM; v++) {
            int prev = -1, page, k;
            setup(V_PART, kit ? PART_808 : PART_909);
            ui.sel[kit] = (uint8_t)v;
            build_pages();
            for (page = 0; page < pg.n; page++) {
                int hole = 0;
                if (pg.group[page] != GR_TRACK) continue;
                for (k = 0; k < 4; k++) {
                    pref_t r = pg.r[page][k];
                    int rank = 100, n;
                    if (r.kind == R_NONE) { hole = 1; continue; }
                    CHECK(!hole);                     /* no empty knob between filled ones */
                    for (n = 0; n < (int)(sizeof names / sizeof *names); n++)
                        if (name_is(engine_param(r.a, r.b, r.c), names[n])) {
                            rank = n;
                            break;
                        }
                    CHECK(rank >= prev);
                    prev = rank;
                }
            }
            for (i = 0; i < (int)(sizeof names / sizeof *names); i++) {
                int a = track_slot_of(t, v, names[i]);
                if (a < 0) continue;
                for (j = i + 1; j < (int)(sizeof names / sizeof *names); j++) {
                    int b = track_slot_of(t, v, names[j]);
                    if (b >= 0) CHECK(a < b);
                }
            }
        }
    }
    /* Hats previously put Decay on K1; Tune must lead when both exist. */
    CHECK(track_slot_of(T_909, 8, "Tune") == 0);
    CHECK(track_slot_of(T_909, 8, "Decay") == 1);
    CHECK(track_slot_of(T_909, 9, "Tune") == 0);
    CHECK(track_slot_of(T_808, 10, "Tune") == 0);
    CHECK(track_slot_of(T_808, 10, "Decay") == 1);
    /* Owner-approved Chicago B primary row: Cutoff / Reso / EnvMod / Wave.
     * Feature-on factory boot can be Miami, so establish engine identity first. */
    setup(V_PART, PART_303A);
#if X0X_BASS_PRESET_PREVIEW
    engine_bass_off(0); engine_bass_off(1);
    CHECK(engine_bass_select_preset(0, FACTORY_BANK_CHICAGO, 0, &proj.sound) == 0);
    for (int n = 0; n < 1024 && engine_bass_preset_pending(0); n++) {
        int32_t audio[512]; engine_render(audio, 256);
    }
    CHECK(!engine_bass_preset_pending(0));
    CHECK(engine_bass_bank(0) == FACTORY_BANK_CHICAGO);
    build_pages();
#endif
    int tone = -1;
    for (int page = 0; page < pg.n; page++)
        if (smc_same(pg.r[page][0], PR(R_ENG, T_303, 0, BASS303_CUTOFF))) tone = page;
    CHECK(tone == 0); /* selected B primary physical row */
    CHECK(smc_same(pg.r[tone][0], PR(R_ENG, T_303, 0, BASS303_CUTOFF)));
    CHECK(smc_same(pg.r[tone][1], PR(R_ENG, T_303, 0, BASS303_RESO)));
    CHECK(smc_same(pg.r[tone][2], PR(R_ENG, T_303, 0, BASS303_ENVMOD)));
    CHECK(smc_same(pg.r[tone][3], PR(R_ENG, T_303, 0, BASS303_WAVE)));
#if X0X_BASS_PRESET_PREVIEW
    int sound = -1;
    for (int page = 0; page < pg.n; page++)
        if (pg.r[page][0].kind == R_BENGINE) sound = page;
    CHECK(sound == pg.n - 1); /* SOUND follows the selected B parameter matrix. */
    CHECK(smc_same(pg.r[sound][0], PR(R_BENGINE, 0, 0, 0)));
    CHECK(smc_same(pg.r[sound][1], PR(R_BPRESET, 0, 0, 0)));
    CHECK(pg.r[sound][2].kind == R_NONE); /* former physical K7 cancel is now empty */
#endif
    puts("PASS align: drum Tune/Decay ranks; B TONE / OSC mapping and SOUND identity");
}

/* Verify physical grouping in the real framebuffer, rather than accepting any
 * nonempty screen: eight encoders over eight faders, three M/S/R rows, and
 * SELECT represented only by the separately checked title highlight. */
static void test_physical_geometry(void)
{
    setup(V_HOME, PART_909); fader(0, 0); ui_frame();
    for (int n = 0; n < 8; n++) {
        int x = n * 30;
        CHECK(fb[(MAIN_Y + 36) * 240 + x + 8] != 0); /* encoder top */
        CHECK(fb[(MAIN_Y + 90) * 240 + x + 3] != 0);  /* vertical fader */
        for (int row = 0; row < 3; row++)
            CHECK(fb[(MAIN_Y + 62 + row * 12) * 240 + x + 18] != swap16(C_BLACK));
        CHECK(fb[(MAIN_Y + 98) * 240 + x + 18] == swap16(C_BLACK)); /* no phantom fourth row */
    }
    /* An encoder interaction highlights its own physical position; it cannot
     * replace the other seven strips with one large readout. */
    knob(7, 1); ui_frame();
    CHECK(smc_ui.kind == 1 && smc_ui.strip == 7);
    for (int n = 0; n < 7; n++)
        CHECK(fb[(MAIN_Y + 36) * 240 + n * 30 + 8] != 0);
    shot("13-eight-strip-resonance");
    puts("PASS physical layout: all eight knob/fader positions, three physical rows and no phantom fourth row");
}


static void check_selected(int selected)
{
    for (int n = 0; n < 8; n++) {
        CHECK(fb[MAIN_Y * 240 + n * 30 + 2] == swap16(n == selected ? C_WHITE : C_BLACK));
        CHECK(fb[(MAIN_Y + 98) * 240 + n * 30 + 18] == swap16(C_BLACK)); /* SELECT stays in the title */
    }
}
static void test_selected_lane(void)
{
    setup(V_HOME, PART_303A); knob(7, 1); ui_frame(); check_selected(4);
    CHECK(smc_ui.kind == 1 && smc_ui.strip == 7);
    shot("14-selected-part-touched-filter");
    setup(V_MIX, PART_303A); ui.mixsel = PART_BRK; knob(0, 1); ui_frame(); check_selected(2);
    setup(V_FX, PART_303A); knob(0, 1); ui_frame(); check_selected(5);
    *page_of(V_FX) = 1; build_pages(); knob(1, 1); ui_frame(); check_selected(6);
    setup(V_MIX, PART_303A); *page_of(V_MIX) = 3; build_pages();
    knob(3, 1); ui_frame(); check_selected(7);
    for (int kit = 0; kit < 2; kit++) for (int view = 0; view < 2; view++)
        for (int track = 0; track < NDRUM; track++) {
            setup(view ? V_SEQ : V_MIX, kit);
            if (!view) *page_of(V_MIX) = kit + 1;
            ui.sel[kit] = track; build_pages(); knob(0, 1); ui_frame(); check_selected(track % 8);
        }
    setup(V_MIX, PART_808); *page_of(V_MIX) = 2; ui.sel[1] = 9; build_pages();
    CHECK(smc_lock_toggle()); ui.sel[1] = 10; knob(0, 1); ui_frame(); check_selected(2);
    CHECK(smc_drum_track(1, 0) == 8);
    shot("15-locked-bank-selected-other-drum");
    ui.sel[1] = 0; knob(0, 1); ui_frame(); check_selected(-1);
    shot("16-locked-bank-selection-outside");
    ui.part = PART_909; ui.sel[1] = 9; knob(0, 1); ui_frame(); check_selected(1);
    setup(V_MIX, PART_303A); *page_of(V_MIX) = 2; ui.sel[1] = 4; build_pages();
    knob(0, 1); ui_frame(); check_selected(4);
    puts("PASS lane selection: part/FX/master, both kits/views/banks, touch independence, locked live selection and outside bank");
}


/* Compare the actual native title pixels with independently specified sound
 * names. MIX retains its numbered strips even when ui.part is another kit. */
static void check_drum_titles(int kit, int bank, int names)
{
    static const char *const voices[2][11] = {
        {"BD","SD","LT","MT","HT","RS","CP","CH","OH","CR","RD"},
        {"BD","SD","LT","MT","HT","RS","CP","CB","CY","OH","CH"}
    };
    uint32_t old_hash = blit_hash[1];
    memcpy(active_fb, fb, sizeof fb);
    cv_begin(240, 13, C_BLACK); cv_band(0, 13);
    for (int n = 0; n < 8; n++) {
        char number[8]; int track = bank * 8 + n;
        const char *label = track >= NDRUM ? "--" : names ? voices[kit][track] :
            (put_i(number, track + 1), number);
        int selected = ui.sel[kit] == track;
        if (n) cv_rect(n * 30, 0, 1, 13, C_LINE);
        if (selected) cv_rect(n * 30 + 1, 0, 28, 13, C_WHITE);
        text_c(n * 30 + 15, 1, &FONT_T, label, selected ? C_BLACK : C_WHITE);
        CHECK(tw(&FONT_T, label) <= 27);
    }
    cv_commit_rect(1, 0, MAIN_Y);
    CHECK(!memcmp(active_fb + MAIN_Y * 240, fb + MAIN_Y * 240, 13 * 240 * sizeof *fb));
    memcpy(fb, active_fb, sizeof fb);
    blit_hash[1] = old_hash;
}
static void test_drum_titles(void)
{
    for (int kit = 0; kit < 2; kit++) for (int bank = 0; bank < 2; bank++) {
        setup(V_SEQ, kit); ui.sel[kit] = bank * 8; build_pages();
        knob(0, 1); ui_frame(); check_drum_titles(kit, bank, 1);
        shot(kit ? bank ? "25-808-seq-bank2-names" : "24-808-seq-bank1-names" :
                   bank ? "23-909-seq-bank2-names" : "22-909-seq-bank1-names");
        CHECK(smc_lock_toggle());
        ui.part = 1 - kit; ui.sel[kit] = bank ? 0 : 8; set_view(V_HOME); build_pages();
        knob(0, 1); ui_frame(); check_drum_titles(kit, bank, 1);
        setup(V_MIX, 1 - kit); *page_of(V_MIX) = kit + 1; ui.sel[kit] = bank * 8; build_pages();
        knob(0, 1); ui_frame(); check_drum_titles(kit, bank, 0);
    }
    puts("PASS drum titles: 909/808 SEQ names in both banks, inactive lanes, captured kit/view/bank; MIX numbers unchanged");
}

static void test_action_captions_and_knobs(void)
{
    setup(V_HOME, PART_303A);
    CHECK(!strcmp(smc_button_caption(15), "PART"));
    CHECK(!strcmp(smc_button_caption(7), "HOME"));
    CHECK(!strcmp(smc_button_caption(13), "MIX"));
    CHECK(!strcmp(smc_button_caption(14), "GLOB"));
    CHECK(!strcmp(smc_button_caption(5), "LIST"));
    CHECK(!strcmp(smc_button_caption(6), "SONG"));
    for (int n = 0; n < 24; n++) CHECK(tw(&FONT_T, smc_button_caption(n)) <= 22);
    smc_button_apply(5, 0); CHECK(ui.overlay == O_LIST);
    CHECK(!strcmp(smc_button_caption(5), "OK"));
    CHECK(!strcmp(smc_button_caption(6), "BACK"));
    smc_button_apply(6, 0); CHECK(ui.overlay == O_NONE);
    smc_button_apply(6, 0); CHECK(ui.view == V_SONG);
    CHECK(!strcmp(smc_button_caption(6), "HOME"));
    smc_button_apply(7, 0); CHECK(ui.view == V_HOME);
    knob(7, 1); ui_frame(); shot("17-filter-actual-actions");
    smc_button_apply(15, 0); CHECK(ui.view == V_PART);
    smc_button_apply(7, 0); CHECK(ui.view == V_HOME);
    smc_button_apply(13, 0); CHECK(ui.view == V_MIX);
    smc_button_apply(14, 0); CHECK(ui.view == V_GLO && ui.overlay == O_LIST);
    CHECK(!strcmp(smc_button_caption(14), "BACK"));
    smc_button_apply(14, 0); CHECK(ui.view != V_GLO);
    clip_grid_mode = 1; ui.overlay = O_NONE; ck.pg = 2;
    CHECK(!strcmp(smc_button_caption(6), "GRID"));
    smc_button_apply(6, 0); CHECK(ck.pg == 0);
    setup(V_MIX, PART_909); *page_of(V_MIX) = 1; build_pages();
    CHECK(!strcmp(smc_button_caption(7), "R"));
    CHECK(!strcmp(smc_button_caption(15), "S"));
    int rec = ui.rec; smc_button_apply(7, 0); CHECK(ui.rec != rec && ui.sel[0] == 7);
    smc_button_apply(15, 0); CHECK(smc.drum_solo[0] & (1u << 7));
    ui.sel[0] = 8; build_pages(); CHECK(!strcmp(smc_button_caption(15), "--"));
    CHECK(smc_lock_toggle()); CHECK(!strcmp(smc_button_caption(0), "R"));
    CHECK(!strcmp(smc_button_caption(3), "--")); /* empty captured bank slot stays unavailable */
    knob(0, 1); ui_frame(); shot("18-locked-live-actions");
    setup(V_MIX, PART_909); *page_of(V_MIX) = 1; build_pages();
    pref_t pan = smc_knob_target(0);
    for (int v = 0; v <= 2; v++) {
        pref_set(pan, v == 0 ? 0 : v == 1 ? 64 : 127);
        smc_ui_begin(0, 1); ui_frame();
        CHECK(is_pan(pan)); CHECK(smc_scaled(pan) == (v == 0 ? 0 : v == 1 ? 64 : 127));
        if (v) {
            unsigned pixels = 0;
            for (int y = 28; y < 45; y++) for (int x = 5; x < 26; x++)
                pixels += before_fb[(MAIN_Y+y)*240+x] != fb[(MAIN_Y+y)*240+x];
            CHECK(pixels > 0);
        }
        CHECK(fb[(MAIN_Y + 25) * 240 + 14] == swap16(C_WHITE));
        memcpy(before_fb, fb, sizeof fb);
        shot(v == 0 ? "19-pan-left" : v == 1 ? "20-pan-center" : "21-pan-right");
    }
    setup(V_HOME, PART_303A);
    pref_t cut = smc_knob_target(0);
    CHECK(!is_pan(cut)); CHECK(cut.c == MX_CUT);
    pref_set(cut, 0); smc_ui_begin(0, 1); ui_frame(); memcpy(before_fb, fb, sizeof fb);
    pref_set(cut, 127); smc_ui_begin(0, 1); ui_frame();
    {
        unsigned pixels = 0;
        for (int y = 28; y < 45; y++) for (int x = 5; x < 26; x++)
            pixels += before_fb[(MAIN_Y+y)*240+x] != fb[(MAIN_Y+y)*240+x];
        CHECK(pixels > 0);
    }
    unsigned footer_pixels = 0;
    for (int y = KNOB_Y + 65; y < 240; y++) for (int x = 0; x < 240; x++)
        footer_pixels += fb[y * 240 + x] != swap16(C_BLACK);
    CHECK(!footer_pixels);
    puts("PASS action captions: filter PART/HOME/MIX/MENU, drum solo/record dispatch, inactive/locked; knob arc extremes and bipolar center");
}

#ifndef SMC_UI_MAIN
#define SMC_UI_MAIN main
#endif
int SMC_UI_MAIN(void)
{
    test_navigation_replay(); test_distinct_maps(); test_track_knob_align(); test_physical_geometry(); test_selected_lane(); test_drum_titles(); test_action_captions_and_knobs();
    test_restore(); puts("PASS restore: eight views preserve complete navigation state and exact framebuffer");
    test_activity(); test_existing_feedback(); test_navigation(); test_overlays(); test_clip(); test_actions_and_reset();
    printf("smc_ui: PASS (%u checks)\n", checks);
    return 0;
}
