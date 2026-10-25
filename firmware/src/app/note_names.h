/* SPDX-License-Identifier: GPL-3.0-only */
/* One packed copy of the twelve pitch-class names (C … B).
 * The UI switch labels, clip-mode ROOT, and chord spelling all read these bytes.
 * Include once per translation unit. */
#pragma once

static const char X0X_NOTE_PACK[] = "C\0C#\0D\0D#\0E\0F\0F#\0G\0G#\0A\0A#\0B";

_Static_assert(sizeof X0X_NOTE_PACK == 29, "note pack size");
