/* SPDX-License-Identifier: GPL-3.0-only */
/* SMC-07 mapped-page peek / hold / restore beside the SMC control overlay. */
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

static void message(int st, int a, int b)
{
    uint8_t msg[3] = {(uint8_t)st, (uint8_t)a, (uint8_t)b};
    now_ms += 100;
    smc_ble_midi(msg, 3);
    smc_poll();
    smc_frame();
}
static void fader(int n, int v) { message(0xE0 + n, 0, v); }
static void turn(int n, int delta) { message(0xB0, 16 + n, delta > 0 ? delta : 64 - delta); }
static void press(int note) { message(0x90, note, 127); message(0x80, note, 0); }

static void setup(void)
{
    held_btn = held_keys = 0;
    memset(&ui, 0, sizeof ui);
    smc_slot = 0;
    memset(smc_states, 0, sizeof smc_states);
    memset(smc_inputs, 0, sizeof smc_inputs);
    smc_ui_cancel();
    clip_grid_mode = 0;
    ck.pg = 0;
    memset(fields, 0, sizeof fields);
    memset(smc_peek, 0, sizeof *smc_peek);
    boot();
    now_ms = 1000;
    proj.set.autosave_off = 1;
    proj.set.bt = PROJ_BT_SMC_CENTRAL;
    smc_ble_link(1);
    smc_poll();
    /* A synth SEQ page keeps the faders on the main-mixer home targets. */
    ui.part = PART_303A;
    set_view(V_SEQ);
    build_pages();
    *page_of(V_SEQ) = 0;
    ui.mixsel = 2;
    ui.prev_view = V_HOME;
}
static void assert_peek(uint8_t view, int page, uint8_t mixsel)
{
    assert(smc_peek->on);
    assert(ui.view == view);
    assert(cur_page() == page);
    if (view == V_MIX && page == 0)
        assert(ui.mixsel == mixsel);
}

static void advance(uint32_t ms)
{
    now_ms += ms;
    smc_poll();
    smc_frame();
}

