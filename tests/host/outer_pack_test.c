/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#define X0X_HOST 1
#include "../../firmware/src/app/plat.h"

int main(void)
{
    unsigned checks = 0;
    for (unsigned k = 0; k <= 36; k++) {
        if (k > 4 && k < 32)
            continue;
        for (unsigned velocity = 0; velocity < 128; velocity++) {
            for (int octave = -2; octave <= 3; octave++) {
                unsigned pitch = (unsigned)(36 + (int)k + 12 * octave);
                uint32_t word = PLAT_OUTER_PACK(k, velocity, pitch);
                assert(PLAT_OUTER_KEY(word) == k);
                assert(PLAT_OUTER_VEL(word) == velocity);
                assert(PLAT_OUTER_PITCH(word) == pitch);
                /* Independent legacy wire layout, including two hand-checked extremes. */
                assert(word == k + velocity * 128u + pitch * 16384u);
                checks++;
            }
        }
    }
    assert(PLAT_OUTER_PACK(0, 0, 12) == 0x30000u);
    assert(PLAT_OUTER_PACK(36, 127, 108) == 0x1b3fa4u);
    unsigned k = 4, v = 126, p = 99;
    uint32_t word = PLAT_OUTER_PACK(k++, v++, p++);
    assert(k == 5 && v == 127 && p == 100);
    assert(PLAT_OUTER_KEY(word) == 4 && PLAT_OUTER_VEL(word) == 126 && PLAT_OUTER_PITCH(word) == 99);
    printf("PASS: %u outer-key format round trips and single evaluation\n", checks);
}
