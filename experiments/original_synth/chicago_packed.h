/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_PACKED_H
#define ORIGINAL_CHICAGO_PACKED_H
#include <stdint.h>
/* Exact immutable 44100-derived A/E table provider for MP4 prepare_reader.
 * Context must be NULL. Private coefficient payload, not approved for public use. */
int original_chicago_packed_lookup(const void *context,uint32_t index,float *a,float *e);
#endif
