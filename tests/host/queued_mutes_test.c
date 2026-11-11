/* SPDX-License-Identifier: GPL-3.0-only */
/* Queued-mutes toggle: off is immediate (bit-exact with the old mute path);
 * on defers part and track mutes to the next 909 bar; a second press cancels;
 * Stop applies a still-pending queue; a stopped press is immediate. */
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/seq/sequencer.h"

static pattern_t patterns[NPAT];
static seq_t S;
static unsigned checks, failed;

#define CHECK(x) do { checks++; if (!(x)) { failed++; if (failed < 20) fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); } } while (0)

static const seq_sink_t sink;
static void setup(void)
{
    int i;
    for (i = 0; i < NPAT; i++)
        pattern_init(&patterns[i], 1, 2);
    seq_init(&S, patterns);
    S.bpm = 150.0f;
    S.send_clock = 0;
    S.queued_mutes = 0;
}

static void run(unsigned n)
{
    seq_advance(&S, 0, &sink);
    while (n) {
        unsigned k = seq_until_event(&S, n > 128 ? 128 : n);
        seq_advance(&S, k, &sink);
        n -= k;
    }
}

/* 16 steps of 1/16 at 150 BPM: 16 * 4410 samples (seq_eighth_quarter_test). */
enum { BAR = 16u * 4410u, MASK_909 = (1u << NDRUM) - 1u };

static void off_is_immediate(void)
{
    setup();
    S.mute = 0;
    S.queued_mutes = 0;
    seq_start(&S);
    run(1);
    S.mute ^= MASK_909;
    CHECK(S.mute == MASK_909);
    CHECK(S.mute_q == 0);
    run(BAR + 1);
    CHECK(S.mute == MASK_909);
    seq_stop(&S);
    run(1);
    CHECK(S.mute == MASK_909);
}

static void on_waits_for_bar(void)
{
    setup();
    S.queued_mutes = 1;
    seq_start(&S);
    run(1);
    S.mute_q ^= MASK_909;
    CHECK(S.mute == 0);
    CHECK(S.mute_q == MASK_909);
    run(BAR / 2);
    CHECK(S.mute == 0);
    CHECK(S.mute_q == MASK_909);
    run(BAR / 2 + 1);
    CHECK(S.mute == MASK_909);
    CHECK(S.mute_q == 0);
}

static void second_press_cancels(void)
{
    setup();
    S.queued_mutes = 1;
    seq_start(&S);
    run(1);
    S.mute_q ^= MASK_909;
    S.mute_q ^= MASK_909;
    CHECK(S.mute == 0 && S.mute_q == 0);
    run(BAR + 1);
    CHECK(S.mute == 0 && S.mute_q == 0);
}

static void stop_applies_pending(void)
{
    setup();
    S.queued_mutes = 1;
    seq_start(&S);
    run(1);
    S.mute_q ^= MASK_909;
    seq_stop(&S);
    run(1);
    CHECK(S.mute == MASK_909);
    CHECK(S.mute_q == 0);
}

static void track_mute_queued(void)
{
    uint32_t bit = 1u << 1; /* 909 SD */
    setup();
    S.queued_mutes = 1;
    seq_start(&S);
    run(1);
    S.vmute_q ^= bit;
    CHECK(S.vmute == 0);
    run(BAR + 1);
    CHECK(S.vmute == bit && S.vmute_q == 0);
}

int main(void)
{
    off_is_immediate();
    on_waits_for_bar();
    second_press_cancels();
    stop_applies_pending();
    track_mute_queued();
    if (failed) {
        fprintf(stderr, "queued-mutes: %u failed / %u checks\n", failed, checks);
        return 1;
    }
    printf("queued-mutes: %u checks\n", checks);
    return 0;
}
