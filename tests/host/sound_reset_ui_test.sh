#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
SMC_UI_OUT=${SMC_UI_OUT:-build/host/sound-reset-ui} \
SMC_UI_TEST_FILE=tests/host/sound_reset_ui_test.c sh tests/host/smc_ui_test.sh
