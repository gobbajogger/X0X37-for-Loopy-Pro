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
 * mixer_mute is only a null check. Commit mixer edits before this call.
 * An uncommitted mixer byte in the undo region joins the same undo entry,
 * and undoing the reset writes that mixer byte back.
 *
 * Pass factory and sound as two objects that do not overlap. memcpy copies
 * the cells, so a shifted alias is undefined. The call returns
 * SOUND_RESET_BAD when the two pointers are equal.
 *
 * SOUND_RESET_NO_SAVED: every sound cell of the part, including a 303 bass
 * tag, is zero. No saved sound is present. The call leaves the sound, the
 * mixer and the undo pool as they are.
 *
 * SOUND_RESET_OK includes a part that already matches factory. The copy
 * changes nothing, undo_commit adds no entry, and the status is still OK.
 *
 * When the diff does not fit the undo pool, undo_commit clears that pool
 * and returns -1 (the clear lives in undo.c). This call then copies the
 * part's previous sound cells back into the sound and into the undo
 * shadow, and returns SOUND_RESET_BAD. The part matches the bytes from
 * before the call and is not a pending diff. Bytes outside the part stay
 * as undo.c left them: the pool is empty and that region is the baseline.
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
