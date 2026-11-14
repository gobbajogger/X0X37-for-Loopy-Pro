#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# ONLY new scoped identity cases; reuse historical integrated/focused PASS.
set -eu
cd "$(dirname "$0")/../.."
OUT="${1:-build/preset-engine-identity}"
mkdir -p "$OUT"
CC="${CC:-cc}"
for POLICY in 0 1; do
    "$CC" -std=c11 -O2 -Wall -Wextra -Werror \
      -DDNBOX_FACTORY_PRESET_EXPORT="$POLICY" \
      tests/host/preset_engine_identity_test.c firmware/src/app/preset_record.c \
      -o "$OUT/engine-identity-$POLICY"
    "$OUT/engine-identity-$POLICY"
done
"$CC" -std=c11 -O1 -g -Wall -Wextra -Werror \
   -fsanitize=address,undefined -fno-sanitize-recover=all \
   tests/host/preset_engine_identity_test.c firmware/src/app/preset_record.c \
   -o "$OUT/engine-identity-asan"
"$OUT/engine-identity-asan"
echo PASS engine-identity
