#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
export SMC_UI_TEST_FILE=tests/host/smc_audit_contract_test.c
export SMC_UI_OUT="${SMC_AUDIT_OUT:-build/host/smc-audit-contract}"
exec sh tests/host/smc_ui_test.sh
