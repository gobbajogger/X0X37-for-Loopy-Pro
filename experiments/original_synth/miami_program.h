/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_MIAMI_PROGRAM_H
#define ORIGINAL_MIAMI_PROGRAM_H
#include <stdint.h>
/* Original normalized public controls and their canonical native overrides.
 * Common native context is immutable and shared by all 30 original programs.
 * This is an engine-owned snapshot, not a new project/storage format. Existing
 * legacy byte programs keep their separate backend and meaning. */
typedef struct {
 float macro[18];
 float target[24];
 uint32_t revision;
} original_miami_program;
/* A program is valid only after successful init. Set is transactional, finite
 * normalized [0,1] input; source/render must own distinct immutable snapshots.
 * Original macro values stay binary32 and never round-trip through UI bytes. */
int original_miami_program_init(original_miami_program *program,unsigned factory);
int original_miami_program_set(original_miami_program *program,unsigned parameter,float normalized);
int original_miami_program_native(const original_miami_program *program,unsigned native_id,float *normalized);
const char *original_miami_program_name(unsigned factory);
/* Tables/model context are not activated by this API. Callers must still close
 * native conversion, note lifecycle and the audible renderer before adoption. */
#endif
