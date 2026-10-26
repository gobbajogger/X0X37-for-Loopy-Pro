#!/bin/sh
# Combined SMC intentional navigation/lock/FX and fake-clock overlay lifetime.
set -eu
cd "$(dirname "$0")/../.."
export SMC_UI_TEST_FILE=tests/host/smc_navigation_lifetime_test.c
export SMC_UI_OUT="${SMC_NAV_OUT:-build/host/smc-navigation-lifetime}"
exec sh tests/host/smc_ui_test.sh
