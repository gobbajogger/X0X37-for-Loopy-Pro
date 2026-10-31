#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Real scanner, original engines, MIDI/SMC/UI and audio-navigation equivalence.
set -eu
TEST_ROOT=$(CDPATH= cd "$(dirname "$0")/../.." && pwd)
SRC=${X0X_SOURCE_ROOT:-$TEST_ROOT}
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
awk '/^static void smk_wheels\(void\)/{on=1} on{print} on && /^}/{exit}' "$SRC/firmware/src/app/main_fm1.c" > "$OUT/smk_wheels.inc"
test -s "$OUT/smk_wheels.inc"
cd "$SRC"
sh host/build_host.sh --generate-only > "$OUT/assets.log" 2>&1 || { cat "$OUT/assets.log"; exit 1; }
. tests/host/original_build.sh
# The real engine gets different function symbols so only the instrumented
# wheel/UI call sites increment the command counter. Link engine exactly once.
U=$(printf '%s\n' "$ORIGINAL_SOURCES" | tr ' ' '\n' | grep -v '^firmware/src/app/engine.c$' | tr '\n' ' ')
HOST_DEFS='-D_POSIX_C_SOURCE=200809L'
if [ "$(uname -s)" = Darwin ]; then HOST_DEFS="$HOST_DEFS -D_DARWIN_C_SOURCE"; fi
# shellcheck disable=SC2086
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -DX0X_HOST $HOST_DEFS \
    -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen $ORIGINAL_FLAGS \
    -Dengine_set=wheel_real_engine_set -Dengine_set_base=wheel_real_engine_set_base \
    -Dengine_bass_mod=wheel_real_engine_bass_mod -Dengine_bass_mod_param=wheel_real_engine_bass_mod_param \
    -c firmware/src/app/engine.c -o "$OUT/engine.o"
# shellcheck disable=SC2086
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra \
    -Wno-unused-function -Wno-unused-parameter -DX0X_HOST $HOST_DEFS \
    -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen -I"$OUT" \
    -Dengine_set=wheel_test_engine_set -Dengine_set_base=wheel_test_engine_set_base \
    -Dengine_bass_mod=wheel_test_engine_bass_mod -Dengine_bass_mod_param=wheel_real_engine_bass_mod_param \
    "-DX0X_HOST_SOURCE=\"$SRC/host/x0x_host.c\"" \
    "-DSMC_MIXER_SOURCE=\"$SRC/firmware/src/app/smc_mixer.c\"" \
    "$TEST_ROOT/tests/host/smk_wheel_mode_test.c" $ORIGINAL_FLAGS $U "$OUT/engine.o" -lm -o "$OUT/probe"
if [ "$#" -gt 0 ]; then "$OUT/probe" 1 "$@"; exit; fi
for play in 0 1; do for locks in 0 1 2; do for smc in 0 1; do for part in 0 1; do for gesture in 0 1 2 3; do
    "$OUT/probe" 0 "$play" "$locks" "$smc" "$part" "$gesture" > "$OUT/control"
    "$OUT/probe" 1 "$play" "$locks" "$smc" "$part" "$gesture" > "$OUT/navigation"
    diff -u "$OUT/control" "$OUT/navigation"
    printf 'PASS play=%s locks=%s SMC=%s part=%s gesture=%s ' "$play" "$locks" "$smc" "$part" "$gesture"
    tail -1 "$OUT/navigation"
done; done; done; done; done
