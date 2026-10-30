#!/bin/sh
# Reuse real UI/engine fixture, without device I/O.
set -eu
cd "$(dirname "$0")/../.."
export SMC_UI_TEST_FILE=tests/host/smc_pickup_acceptance_test.c
export SMC_UI_OUT="${SMC_PICKUP_OUT:-build/host/smc-pickup-acceptance}"
exec sh tests/host/smc_ui_test.sh
