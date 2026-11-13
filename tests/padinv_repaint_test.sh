#!/bin/sh
# Cut the real clip code out of main_fm1.c (line ranges of 11d81a7) and run tests/padinv_repaint_test.c.
set -e
cd "$(dirname "$0")/.."
mkdir -p build/padinv
awk 'NR>=191 && NR<=210' firmware/src/app/main_fm1.c > build/padinv/apc_a.inc
awk 'NR>=213 && NR<=564' firmware/src/app/main_fm1.c > build/padinv/apc_b.inc
{ awk 'NR>=633 && NR<=656' firmware/src/app/main_fm1.c; echo "}"; } > build/padinv/apc_c.inc
cc -std=c99 -O0 -w -Ifirmware/src tests/padinv_repaint_test.c -o build/padinv/repaint
build/padinv/repaint
