#!/bin/sh
# Production SMC unlock/clock/pending-cue regressions; no device I/O.
set -eu
cd "$(dirname "$0")/../.."
export SMC_UI_TEST_FILE=tests/host/smc_control_lifecycle_test.c
export SMC_UI_OUT="${SMC_CONTROL_OUT:-build/host/smc-control-lifecycle}"
exec sh tests/host/smc_ui_test.sh
