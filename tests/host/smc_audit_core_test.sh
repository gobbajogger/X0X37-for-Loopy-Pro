#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
export SMC_UI_TEST_FILE=tests/host/smc_audit_core_test.c
export SMC_UI_OUT="${SMC_AUDIT_CORE_OUT:-build/host/smc-audit-core}"
exec sh tests/host/smc_ui_test.sh
