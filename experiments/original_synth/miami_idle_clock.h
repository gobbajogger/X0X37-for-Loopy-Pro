/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_MIAMI_IDLE_CLOCK_H
#define ORIGINAL_MIAMI_IDLE_CLOCK_H
#include "binary32_math.h"

/* Positive finite clocks, positive BPM, round-to-nearest binary32. Consume
 * only additions strictly below threshold. Within one binade each addition
 * has a constant integer-ULP stride. A halfway addition lands on an even
 * significand; after that the stride is even and remains constant. Probe two
 * strides after the first add, then jump without any floating multiplication.
 * Re-enter scalar arithmetic at every exponent/threshold boundary. */
static uint32_t original_miami_idle_clock_below(float *value, float bpm,
    float threshold, uint32_t frames)
{
    uint32_t used = 0, limit = original_math_bits(threshold);
    float current = *value;
    while (used < frames) {
        float next = current + bpm;
        if (next >= threshold) break;
        current = next;
        ++used;
        if (frames - used < 2) continue;
        float second = current + bpm, third = second + bpm;
        uint32_t a = original_math_bits(current), b = original_math_bits(second);
        uint32_t c = original_math_bits(third), exponent = a & 0x7f800000u;
        if (!exponent || exponent != (b & 0x7f800000u) ||
            exponent != (c & 0x7f800000u) || b - a != c - b) continue;
        uint32_t stride = b - a;
        if (!stride) { used = frames; break; }
        uint32_t end = exponent + 0x800000u;
        if (limit < end) end = limit;
        uint32_t count = (end - 1u - a) / stride;
        if (count > frames - used) count = frames - used;
        current = original_math_float(a + stride * count);
        used += count;
    }
    *value = current;
    return used;
}
#endif
