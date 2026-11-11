/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
#include "../../../experiments/original_synth/chicago_program.h"
#include "../../../experiments/original_synth/miami_program.h"
#include "../dsp/x0x_param.h"

#define ORIGINAL_SOUND_BYTES 32u
#define ORIGINAL_SOUND_CHICAGO 0u
#define ORIGINAL_SOUND_MIAMI 1u
#define ORIGINAL_SOUND_CHICAGO_ID UINT32_C(0x4f434831)
#define ORIGINAL_SOUND_MIAMI_ID UINT32_C(0x4f4d4931)
#define ORIGINAL_SOUND_SCHEMA 2u
#define ORIGINAL_CHICAGO_FACTORIES 55u
#define ORIGINAL_CHICAGO_FX_CHOICES 7u

/* Public control positions used by the physical performance wheels. */
enum { ORIGINAL_CHICAGO_CUTOFF = 2, ORIGINAL_CHICAGO_FX_TYPE = 17, ORIGINAL_CHICAGO_PITCH_BEND = 20,
       ORIGINAL_MIAMI_WOBBLE_SHAPE = 11, ORIGINAL_MIAMI_WOBBLE_DEPTH = 12, ORIGINAL_MIAMI_WOBBLE_RATE = 13,
       ORIGINAL_MIAMI_PITCH_BEND = 16 };

/* The final two macro IDs remain native runtime inputs, outside patch edits. */
static inline int original_sound_is_performance(unsigned kind, unsigned parameter)
{
    return (kind == ORIGINAL_SOUND_CHICAGO &&
            (parameter == ORIGINAL_CHICAGO_PITCH_BEND || parameter == ORIGINAL_CHICAGO_PITCH_BEND + 1u)) ||
           (kind == ORIGINAL_SOUND_MIAMI &&
            (parameter == ORIGINAL_MIAMI_PITCH_BEND || parameter == ORIGINAL_MIAMI_PITCH_BEND + 1u));
}

/* New firmware patch representation, not a legacy bank decoder:
 * [0..21] macro display bytes; [22..25] LE edit mask; [26..27] 'OS'; [28] version;
 * [29] engine kind; [30] selected factory index; [31] reserved zero.
 * Chicago indexes55 admitted programs and FxType stores a seven-choice index.
 * Both map explicitly to the original program/selector; schema1 is refused.
 * Untouched controls come from the immutable binary32 factory data, never
 * their rounded display bytes. Edits represent the firmware's 7-bit controls.
 * Pitch bend and damper are performance-only. Older schema2 bytes/edit bits
 * remain valid storage, but patch restoration ignores them and reports neutral
 * bend0.5/damper0. Their native IDs/counts remain available to wheels and MIDI.
 * This stores a patch, not arbitrary AU floats or live voice/history state.
 * Every cell argument points to 32 accessible bytes. Source/output may alias;
 * failed validation or preparation leaves the destination unchanged. */
unsigned original_sound_count(unsigned kind);
unsigned original_sound_factories(unsigned kind);
/* Returns the immutable original program index, or -1 on invalid input. */
int original_sound_factory_source(unsigned kind, unsigned factory);
/* Convert a firmware control to its actual original macro. FxType is discrete;
 * Chicago ENV4 and Miami TUNE7 have exact midpoint64; all others use q/127.
 * Failed conversion does not change *value. */
int original_sound_control_value(unsigned kind, unsigned parameter, unsigned control, float *value);
const char *original_sound_fx_name(unsigned choice);
uint32_t original_sound_engine_id(unsigned kind);
const char *original_sound_name(unsigned kind, unsigned factory);
const x0x_param_t *original_sound_param(unsigned kind, unsigned parameter);
int original_sound_valid(const uint8_t cell[ORIGINAL_SOUND_BYTES]);
int original_sound_init(uint8_t cell[ORIGINAL_SOUND_BYTES], unsigned kind, unsigned factory);
int original_sound_set(uint8_t cell[ORIGINAL_SOUND_BYTES], unsigned parameter, unsigned value);
uint32_t original_sound_edit_mask(const uint8_t cell[ORIGINAL_SOUND_BYTES]);
int original_sound_restore_miami(original_miami_program *, const uint8_t cell[ORIGINAL_SOUND_BYTES]);
int original_sound_restore_chicago(original_chicago_program *, const uint8_t cell[ORIGINAL_SOUND_BYTES]);
int original_sound_value(const uint8_t cell[ORIGINAL_SOUND_BYTES], unsigned parameter, float *value);
