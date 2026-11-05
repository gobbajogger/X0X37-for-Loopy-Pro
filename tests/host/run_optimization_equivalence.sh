#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Recut optimization contracts: exact output, state and persisted bytes.
set -eu
cd "$(dirname "$0")/../.."
OUT=build/optimization-equivalence
mkdir -p "$OUT" build/gen
[ -f build/gen/x0x_drum_tables.h ] || python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h
CC=${CC:-cc}
check() {
    echo "CHECK $*"
    "$@"
}
compile_run() {
    name=$1; shift
    check "$CC" -std=c11 -O2 -ffp-contract=off -fno-fast-math -Wall -Wextra -Werror \
        -Wno-unused-function -ffunction-sections -fdata-sections \
        -DX0X_HOST -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen \
        "$@" -Wl,--gc-sections -lm -o "$OUT/$name"
    check "$OUT/$name"
}
compile_run lz-max-match tests/host/lz_max_match_test.c
compile_run undo-word-scan tests/host/undo_word_scan_test.c
check sh tests/host/breaks_silent_skip_test.sh
check sh tests/host/clip_latency_equivalence_test.sh
check sh tests/host/run_project_stream_copy.sh
check python3 tests/host/run_a3_dsp_equivalence.py master
check python3 tests/host/run_a3_dsp_equivalence.py drum808
check python3 tests/host/run_a3_dsp_equivalence.py fxbus
check sh tests/host/master_idle_identity_test.sh "$OUT/master-idle"
check sh tests/host/fxbus_idle_identity_test.sh "$OUT/fxbus-idle"
check sh tests/host/fxbus_coef_cache_test.sh
check sh tests/host/chicago_peaking_coef_cache_test.sh
check sh tests/host/vca_sustain_test.sh
check python3 tests/host/run_chicago_ens_hoist.py
check python3 tests/host/run_chicago_modulation_hoist.py
check python3 tests/host/engine_mix_lut_test.py level
check python3 tests/host/engine_mix_lut_test.py send
compile_run control-lut tests/host/original_control_lut_test.c
compile_run gain-lut tests/host/original_gain_lut_test.c
compile_run break-retrigger-lut tests/host/break_retrigger_lut_test.c
compile_run busfx-invariants tests/host/busfx_invariant_test.c
check bash tests/host/run_ima_delta_lut.sh
check sh tests/host/run_drum909_emit_equivalence.sh
printf 'PASS all optimization equivalence checks\n'
