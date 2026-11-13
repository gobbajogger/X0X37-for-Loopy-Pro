#!/bin/sh
# Real firmware exports followed by the unchanged companion transfer machinery.
set -eu
cd "$(dirname "$0")/../.."
PROJECT_RATE_FIXTURES="$PWD/build/host/project-rates-fixtures"
export PROJECT_RATE_FIXTURES
mkdir -p "$PROJECT_RATE_FIXTURES"
SMC_UI_OUT=build/host/project-rates SMC_UI_TEST_FILE=tests/host/project_rates_export_test.c sh tests/host/smc_ui_test.sh
node web/test_projects.mjs
