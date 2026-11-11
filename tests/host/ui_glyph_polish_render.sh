#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Render SMK-37 frames; OUT must be a scratch directory, never tracked captures.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?usage: ui_glyph_polish_render.sh SCRATCH_OUTPUT}
mkdir -p "$OUT"
export SMC_UI_OUT="$OUT"
export SMC_UI_TEST_FILE=tests/host/ui_glyph_polish_render.c
exec sh tests/host/smc_ui_test.sh
