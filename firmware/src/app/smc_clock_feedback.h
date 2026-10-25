/* SPDX-License-Identifier: GPL-3.0-only */
/* Main-loop visual observer only. The sequencer's free-running sample clock is
 * authoritative even while transport is stopped. Start/Continue/Stop do not
 * restart the visual beat; only an incoming clock phase can realign it.
 * No musical state is written. */
#pragma once
#include "../seq/sequencer.h"
#define SMC_BEAT_UNITS (FS * 60u * 256u)
static struct {
    uint32_t sample, phase, bpm, project, tick;
    uint8_t ready, external;
} smc_beat;
static uint32_t smc_bpm_units(float bpm)
{
    /* Project tempo is bounded by its UI; external MIDI clock can be faster or
     * slower. Reject only non-finite/unrepresentable observer input. */
    if (!(bpm > 0.0f) || !(bpm < 16777216.0f)) return 0;
    return (uint32_t)(bpm * 256.0f + 0.5f);
}
static __attribute__((noinline)) int smc_clock_feedback(const seq_t *s)
{
    const volatile seq_t *clock = s;
    uint32_t now = 0, since = 0, period = 0;
    float project_bpm = 0, external_bpm = 0;
    uint8_t ext = 0, qtick = 0, lost = 0;
    int coherent = 0;
    /* Main-loop reads may straddle the audio ISR. Retry one bounded collection
     * if any observed field changes; an unstable frame just carries the last
     * phase/tempo. This is a consistency check, not a cross-core seqlock. */
    for (int tries = 0; tries < 2; tries++) {
        now = clock->ext_now; since = clock->ext_since; period = clock->ext_period;
        project_bpm = clock->bpm; external_bpm = clock->ext_bpm;
        ext = clock->ext; qtick = clock->ext_q; lost = clock->clk_lost;
        __asm__ volatile("" ::: "memory");
#ifdef SMC_CLOCK_SNAPSHOT_TEST_HOOK
        SMC_CLOCK_SNAPSHOT_TEST_HOOK(tries);
#endif
        if (now == clock->ext_now && since == clock->ext_since && period == clock->ext_period &&
            project_bpm == clock->bpm && external_bpm == clock->ext_bpm &&
            ext == clock->ext && qtick == clock->ext_q && lost == clock->clk_lost) {
            coherent = 1; break;
        }
    }
    uint32_t project = coherent ? smc_bpm_units(project_bpm) : smc_beat.project;
    uint32_t delta = now - smc_beat.sample;
    int external = coherent && ext && period && !lost && since < FS / 2u;
    if (!smc_beat.ready) {
        if (!coherent) return 1;
        smc_beat.ready = 1;
        smc_beat.bpm = smc_beat.project = project;
        delta = 0;
    }
    /* A project reinitialization can reset the sample counter; natural uint32
     * wrap must continue normally. Neither event changes musical transport. */
    if (now < smc_beat.sample && smc_beat.sample - now < 0x80000000u) delta = 0;
    smc_beat.phase = (uint32_t)(((uint64_t)smc_beat.phase + (uint64_t)delta * smc_beat.bpm) % SMC_BEAT_UNITS);
    smc_beat.sample = now;
    if (!coherent) return smc_beat.phase < SMC_BEAT_UNITS / 2u;
    /* Loss may round the sequencer's internal fallback BPM. Retain the actual
     * last observed external BPM through loss/Stop; a later tempo edit wins. */
    if (external) {
        uint32_t bpm = smc_bpm_units(external_bpm);
        if (bpm) smc_beat.bpm = bpm;
    }
    else if (!smc_beat.external && project && project != smc_beat.project) smc_beat.bpm = project;
    smc_beat.project = project;
    if (external) {
        uint32_t tick = now - since;
        if (!smc_beat.external || tick != smc_beat.tick) {
            /* ext_q increments on the tick: its first tick is the downbeat.
             * Batched BLE ticks retain the same authoritative modulo-24 phase. */
            uint32_t q = (qtick + 23u) % 24u;
            smc_beat.phase = (uint32_t)(((uint64_t)q * (SMC_BEAT_UNITS / 24u) + (uint64_t)since * smc_beat.bpm) % SMC_BEAT_UNITS);
            smc_beat.tick = tick;
        }
    }
    smc_beat.external = (uint8_t)external;
    return smc_beat.phase < SMC_BEAT_UNITS / 2u; /* quarter-note pulse, half duty */
}
