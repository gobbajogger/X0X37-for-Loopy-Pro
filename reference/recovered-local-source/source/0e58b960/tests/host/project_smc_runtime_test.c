/* Actual UI + SMC queues: a confirmed project replacement must discard controls
 * behind the confirmation, including messages in the other producer queues. */
#define BOARD_SMK37 1
#define X0X_DEV_PADCAL 1
#define main x0x_host_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>

uint8_t smk_clip_mode_on(void) { return 0; }
int smk_ck_smc_turn(int n, int d) { (void)n; (void)d; return 0; }
int smk_ck_smc_nav(uint8_t n) { (void)n; return 0; }
uint8_t smk_clip_transport_button(uint8_t b, uint8_t p) { (void)b; (void)p; return 0; }
uint8_t smk_clip_history_button(uint8_t b, uint8_t p, uint8_t h) { (void)b; (void)p; (void)h; return 0; }
void smk_play_note_button(uint8_t p, uint8_t h) { (void)p; (void)h; }
uint8_t smk_ck_flash_busy(void) { return 0; }
uint8_t smk_ck_ext(void) { return 0; }
uint8_t smk_ck_knobs_owned(void) { return 0; }
uint8_t smk_ck_panel(uint32_t *p, uint32_t f) { (void)p; (void)f; return 0; }
uint8_t smk_clip_lcd_draw(uint32_t w, uint32_t h) { (void)w; (void)h; return 0; }
void smk_clip_lcd_force(void) {}
uint8_t smk_clip_wait(void) { return 0; }
uint8_t smk_apc_transport_led_state(uint8_t n) { (void)n; return 0; }
uint8_t smk_padcal_on(void) { return 0; }
void smk_padcal_enter(void) {}
uint8_t smk_padcal_draw(uint32_t w, uint32_t h) { (void)w; (void)h; return 0; }
static int feedback;
int plat_ble_smc_send(const uint8_t *m, uint8_t n) { (void)m; (void)n; feedback++; return 1; }
#ifndef SMC_SLOT_SEND_IMPLEMENTED
/* Existing one-peer fixtures record peer zero. Dual-peer tests override this. */
int plat_ble_smc_send_slot(unsigned slot, const uint8_t *m, uint8_t n)
{ return slot == 0 ? plat_ble_smc_send(m, n) : 1; }
#endif

#include "app/smc_mixer.c"

static project_t before;
static seq_t seq_before;
static void note(int n)
{
    uint8_t msg[3] = {0x90, (uint8_t)n, 127};
    smc_ble_midi(msg, sizeof msg);
}
static void knob(void)
{
    uint8_t msg[3] = {0xB0, 16, 1};
    smc_ble_midi(msg, sizeof msg);
}
static void render(void)
{
    int32_t block[512];
    engine_render(block, 256);
}
static void check_queues(void)
{
    assert(smc_qr == smc_qw && smc_host_r == smc_host_w);
    for (int i = 0; i < 2; i++) assert(smc_mmc_r[i] == smc_mmc_w[i]);
}
static void pending_edits(void)
{
    note(0x5E);                                      /* PLAY */
    note(0x5F);                                      /* REC */
    note(16);                                        /* mute */
    knob();
    assert(!smc_midi_host(0x0100BF0Bu | 16u << 16));  /* retired host route stays unconsumed */
    smc_mmc_host(6, 0);                              /* MMC RECORD */
    smc_mmc_host(2, 1);                              /* MMC PLAY */
}

int main(void)
{
    boot();
    render();
    proj.set.bt |= PROJ_BT_SMC_CENTRAL | PROJ_BT_SMC_USER;
    smc_ble_link(1);
    smc_poll();
    smc.solo = 1;
    engine_smc_solo(1);
    assert(ui_project_begin() == UI_PROJECT_BUSY && smc.solo == 1);
    smc.picked = smc.seen = 0xFF;
    smc.fx_restore[0] = 89;
    memset(smc.known, 1, sizeof smc.known);
    seq.mute = 0x123;
    ui.rec = 1;
    ask(ACT_RESET, 0, "RESET PROJECT?", "SAMPLES ARE KEPT");
    note(5);                                         /* confirm, then obsolete input in the same batch */
    pending_edits();
    smc_poll();
    check_queues();
    assert(!seq.playing && !seq.req_start && !ui.rec && !seq.mute);
    assert(!smc.solo && !smc.picked && !smc.seen && !smc.fx_restore[0]);
    assert(smc.up && smc.fresh && smc.pending == 0xFF);
    for (int i = 0; i < 128; i++) assert(!smc.known[i]);
    assert((proj.set.bt & (PROJ_BT_SMC_CENTRAL | PROJ_BT_SMC_USER)) ==
           PROJ_BT_SMC_CENTRAL); /* reset normalizes retired bit, preserves role */
    render();

    set_view(V_HOME);
    before = proj; seq_before = seq;
    int old_feedback = feedback;
    assert(ui_project_begin() == 0);
    pending_edits();
    smc_poll();
    smc_frame();
    check_queues();
    assert(feedback == old_feedback);
    assert(!memcmp(&proj, &before, sizeof proj) && !memcmp(&seq, &seq_before, sizeof seq));
    ui_project_end(0);
    smc_poll();
    assert(!memcmp(&proj, &before, sizeof proj) && !memcmp(&seq, &seq_before, sizeof seq));
    /* HOME strip-0 knob is MIX CUT, open-at-max by default. Seed mid-range so a
     * +1 relative detent can prove input resumed after the abort (same ceiling
     * trap as smc_drum_mutes continuous_probe). */
    {
        pref_t r = smc_knob_target(0);
        const x0x_param_t *d = pref_desc(r);
        assert(d && d->max);
        pref_set(r, d->max / 2);
    }
    int value = pref_get(smc_knob_target(0));
    knob(); smc_poll();
    assert(pref_get(smc_knob_target(0)) == value + 1); /* input resumes after abort */
    puts("project_smc_runtime: reset drops all queued routes; lock/abort/solo/pickup state ok");
    return 0;
}
