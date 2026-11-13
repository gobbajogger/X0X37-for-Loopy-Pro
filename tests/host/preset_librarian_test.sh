#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host-only. No SDK, firmware flash, device access or external plugin files.
set -eu
cd "$(dirname "$0")/../.."
OUT="${1:-build/preset-librarian}"
mkdir -p "$OUT"
CC="${CC:-cc}"
for EXPORT in 0 1; do
  "$CC" -std=c11 -O2 -Wall -Wextra -Werror \
    -DDNBOX_FACTORY_PRESET_EXPORT="$EXPORT" \
    tests/host/preset_librarian_test.c firmware/src/app/preset_record.c \
    -o "$OUT/librarian-$EXPORT"
  "$OUT/librarian-$EXPORT"
done
"$CC" -std=c11 -O1 -g -Wall -Wextra -Werror \
  -fsanitize=address,undefined -fno-sanitize-recover=all \
  tests/host/preset_librarian_test.c firmware/src/app/preset_record.c \
  -o "$OUT/librarian-asan"
"$OUT/librarian-asan"
echo PASS preset-librarian
