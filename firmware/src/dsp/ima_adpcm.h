/* SPDX-License-Identifier: GPL-3.0-only */
/* IMA ADPCM decoder step, shared by the BREAK player (breaks.c) and the 909 cymbals
 * (drum909.c). Format: 4 bit, mono, low nibble first, initial state predictor 0 /
 * index 0 (tools/sampleio.py ima_encode writes it).
 *
 * The tables and the decoder step are from Felucca's eng_sample.c
 * (Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hugelton Instruments, GPL-3.0).
 *
 * Tables are defined once in the TU that sets X0X_IMA_TABLES_DEFINE (drum909.c), and
 * declared extern elsewhere — same pattern as x0x_drum_tables.h. Host unit tests that
 * include a DSP file alone may define X0X_IMA_TABLES_DEFINE before the include. */
#pragma once
#include <stdint.h>

#ifdef X0X_IMA_TABLES_DEFINE
const int16_t IMA_STEP_T[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97,
    107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428,
    4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350,
    22385, 24623, 27086, 29794, 32767};
const int8_t IMA_IDX_T[8] = {-1, -1, -1, -1, 2, 4, 6, 8};
#else
extern const int16_t IMA_STEP_T[89];
extern const int8_t IMA_IDX_T[8];
#endif

/* decode nibble i of d; updates the decoder state and returns the new sample */
static inline int32_t ima_decode(const uint8_t *d, uint32_t i, int32_t *pred, int32_t *idx)
{
    uint32_t byte = d[i >> 1], code = (i & 1u) ? (byte >> 4) : (byte & 15u);
    int32_t step = IMA_STEP_T[*idx], vd = step >> 3, p, x;
    if (code & 4u)
        vd += step;
    if (code & 2u)
        vd += step >> 1;
    if (code & 1u)
        vd += step >> 2;
    p = *pred + ((code & 8u) ? -vd : vd);
    *pred = p < -32768 ? -32768 : p > 32767 ? 32767 : p;
    x = *idx + IMA_IDX_T[code & 7u];
    *idx = x < 0 ? 0 : x > 88 ? 88 : x;
    return *pred;
}
