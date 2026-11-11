#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# The 909 cymbals as IMA ADPCM: tests/host/drum909_adpcm_test.c against tests/host/cymbal_ref.py's
# references (the approved A/B study in ../adpcm_ab when it is there; set CYM_AB to another folder).
#   tests/host/run_drum909_adpcm.sh
set -e
cd "$(dirname "$0")/../.."
CC="${CC:-cc}"
OUT=build/host
mkdir -p "$OUT"
python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h >/dev/null
python3 tests/host/cymbal_ref.py "$OUT/cym_ref" ${CYM_AB:+"$CYM_AB"}
"$CC" -std=c99 -O2 -ffp-contract=off -Wall -Wextra -Wdouble-promotion -Wno-unused-function -Werror \
    -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen -o "$OUT/drum909_adpcm_test" tests/host/drum909_adpcm_test.c -lm
"$OUT/drum909_adpcm_test" "$OUT/cym_ref"
