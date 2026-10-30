#!/bin/sh
# CLP-80: the clip mode screen waits for Loopy Pro. Runs the REAL clip code of main_fm1.c (cut out as in
# tests/smk_clip_dark_test.sh) and checks the waiting branch of ui.c draw_main (tests/host/smk_clip_wait_test.py).
set -e
cd "$(dirname "$0")/../.."
OUT=build/host/clip_wait
mkdir -p $OUT
M=firmware/src/app/main_fm1.c
awk '/^static volatile uint8_t clip_grid_mode;/{on=1} /^#include "app\/smk_clip_keys.h"/{on=0} on' $M > $OUT/apc_a.inc
awk '/^\/\* Loopy Undo \/ Redo/{on=0} on; /^static uint8_t knob_bank_prev;/{on=1}' $M > $OUT/apc_b.inc
awk '/^#define PADLED_N 16u/{on=1} on && /^#endif/{exit} on' firmware/src/lcd.c > $OUT/padled.inc
for f in apc_a apc_b padled; do
    test -s $OUT/$f.inc || { echo "FAIL: marker for $f.inc not found"; exit 1; }
done
${CC:-cc} -std=gnu99 -O1 -w -Ifirmware/src -I$OUT -o $OUT/smk_clip_wait_test tests/host/smk_clip_wait_test.c
$OUT/smk_clip_wait_test
python3 tests/host/smk_clip_wait_test.py
