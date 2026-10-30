#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Read-only baseline source root argument is mandatory. Writes only to OUT.
# Exact audio comparison; --bench is host comparative timing, not device load.
set -eu
cd "$(dirname "$0")/../.."
BASE=$(cd "${1:?usage: script BASELINE_ROOT OUTPUT_DIR}" && pwd)
OUT=${2:?output directory required}; mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
for mode in normal sanitized; do
 flags='-O2'
 if [ "$mode" = sanitized ]; then flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'; fi
 for version in baseline candidate; do
  root=.; [ "$version" = baseline ] && root="$BASE"
  ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -Wno-error=attributes -ffp-contract=off \
   -I"$root/experiments/wobble" tests/host/miami_invariant_cache_test.c \
   "$root/experiments/wobble/wobble.c" -lm -o "$OUT/$version-$mode"
  "$OUT/$version-$mode" >"$OUT/$version-$mode.f32"
 done
 cmp "$OUT/baseline-$mode.f32" "$OUT/candidate-$mode.f32"
 sha256sum "$OUT/"*"-$mode.f32"
 echo "PASS $mode exact stereo audio: 5 sample rates x 16 modes x all 23 controls x 128 values; reset, release, reinit, invalid/clamped controls"
done
# Alternate run order to reduce systematic warm-up/order bias.
for round in 1 2 3 4 5; do
 versions='baseline candidate'; [ $((round % 2)) -eq 0 ] && versions='candidate baseline'
 for version in $versions; do "$OUT/$version-normal" --bench >"$OUT/$version-bench-$round.txt"; done
done
