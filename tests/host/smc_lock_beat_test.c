/* SPDX-License-Identifier: GPL-3.0-only */
/* Owner contract LX-REQ-006/022, replayed through production input queues,
 * sequencer clock and SMC feedback. No device or physical-light claim. */
#define SMC_UI_MAIN smc_ui_baseline_main
#include "smc_ui_test.c"

static void pause_edge(int host, int down)
{
    if (host) {
        CHECK(smc_midi_host((down ? 9u : 8u) | (down ? 0x9Fu : 0x8Fu) << 8 |
                           0x5Du << 16 | (down ? 127u << 24 : 0)));
        smc_poll();
    } else message(down ? 0x90 : 0x80, 0x5D, down ? 127 : 0);
}
static void hold_ms(unsigned ms) { now_ms += ms; smc_poll(); }
static void lost_release(int host, unsigned note)
{
    for (unsigned n = 0; n < SMC_Q; n++) {
        if (host) CHECK(smc_midi_host(11u | 0xBFu << 8 | 16u << 16));
        else { const uint8_t m[3] = {0xB0, 16, 0}; smc_ble_midi(m, 3); }
    }
    if (host) CHECK(smc_midi_host(8u | 0x8Fu << 8 | note << 16));
    else { const uint8_t m[3] = {0x80, (uint8_t)note, 0}; smc_ble_midi(m, 3); }
}
static void gesture(void)
{
    for (int host = 0; host < 2; host++) {
        setup(V_SEQ, PART_909); proj.set.bt |= PROJ_BT_SMC_USER;
        unsigned start = seq.req_start, stop = seq.req_stop;
        pause_edge(host, 1); CHECK(smc.locked);
        hold_ms(1500); CHECK(smc.locked); /* the capture press cannot unlock */
        pause_edge(host, 1); CHECK(smc.locked);
        pause_edge(host, 0);
        pause_edge(host, 1); CHECK(smc.locked); /* must not immediately toggle */
        hold_ms(899); CHECK(smc.locked);
        pause_edge(host, 0); hold_ms(1000); CHECK(smc.locked); /* cancellation */
        pause_edge(host, 1); hold_ms(450); pause_edge(host, 1);
        hold_ms(449); CHECK(smc.locked);
        hold_ms(1); CHECK(!smc.locked); /* no repeat event required at 900 ms */
        pause_edge(host, 1); hold_ms(1000); CHECK(!smc.locked);
        pause_edge(host, 0); pause_edge(host, 0); CHECK(!smc.locked);
        CHECK(seq.req_start == start && seq.req_stop == stop);
        pause_edge(host, 1); pause_edge(host, 0); CHECK(smc.locked);
        pause_edge(host, 1); now_ms += 900; pause_edge(host, 0);
        CHECK(!smc.locked && !smc.lock_down); /* deadline key-up, no intervening poll */

        pause_edge(host, 1); pause_edge(host, 0); CHECK(smc.locked);
        pause_edge(host, 1); hold_ms(899); lost_release(host, 0x5D);
        hold_ms(1); CHECK(smc.locked && !smc.lock_down); /* drain loss before timer */
        pause_edge(host, 1); hold_ms(450); lost_release(host, 8);
        hold_ms(450); CHECK(!smc.locked); /* unrelated key-up cannot cancel */
        pause_edge(host, 0);
        pause_edge(host, 1); pause_edge(host, 0);
        now_ms = UINT32_MAX - 449u; pause_edge(host, 1);
        hold_ms(899); CHECK(smc.locked); hold_ms(1); CHECK(!smc.locked);
    }
    for (int cancel = 0; cancel < 4; cancel++) {
        setup(V_HOME, PART_303A);
        pause_edge(0, 1); pause_edge(0, 0); pause_edge(0, 1); hold_ms(899);
        if (cancel == 0) { smc_ble_link(0); smc_ble_link(1); }
        if (cancel == 1) ui.overlay = O_ASK;
        if (cancel == 2) perf.test = 0;
        if (cancel == 3) { CHECK(ui_project_begin() == 0); }
        hold_ms(1); CHECK(smc.locked && !smc.lock_down);
        if (cancel == 1) ui.overlay = O_NONE;
        if (cancel == 2) perf.test = PT_IDLE;
        if (cancel == 3) ui_project_end(0);
        pause_edge(0, 0); hold_ms(1000); CHECK(smc.locked);
        pause_edge(0, 1); hold_ms(900); CHECK(!smc.locked);
        pause_edge(0, 0); pause_edge(0, 1); pause_edge(0, 0); pause_edge(0, 1);
        smc_project_reset(); hold_ms(1000); CHECK(!smc.locked && !smc.lock_down);
    }
    puts("PASS 900 ms gesture: BLE/host, same press, repeat, 899 ms cancel, threshold, wrap, loss, reconnect, guards, reset, no transport");
}
static void clock_off(void *ctx, int part) { (void)ctx; (void)part; }
static const seq_sink_t clock_sink = {.bass_off = clock_off};
static void advance_clock(unsigned samples)
{
    seq_advance(&seq, samples, &clock_sink);
    now_ms += samples * 1000u / FS;
}
static unsigned clock_tick_samples = 882;
static void tick(void)
{
    CHECK(seq_clk_take(&seq, 0xF8, 0)); CHECK(seq_midi_rt(&seq, 0xF8));
    advance_clock(clock_tick_samples); /* exact 62.5/125/250 BPM tick periods */
}
static void beat(void)
{
    for (unsigned speed = 0; speed < 3; speed++)
    for (int external_only = 0; external_only < 2; external_only++) {
        clock_tick_samples = 1764u >> speed;
        float bpm = 62.5f * (float)(1u << speed);
        setup(V_HOME, PART_303A); seq.bpm = 125;
        seq.clk_src = external_only ? SEQ_CLK_EXT : SEQ_CLK_AUTO;
        advance_clock(1);
        for (unsigned n = 1; n <= 145; n++) {
            tick(); CHECK(smc_led(0x5D) == (((n - 1u) % 24u) < 12u));
        }
        CHECK(!seq.playing); /* clock pickup works while stopped */
        CHECK(seq.ext_period && seq.ext_bpm > bpm - 0.1f && seq.ext_bpm < bpm + 0.1f);
        seq.playing = 1; CHECK(seq_midi_rt(&seq, 0xFC)); advance_clock(1); CHECK(!seq.playing);
        CHECK(smc_led(0x5D));
        for (unsigned n = 0; n < 96; n++) {
            advance_clock(clock_tick_samples);
            CHECK(smc_led(0x5D) == (((n + 1u) % 24u) < 12u));
        }
        CHECK(external_only ? seq.clk_lost : !seq.ext);
        CHECK(seq.bpm == (float)(int)(bpm + 0.5f)); /* loss retains measured tempo, not the 1024 ms wall blink */
        smc_ble_link(0); smc_poll(); advance_clock(clock_tick_samples * 24u); smc_ble_link(1); smc_poll();
        CHECK(smc_led(0x5D)); /* reconnect does not restart the visual phase */
        for (unsigned n = 1; n <= 48; n++) {
            tick(); CHECK(smc_led(0x5D) == (((n - 1u) % 24u) < 12u));
        }
        CHECK(smc_lock_toggle());
        for (unsigned n = 0; n < 48; n++) { tick(); CHECK(smc_led(0x5D)); }
        CHECK(smc_lock_toggle()); CHECK(!smc_led(0x5D)); /* current phase, not a new beat */
        smc_project_reset(); CHECK(!smc.locked); CHECK(smc_led(0x5D) == 0);
    }
    setup(V_HOME, PART_303A); seq.bpm = 125; seq.ext = 0;
    CHECK(smc_led(0x5D)); advance_clock(10584); CHECK(!smc_led(0x5D));
    advance_clock(10584); CHECK(smc_led(0x5D));
    seq.bpm = 250; CHECK(smc_led(0x5D)); advance_clock(5292); CHECK(!smc_led(0x5D));
    smc_project_reset(); seq.ext_now = UINT32_MAX - 5291u;
    CHECK(smc_led(0x5D)); seq.ext_now += 5292; CHECK(!smc_led(0x5D));
    seq.ext_now += 5292; CHECK(smc_led(0x5D));
    puts("PASS beat: 62.5/125/250 BPM pickup, AUTO/EXTERNAL last-BPM loss, STOP, reconnect, solid lock, unlock/reset, internal tempo, wrap");
}
int main(int argc, char **argv)
{
    if (argc < 2 || !strcmp(argv[1], "gesture")) gesture();
    if (argc < 2 || !strcmp(argv[1], "beat")) beat();
    printf("smc_lock_beat: PASS (%u production checks)\n", checks);
    return 0;
}
