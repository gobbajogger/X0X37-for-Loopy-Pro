/* SPDX-License-Identifier: GPL-3.0-only */
#include "bass_engine_select.h"

/* Live backend per bass voice. Defaults to native303. A future WBL1 runtime
 * (X0X_DEV_WOBBLE or public switch) must call bass_engine_select after the
 * audio boundary commits — never silently replace part A without updating this. */
static uint8_t bass_live_engine[NBASS];

void bass_engine_select_reset(void)
{
    int p;
    for (p = 0; p < NBASS; p++)
        bass_live_engine[p] = (uint8_t)BASS_ENG_303;
}

int bass_engine_available(int engine)
{
    if (engine == BASS_ENG_303)
        return 1;
#if X0X_DEV_WOBBLE
    if (engine == BASS_ENG_WBL1)
        return 1;
#endif
    return 0;
}

int bass_engine_active(int part)
{
    if ((unsigned)part >= NBASS)
        return 0;
    return (int)bass_live_engine[part];
}

int bass_engine_select(int part, int engine)
{
    if ((unsigned)part >= NBASS)
        return -2;
    if (engine != BASS_ENG_303 && engine != BASS_ENG_WBL1)
        return -2;
    if (!bass_engine_available(engine))
        return -1;
    bass_live_engine[part] = (uint8_t)engine;
    return 0;
}
