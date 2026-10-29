/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_MASTER_UNMUTED_H
#define ORIGINAL_MASTER_UNMUTED_H
#include <stdint.h>
typedef struct {float current_gain;} original_master_unmuted;
/* Explicit initial gain, NOT an inferred original constructor/reset value.
 * Only unchanged unmuted original branch. Mute transitions remain unsupported. */
int original_master_unmuted_seed(original_master_unmuted *s,float gain);
/* Caller supplies current native96 control0..1; increment gain before each
 * stereo frame, then identical cleanup polynomial; force final gain to target.
 * n0 native-empty is no-op. In/out aliases allowed, channels disjoint. */
int original_master_unmuted_process(original_master_unmuted *s,float native96,
 const uint32_t *inL,const uint32_t *inR,uint32_t *outL,uint32_t *outR,uint32_t n);
#endif
