/* SPDX-License-Identifier: GPL-3.0-only */
/* One packed copy of the twelve pitch-class names (C … B), plus one pointer
 * table into that pack. The UI note labels, clip-mode ROOT, and chord spelling
 * all read these — keep a single copy in each translation unit.
 * Include once per translation unit. */
#pragma once

static const char X0X_NOTE_PACK[] = "C\0C#\0D\0D#\0E\0F\0F#\0G\0G#\0A\0A#\0B";

_Static_assert(sizeof X0X_NOTE_PACK == 29, "note pack size");

/* Pointers into X0X_NOTE_PACK (same offsets as the old per-literal NOTE_N). */
static const char *const X0X_NOTE_N[12] = {
    X0X_NOTE_PACK + 0,  X0X_NOTE_PACK + 2,  X0X_NOTE_PACK + 5,  X0X_NOTE_PACK + 7,
    X0X_NOTE_PACK + 10, X0X_NOTE_PACK + 12, X0X_NOTE_PACK + 14, X0X_NOTE_PACK + 17,
    X0X_NOTE_PACK + 19, X0X_NOTE_PACK + 22, X0X_NOTE_PACK + 24, X0X_NOTE_PACK + 27
};
