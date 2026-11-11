#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# B4 real writers + opt-in trace. Run on the Orchestrator's box, not in Codex.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-build/param-write-sources}
mkdir -p "$OUT"
awk '/^static void smk_wheels\(void\)/{on=1} on{print} on && /^}/{exit}' firmware/src/app/main_fm1.c > "$OUT/smk_wheels.inc"
test -s "$OUT/smk_wheels.inc"
sh host/build_host.sh --generate-only >"$OUT/assets.log" 2>&1
. tests/host/original_build.sh
# shellcheck disable=SC2086
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter ${SAN_FLAGS:-} \
 -DX0X_HOST -DX0X_PARAM_TRACE=1 -DX0X_BASS_PUBLISH_TEST=1 -DX0X_SMK_CAPTIONS=1 -DBLEC_TWO_PEER=1 \
 -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen -I"$OUT" \
 tests/host/param_write_sources_test.c $ORIGINAL_FLAGS $ORIGINAL_SOURCES -lm -o "$OUT/test"
for voice in 0 1; do
    for bank in 0 1; do
        log="$OUT/voice-$voice-bank-$bank.log"
        if ! "$OUT/test" "$voice" "$bank" >"$log" 2>&1; then cat "$log"; exit 1; fi
        for source in wheel-pitch wheel-mod panel-knob smc-knob motion motion-restore preset midi; do
            if ! grep -q "source=$source target=2 voice=$voice " "$log"; then
                cat "$log"; echo "FAIL: missing B4 $source trace for voice $voice" >&2; exit 1
            fi
        done
        if ! grep -q 'source=smc-pickup target=5 ' "$log"; then
            cat "$log"; echo 'FAIL: missing B4 normal mixer pickup trace' >&2; exit 1
        fi
        tail -1 "$log"
    done
done
