/* SPDX-License-Identifier: GPL-3.0-only */
/* SMC momentary SELECT on all eight home-row strips (notes 24–31). Effect ids match busfx.h. */
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
enum {
    SMCFX_909 = 0,
    SMCFX_808 = 1,
    SMCFX_303A = 2,
    SMCFX_303B = 3,
    SMCFX_BRK = 4,
    SMCFX_RVB = 5,
    SMCFX_MDLY = 6,
    SMCFX_FLT = 7
};
enum { SMCFX_N = 8 };
enum { SMCFX_NFX = 10 };
enum { SMCFX_SUIT_MAX = 8 };
enum { SMCFX_SRC_N = 5 };
#define SMCFX_NPAGE 3

static const uint8_t SMCFX_PART[SMCFX_SRC_N] = {0, 1, 2, 3, 4};
static const uint8_t SMCFX_NOTE[SMCFX_N] = {24, 25, 26, 27, 28, 29, 30, 31};

static const char *const SMCFX_BUS_NAME[SMCFX_N] = {
    "909", "808", "303A", "303B", "BREAK", "REVERB", "DELAY", "MASTER"};

static const uint8_t SMCFX_SUIT[SMCFX_N][SMCFX_SUIT_MAX] = {
    {SMCFX_REV, SMCFX_DLY, SMCFX_STUT, SMCFX_SHUF},
    {SMCFX_DLY, SMCFX_REV, SMCFX_STUT, SMCFX_SHUF},
    {SMCFX_HP, SMCFX_DLY, SMCFX_REV, SMCFX_LP, SMCFX_GATE8, SMCFX_GATE16, SMCFX_TRANCE},
    {SMCFX_HP, SMCFX_LP, SMCFX_DLY, SMCFX_REV, SMCFX_GATE16, SMCFX_GATE8},
    {SMCFX_GATE, SMCFX_REV, SMCFX_DLY, SMCFX_HP},
    {SMCFX_REV, SMCFX_GATE8, SMCFX_GATE16, SMCFX_TRANCE, SMCFX_GATE, SMCFX_STUT, SMCFX_SHUF},
    {SMCFX_DLY, SMCFX_STUT, SMCFX_SHUF, SMCFX_GATE16, SMCFX_GATE8},
    {SMCFX_GATE16, SMCFX_GATE8, SMCFX_TRANCE, SMCFX_STUT, SMCFX_SHUF, SMCFX_HP, SMCFX_LP},
};
static const uint8_t SMCFX_SUIT_N[SMCFX_N] = {4, 4, 7, 6, 4, 7, 5, 7};

static const uint8_t SMCFX_PAGE[SMCFX_NPAGE][SMCFX_N] = {
    {SMCFX_REV, SMCFX_DLY, SMCFX_HP, SMCFX_LP, SMCFX_GATE, SMCFX_REV, SMCFX_DLY, SMCFX_GATE16},
    {SMCFX_DLY, SMCFX_REV, SMCFX_HP, SMCFX_DLY, SMCFX_HP, SMCFX_GATE16, SMCFX_STUT, SMCFX_TRANCE},
    {SMCFX_DLY, SMCFX_REV, SMCFX_REV, SMCFX_REV, SMCFX_DLY, SMCFX_STUT, SMCFX_SHUF, SMCFX_GATE8},
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
