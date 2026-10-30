#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host proof: pre-snd-rev-6 Wave motion 1→127 migrate (T_303 / BASS303_WAVE lanes).
set -e
cd "$(dirname "$0")/../.."
case "$(uname -s)" in
    Darwin) GC=-Wl,-dead_strip ;;
    *) GC=-Wl,--gc-sections ;;
esac
mkdir -p build/host build/gen
[ -f build/gen/x0x_drum_tables.h ] || python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h >/dev/null
${CC:-cc} -std=c99 -O2 -Wall -Wextra -Werror -Wno-unused-function \
    -ffunction-sections -fdata-sections -DX0X_HOST \
    -Ibuild/gen -Ifirmware/src tests/host/wave_motion_migrate_test.c \
    "$GC" -lm -o build/host/wave_motion_migrate_test
build/host/wave_motion_migrate_test
