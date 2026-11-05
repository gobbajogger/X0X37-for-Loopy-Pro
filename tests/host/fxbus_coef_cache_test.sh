#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Compare live FX-bus samples and complete state to the immutable pre-cache source.
# Requires the pinned base object in the local repository; never uses the network.
set -eu
cd "$(dirname "$0")/../.."
OUT=${X0X_FXBUS_COEF_OUT:-/tmp/x0x-fxbus-coef-cache}
mkdir -p "$OUT"
BASE=e9e6643e612b9fb4e968a77d016f3bb24213dc88
git show "$BASE:firmware/src/dsp/fxbus.h" > "$OUT/fxbus.h"
git show "$BASE:firmware/src/dsp/fxbus.c" | sed 's|"../x0x_cold.h"|"x0x_cold.h"|' > "$OUT/reference.c"
cat > "$OUT/wrapper.c" <<'WRAP'
#include "reference.c"
#include <string.h>
#include <stddef.h>
size_t ref_size(void) { return sizeof(fxbus_t); }
void ref_init(void *p, int16_t *d, int n) { fxbus_init(p,d,n); }
void ref_set(void *p, int i, int v) { fxbus_set(p,i,v); }
void ref_bpm(void *p, float v) { fxbus_set_bpm(p,v); }
void ref_process(void *p, const float *d, const float *r, const float *l, float *o, int n) { fxbus_process(p,d,r,l,o,n); }
void ref_st(void *p, const float *dl, const float *dr, const float *r, const float *l, float *ol, float *or_, int n) { fxbus_process_st(p,dl,dr,r,l,ol,or_,n); }
void ref_snapshot(void *p, void *out) {
    fxbus_t s = *(fxbus_t *)p;
    /* Only the representation of the cached scalar changes. */
    s.rv_tone = 0.75f - s.rv_tone * 0.55f;
    memcpy(out,&s,sizeof s);
}
WRAP
RENAMES=""
for name in fxbus_init fxbus_set fxbus_set_bpm fxbus_get fxbus_param fxbus_nparams fxbus_param_si fxbus_process fxbus_process_st; do
    RENAMES="$RENAMES -D$name=old_$name"
done
CC=${CC:-cc}
FLAGS="-O2 -std=c99 -fno-fast-math -ffp-contract=off -Wall -Wextra -Werror -Ifirmware/src -Ifirmware/src/dsp -I$OUT"
# Generate only the required nonsecret mathematical tables, in scratch storage.
python3 tools/gen_drum_samples.py "$OUT/x0x_drum_samples.h" >/dev/null
set -x
$CC $FLAGS ${EXTRA_CFLAGS:-} $RENAMES -c "$OUT/wrapper.c" -o "$OUT/reference.o"
$CC $FLAGS ${EXTRA_CFLAGS:-} tests/host/fxbus_coef_cache_test.c firmware/src/dsp/fxbus.c "$OUT/reference.o" -lm -o "$OUT/test"
"$OUT/test"
