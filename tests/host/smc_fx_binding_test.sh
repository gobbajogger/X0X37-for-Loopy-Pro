#!/bin/sh
# Full physical FX strip mapping and release/feedback replay; no device I/O.
set -eu
cd "$(dirname "$0")/../.."
export SMC_UI_TEST_FILE=tests/host/smc_fx_binding_test.c
export SMC_UI_OUT="${SMC_FX_BINDING_OUT:-build/host/smc-fx-binding}"
exec sh tests/host/smc_ui_test.sh
