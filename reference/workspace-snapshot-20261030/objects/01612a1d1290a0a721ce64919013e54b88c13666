#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -e
cd "$(dirname "$0")/../.."
case "$(uname -s)" in
    Darwin) GC=-Wl,-dead_strip ;;
    *) GC=-Wl,--gc-sections ;;
esac
mkdir -p build/host build/gen
[ -f build/gen/x0x_drum_tables.h ] || python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h >/dev/null
${CC:-cc} -std=c99 -DX0X_HOST -O2 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
    -Ibuild/gen tests/host/project_303_stream_test.c "$GC" -lm -o build/host/project_303_stream_test
build/host/project_303_stream_test
