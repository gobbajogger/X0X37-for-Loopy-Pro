#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Production original Chicago/Miami UI, queues, engine and renderer; no device I/O.
set -eu
cd "$(dirname "$0")/../.."
export SMC_UI_TEST_FILE=tests/host/mixer_ux_test.c
export SMC_UI_OUT="${MIXER_UX_OUT:-build/host/mixer-ux}"
export SMC_UI_ORIGINAL=1
mkdir -p "$SMC_UI_OUT"
awk '/^static void smk_faders\(void\)$/{on=1} on{print} on && /^}/{on=0}' \
    firmware/src/app/main_fm1.c > "$SMC_UI_OUT/keyboard_faders.inc"
test -s "$SMC_UI_OUT/keyboard_faders.inc"
exec sh tests/host/smc_ui_test.sh
