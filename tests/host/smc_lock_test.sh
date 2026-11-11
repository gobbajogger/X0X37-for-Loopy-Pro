#!/bin/sh
# Production target/context and captured Pause input regression; no device I/O.
set -eu
cd "$(dirname "$0")/../.."
export SMC_UI_TEST_FILE=tests/host/smc_lock_test.c
export SMC_UI_OUT="${SMC_LOCK_OUT:-build/host/smc-lock}"
exec sh tests/host/smc_ui_test.sh
