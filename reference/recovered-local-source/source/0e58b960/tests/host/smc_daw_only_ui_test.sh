#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
SMC_UI_TEST_FILE=tests/host/smc_daw_only_ui_test.c SMC_UI_OUT="${SMC_DAW_UI_OUT:-build/host/smc-daw-only-ui}" sh tests/host/smc_ui_test.sh
