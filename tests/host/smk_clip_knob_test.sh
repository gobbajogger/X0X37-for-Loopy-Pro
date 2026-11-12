#!/bin/sh
# Clip mode knobs (owner report 2026-10-17 02:49 on c6bf44a: in clip mode the knobs do nothing).
# Runs the REAL clip keybed code of main_fm1.c (smk_ck_poll, ck_controls: knobs 2-8 send CC 48-54), the
# REAL clip feedback code (apc_out, the CLP-18 link code) and the REAL usb.c (usb_poll, ep1_tx), and
# checks the CC that goes to the host on USB (EP1 IN) and on Bluetooth (ble_out).
# K1-K8 pass on f9bcc46 and on each commit up to c6bf44a (bisect 2026-10-17 03:00).
# K9: knobs 2-5 set SCALE, CHORD or ARP (CLP-62, CLP-63). K9 fails from e8b2c7f up to aa7f666.
# K10: only the stage state gives the knobs (not a page, a pad or Loopy), the keybed notes change, knobs 6-8
# keep no turns. K10 fails on 54f92b2 (5 fail).
# SRC=<dir> runs it on the firmware sources of another commit (the bisect), default firmware/src.
# --extract-only prepares the shared harness without compiling or running the knob probe.
set -e
EXTRACT_ONLY=0
case "$#:$*" in
    0:) ;;
    1:--extract-only) EXTRACT_ONLY=1 ;;
    *) echo "usage: $0 [--extract-only]" >&2; exit 2 ;;
esac
cd "$(dirname "$0")/../.."
SRC=${SRC:-firmware/src}
OUT=${OUT:-build/host/clip_knob}
mkdir -p $OUT/src $OUT/hal
cp $SRC/usb.c $OUT/src/usb.c
cp tests/host/smk_clip_knob_fake_usb.h $OUT/hal/fm1_usb.h
M=$SRC/app/main_fm1.c
awk '/^static volatile uint8_t clip_grid_mode;/{on=1} /^#include "app\/smk_clip_keys.h"/{on=0} on' $M > $OUT/apc_a.inc
awk '/^\/\* Loopy Undo \/ Redo/{on=0} /^static uint8_t knob_bank_prev;/{on=1} on' $M > $OUT/apc_b.inc
awk '/^\/\* -+ clip mode keybed/{on=1} /^\/\* ------- clip mode screen/{on=0} on' $M > $OUT/ck_a.inc
awk '/^CK_FN void smk_ck_poll\(void\)/{on=1} on; on && /^}/{exit}' $M > $OUT/ck_b.inc
awk '/^#define AUTOSAVE_QUIET/{print} /^\/\* AUTOSAVE:/{on=1} /^void ui_frame\(void\)/{on=0} on' $SRC/app/ui.c > $OUT/autosave.inc
awk '/^#define PADLED_N 16u/{on=1} on && /^#endif/{exit} on' $SRC/lcd.c > $OUT/padled.inc
for f in apc_a apc_b ck_a ck_b autosave padled; do
    test -s $OUT/$f.inc || { echo "FAIL: marker for $f.inc not found"; exit 1; }
done
[ "$EXTRACT_ONLY" = 0 ] || exit 0
# CK_RATE_T / ck_arp_pick live in smk_clip_arp_share.c (size share); same link as smk-clip-keys.
${CC:-cc} -std=gnu99 -O1 -w -I$SRC -Ifirmware/hal -I$OUT -o $OUT/smk_clip_knob_test \
  tests/host/smk_clip_knob_test.c firmware/src/app/smk_clip_arp_share.c
$OUT/smk_clip_knob_test
