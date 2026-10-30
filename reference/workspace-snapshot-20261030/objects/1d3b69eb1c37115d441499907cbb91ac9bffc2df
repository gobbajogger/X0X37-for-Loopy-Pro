#!/bin/sh
# Production LX-REQ-006/022 hold/beat contract; no device I/O.
set -eu
cd "$(dirname "$0")/../.."
export SMC_UI_TEST_FILE=tests/host/smc_lock_beat_test.c
export SMC_UI_OUT="${SMC_LOCK_BEAT_OUT:-build/host/smc-lock-beat}"
exec sh tests/host/smc_ui_test.sh
