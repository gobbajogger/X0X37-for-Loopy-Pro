#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Production Night geometry/state test, or NIGHT_UI_TEST_FILE draw-only benchmark.
set -eu
cd "$(dirname "$0")/../.."
export SMC_UI_TEST_FILE=${NIGHT_UI_TEST_FILE:-tests/host/night_ui_test.c}
export SMC_UI_OUT=${1:-build/host/night-ui}
export X0X_HOST_DEFS="${X0X_HOST_DEFS:-} -DX0X_WOBBLE=1 -DX0X_DEV_WOBBLE=1 -DX0X_BASS_PRESET_PREVIEW=1 -DX0X_BASS_TAGGED_SOUND=1 -DX0X_BASS_NOTE_SOUND=0"
exec sh tests/host/smc_ui_test.sh
