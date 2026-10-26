#!/bin/sh
# Real production SMC SELECT dispatch/lifecycle, no device I/O.
set -eu
cd "$(dirname "$0")/../.."
export SMC_UI_TEST_FILE=tests/host/smc_select_mode_test.c
export SMC_UI_OUT="${SMC_SELECT_OUT:-build/host/smc-select-mode}"
exec sh tests/host/smc_ui_test.sh
