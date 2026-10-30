#!/bin/sh
# Independent production Pause loss and feedback regression; no device I/O.
set -eu
cd "$(dirname "$0")/../.."
export SMC_UI_TEST_FILE=tests/host/smc_pause_loss_test.c
export SMC_UI_OUT="${SMC_PAUSE_LOSS_OUT:-build/host/smc-pause-loss}"
exec sh tests/host/smc_ui_test.sh
