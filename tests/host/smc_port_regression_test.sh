#!/bin/sh
# Real production SMC ingress/epoch/STOP/feedback checks; no device I/O.
set -eu
cd "$(dirname "$0")/../.."
export SMC_UI_TEST_FILE=tests/host/smc_port_regression_test.c
export SMC_UI_OUT="${SMC_PORT_OUT:-build/host/smc-port}"
exec sh tests/host/smc_ui_test.sh
