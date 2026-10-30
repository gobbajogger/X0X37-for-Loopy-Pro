/* SPDX-License-Identifier: GPL-3.0-only */
/* Shared pan / glide bodies (one copy for 808, 909, and engine). */

#include "x0x_param.h"

void x0x_pan_gains(int pot, float *l, float *r)
{
    float q = pot >= 64 ? (float)(pot - 64) / 63.0f : (float)(pot - 64) / 64.0f, x, c;
    x = q * 1.5707963f;
    x *= x;
    c = 1.0f + x * (-0.5f + x * (0.041666667f + x * (-0.0013888889f + x * 0.0000248016f)));
    if (pot <= 0 || pot >= 127)
        c = 0.0f;                                 /* hard over: nothing on the far side, exactly */
    *l = q > 0.0f ? c : 1.0f;
    *r = q < 0.0f ? c : 1.0f;
}

float x0x_glide_block(float *cur, float tgt, int n)
{
    float c = *cur, e;
    if (c == tgt)
        return 0.0f;
    e = (tgt - c) * (n >= 441 ? 1.0f : (float)n * (1.0f / 441.0f));   /* ~1 - exp(-n / 441) for small n */
    if (tgt - (c + e) < 1e-5f && (c + e) - tgt < 1e-5f)
        e = tgt - c;
    *cur = c + e;
    return e / (float)n;
}
