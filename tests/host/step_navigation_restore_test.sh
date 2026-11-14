#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Recovered F2 #15 page/list failures plus multi-page exact cleanup controls.
set -eu
cd "$(dirname "$0")/../.."
SMC_UI_OUT=${SMC_UI_OUT:-build/host/step-navigation-restore} \
SMC_UI_TEST_FILE=tests/host/step_navigation_restore_test.c sh tests/host/smc_ui_test.sh