int main(void)
{
    int master_page, saved_prev;
    uint8_t saved_mix;

    /* Strip 1–5 → MIX PARTS with mixsel = strip; hold refresh; restore. */
    setup();
    saved_mix = ui.mixsel;
    saved_prev = ui.prev_view;
    assert(ui.view == V_SEQ);
    fader(0, 0);                      /* waiting pickup still peeks */
    assert(smc_ui_active());       /* the control overlay shows */
    assert_peek(V_MIX, 0, 0);
    assert(smc_peek->saved_view == V_SEQ);
    assert(smc_peek->saved_mixsel == saved_mix);
    assert(smc_peek->saved_prev == saved_prev);
    {
        uint32_t until = smc_peek->until;
        now_ms += 400;
        fader(0, 10);                 /* same peek refreshes hold */
        assert(smc_peek->on && smc_peek->until > until);
        assert_peek(V_MIX, 0, 0);
    }
    fader(2, 0);                      /* switch strip keeps original restore point */
    assert_peek(V_MIX, 0, 2);
    assert(smc_peek->saved_view == V_SEQ);
    advance(SMC_PEEK_MS + 50);
    assert(!smc_peek->on);
    assert(ui.view == V_SEQ && ui.mixsel == saved_mix && ui.prev_view == saved_prev);

    /* Strip 6 → FX REVERB; strip 7 → FX DELAY; strip 8 → MIX MASTER. */
    setup();
    fader(5, 0);
    assert_peek(V_FX, 0, ui.mixsel);
    advance(SMC_PEEK_MS + 50);
    assert(ui.view == V_SEQ);
    fader(6, 0);
    assert_peek(V_FX, 1, ui.mixsel);
    advance(SMC_PEEK_MS + 50);
    assert(ui.view == V_SEQ);
    set_view(V_MIX);
    build_pages();
    for (master_page = 0; master_page < pg.n && strcmp(pg.title[master_page], "MASTER"); master_page++) {}
    assert(master_page == SMC_PEEK_MASTER);
    set_view(V_SONG);
    build_pages();
    fader(7, 0);
    assert_peek(V_MIX, master_page, ui.mixsel);
    assert(!strcmp(pg.title[cur_page()], "MASTER"));
    advance(SMC_PEEK_MS + 50);
    assert(ui.view == V_SONG && !smc_peek->on);

    /* HOME knobs edit CUT/PAN/REV/DLY banks, not the home strip targets:
     * the overlay shows, no peek. */
    setup();
    set_view(V_HOME);
    build_pages();
    turn(0, 1);
    assert(smc_ui_active());
    assert(!smc_peek->on && ui.view == V_HOME);

    /* Guards: help, ASK, clip, perf — no peek; header may still update where allowed. */
    setup();
    ui.help = 1;
    fader(0, 127);
    assert(!smc_peek->on && ui.view == V_SEQ);
    ui.help = 0;

    setup();
    ask(ACT_SONG_CLR, 0, "TEST", 0);
    assert(ui.overlay == O_ASK);
    fader(0, 127);
    assert(!smc_peek->on && ui.overlay == O_ASK && ui.view == V_SEQ);
    press(6);                         /* cancel */

    setup();
    press(5);                         /* deep menu on SEQ */
    assert(ui.overlay == O_LIST);
    fader(0, 0);
    assert(!smc_peek->on && ui.overlay == O_LIST && ui.view == V_SEQ);
    press(6);

    setup();
    clip_grid_mode = 1;
    fader(0, 0);
    assert(!smc_peek->on && ui.view == V_SEQ);
    assert(smc_ui_active());       /* the control overlay shows */
    clip_grid_mode = 0;

    setup();
    {
        int saved_test = perf.test, saved_phase = perf.phase;
        perf.test = 0;
        perf.phase = 0;
        assert(perf_testing());
        fader(0, 127);
        assert(!smc_peek->on && ui.view == V_SEQ);
        perf.test = saved_test;
        perf.phase = saved_phase;
    }

    /* Already on mapped page: no nested peek save. */
    setup();
    set_view(V_MIX);
    ui.mixsel = 1;
    build_pages();
    *page_of(V_MIX) = 0;
    fader(1, 0);
    assert(!smc_peek->on);
    assert(ui.view == V_MIX && cur_page() == 0 && ui.mixsel == 1);

    /* Navigation while peeking abandons restore. */
    setup();
    fader(0, 0);
    assert_peek(V_MIX, 0, 0);
    press(0x61);                      /* Down: leave MIX via smc_view */
    assert(!smc_peek->on);
    assert(ui.view != V_SEQ);         /* navigated forward, not restored to SEQ */

    /* The control overlay stays beside the peek and in menus. */
    setup();
    fader(0, 0);
    assert_peek(V_MIX, 0, 0);
    assert(smc_ui_active());
    setup();
    press(5);
    assert(ui.overlay == O_LIST);
    fader(0, 0);
    assert(!smc_peek->on && smc_ui_active());
    press(6);
    setup();
    clip_grid_mode = 1;
    fader(0, 0);
    assert(!smc_peek->on && smc_ui_active());
    clip_grid_mode = 0;

    /* Overlay expiry alone does not drop the restore. */
    setup();
    fader(0, 0);
    smc_ui_off();
    assert(smc_peek->on);
    advance(SMC_PEEK_MS + 50);
    assert(ui.view == V_SEQ && !smc_peek->on);

    /* Navigation edge (smc_ui_cancel) drops the restore. */
    setup();
    fader(0, 0);
    smc_ui_cancel();
    assert(!smc_peek->on && ui.view == V_MIX);

    /* Two peers: the first peer owns the peek. */
    setup();
    smc_ble_link_slot(1, 1);
    smc_poll();
    fader(0, 0);
    assert(smc_peek->on && smc_peek->peer == 0);
    {
        uint8_t m[3] = {0xE5, 0, 0};
        now_ms += 100;
        smc_ble_midi_slot(1, m, 3);
        smc_poll();
    }
    assert_peek(V_MIX, 0, 0);
    advance(SMC_PEEK_MS + 50);
    assert(ui.view == V_SEQ && !smc_peek->on);

    /* SMC lost cancels without restore. */
    setup();
    fader(0, 0);
    smc_ble_link(0);
    smc_poll();
    assert(!smc_peek->on && ui.view == V_MIX);

    puts("PASS: SMC UI peek map, hold refresh, restore and guards");
    return 0;
}
