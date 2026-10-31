/* SPDX-License-Identifier: GPL-3.0-only */
/* Default build: SMC input shows only its own pop-up overlay and never
 * changes the keyboard view, page, part or mixer selection. */
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

static void assert_keyboard(uint8_t view, int page, uint8_t mixsel, uint8_t part)
{
    assert(!smc_peek->on);
    assert(ui.view == view && cur_page() == page);
    assert(ui.mixsel == mixsel && ui.part == part);
}

int main(void)
{
    setup();
    uint8_t v = ui.view, mix = ui.mixsel, part = ui.part;
    int page = cur_page();
    for (int n = 0; n < 8; n++) {     /* all faders: overlay, keyboard kept */
        fader(n, 0);
        assert(smc_ui_active());
        assert_keyboard(v, page, mix, part);
        advance(2000);
        assert(!smc_ui_active());
        assert_keyboard(v, page, mix, part);
    }
    int shown = 0;
    for (int n = 0; n < 8; n++) {     /* all knobs (empty cells stay silent) */
        turn(n, 1);
        shown += smc_ui_active();
        assert_keyboard(v, page, mix, part);
        advance(2000);
    }
    set_view(V_HOME); build_pages(); smc_poll();
    for (int n = 0; n < 8; n++) {     /* HOME knobs used to peek */
        turn(n, 1);
        shown += smc_ui_active();
        assert_keyboard(V_HOME, cur_page(), mix, part);
        advance(2000);
    }
    assert(shown >= 8);
    puts("PASS smc own popup: faders/knobs never change the keyboard view/page");
    return 0;
}
