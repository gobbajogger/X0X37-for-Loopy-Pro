#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
SMC_UI_OUT=${SMC_UI_OUT:-build/host/glyph-polish} SMC_UI_TEST_FILE=tests/host/ui_glyph_polish_test.c sh tests/host/smc_ui_test.sh
