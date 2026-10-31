/* SPDX-License-Identifier: GPL-3.0-only */
/* Chicago ENVELOPE row graph: A/D/S/R sweep over the production UI and framebuffer.
 * Built through tests/host/smc_ui_test.sh with SMC_UI_TEST_FILE. No device I/O. */
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
static int env_page(void)
{
    for (int p = 0; p < pg.n; p++) if (!strcmp(pg.title[p], "ENVELOPE")) return p;
    return -1;
}
static void set_env(int a, int d, int s, int r)
{
    int p = env_page(), v[4] = {a, d, s, r};
    for (int k = 0; k < 4; k++) pref_set(pg.r[p][k], v[k]);
}
/* Panel geometry from draw_part_sound: four 49 px rows on a 50 px step. */
enum { ROW_Y = 4 + 3 * 50, ROW_H = 49, GX0 = 13, GW = 214 };
static void layout(int xs[5], float dur[3], ui_adsr_t *e)
{
    adsr_values(e, env_page());
    adsr_layout(e, GX0, GW, xs, dur);
}
static int curve_y(int x, uint16_t c)  /* topmost curve pixel in a column, or -1 */
{
    for (int y = ROW_Y + 14; y <= ROW_Y + ROW_H - 5; y++)
        if (fb[(MAIN_Y + y) * 240 + x] == swap16(c)) return y;
    return -1;
}
static void render_check(int a, int d, int s, int r)
{
    int xs[5]; float dur[3]; ui_adsr_t e;
    set_env(a, d, s, r);
    ui.touched = -1; now_ms += 50; ui_frame();
    layout(xs, dur, &e);
    uint16_t c = matrix_col();
    int top = ROW_Y + 14, y0 = ROW_Y + ROW_H - 5;
    /* Layout: ordered, every timed stage visible, fixed right edge. */
    CHECK(xs[0] == GX0 && xs[4] == GX0 + GW);
    for (int k = 0; k < 4; k++) CHECK(xs[k + 1] > xs[k]);
    CHECK(xs[1] - xs[0] >= 3 && xs[2] - xs[1] >= 3 && xs[4] - xs[3] >= 3);
    /* No curve pixel outside the graph box inside the row panel. */
    for (int y = ROW_Y; y < ROW_Y + ROW_H; y++) for (int x = 0; x < 240; x++)
        if (fb[(MAIN_Y + y) * 240 + x] == swap16(c))
            CHECK(x >= GX0 && x <= GX0 + GW && y >= top && y <= y0);
    /* Monotonic per stage on the drawn framebuffer. */
    int prev = -1;
    for (int x = xs[0] + 1; x <= xs[1]; x++) { int y = curve_y(x, c); if (y >= 0) { if (prev >= 0) CHECK(y <= prev); prev = y; } }
    prev = -1;
    for (int x = xs[1] + 1; x <= xs[2]; x++) { int y = curve_y(x, c); if (y >= 0) { if (prev >= 0) CHECK(y >= prev); prev = y; } }
    prev = -1;
    for (int x = xs[3] + 1; x <= xs[4]; x++) { int y = curve_y(x, c); if (y >= 0) { if (prev >= 0) CHECK(y >= prev); prev = y; } }
    /* Sustain plateau height tracks S. */
    int ys = y0 - (int)(e.sustain * (float)(y0 - top) + 0.5f);
    CHECK(fabsf(e.sustain - (float)s / 127.0f) < 1e-3f);   /* native 34, 1001-step map */
    for (int x = xs[2] + 2; x < xs[3] - 1; x++) CHECK(fb[(MAIN_Y + ys) * 240 + x] == swap16(c));
}
static void shot_env(const char *name, int a, int d, int s, int r)
{
    set_env(a, d, s, r); ui.touched = -1; now_ms += 50; ui_frame(); shot(name);
}
int main(void)
{
    setup(V_PART, PART_303B);
    CHECK(engine_bass_sound_bank(&proj.sound, bidx()) == ORIGINAL_SOUND_CHICAGO);
    int p = env_page(); CHECK(p >= 0);
    *page_of(V_PART) = (uint8_t)p; build_pages(); ui_frame();
    CHECK(adsr_row(p));
    static const int v[] = {0, 1, 16, 32, 48, 64, 80, 96, 112, 126, 127};
    enum { NV = sizeof v / sizeof *v };
    for (int i = 0; i < NV; i++) for (int j = 0; j < NV; j += 2)
        for (int k = 0; k < NV; k += 3) for (int l = 0; l < NV; l += 2) render_check(v[i], v[j], v[k], v[l]);
    for (int stage = 0; stage < 3; stage++) {   /* widths grow with A, D and R */
        int last = -1;
        for (int x = 0; x <= 127; x++) {
            int xs[5]; float dur[3]; ui_adsr_t e;
            set_env(stage == 0 ? x : 40, stage == 1 ? x : 40, 80, stage == 2 ? x : 40);
            layout(xs, dur, &e);
            int w = stage == 0 ? xs[1] - xs[0] : stage == 1 ? xs[2] - xs[1] : xs[4] - xs[3];
            CHECK(w >= last); last = w;
        }
        CHECK(last > 3);
    }
    shot_env("all-min", 0, 0, 0, 0);
    shot_env("all-max", 127, 127, 127, 127);
    shot_env("attack-max", 127, 0, 0, 0);
    shot_env("decay-max", 0, 127, 0, 0);
    shot_env("release-max", 0, 0, 64, 127);
    shot_env("sustain-0", 40, 60, 0, 60);
    shot_env("sustain-127", 40, 60, 127, 60);
    shot_env("typical-mid", 34, 51, 73, 22);
    {   /* inactive row: the graph still replaces the A/D/S/R boxes */
        set_env(40, 60, 80, 40);
        *page_of(V_PART) = 0; build_pages(); ui.touched = -1; now_ms += 50; ui_frame();
        CHECK(adsr_row(p) && cur_page() != p);
        int lit = 0, xs[5]; float dur[3]; ui_adsr_t e; layout(xs, dur, &e);
        int top = ROW_Y + 14, y0 = ROW_Y + ROW_H - 5;
        int ys = y0 - (int)(e.sustain * (float)(y0 - top) + 0.5f);
        for (int x = xs[2] + 2; x < xs[3] - 1; x++) lit += fb[(MAIN_Y + ys) * 240 + x] == swap16(SOUND_WHITE);
        CHECK(lit >= xs[3] - xs[2] - 4);
    }
    {   /* a note runs a playhead over the active graph */
        *page_of(V_PART) = (uint8_t)p; build_pages(); ui.touched = -1; now_ms += 50; ui_frame();
        uint16_t snap[240 * 240]; memcpy(snap, fb, sizeof fb);
        engine_bass_on_velocity(bidx(), 48, 0, 0, 120);
        for (int i = 0; i < 3; i++) { int32_t a[512]; engine_render(a, 256); }
        now_ms += 50; ui_frame();
        float adsr[4], lv; int st; CHECK(engine_bass_env_view(bidx(), adsr, &lv, &st) && st != 6);
        int diff = 0;
        for (int y = ROW_Y; y < ROW_Y + ROW_H; y++) for (int x = 0; x < 240; x++)
            diff += fb[(MAIN_Y + y) * 240 + x] != snap[(MAIN_Y + y) * 240 + x];
        CHECK(diff > 0);
        engine_bass_off(bidx());
    }
    printf("chicago_adsr_graph: PASS (%u checks)\n", checks);
    return 0;
}
