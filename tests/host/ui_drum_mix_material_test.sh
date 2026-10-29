#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Both production 909/808 MIX strips: captions, levels, mute/hit, pan and clipping.
set -eu
cd "$(dirname "$0")/../.."
SMC_UI_TEST_FILE=tests/host/ui_drum_mix_material_test.c
SMC_UI_OUT=${1:-build/host/ui-drum-mix-material}
export SMC_UI_TEST_FILE SMC_UI_OUT
exec sh tests/host/smc_ui_test.sh
