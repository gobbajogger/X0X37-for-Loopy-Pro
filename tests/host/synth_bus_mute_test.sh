#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=${SMC_UI_OUT:-build/host/synth-bus-mute}
mkdir -p "$OUT"
awk '/^static void smk_faders\(void\)/{on=1} on{print} on && /^}/{exit}' \
    firmware/src/app/main_fm1.c > "$OUT/smk_faders.inc"
SMC_UI_OUT="$OUT" \
SMC_UI_TEST_FILE=tests/host/synth_bus_mute_test.c sh tests/host/smc_ui_test.sh
