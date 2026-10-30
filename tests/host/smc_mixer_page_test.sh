#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
export SMC_UI_TEST_FILE=tests/host/smc_mixer_page_test.c
export SMC_UI_OUT="${SMC_PAGE_OUT:-build/host/smc-mixer-page}"
exec sh tests/host/smc_ui_test.sh
