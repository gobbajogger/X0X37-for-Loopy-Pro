#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Run from the repository root; use a disposable OUT, not the firmware build.
set -eu
cd "$(dirname "$0")/../.."
OUT="${1:-build/preset-file-boundary}"
mkdir -p "$OUT"
CC="${CC:-cc}"
for SWITCH in 0 1; do
  "$CC" -std=c11 -O2 -Wall -Wextra -Werror -fno-fast-math \
    -DDNBOX_FACTORY_PRESET_EXPORT="$SWITCH" \
    tests/host/preset_file_boundary_test.c firmware/src/app/preset_record.c \
    -o "$OUT/preset-file-boundary-$SWITCH"
  "$OUT/preset-file-boundary-$SWITCH"
done
"$CC" -std=c11 -O1 -g -Wall -Wextra -Werror \
  -fsanitize=address,undefined -fno-sanitize-recover=all \
  tests/host/preset_file_boundary_test.c firmware/src/app/preset_record.c \
  -o "$OUT/preset-file-boundary-asan"
"$OUT/preset-file-boundary-asan"
echo PASS preset-file-boundary
