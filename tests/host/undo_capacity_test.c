/* SPDX-License-Identifier: GPL-3.0-only */
/* Capacity guard: whole-pattern replacements are the largest routine edits. */
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/seq/pattern.h"
#include "../../firmware/src/app/undo.c"

#ifndef EXPECTED
#error EXPECTED must be set for the configured UNDO_POOL
#endif

static uint8_t region[NPAT * sizeof(pattern_t)];
static uint8_t shadow[sizeof region];
static undo_t u;

int main(void)
{
    uint32_t lo, hi;
    int i, kept;
    undo_init(&u, region, shadow, sizeof region);
    for (i = 0; i < 48; i++) {
        memset(region + (i % NPAT) * sizeof(pattern_t), i + 1, sizeof(pattern_t));
        if (undo_commit(&u) != 1) {
            printf("pattern replacement %d was not recorded\n", i);
            return 1;
        }
    }
    kept = u.n;
    if (kept != EXPECTED) {
        printf("%u-byte undo pool kept %d replacements, expected %d\n", UNDO_POOL, kept, EXPECTED);
        return 1;
    }
    for (i = 0; i < kept; i++) {
        const int edit = 47 - i;
        const uint8_t previous = edit >= NPAT ? (uint8_t)(edit - NPAT + 1) : 0;
        const uint8_t *p = region + (edit % NPAT) * sizeof(pattern_t);
        if (!undo_undo(&u, 0, &lo, &hi)) {
            puts("missing undo entry");
            return 1;
        }
        for (size_t j = 0; j < sizeof(pattern_t); j++)
            if (p[j] != previous) {
                printf("undo of replacement %d corrupted byte %zu\n", edit, j);
                return 1;
            }
    }
    if (undo_undo(&u, 0, &lo, &hi)) {
        puts("undid beyond retained history");
        return 1;
    }
    for (i = 0; i < kept; i++)
        if (!undo_redo(&u, 0, &lo, &hi)) {
            puts("missing redo entry");
            return 1;
        }
    for (i = 0; i < NPAT; i++) {
        const uint8_t *p = region + i * sizeof(pattern_t);
        for (size_t j = 0; j < sizeof(pattern_t); j++)
            if (p[j] != (uint8_t)(i < 16 ? i + 33 : i + 1)) {
                printf("pattern %d corrupted at byte %zu\n", i, j);
                return 1;
            }
    }
    undo_init(&u, region, shadow, sizeof region);
    for (i = 0; i < 48; i++) {
        region[i] ^= 0x80u;
        if (undo_commit(&u) != 1) {
            puts("small edit was not recorded");
            return 1;
        }
    }
    if (u.n != UNDO_MAX) {
        printf("small-edit history kept %d entries, expected %d\n", u.n, UNDO_MAX);
        return 1;
    }
    printf("%u-byte pool: %d whole-pattern replacements retained; 48 edits redone without corruption\n", UNDO_POOL, kept);
    return 0;
}
