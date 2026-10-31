#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
: "${SMC_UI_TEST_FILE:=tests/host/smc_remainder_test.c}"
: "${SMC_UI_OUT:=build/host/smc-remainder}"
export SMC_UI_TEST_FILE SMC_UI_OUT
exec sh tests/host/smc_ui_test.sh
