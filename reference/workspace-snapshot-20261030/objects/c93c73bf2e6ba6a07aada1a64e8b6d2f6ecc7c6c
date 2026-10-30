/* SPDX-License-Identifier: GPL-3.0-only */
/* Seeded two-bank live-event property test, compiled against production breaks.c.
 * Legacy and optimized BREAK decoders must emit bit-identical PCM under the same
 * press/release/tempo/pitch/bank-replacement/retrigger sequence. */
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "firmware/src/dsp/breaks.h"

#define BLOCK 64
#define LIVE_BLOCKS 160
#define QUIET_BLOCKS 16
#define LENGTHS 7
#define SEEDS 4

static uint32_t rnd(uint32_t *state)
{
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *state = x;
}

static void demand(int yes, const char *what)
{
    if (!yes) {
        fprintf(stderr, "FAIL: %s\n", what);
        exit(1);
    }
}

static FILE *pcm;
static uint64_t pressed, released, banks_replaced, reversed, tempo_changes;

static void emit(breaks_t *b, uint64_t *audible)
{
    struct {
        uint32_t before;
        float data[BLOCK];
        uint32_t after;
    } guarded;
    guarded.before = UINT32_C(0x4e4f5445);
    guarded.after = UINT32_C(0x4f464621);
    for (unsigned i = 0; i < BLOCK; i++)
        guarded.data[i] = NAN;
    breaks_render(b, guarded.data, BLOCK);
    demand(guarded.before == UINT32_C(0x4e4f5445) &&
           guarded.after == UINT32_C(0x4f464621), "renderer overran output buffer");
    for (unsigned i = 0; i < BLOCK; i++) {
        demand(isfinite(guarded.data[i]) && fabsf(guarded.data[i]) <= 2.f,
               "non-finite or unbounded audio sample");
        if (guarded.data[i] != 0.f)
            ++*audible;
    }
    demand(fwrite(guarded.data, sizeof(float), BLOCK, pcm) == BLOCK, "write PCM");
}

static void run_one(uint32_t n, uint32_t seedno)
{
    uint32_t state = UINT32_C(0x7331ad57) ^ n ^ (seedno * UINT32_C(0x9e3779b9));
    uint8_t *raw_a = malloc(((size_t)n + 1u) / 2u);
    uint8_t *raw_b = malloc(((size_t)n + 1u) / 2u);
    breaks_t *b = calloc(1u, sizeof(*b));
    demand(raw_a && raw_b && b, "test allocation");
    for (uint32_t i = 0; i < (n + 1u) / 2u; i++) {
        raw_a[i] = (uint8_t)rnd(&state);
        raw_b[i] = (uint8_t)rnd(&state);
    }
    unsigned bars = (seedno & 1u) ? 1u : 2u;
    brk_loop_t a = {.adpcm = raw_a, .nsamples = n, .rate = 22050, .bars = bars};
    brk_loop_t c = {.adpcm = raw_b, .nsamples = n, .rate = 22050, .bars = bars};
    breaks_init(b);
    breaks_set_loop(b, 0, &a);
    breaks_set_loop(b, 1, &c);
    demand(b->bank[0].valid && b->bank[1].valid, "generated banks valid");
    breaks_set(b, BRK_LEVEL, 127);
    breaks_set(b, BRK_COMPLEX, 100);
    breaks_set(b, BRK_BCHANCE, 100);
    breaks_set(b, BRK_ROLL, 67);
    breaks_set(b, BRK_R2, 80);
    breaks_set(b, BRK_R4, 100);
    breaks_set(b, BRK_ALEN, 0);
    breaks_set(b, BRK_BLEN, 0);
    const float rates[] = {31.875f, 32.125f, 64.875f, 257.125f};
    float samples16 = ((float)n / ((float)bars * 16.f)) / rates[seedno & 3u];
    uint16_t held = 0;
    uint64_t audible = 0;
    breaks_live(b, 8, 1);
    held |= UINT16_C(1) << 8;
    breaks_step(b, 0, 0, samples16, 1);
    for (unsigned block = 0; block < LIVE_BLOCKS; block++) {
        uint32_t action = rnd(&state);
        if (block % 2u == 0u) {
            unsigned key = (action >> 17) % 8u;
            uint16_t bit = (uint16_t)(UINT16_C(1) << key);
            int down = !(held & bit);
            breaks_live(b, (int)key, down);
            held = down ? (uint16_t)(held | bit) : (uint16_t)(held & ~bit);
            if (down) pressed++; else released++;
        }
        if (block % 5u == 0u) {
            unsigned key = 8u + (action % 3u);
            uint16_t bit = (uint16_t)(UINT16_C(1) << key);
            int down = !(held & bit);
            breaks_live(b, (int)key, down);
            held = down ? (uint16_t)(held | bit) : (uint16_t)(held & ~bit);
            if (key == 8u) reversed++;
        }
        if (block % 7u == 0u)
            breaks_set(b, BRK_PITCH, (int)((action >> 9) % 25u));
        if (block % 13u == 0u) {
            /* Different bank generation, overlapping release and new slice. */
            unsigned which = (action >> 10) & 1u;
            breaks_set_loop(b, (int)which, which ? &c : &a);
            banks_replaced++;
        }
        if (block % 9u == 0u)
            breaks_pin(b, (int)((action >> 4) % 8u));
        if (block % 11u == 0u) {
            /* Exact same clock/BPM data for both compiler branches. */
            float factor = (action & 1u) ? 1.125f : 0.875f;
            samples16 *= factor;
            samples16 = fmaxf(2.f, fminf(samples16, 1000000.f));
            tempo_changes++;
        }
        if (block % 4u == 0u)
            breaks_step(b, (int)((block / 4u) & 15u), (int)(block / 64u),
                        samples16, (block % 12u) != 0u);
        emit(b, &audible);
    }
    /* No held-key NoteOff leaks, even after rapid overlapping key changes. */
    for (int key = 0; key <= 10; key++)
        breaks_live(b, key, 0);
    breaks_stop(b);
    breaks_set_quiet(b, 1);
    for (unsigned block = 0; block < QUIET_BLOCKS; block++) {
        emit(b, &audible);
        if (block == QUIET_BLOCKS - 1)
            demand(breaks_silent(b), "silence/drain after NoteOff and STOP");
    }
    demand(audible != 0, "test must have audible signal before STOP");
    printf("n=%" PRIu32 " seed=%" PRIu32 " audible=%" PRIu64 " silent=1\n", n, seedno, audible);
    free(b);
    free(raw_a);
    free(raw_b);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fputs("usage: property-test /path/to/pcm\n", stderr);
        return 2;
    }
    pcm = fopen(argv[1], "wb");
    if (!pcm) { perror("open PCM"); return 3; }
    static const uint32_t lengths[LENGTHS] = {65u, 511u, 512u, 513u, 4097u, 176400u, 2116800u};
    for (unsigned n = 0; n < LENGTHS; n++)
        for (unsigned seedno = 0; seedno < SEEDS; seedno++)
            run_one(lengths[n], seedno);
    demand(pressed > 0 && released > 0 && banks_replaced > 0 && reversed > 0 && tempo_changes > 0,
           "live event types must actually occur");
    printf("events: pressed=%" PRIu64 " released=%" PRIu64 " bank_replace=%" PRIu64
           " reverse=%" PRIu64 " tempo=%" PRIu64 "\n",
           pressed, released, banks_replaced, reversed, tempo_changes);
    if (fclose(pcm)) { perror("close PCM"); return 3; }
    return 0;
}
