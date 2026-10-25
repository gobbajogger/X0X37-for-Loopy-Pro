/* SPDX-License-Identifier: GPL-3.0-only */
/* SMC momentary SELECT keys on the home row: four source strips (909, 808, 303A,
 * BREAK) and three main strips (reverb send, delay send, master filter). Notes
 * match the M-VAVE SMC-Mixer SELECT row. Effect ids match busfx.h. */
#pragma once
#include <stdint.h>

enum {
    SMCFX_GATE = 0,
    SMCFX_REV = 1,
    SMCFX_HP = 2,
    SMCFX_DLY = 3,
    SMCFX_LP = 4,
    SMCFX_GATE8 = 5,
    SMCFX_GATE16 = 6,
    SMCFX_TRANCE = 7,
    SMCFX_STUT = 8,
    SMCFX_SHUF = 9
};
enum { SMCFX_909 = 0, SMCFX_808 = 1, SMCFX_303A = 2, SMCFX_BRK = 3, SMCFX_RVB = 4, SMCFX_MDLY = 5, SMCFX_FLT = 6 };
enum { SMCFX_N = 7 };
enum { SMCFX_NFX = 10 };
enum { SMCFX_SUIT_MAX = 8 };
#define SMCFX_NPAGE 3

/* PART_909, PART_808, PART_303A, PART_BRK (main strips use engine send/master paths). */
static const uint8_t SMCFX_PART[SMCFX_N - 3] = {0, 1, 2, 4};
static const uint8_t SMCFX_NOTE[SMCFX_N] = {24, 25, 26, 28, 29, 30, 31};

static const char *const SMCFX_BUS_NAME[SMCFX_N] = {
    "909", "808", "303A", "BREAK", "REVERB", "DELAY", "MASTER"};

/* Curated per-bus lists (Home+cycle order). 303A = Chicago acid and Miami (WBL1) bass. */
static const uint8_t SMCFX_SUIT[SMCFX_N][SMCFX_SUIT_MAX] = {
    {SMCFX_REV, SMCFX_DLY, SMCFX_STUT, SMCFX_SHUF},
    {SMCFX_DLY, SMCFX_REV, SMCFX_STUT, SMCFX_SHUF},
    {SMCFX_HP, SMCFX_DLY, SMCFX_REV, SMCFX_LP, SMCFX_GATE8, SMCFX_GATE16, SMCFX_TRANCE},
    {SMCFX_GATE, SMCFX_REV, SMCFX_DLY, SMCFX_HP},
    {SMCFX_REV, SMCFX_GATE8, SMCFX_GATE16, SMCFX_TRANCE, SMCFX_GATE, SMCFX_STUT, SMCFX_SHUF},
    {SMCFX_DLY, SMCFX_STUT, SMCFX_SHUF, SMCFX_GATE16, SMCFX_GATE8},
    {SMCFX_GATE16, SMCFX_GATE8, SMCFX_TRANCE, SMCFX_STUT, SMCFX_SHUF, SMCFX_HP, SMCFX_LP},
};
static const uint8_t SMCFX_SUIT_N[SMCFX_N] = {4, 4, 7, 4, 7, 5, 7};

static const uint8_t SMCFX_PAGE[SMCFX_NPAGE][SMCFX_N] = {
    {SMCFX_REV, SMCFX_DLY, SMCFX_HP, SMCFX_GATE, SMCFX_REV, SMCFX_DLY, SMCFX_GATE16},
    {SMCFX_DLY, SMCFX_REV, SMCFX_HP, SMCFX_HP, SMCFX_GATE16, SMCFX_STUT, SMCFX_TRANCE},
    {SMCFX_DLY, SMCFX_REV, SMCFX_REV, SMCFX_DLY, SMCFX_STUT, SMCFX_SHUF, SMCFX_GATE8},
};
static const char *const SMCFX_PAGE_NAME[SMCFX_NPAGE] = {"ROOM", "TIGHT", "ECHO"};
static const char *const SMCFX_SHORT[SMCFX_NFX] = {
    "GATE", "REV", "HPF", "DLY", "LPF", "G8", "G16", "TRNC", "STUT", "SHUF"};
static const char *const SMCFX_LONG[SMCFX_NFX] = {"DECAY GATE", "REV THROW", "HIGH-PASS", "DLY THROW", "LOW-PASS",
                                                    "8TH GATE", "16TH GATE", "TRANCE GATE", "STUTTER", "SHUFFLER"};

static inline int smcfx_is_main(int bus) { return bus >= SMCFX_RVB; }

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
