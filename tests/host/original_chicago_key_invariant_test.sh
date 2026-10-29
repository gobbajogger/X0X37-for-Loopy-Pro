#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Actual producer sequence/property check, no reference/runtime execution.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?output directory};mkdir -p "$OUT";OUT=$(cd "$OUT"&&pwd)
for mode in normal sanitized;do
 flags='-O2';[ "$mode" != sanitized ]||flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
 exe="$OUT/keys-$mode"
 ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic -Iexperiments/original_synth tests/host/original_chicago_key_invariant_test.c experiments/original_synth/chicago_key_admission.c experiments/original_synth/chicago_allocator.c experiments/original_synth/chicago_scheduler.c experiments/original_synth/chicago_selector.c experiments/original_synth/chicago_arp_control.c -lm -o "$exe"
 "$exe" > "$exe.log"
done
cmp "$OUT/keys-normal.log" "$OUT/keys-sanitized.log"
cat "$OUT/keys-normal.log"
