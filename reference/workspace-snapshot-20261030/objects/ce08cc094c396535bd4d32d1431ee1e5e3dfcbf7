#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Build tools/gen_builtin_break.c with the host compiler against the 909 port and run it:
# writes OUTDIR/x0x_break_bank.h: the BREAK part's default loops (no recordings).
# Parallel host tests (class P self-gen beside class G generate-only) used to overwrite
# the shared build/host/gen_builtin_break path and fail with ETXTBSY / "Text file busy".
# Compile a private binary, serialize writers, and publish the header with an atomic mv.
set -e
cd "$(dirname "$0")/.."
OUT="${1:-build/gen}"
mkdir -p "$OUT" build/host
lockdir=build/host/.gen_builtin_break.lockdir
bin=
hdr=
while ! mkdir "$lockdir" 2>/dev/null; do sleep 0.05; done
trap 'rm -f ${bin:+"$bin"} ${hdr:+"$hdr"}; rmdir "$lockdir" 2>/dev/null' EXIT
# drum909_dsp.h needs x0x_drum_tables.h beside samples; regenerate if either is missing.
[ -f "$OUT/x0x_drum_samples.h" ] && [ -f "$OUT/x0x_drum_tables.h" ] \
    || python3 tools/gen_drum_samples.py "$OUT/x0x_drum_samples.h" >/dev/null
bin=$(mktemp "$PWD/build/host/gen_builtin_break.XXXXXX")
hdr=$(mktemp "$PWD/$OUT/x0x_break_bank.h.XXXXXX")
${CC:-cc} -O2 -ffp-contract=off -w -Ifirmware/src -I"$OUT" -o "$bin" \
    tools/gen_builtin_break.c firmware/src/dsp/drum909.c firmware/src/dsp/x0x_param.c -lm
"$bin" "$hdr" ${2:+"$2"}
mv -f "$hdr" "$OUT/x0x_break_bank.h"
hdr=
# Stable name for local debugging only; never executed by this script.
cp -f "$bin" build/host/gen_builtin_break 2>/dev/null || true
