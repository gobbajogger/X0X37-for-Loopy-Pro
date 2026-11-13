#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host reproduction of SEQ chord display at 94197fc8. Compiles the real
# ui.c draw path; writes native 240x240 BEFORE PNGs. No device, no firmware
# source edits. Optional: $1 = build dir, SEQ_CHORD_EVIDENCE = PNG directory.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-build/host/seq-chord-display}
EVIDENCE=${SEQ_CHORD_EVIDENCE:-$PWD/evidence/before}
mkdir -p "$OUT" "$EVIDENCE"
export SEQ_CHORD_EVIDENCE="$EVIDENCE"
export NIGHT_UI_TEST_FILE=tests/host/seq_chord_display_test.c
# night_ui_test.sh / smc_ui_test.sh put shots in $OUT; the test writes
# evidence itself via SEQ_CHORD_EVIDENCE and png_write().
exec sh tests/host/night_ui_test.sh "$OUT"
