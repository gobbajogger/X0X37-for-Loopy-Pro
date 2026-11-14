#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Exercise the retained capture driver with fatal array-bounds instrumentation.
set -eu
cd "$(dirname "$0")/../.."
SMC_UI_TEST_FILE=tests/host/smc_remainder_capture.c
SMC_UI_OUT=${SMC_UI_OUT:-build/host/smc-remainder-capture-bounds}
SAN_FLAGS="${SAN_FLAGS:-} -fsanitize=bounds -fno-sanitize-recover=bounds"
export SMC_UI_TEST_FILE SMC_UI_OUT SAN_FLAGS
exec sh tests/host/smc_ui_test.sh
