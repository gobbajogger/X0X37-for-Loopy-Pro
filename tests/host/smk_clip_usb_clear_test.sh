#!/bin/sh
# Pad clear on USB loss (owner report 2026-10-17 01:28, item 1). Runs the REAL usb.c usb_poll and
# ep1_rx with a model of the SIE registers (tests/host/smk_clip_usb_clear_fake_usb.h), the REAL clip
# code of main_fm1.c and the REAL pad LED code of lcd.c (cut out as in tests/smk_clip_dark_test.sh).
# Registered as smk-clip-usb-clear; case U1 originally failed on f9bcc46.
set -e
cd "$(dirname "$0")/../.."
OUT=build/host/clip_usb_clear
mkdir -p $OUT/src $OUT/hal
SRC=${SRC:-firmware/src}
cp "$SRC/usb.c" $OUT/src/usb.c
cp tests/host/smk_clip_usb_clear_fake_usb.h $OUT/hal/fm1_usb.h
M=$SRC/app/main_fm1.c
awk '/^static volatile uint8_t clip_grid_mode;/{on=1} /^#include "app\/smk_clip_keys.h"/{on=0} on' $M > $OUT/apc_a.inc
awk '/^\/\* Loopy Undo \/ Redo/{on=0} on; /^static uint8_t knob_bank_prev;/{on=1}' $M > $OUT/apc_b.inc
awk '/^    int drum = drum_mix_page\(\);/ || /^    uint16_t c = PART_COL/{exit} /^static void smk_pads\(void\)/{on=1} on; END{print "}"}' $M > $OUT/apc_c.inc
awk '/^#define PADLED_N 16u/{on=1} on && /^#endif/{exit} on' "$SRC/lcd.c" > $OUT/padled.inc
for f in apc_a apc_b apc_c padled; do
    test -s $OUT/$f.inc || { echo "FAIL: marker for $f.inc not found"; exit 1; }
done
${CC:-cc} -std=gnu99 -O1 -w -I"$SRC" -I$OUT -o $OUT/smk_clip_usb_clear_test tests/host/smk_clip_usb_clear_test.c
$OUT/smk_clip_usb_clear_test
