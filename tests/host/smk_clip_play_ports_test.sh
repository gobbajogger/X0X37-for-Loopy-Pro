#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# CLP-50: cut the real clip mode PLAY send code out of main_fm1.c (apc_out, apc_note and
# smk_clip_transport_button, by text markers, not line numbers) and run
# tests/host/smk_clip_play_ports_test.c. One PLAY press must send one APC PLAY note on in total.
set -e
cd "$(dirname "$0")/../.."
OUT=build/host/clip_play_ports
mkdir -p $OUT
M=firmware/src/app/main_fm1.c
awk '/^#include "app\/smk_clip_keys.h"/{exit} /^static void apc_out\(uint32_t pkt\)/{on=1} on' $M > $OUT/apc_out.inc
awk 'on && /^}/{print; exit} /^uint8_t smk_clip_transport_button\(uint8_t button, uint8_t pressed\)/{on=1} on' $M > $OUT/clip_play.inc
for f in apc_out clip_play; do
    test -s $OUT/$f.inc || { echo "FAIL: marker for $f.inc not found"; exit 1; }
done
${CC:-cc} -std=c99 -O2 -Wall -Wextra -Wno-unused-function -Werror -Ifirmware/src -I$OUT \
    -o $OUT/smk_clip_play_ports_test tests/host/smk_clip_play_ports_test.c
$OUT/smk_clip_play_ports_test
