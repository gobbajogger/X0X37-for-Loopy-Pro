/* SPDX-License-Identifier: GPL-3.0-only */
/* Focused host coverage for per-part bass engine select/label without WBL1 DSP. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/app/bass_engine_select.c"
#include "../../firmware/src/seq/motion.h"

int main(void)
{
    assert(!strcmp(bass_engine_label(BASS_ENG_303), "303"));
    assert(!strcmp(bass_engine_label(BASS_ENG_WBL1), "WBL1"));
    assert(!bass_engine_label(0)[0] && !bass_engine_label(3)[0]);
    assert(!strcmp(motion_snapshot_engine_label(1), bass_engine_label(1)));
    assert(!strcmp(motion_snapshot_engine_label(2), bass_engine_label(2)));

    bass_engine_select_reset();
    assert(bass_engine_active(0) == BASS_ENG_303);
    assert(bass_engine_active(1) == BASS_ENG_303);
    assert(bass_engine_active(-1) == 0);
    assert(bass_engine_active(NBASS) == 0);

    assert(bass_engine_available(BASS_ENG_303));
#if X0X_DEV_WOBBLE
    assert(bass_engine_available(BASS_ENG_WBL1));
    assert(bass_engine_select(0, BASS_ENG_WBL1) == 0);
    assert(bass_engine_active(0) == BASS_ENG_WBL1);
    assert(bass_engine_active(1) == BASS_ENG_303); /* part B untouched */
#else
    /* Product tip / operator builds: WBL1 menu exists, runtime does not. */
    assert(!bass_engine_available(BASS_ENG_WBL1));
    assert(bass_engine_select(0, BASS_ENG_WBL1) == -1);
    assert(bass_engine_active(0) == BASS_ENG_303); /* refused: no silent swap */
#endif
    assert(bass_engine_select(0, 99) == -2);
    assert(bass_engine_select(-1, BASS_ENG_303) == -2);
    assert(bass_engine_select(0, BASS_ENG_303) == 0);
    assert(bass_engine_active(0) == BASS_ENG_303);

    /* Selecting 303 again is idempotent. */
    assert(bass_engine_select(1, BASS_ENG_303) == 0);
    bass_engine_select_reset();
    assert(bass_engine_active(0) == BASS_ENG_303 && bass_engine_active(1) == BASS_ENG_303);

    puts("bass engine select: labels, refuse-WBL1-without-runtime, reset, bounds PASS");
    return 0;
}
