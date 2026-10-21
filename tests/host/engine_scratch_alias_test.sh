#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Full-engine differential for scratch reuse; software only, no radio/device I/O.
# Compare current engine with BASE_REF engine using identical current DSP/assets.
# Shift UB at unchanged drum808.c:184 and unavailable ptrace LeakSanitizer are
# excluded explicitly; remaining address/undefined checks halt on first error.
set -eu
cd "$(dirname "$0")/../.."
ROOT=$(pwd)
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
BASE_REF=${BASE_REF:-2695701fe9ae63dbb6f1f76d4dc11ebbf39da834}
git show "$BASE_REF:firmware/src/app/engine.c" > "$OUT/engine-base.c"
sh host/build_host.sh --generate-only > "$OUT/assets.log" 2>&1 || { cat "$OUT/assets.log"; exit 1; }
for variant in base current; do
    ENGINE="$ROOT/firmware/src/app/engine.c"
    [ "$variant" != base ] || ENGINE="$OUT/engine-base.c"
    ${CC:-cc} -O2 -g -ffp-contract=off -std=c99 -Wno-unused-function -Wno-unused-parameter \
        -DX0X_HOST -D_POSIX_C_SOURCE=200809L -fsanitize=address,undefined -fno-sanitize=shift \
        -pthread -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen \
        "-DHOST_SOURCE=\"$ROOT/host/x0x_host.c\"" "-DENGINE_SOURCE=\"$ENGINE\"" \
        tests/host/engine_scratch_alias_test.c \
        firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c \
        firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c \
        firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c \
        firmware/src/seq/motion.c -lm -o "$OUT/$variant"
    for mode in 0 1 2; do
        ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 \
            "$OUT/$variant" "$mode" > "$OUT/$variant-$mode.bin"
    done
done
for mode in 0 1 2; do cmp "$OUT/base-$mode.bin" "$OUT/current-$mode.bin"; done
for mode in 1 2; do cmp "$OUT/base-0.bin" "$OUT/base-$mode.bin"; done
sha256sum "$OUT"/*.bin
printf 'PASS: 4096 blocks each; PCM + float scratch/glides/meters/scope; serial, inline and threaded\n'
