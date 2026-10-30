/* SPDX-License-Identifier: GPL-3.0-only */
/* QOL-SAVE-01: real incoming MIDI, panel SAVE edges and ordinary UI frames.
 * The host store counts writes; this does not measure physical flash/audio. */
#define main x0x_host_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>

/* Disconnected-SMC/clip-input substitutes follow the maintained rec_live fixture.
 * Production UI, MIDI ingestion, engine and project storage run unchanged. */
#if BOARD_SMK37
static uint8_t test_clip_mode, test_clip_busy;
uint8_t smk_clip_mode_on(void) { return test_clip_mode; }
uint8_t smk_padcal_on(void) { return 0; }
uint8_t smk_padcal_draw(uint32_t w, uint32_t h) { (void)w; (void)h; return 0; }
void smk_padcal_enter(void) {}
uint8_t smk_clip_lcd_draw(uint32_t w, uint32_t h) { (void)w; (void)h; return 0; }
void smk_clip_lcd_force(void) {}
uint8_t smk_clip_wait(void) { return 0; }
uint8_t smk_ck_ext(void) { return 0; }
uint8_t smk_ck_flash_busy(void) { return test_clip_busy; }
uint8_t smk_ck_knobs_owned(void) { return 0; }
uint8_t smk_ck_panel(uint32_t *last, uint32_t force) { (void)last; (void)force; return 0; }
uint8_t smk_clip_history_button(uint8_t b, uint8_t p, uint8_t h) { (void)b; (void)p; (void)h; return 0; }
uint8_t smk_clip_transport_button(uint8_t b, uint8_t p) { (void)b; (void)p; return 0; }
void smk_play_note_button(uint8_t p, uint8_t h) { (void)p; (void)h; }
void smk_apc_transport_led_set(uint8_t n, uint8_t v) { (void)n; (void)v; }
uint8_t smk_apc_transport_led_state(uint8_t n) { (void)n; return 0; }
void smk_apc_clock_tick(void) {}
void smk_apc_clock_start(void) {}
void smk_apc_clock_position(uint32_t s) { (void)s; }
void smk_ck_midi_stop(void) {}
int smc_connected(void) { return 0; }
int smc_project_active(void) { return 0; }
void smc_project_reset(void) {}
void smc_frame(void) {}
void smc_poll(void) {}
void smc_pad_diag_boundary(void) {} /* disconnected SMC fixture */
int smc_midi_host(uint32_t p) { (void)p; return 0; }
void smc_ui_cancel(void) {}
int smc_ui_active(void) { return 0; }
void smc_ui_draw(void) {}
void smc_ui_clip_show(int n, const char *name, const char *value) { (void)n; (void)name; (void)value; }
#endif

static unsigned checks, failures;
#define CHECK(c, msg) do { checks++; if (!(c)) { failures++; printf("FAIL: %s\n", msg); } } while (0)
static int32_t block[512];
static void audio(void) { engine_render(block, 256); }
static void midi_note(unsigned src, int down)
{
    min_q[mi_w++ % MQ] = (down ? 9u : 8u) | (src << 4) |
        ((uint32_t)((down ? 0x90u : 0x80u) | seq.ch[TRK_BASS0]) << 8) |
        (48u << 16) | (down ? 100u << 24 : 0u);
    audio();
}
static void save_button(void)
{
    held_btn = 1u << B_SAVE; ui_input_only();
    held_btn = 0; ui_input_only();
}
static void fixture(void)
{
    held_btn = held_keys = mi_r = mi_w = 0;
    boot(); audio(); engine_guard_enable(0);
    assert(!seq.playing && !ui.rec && !engine_project_notes());
    assert(project_save() == 0);
    proj.sound.v[T_303][0][BASS303_CUTOFF] ^= 1;
    mark_dirty();
}
static void quiet_frame(void)
{
    now_ms += AUTOSAVE_QUIET + AUTOSAVE_GAP + 1;
    ui_frame();
}
static void release(unsigned src)
{
    midi_note(src, 0);
    for (unsigned k = 0; k < 3000 && engine_project_notes(); k++) audio();
    assert(!engine_project_notes());
}
static void held_note(unsigned src, int manual)
{
    fixture();
    midi_note(src, 1);
    assert(engine_project_notes() && (engine_midi_held() & 1));
    unsigned writes = store_writes;
    if (manual) {
        save_button(); save_button();
        CHECK(save_wait && ui.dirty, "repeated SAVE stays queued and dirty while a MIDI note is held");
        CHECK(!strcmp(ui.msg, "SAVES WHEN QUIET"), "SAVE names the quiet condition rather than silently returning BUSY");
        CHECK(!strcmp(queued_save_label(), "SAVE WHEN QUIET"), "stopped held-note queue does not ask the player to stop again");
    }
    quiet_frame();
    CHECK(store_writes == writes && ui.dirty, "quiet-time autosave does not write during a held external MIDI note");
    CHECK(save_wait == manual, "a manual queue survives; autosave does not invent an explicit queue");
    CHECK(engine_project_notes(), "save deferral does not cut the held note");
    release(src);
    CHECK(store_writes == writes, "audio release alone does not perform storage writes");
    ui_frame();
    CHECK(store_writes == writes + 1 && !ui.dirty && !save_wait && !ui.save_failed,
          "release permits exactly one changed-object save and clears pending/dirty state");
    quiet_frame();
    CHECK(store_writes == writes + 1, "later frames do not repeat the completed save");
    printf("checked %s save, MIDI source %u\n", manual ? "manual" : "automatic", src);
}
static void rec_and_failure(void)
{
    fixture();
    unsigned writes = store_writes;
    ui.rec = 1; save_button(); quiet_frame();
    CHECK(save_wait && ui.dirty && store_writes == writes, "armed REC still postpones SAVE");
    CHECK(!strcmp(queued_save_label(), "REC OFF TO SAVE"), "stopped REC queue names its actual blocker");
    ui.rec = 0; store_fail = 1; ui_frame();
    CHECK(ui.save_failed && ui.dirty && !save_wait, "a real storage failure is failed, not saved or indefinitely queued");
    store_fail = 0; save_button();
    CHECK(!ui.save_failed && !ui.dirty && !save_wait, "explicit retry succeeds after the storage failure clears");
}
static void clip_quiet(void)
{
#if BOARD_SMK37
    fixture();
    unsigned writes = store_writes;
    test_clip_mode = test_clip_busy = 1;
    save_button(); quiet_frame();
    CHECK(save_wait && store_writes == writes, "clip activity still blocks queued SAVE");
    test_clip_busy = 0; ui.act_t = now_ms;
    ui_frame();
    CHECK(save_wait && store_writes == writes, "clip SAVE retains its four-second quiet interval");
    now_ms += AUTOSAVE_QUIET; ui_frame();
    CHECK(!save_wait && !ui.dirty && store_writes == writes + 1, "quiet clip SAVE completes once");
    test_clip_mode = 0;
#endif
}
int main(void)
{
    load_force = 0;
    for (unsigned src = 0; src < 3; src++)
        for (int manual = 0; manual < 2; manual++) held_note(src, manual);
    rec_and_failure();
    clip_quiet();
    printf("save_note_quiet: %u checks, %u failures\n", checks, failures);
    return failures != 0;
}
