#!/bin/sh
# Exact production page-map regression; no device I/O.
set -eu
cd "$(dirname "$0")/../.."
export SMC_UI_TEST_FILE=tests/host/ui_page_layout_test.c
export SMC_UI_OUT="${UI_PAGE_LAYOUT_OUT:-build/host/ui-page-layout}"
exec sh tests/host/smc_ui_test.sh
