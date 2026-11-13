/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_PROGRAM_ENVELOPE_H
#define ORIGINAL_PROGRAM_ENVELOPE_H
#include <stddef.h>
#include "miami_program.h"
#include "chicago_program.h"
#define ORIGINAL_MIAMI_PROGRAM_ENVELOPE_BYTES 188u
#define ORIGINAL_CHICAGO_PROGRAM_ENVELOPE_BYTES 224u
/* Proposed engine-owned pointer-free envelope v1, not an adopted storage
 * format or legacy migration. Explicit tag/version/backend, little endian
 * binary32 macro/target words and revision, with FNV32 integrity checksum.
 * Every original target bit is saved; no UI-byte round trip/recomputation.
 * Decode validates the whole envelope before publishing a program. Legacy,
 * wrong-kind/version/length, nonfinite and out-of-domain input leaves it
 * unchanged. Existing storage owner chooses routing/atomic adoption and also
 * owns full per-note/FX/history/queue persistence; this envelope is PROGRAM ONLY.
 * Output/source/program arrays do not overlap for encode. Decode may overlap
 * input since it stages the complete small program before the final assignment.
 * No allocation, filesystem access, activation or public writes. */
int original_miami_program_encode(const original_miami_program *,void *bytes,size_t capacity);
int original_miami_program_decode(original_miami_program *,const void *bytes,size_t length);
int original_chicago_program_encode(const original_chicago_program *,void *bytes,size_t capacity);
int original_chicago_program_decode(original_chicago_program *,const void *bytes,size_t length);
#endif
