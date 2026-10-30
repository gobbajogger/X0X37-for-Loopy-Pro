#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Drum part-page glyphs: dispatch per control and live motion on a hit.
set -eu
cd "$(dirname "$0")/../.."
SMC_UI_OUT=${1:-build/ui-drum-glyph} SMC_UI_TEST_FILE=tests/host/ui_drum_glyph_test.c sh tests/host/smc_ui_test.sh
