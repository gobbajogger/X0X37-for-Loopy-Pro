/* SPDX-License-Identifier: GPL-3.0-only */
/* Four SMC momentary buttons, one per real main-mixer source bus.
 * Notes are the SELECT row beside those faders (M-VAVE SMC-Mixer).
 * 303 B stays a normal strip: it has its own cutoff knob, and SELECT still
 * opens it. Effect ids match busfx.h. */
#pragma once
#include <stdint.h>

enum { SMCFX_GATE = 0, SMCFX_REV = 1, SMCFX_HP = 2, SMCFX_DLY = 3, SMCFX_LP = 4 };
enum { SMCFX_909 = 0, SMCFX_808 = 1, SMCFX_303A = 2, SMCFX_BRK = 3, SMCFX_N = 4 };
enum { SMCFX_NFX = 5 };
#define SMCFX_NPAGE 3

/* PART_909, PART_808, PART_303A, PART_BRK. */
static const uint8_t SMCFX_PART[SMCFX_N] = {0, 1, 2, 4};
static const uint8_t SMCFX_NOTE[SMCFX_N] = {24, 25, 26, 28};

/* Curated per-bus lists (Home+cycle order). Same effect may repeat on buses; omit
 * effects that do not suit a bus — no padding to a fixed count. */
static const uint8_t SMCFX_SUIT[SMCFX_N][4] = {
    {SMCFX_REV, SMCFX_DLY, SMCFX_GATE, 0},
    {SMCFX_DLY, SMCFX_REV, SMCFX_GATE, 0},
    {SMCFX_HP, SMCFX_DLY, SMCFX_REV, SMCFX_LP},
    {SMCFX_GATE, SMCFX_REV, SMCFX_DLY, SMCFX_HP},
};
static const uint8_t SMCFX_SUIT_N[SMCFX_N] = {3, 3, 4, 4};

/* Preset banks: one assignment per bus, each must be in that bus's SMCFX_SUIT list. */
static const uint8_t SMCFX_PAGE[SMCFX_NPAGE][SMCFX_N] = {
    {SMCFX_REV, SMCFX_DLY, SMCFX_HP, SMCFX_GATE},
    {SMCFX_GATE, SMCFX_GATE, SMCFX_HP, SMCFX_HP},
    {SMCFX_DLY, SMCFX_REV, SMCFX_REV, SMCFX_DLY},
};
static const char *const SMCFX_PAGE_NAME[SMCFX_NPAGE] = {"ROOM", "TIGHT", "ECHO"};
static const char *const SMCFX_SHORT[SMCFX_NFX] = {"GATE", "REV", "HPF", "DLY", "LPF"};
static const char *const SMCFX_LONG[SMCFX_NFX] = {
    "DECAY GATE", "REV THROW", "HIGH-PASS", "DLY THROW", "LOW-PASS"};

static inline int smcfx_suit_ok(int bus, uint8_t fx)
{
    int k;
    if (bus < 0 || bus >= SMCFX_N)
        return 0;
    for (k = 0; k < SMCFX_SUIT_N[bus]; k++)
        if (SMCFX_SUIT[bus][k] == fx)
            return 1;
    return 0;
}

static inline int smcfx_bus(unsigned note)
{
    int i;
    for (i = 0; i < SMCFX_N; i++)
        if (SMCFX_NOTE[i] == note)
            return i;
    return -1;
}

static inline uint8_t smcfx_cycle(int bus, uint8_t cur)
{
    int n, k;
    if (bus < 0 || bus >= SMCFX_N)
        return cur;
    n = SMCFX_SUIT_N[bus];
    if (n <= 0)
        return cur;
    for (k = 0; k < n; k++)
        if (SMCFX_SUIT[bus][k] == cur)
            break;
    if (k >= n)
        k = 0;
    else
        k = (k + 1) % n;
    return SMCFX_SUIT[bus][k];
}
