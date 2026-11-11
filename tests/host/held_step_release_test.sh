#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
SMC_UI_OUT=${SMC_UI_OUT:-build/host/held-step-release} \
SMC_UI_TEST_FILE=tests/host/held_step_release_test.c sh tests/host/smc_ui_test.sh
