#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# BF-021 memory-full save preflight + FNV32 sound-hash collision.
set -e
cd "$(dirname "$0")/../.."
case "$(uname -s)" in
    Darwin) GC=-Wl,-dead_strip ;;
    *) GC=-Wl,--gc-sections ;;
esac
mkdir -p build/host build/gen
[ -f build/gen/x0x_drum_tables.h ] || python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h >/dev/null
${CC:-cc} -std=c99 -O2 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
    -DX0X_HOST -Ibuild/gen -Ifirmware/src -Ifirmware/src/dsp -Ifirmware/src/app \
    tests/host/project_save_preflight_test.c \
    firmware/src/seq/pattern.c firmware/src/seq/tb3po.c \
    "$GC" -lm -o build/host/project_save_preflight_test
build/host/project_save_preflight_test
