#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host test of firmware/src/dsp/breaks.c (the BREAK part): generator rules, audio slice check at
# 90 / 170 BPM, step gate, live keys, click bound (+ its positive control), build/breaks_demo.wav.
set -e
cd "$(dirname "$0")/../.."
mkdir -p build
CFLAGS="-DX0X_HOST -O2 -ffp-contract=off -Wall -Wextra -Wdouble-promotion -Werror -Wno-error=attributes"
# the DSP file alone, strict C99 (define IMA tables: header is extern without it)
${CC:-cc} $CFLAGS -std=c99 -pedantic -DX0X_IMA_TABLES_DEFINE -Ifirmware/src -c firmware/src/dsp/breaks.c -o build/breaks_host.o
${CC:-cc} $CFLAGS tests/host/breaks_test.c -lm -o build/breaks_test
./build/breaks_test build/breaks_demo.wav
# the same tests with a 44.1 kHz source loop (the default rate of the built-in and uploaded loops)
${CC:-cc} $CFLAGS -DSRC_FS=44100 -DLOOP_N=176400 tests/host/breaks_test.c -lm -o build/breaks_test_44k
./build/breaks_test_44k
# positive control: with the crossfade compiled out the click check must fire
${CC:-cc} $CFLAGS -DBRK_FADE=1 -DBRK_CLICK_CONTROL tests/host/breaks_test.c -lm -o build/breaks_test_control
if ./build/breaks_test_control; then
    echo "FAIL: click check did not fire with the crossfade removed"; exit 1
else
    rc=$?
    [ "$rc" -eq 3 ] || { echo "FAIL: control exited $rc"; exit 1; }
    echo "control ok: click check fires without the crossfade"
fi
echo "breaks: all ok"
