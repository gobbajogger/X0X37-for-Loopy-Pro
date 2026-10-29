#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -e
cd "$(dirname "$0")/../.."
. tests/host/original_build.sh
case "$(uname -s)" in
    Darwin) GC=-Wl,-dead_strip ;;
    *) GC=-Wl,--gc-sections ;;
esac
mkdir -p build/host build/gen
[ -f build/gen/x0x_drum_tables.h ] || python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h >/dev/null
[ -f build/gen/x0x_break_bank.h ] || sh tools/gen_builtin_break.sh build/gen >/dev/null
${CC:-cc} -std=c99 -O2 -ffp-contract=off -Wall -Wextra -Werror -Wno-unused-function ${SAN_FLAGS:-} \
    -ffunction-sections -fdata-sections -DX0X_HOST -DBOARD_SMK37=1 \
    -Ibuild/gen -Ifirmware/src tests/host/rec_endpoint_storage_test.c \
       $ORIGINAL_FLAGS $ORIGINAL_SOURCES \
    "$GC" -lm -o build/host/rec_endpoint_storage_test
build/host/rec_endpoint_storage_test
