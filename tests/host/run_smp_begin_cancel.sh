#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -e
cd "$(dirname "$0")/../.."
mkdir -p build/host
[ -f build/gen/x0x_drum_tables.h ] || sh host/build_host.sh --generate-only >/dev/null
${CC:-cc} -std=c99 -O1 -Wall -Wextra -Wno-unused-function -Wno-unused-variable \
    ${SAN_FLAGS:-} -Ibuild/gen tests/host/smp_begin_cancel_test.c -o build/host/smp_begin_cancel_test
build/host/smp_begin_cancel_test
