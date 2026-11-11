/* SPDX-License-Identifier: GPL-3.0-only */
/* One copy of SMC momentary FX map tables.
 * engine.c is a separate -O2 unit from x0x.c (smc_mixer); the header used to
 * emit a static copy in each TU. Keep the labels here and extern them from
 * smc_fx_map.h. Behavior and labels unchanged. */
#include "smc_fx_map.h"

const uint8_t SMCFX_PART[SMCFX_SRC_N] = {0, 1, 2, 3, 4};
const uint8_t SMCFX_NOTE[SMCFX_N] = {24, 25, 26, 27, 28, 29, 30, 31};

const char *const SMCFX_BUS_NAME[SMCFX_N] = {
    "909", "808", "BASS A", "BASS B", "BREAK", "REVERB", "DELAY", "MASTER"};

const uint8_t SMCFX_SUIT[SMCFX_N][SMCFX_SUIT_MAX] = {
    {SMCFX_REV, SMCFX_DLY, SMCFX_STUT, SMCFX_SHUF, SMCFX_GATE, SMCFX_HP},
    {SMCFX_REV, SMCFX_DLY, SMCFX_STUT, SMCFX_SHUF, SMCFX_GATE, SMCFX_HP},
    {SMCFX_HP, SMCFX_DLY, SMCFX_REV, SMCFX_LP, SMCFX_GATE8, SMCFX_GATE16, SMCFX_TRANCE},
    {SMCFX_HP, SMCFX_LP, SMCFX_DLY, SMCFX_REV, SMCFX_GATE16, SMCFX_GATE8},
    {SMCFX_REV, SMCFX_DLY, SMCFX_STUT, SMCFX_SHUF, SMCFX_GATE, SMCFX_HP},
    {SMCFX_REV, SMCFX_GATE8, SMCFX_GATE16, SMCFX_TRANCE, SMCFX_GATE, SMCFX_STUT, SMCFX_SHUF},
    {SMCFX_DLY, SMCFX_STUT, SMCFX_SHUF, SMCFX_GATE16, SMCFX_GATE8},
    {SMCFX_GATE16, SMCFX_GATE8, SMCFX_TRANCE, SMCFX_STUT, SMCFX_SHUF, SMCFX_HP, SMCFX_LP},
};
/* Owner 2026-11-12 02:33 UTC+10: 909, 808 and BREAK share one suit (supersedes #370). */
const uint8_t SMCFX_SUIT_N[SMCFX_N] = {6, 6, 7, 6, 6, 7, 5, 7};

/* The SELECT fallback stays suited when a page-two SELECT cell is empty.
 * Audible effects and displayed captions use each independent button mapping. */
/* ROOM/TIGHT/ECHO SELECT maps. ECHO is the third page from 7b2d1726 (#688).
 * ROOM comparison: the source ROOM was REV/DLY/HP/LP/GATE/REV/DLY/GATE16.
 * The tip ROOM (SHUF/SHUF/LP/REV/HP/TRANCE/GATE16/STUT) is later and was
 * the approved 0.0.25 layout (7e26c97). Both are fully suited after the
 * shared drum suit (470c94c). Keep the tip ROOM; add the source ECHO. */
const uint8_t SMCFX_PAGE[SMCFX_NPAGE][SMCFX_N] = {
    {SMCFX_SHUF, SMCFX_SHUF, SMCFX_LP, SMCFX_REV, SMCFX_HP, SMCFX_TRANCE, SMCFX_GATE16, SMCFX_STUT},
    {SMCFX_DLY, SMCFX_REV, SMCFX_HP, SMCFX_DLY, SMCFX_HP, SMCFX_GATE16, SMCFX_STUT, SMCFX_TRANCE},
    {SMCFX_DLY, SMCFX_REV, SMCFX_REV, SMCFX_REV, SMCFX_DLY, SMCFX_STUT, SMCFX_SHUF, SMCFX_GATE8},
};
const char *const SMCFX_PAGE_NAME[SMCFX_NPAGE] = {"ROOM", "TIGHT", "ECHO"};
const char *const SMCFX_SHORT[SMCFX_NFX] = {
    "GATE", "REV", "HPF", "DLY", "LPF", "G8", "G16", "TRNC", "STUT", "SHUF"};
const char *const SMCFX_LONG[SMCFX_NFX] = {"DECAY GATE", "REV THROW", "HIGH-PASS", "DLY THROW", "LOW-PASS",
                                            "8TH GATE", "16TH GATE", "TRANCE GATE", "STUTTER", "SHUFFLER"};
