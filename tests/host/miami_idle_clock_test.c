/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_idle_clock.h"
#include <stdio.h>
#include <stdlib.h>

static uint32_t random_state = 0x6d70379bu;
static uint32_t random_word(void) {
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}
static void check(float start, float bpm, float limit, uint32_t frames) {
    float expected = start, actual = start;
    uint32_t count = 0;
    while (count < frames) {
        float next = expected + bpm;
        if (next >= limit) break;
        expected = next;
        ++count;
    }
    uint32_t used = original_miami_idle_clock_below(&actual, bpm, limit, frames);
    if (count != used || original_math_bits(expected) != original_math_bits(actual)) {
        fprintf(stderr, "clock mismatch %08x + %08x < %08x n%u: %08x/%u != %08x/%u\n",
            original_math_bits(start), original_math_bits(bpm), original_math_bits(limit),
            frames, original_math_bits(expected), count, original_math_bits(actual), used);
        exit(1);
    }
}
int main(void) {
    unsigned tests = 0;
    /* All finite positive binade transitions, neighboring significands,
     * halfway additions of either parity, and exact threshold hits. */
    for (uint32_t exponent = 1; exponent < 254; ++exponent) {
        for (int offset = -4; offset <= 4; ++offset) {
            uint32_t word = (exponent << 23) + offset;
            if (word > 0x7f7fffffu) continue;
            float start = original_math_float(word);
            for (int shift = -1; shift <= 1; ++shift) {
                int be = (int)exponent - 24 + shift;
                if (be < 1 || be > 253) continue;
                float bpm = original_math_float((uint32_t)be << 23);
                check(start, bpm, original_math_float(0x7f7fffffu), 256);++tests;
                check(start, bpm, start + bpm, 65);++tests;
            }
        }
    }
    for (unsigned i = 0; i < 1000000; ++i) {
        /* Full binary32 tempo precision rather than quarter-BPM values. */
        float bpm = original_math_float(0x41a00000u + random_word() % (0x43c80001u - 0x41a00000u));
        float start = original_math_float(random_word() % 0x7f800000u);
        float limit = i & 1 ? original_math_float(0x7f7fffffu) :
            start + bpm * (float)(1 + random_word() % 256);
        if (original_math_bits(limit) >= 0x7f800000u) limit = original_math_float(0x7f7fffffu);
        check(start, bpm, limit, random_word() % 257);++tests;
    }
    printf("PASS Miami idle clock: %u scalar-oracle comparisons\n", tests);
    return 0;
}
