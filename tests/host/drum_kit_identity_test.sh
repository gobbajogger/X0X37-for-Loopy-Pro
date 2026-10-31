#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host-only model identity guard. Run from any working directory.
set -eu
cd "$(dirname "$0")/../.."
OUT=${DRUM_IDENTITY_OUT:-build/host/drum-kit-identity}
mkdir -p "$OUT"
# drum909_dsp.h includes generated lookup tables. Regenerate in the test-only
# output directory to avoid stale shared build/gen and unrelated font/break inputs.
python3 tools/gen_drum_samples.py "$OUT/x0x_drum_samples.h" > "$OUT/generate.log" 2>&1 || { cat "$OUT/generate.log"; exit 1; }
${CC:-cc} -std=c11 -O2 -Wall -Wextra -Werror \
  -I"$OUT" -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ifirmware/src/seq \
  tests/host/drum_kit_identity_test.c -o "$OUT/test"
"$OUT/test"
