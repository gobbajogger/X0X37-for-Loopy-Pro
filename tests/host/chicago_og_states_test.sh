#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Chicago AMP/ARP pages must not restore the program on a steady UI frame.
set -eu
cd "$(dirname "$0")/../.."
sh host/build_host.sh --generate-only >/tmp/chicago-og-states-gen.log 2>&1 || {
    cat /tmp/chicago-og-states-gen.log
    exit 1
}
. tests/host/original_build.sh
OUT=${1:-build/host/chicago-og-states}
mkdir -p "$(dirname "$OUT")"
${CC:-cc} -O2 -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable \
    -D_POSIX_C_SOURCE=200809L -DX0X_HOST $ORIGINAL_FLAGS \
    -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen \
    -Wl,--wrap=original_sound_restore_chicago \
    -Wl,--wrap=original_chicago_program_init \
    tests/host/chicago_og_states_test.c $ORIGINAL_SOURCES -lm -o "$OUT"
"$OUT"
