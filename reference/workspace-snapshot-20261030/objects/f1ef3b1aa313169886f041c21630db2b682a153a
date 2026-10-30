#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -e
cd "$(dirname "$0")/../.."
mkdir -p build/host
for spec in 20480:30 16128:24; do
    bytes=${spec%%:*}
    kept=${spec#*:}
    ${CC:-cc} -O2 -std=c99 -Wall -Wextra -Werror \
        -DUNDO_POOL="${bytes}u" -DEXPECTED="$kept" \
        tests/host/undo_capacity_test.c -o "build/host/undo_capacity_$bytes"
    "build/host/undo_capacity_$bytes"
done
