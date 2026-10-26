/* SPDX-License-Identifier: GPL-3.0-only */
/* Real queue/UI/sequencer: deliberate unlock, clock phase and pending cues. */
#define SMC_UI_MAIN smc_ui_existing_main
#include "smc_ui_test.c"
#undef SMC_UI_MAIN

static void pause_hold(void)
{
    setup(V_PART, PART_303A);
    message(0x90, 0x5D, 127); CHECK(smc.locked);
    now_ms += 1000; smc_poll(); CHECK(smc.locked); /* original capture cannot unlock */
    message(0x80, 0x5D, 0);
    message(0x90, 0x5D, 127); CHECK(smc.locked);
    now_ms += 599; smc_poll(); CHECK(smc.locked);
    message(0x90, 0x5D, 0); CHECK(smc.locked); /* short second tap retains lock */
    message(0x90, 0x5D, 127); CHECK(smc.locked);
    now_ms += 599; smc_poll(); CHECK(smc.locked);
    now_ms++; smc_poll(); CHECK(!smc.locked);
    message(0x90, 0x5D, 127); CHECK(!smc.locked); /* held repeat cannot recapture */
    message(0x80, 0x5D, 0);
    message(0x90, 0x5D, 127); message(0x80, 0x5D, 0); CHECK(smc.locked);
    message(0x90, 0x5D, 127); now_ms += 599;
    smc_ble_link(0); smc_poll(); now_ms += 1000; smc_poll(); CHECK(smc.locked);
    smc_ble_link(1); smc_poll(); CHECK(smc.locked);
    message(0x90, 0x5D, 127); now_ms += 600;
    message(0x80, 0x5D, 0); CHECK(!smc.locked); /* elapsed release, without an idle poll */
    CHECK(!seq.req_stop && !seq.req_start);
    puts("PASS Pause: tap captures, fresh 600ms hold unlocks, short tap/repeat/link loss do not");
}
static void pause_lost_and_accepted_release(void)
{
    for (int lost_host = 0; lost_host < 2; lost_host++) {
        setup(V_PART, PART_303A); proj.set.bt |= PROJ_BT_SMC_USER;
        message(0x90, 0x5D, 127); message(0x80, 0x5D, 0);
        message(0x90, 0x5D, 127); now_ms += 600;
        const uint8_t filler[3] = {0xB0, 16, 0}, release[3] = {0x80, 0x5D, 0};
        for (unsigned i = 0; i < SMC_Q; i++) {
            if (lost_host) CHECK(smc_midi_host(11u | 0xBFu << 8 | 16u << 16));
            else smc_ble_midi(filler, 3);
        }
        if (lost_host) {
            CHECK(smc_midi_host(8u | 0x8Fu << 8 | 0x5Du << 16));
            smc_ble_midi(release, 3);
        } else {
            smc_ble_midi(release, 3);
            CHECK(smc_midi_host(8u | 0x8Fu << 8 | 0x5Du << 16));
        }
        smc_poll(); CHECK(smc.locked); CHECK(!smc.lock_down);
        now_ms += 1000; smc_poll(); CHECK(smc.locked);
        message(0x90, 0x5D, 127); now_ms += 600; smc_poll(); CHECK(!smc.locked);
    }
    puts("PASS Pause: dropped plus accepted cross-route releases cancel before unlock");
}
static void pause_clock(void)
{
    const int tempos[] = {60, 120, 180};
    for (unsigned i = 0; i < sizeof tempos / sizeof *tempos; i++) {
        setup(V_HOME, PART_303A); seq.playing = 0; seq.bpm = (float)tempos[i];
        unsigned period = 60000u / tempos[i];
        now_ms = period; CHECK(smc_led(0x5D));
        now_ms = period + period / 2; CHECK(!smc_led(0x5D));
        seq.playing = 1; seq.ext = 0; seq.start_at = 17003;
        unsigned samples = 60u * FS / tempos[i];
        seq.ext_now = seq.start_at; CHECK(smc_led(0x5D));
        seq.ext_now += samples / 2; CHECK(!smc_led(0x5D));
        now_ms += 999; CHECK(!smc_led(0x5D)); /* wall time cannot move playing phase */
        seq.ext_now = seq.start_at + samples; CHECK(smc_led(0x5D));
        seq.ext = 1; seq.ext_bpm = 99;
        seq.ext_q = 1; CHECK(smc_led(0x5D)); seq.ext_q = 12; CHECK(smc_led(0x5D));
        seq.ext_q = 13; CHECK(!smc_led(0x5D)); seq.ext_q = 0; CHECK(!smc_led(0x5D));
        CHECK(smc_lock_toggle()); CHECK(smc_led(0x5D));
        now_ms += 1000; CHECK(smc_led(0x5D));
    }
    puts("PASS Pause feedback: 60/120/180 BPM, sample phase, external tick phase, steady lock");
}
static void pending_cues(void)
{
    for (int captured = 0; captured < 2; captured++) for (int part = 0; part < NPARTS; part++) {
        setup(V_PART, part); seq.playing = 1; seq.ppat[part] = 1;
        if (captured) {
            CHECK(smc_lock_toggle());
            ui.part = (uint8_t)((part + 1) % NPARTS); build_pages();
        }
        message(0x90, 0x5C, 127); message(0x80, 0x5C, 0); CHECK(seq_cue_of(&seq, part) == 2);
        message(0x90, 0x5C, 127); message(0x80, 0x5C, 0); CHECK(seq_cue_of(&seq, part) == 3);
        message(0x90, 0x5B, 127); message(0x80, 0x5B, 0); CHECK(seq_cue_of(&seq, part) == 2);
        message(0x90, 0x5B, 127); message(0x80, 0x5B, 0); CHECK(seq_cue_of(&seq, part) == -1);
        message(0x90, 0x5B, 127); message(0x80, 0x5B, 0); CHECK(seq_cue_of(&seq, part) == 0);
        message(0x90, 0x5B, 127); message(0x80, 0x5B, 0); CHECK(seq_cue_of(&seq, part) == 0);
        message(0x90, 0x5C, 127); message(0x80, 0x5C, 0); CHECK(seq_cue_of(&seq, part) == -1);
        seq.ppat[part] = NPAT - 2;
        message(0x90, 0x5C, 127); message(0x80, 0x5C, 0); CHECK(seq_cue_of(&seq, part) == NPAT - 1);
        message(0x90, 0x5C, 127); message(0x80, 0x5C, 0); CHECK(seq_cue_of(&seq, part) == NPAT - 1);
        message(0x90, 0x5B, 127); message(0x80, 0x5B, 0); CHECK(seq_cue_of(&seq, part) == -1);
        for (int other = 0; other < NPARTS; other++) if (other != part)
            CHECK(seq_cue_of(&seq, other) == -1);
    }
    puts("PASS pattern arrows: pending accumulation, opposite cancellation, bounds and captured/current part");
}
int main(void)
{
    const char *only = getenv("SMC_CONTROL_CASE");
    if (!only || !strcmp(only, "hold")) { pause_hold(); pause_lost_and_accepted_release(); }
    if (!only || !strcmp(only, "clock")) pause_clock();
    if (!only || !strcmp(only, "cue")) pending_cues();
    printf("PASS SMC control lifecycle: %u checks\n", checks);
    return 0;
}
