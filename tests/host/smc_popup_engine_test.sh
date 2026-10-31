#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
export SMC_UI_TEST_FILE=tests/host/smc_popup_engine_test.c
export SMC_UI_OUT="${SMC_ENGINE_OUT:-build/host/smc-popup-engine}"
exec sh tests/host/smc_ui_test.sh
