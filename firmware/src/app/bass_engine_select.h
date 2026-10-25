/* SPDX-License-Identifier: GPL-3.0-only */
/* Per-part live bass backend selection (native303 ↔ WBL1).
 * Identities match MOT_SNAPSHOT engine bytes: 1 = 303, 2 = WBL1.
 * Product tip builds have native303 only; WBL1 select returns -1 until a
 * DEV_WOBBLE (or later public) runtime is linked. Session state only — not
 * project/stream schema. */
#pragma once
#include <stdint.h>
#include "../seq/pattern.h"

#ifndef X0X_DEV_WOBBLE
#define X0X_DEV_WOBBLE 0
#endif

enum { BASS_ENG_303 = 1, BASS_ENG_WBL1 = 2 };

void bass_engine_select_reset(void);
int bass_engine_active(int part);                 /* 1/2, or 0 if part out of range */
int bass_engine_available(int engine);            /* 1 if this build can make it live */
int bass_engine_select(int part, int engine);     /* 0 ok; -1 unavailable; -2 bad args */
/* Same short names as motion_snapshot_engine_label; empty if unknown. */
static inline const char *bass_engine_label(int engine)
{
    return engine == BASS_ENG_303 ? "303" : engine == BASS_ENG_WBL1 ? "WBL1" : "";
}
