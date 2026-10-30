#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Strict C99 must link header helpers from multiple host units, even at -O0.
set -eu
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
mkdir -p build/gen
[ -f build/gen/x0x_drum_tables.h ] || python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h >/dev/null
cat > "$OUT/unit.c" <<'C'
#include "dsp/drum909_dsp.h"
float ENTRY(float x)
{
    d9_biquad_t f = {0};
    d9_shape_t s;
    d9_biquad_set(&f, D9_LP, 1000.0f, 0.7f);
    d9_shape_prep(&s, 1.0f, 0);
    return fm_floorf(x) + fm_sin_pi(x) + fm_wrap_pi(x)
         + fm_sinf(x) + fm_cosf(x) + d9_tanh(x) + f.b0 + s.g;
}
C
cat > "$OUT/main.c" <<'C'
#include <assert.h>
#include <stdio.h>
#define X0X_DRUM_TABLES_DEFINE
#include "x0x_drum_tables.h"
float unit_a(float x);
float unit_b(float x);
int main(void)
{
    float a = unit_a(0.25f), b = unit_b(0.25f);
    assert(a == b);
    assert(a > 3.0f && a < 4.0f);
    puts("dsp host linkage: PASS (two units, real shared table)");
    return 0;
}
C
for opt in 0 2; do
    "${CC:-cc}" -std=c99 -O"$opt" -ffp-contract=off -Wall -Wextra -Werror \
        -DX0X_HOST -DENTRY=unit_a -Ifirmware/src -Ibuild/gen -c "$OUT/unit.c" -o "$OUT/a.o"
    "${CC:-cc}" -std=c99 -O"$opt" -ffp-contract=off -Wall -Wextra -Werror \
        -DX0X_HOST -DENTRY=unit_b -Ifirmware/src -Ibuild/gen -c "$OUT/unit.c" -o "$OUT/b.o"
    "${CC:-cc}" -std=c99 -O"$opt" -Wall -Wextra -Werror -Ibuild/gen \
        "$OUT/main.c" "$OUT/a.o" "$OUT/b.o" -o "$OUT/test"
    "$OUT/test"
done
