#!/bin/sh
# Actual SMC renderer bounds regression. No device I/O or UI redesign.
set -eu
cd "$(dirname "$0")/../.."
export SMC_UI_TEST_FILE=tests/host/smc_ui_bounds_test.c
export SMC_UI_OUT="${SMC_UI_BOUNDS_OUT:-build/host/smc-ui-bounds}"
export SAN_FLAGS="${SAN_FLAGS:--fsanitize=undefined -fno-sanitize-recover=all}"
exec sh tests/host/smc_ui_test.sh
