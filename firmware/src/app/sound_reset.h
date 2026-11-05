/* SPDX-License-Identifier: GPL-3.0-only */
/* Sound-only reset of one part's saved sound.
 *
 * Include undo.c in this translation unit, then sound_reset.c. The live
 * sound_t is the start of that undo region: the same bytes the app passes
 * to undo_init from the project sound. factory is the power-on image of
 * those cells (the part bytes engine_sound_defaults writes). A successful
 * reset copies that part's sound cells, and for a 303 its bass tag, then
 * commits one undo entry. Undo restores the previous sound.
 *
 * The write set is those sound cells. Mixer level, pan, reverb send, delay
 * send, cutoff and the mute byte at mixer_mute stay as they are.
 *
 * SOUND_RESET_NO_SAVED: every sound cell of the part, including a 303 bass
 * tag, is zero. No saved sound is present. The call leaves the sound, the
 * mixer and the undo pool as they are.
 */
#pragma once
#include "engine.h"

enum {
    SOUND_RESET_OK = 0,
    SOUND_RESET_NO_SAVED = 1,
    SOUND_RESET_BAD = -1
};

/* undo_t comes from the included undo.c. */
static int sound_reset_part(sound_t *sound, const sound_t *factory, int part,
                            uint8_t *mixer_mute, undo_t *undo);
