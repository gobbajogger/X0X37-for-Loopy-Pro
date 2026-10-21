/* SPDX-License-Identifier: GPL-3.0-only */
/* Include the private set-time helper for exact bit-stream comparison. */
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include SOURCE
static void emit(float x)
{
    fm_bits_t b;
    b.f = d8_exp2_cr(x);
    assert(fwrite(&b.u, sizeof b.u, 1, stdout) == 1);
}
int main(void)
{
    int i;
    /* Exact powers independently verify the intended exponent adjustment. */
    for (i = -126; i <= 127; ++i) {
        fm_bits_t b;
        b.f = d8_exp2_cr((float)i);
        assert(b.u == (uint32_t)(i + 127) * UINT32_C(8388608));
    }
    /* Dense domain sweep, including clamped regions. */
    for (i = -128 * 4096; i <= 129 * 4096; ++i)
        emit((float)i / 4096.0f);
    /* Both adjacent representable inputs at every rounding transition. */
    for (i = -126; i < 127; ++i) {
        float x = (float)i + 0.5f;
        emit(nextafterf(x, -INFINITY)); emit(x); emit(nextafterf(x, INFINITY));
    }
    emit(-INFINITY); emit(INFINITY); emit(-0.0f);
    emit(nextafterf(-126.0f, -INFINITY));
    emit(nextafterf(-126.0f, INFINITY));
    emit(nextafterf(127.0f, -INFINITY));
    emit(nextafterf(127.0f, INFINITY));
    return 0;
}
