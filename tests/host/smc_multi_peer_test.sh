#!/bin/sh
# Production two-peer queues, target locks, lifecycle and feedback; no device I/O.
set -eu
cd "$(dirname "$0")/../.."
export SMC_UI_TEST_FILE=tests/host/smc_multi_peer_test.c
export SMC_UI_OUT="${SMC_MULTI_OUT:-build/host/smc-multi-peer}"
exec sh tests/host/smc_ui_test.sh
