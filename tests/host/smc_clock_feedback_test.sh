#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
export SMC_UI_TEST_FILE=tests/host/smc_clock_feedback_test.c
export SMC_UI_OUT="${SMC_CLOCK_FEEDBACK_OUT:-build/host/smc-clock-feedback}"
exec sh tests/host/smc_ui_test.sh
