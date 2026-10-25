#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
export SMC_UI_TEST_FILE=tests/host/smc_pattern_navigation_test.c
export SMC_UI_OUT="${SMC_PATTERN_NAVIGATION_OUT:-build/host/smc-pattern-navigation}"
exec sh tests/host/smc_ui_test.sh
