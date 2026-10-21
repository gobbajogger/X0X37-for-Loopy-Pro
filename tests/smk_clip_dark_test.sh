#!/bin/sh
# a76: cut the real clip code out of main_fm1.c and the real pad LED code out of lcd.c (by text
# markers, not line numbers) and run tests/smk_clip_dark_test.c. BEHAVIOR.md CLP-18.
set -e
cd "$(dirname "$0")/.."
OUT=build/host/clip_dark
mkdir -p $OUT
SRC=${SRC:-firmware/src}
M=$SRC/app/main_fm1.c
awk '/^static volatile uint8_t clip_grid_mode;/{on=1} /^#include "app\/smk_clip_keys.h"/{on=0} on' $M > $OUT/apc_a.inc
awk '/^\/\* Loopy Undo \/ Redo/{on=0} on; /^static uint8_t knob_bank_prev;/{on=1}' $M > $OUT/apc_b.inc
awk '/^    int drum = drum_mix_page\(\);/ || /^    uint16_t c = PART_COL/{exit} /^static void smk_pads\(void\)/{on=1} on; END{print "}"}' $M > $OUT/apc_c.inc
awk '/^#define PADLED_N 16u/{on=1} on && /^#endif/{exit} on' "$SRC/lcd.c" > $OUT/padled.inc
for f in apc_a apc_b apc_c padled; do
    test -s $OUT/$f.inc || { echo "FAIL: marker for $f.inc not found"; exit 1; }
done
${CC:-cc} -std=c99 -O1 -w -I"$SRC" -I$OUT -o $OUT/smk_clip_dark_test tests/smk_clip_dark_test.c
$OUT/smk_clip_dark_test
