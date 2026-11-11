#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Repeatable 512-frame whole-app stress probe. Host timings are not AC79 timing.
set -eu
cd "$(dirname "$0")/../.."
OUT=${X0X_STRESS_OUT:-build/host/audio-stress}
mkdir -p "$OUT"
python3 tests/host/audio_stress_report_test.py
awk '/^static void smk_wheels\(void\)/{on=1} on{print} on && /^}/{exit}' firmware/src/app/main_fm1.c > "$OUT/smk_wheels.inc"
test -s "$OUT/smk_wheels.inc"
. tests/host/original_build.sh
case "${X0X_STRESS_EXACT:-0}" in 0) ORIGINAL_FLAGS="$ORIGINAL_FLAGS -DX0X_FAST_FMA" ;; 1) ;; *) exit 2 ;; esac
sh host/build_host.sh --generate-only >"$OUT/assets.log" 2>&1
compile() {
    ${CC:-cc} -O2 -g -std=c99 -Wno-unused-function -Wno-unused-parameter \
        -DX0X_HOST $ORIGINAL_FLAGS ${SAN_FLAGS:-} -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen -I"$OUT" "$@"
}
# Rename only the runtime object. Its internal calls remain internal, so one
# outer timed entry includes their work exactly once. Works with ELF and Mach-O.
renames=
for entry in prepare key param init render release_source cut_source all_off retire; do
    renames="$renames -Doriginal_runtime_$entry=stress_real_original_runtime_$entry"
done
compile $renames -c firmware/src/app/original_runtime.c -o "$OUT/runtime.o"
sources=
for source in $ORIGINAL_SOURCES; do
    case "$source" in firmware/src/app/original_runtime.c) ;; *) sources="$sources $source" ;; esac
done
compile tests/host/audio_stress_test.c $sources "$OUT/runtime.o" -lm -o "$OUT/stress"
# A missing injected note must fail the consumer accounting, even though the
# remaining voices still sound and both source bitmasks would have been set.
status=0
X0X_STRESS_DROP_ONE=1 "$OUT/stress" "$OUT/negative.csv" "$OUT/negative.s32" 64 0 0 >"$OUT/negative.log" 2>&1 || status=$?
if [ "$status" != 3 ]; then cat "$OUT/negative.log"; echo 'FAIL: missing-note negative control'; exit 1; fi
echo 'PASS missing-note negative control'
for workload in plain chorus peaking; do
    case "$workload" in plain) factory=0; choice=0 ;; chorus) factory=29; choice=1 ;; peaking) factory=18; choice=6 ;; esac
    mkdir -p "$OUT/$workload"
    "$OUT/stress" "$OUT/$workload/blocks.csv" "$OUT/$workload/audio.s32" "${X0X_STRESS_BLOCKS:-1024}" "$factory" "$choice"
    python3 tools/audio_stress_report.py --csv "$OUT/$workload/blocks.csv" --pcm "$OUT/$workload/audio.s32" --out "$OUT/$workload/report.json"
done
