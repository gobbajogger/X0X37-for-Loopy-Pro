#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Production UI renderer glyph endpoints, pages and project nonmutation.
set -eu
cd "$(dirname "$0")/../.."
SMC_UI_TEST_FILE=tests/host/ui_shared_glyph_test.c
SMC_UI_OUT=${1:-build/host/ui-shared-glyph}
export SMC_UI_TEST_FILE SMC_UI_OUT
exec sh tests/host/smc_ui_test.sh
