/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_MIAMI_ZERO_H
#define ORIGINAL_MIAMI_ZERO_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Idle buffers must contain actual positive-zero samples, including every
 * scratch/workspace byte. Keep the caller-visible writes while letting the
 * compiler choose its optimized bulk store rather than expanding a 32-sample
 * scalar assignment tree per channel. Explicit builtins still optimize with
 * the host proof's -fno-builtin flag. No DSP or floating-point math is done. */
#if defined(__GNUC__) || defined(__clang__)
#define MIAMI_ZERO_BYTES(p, n) __builtin_memset((p), 0, (n))
#else
#define MIAMI_ZERO_BYTES(p, n) memset((p), 0, (n))
#endif

static inline void original_miami_zero_words(uint32_t *p, uint32_t n)
{
    if (n) MIAMI_ZERO_BYTES(p, (size_t)n * sizeof *p);
}

static inline void original_miami_zero_halves(uint16_t *p, uint32_t n)
{
    if (n) MIAMI_ZERO_BYTES(p, (size_t)n * sizeof *p);
}

static inline void original_miami_zero_floats(float *p, uint32_t n)
{
    /* The Miami runtime uses IEEE-754 binary32; all-zero bytes are +0.0f. */
    if (n) MIAMI_ZERO_BYTES(p, (size_t)n * sizeof *p);
}

#undef MIAMI_ZERO_BYTES
#endif
