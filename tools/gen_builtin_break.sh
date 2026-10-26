#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Build tools/gen_builtin_break.c with the host compiler against the 909 port and run it:
# writes OUTDIR/x0x_break_bank.h: the BREAK part's default loops (no recordings).
set -e
cd "$(dirname "$0")/.."
OUT="${1:-build/gen}"
mkdir -p "$OUT" build/host
# drum909_dsp.h needs x0x_drum_tables.h beside samples; regenerate if either is missing.
[ -f "$OUT/x0x_drum_samples.h" ] && [ -f "$OUT/x0x_drum_tables.h" ] \
    || python3 tools/gen_drum_samples.py "$OUT/x0x_drum_samples.h" >/dev/null
${CC:-cc} -O2 -ffp-contract=off -w -Ifirmware/src -I"$OUT" -o build/host/gen_builtin_break \
    tools/gen_builtin_break.c firmware/src/dsp/drum909.c -lm
build/host/gen_builtin_break "$OUT/x0x_break_bank.h" ${2:+"$2"}
