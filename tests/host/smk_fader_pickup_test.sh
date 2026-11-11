#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
awk '/^static void smk_faders\(void\)/{on=1} on{print} on && /^}/{exit}' firmware/src/app/main_fm1.c > "$OUT/smk_faders.inc"
test -s "$OUT/smk_faders.inc"
awk '/^static const uint8_t PART_ORDER/ || /^static int part_at\(/{print}' firmware/src/app/ui.c > "$OUT/part_order.inc"
test -s "$OUT/part_order.inc"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -I"$OUT" -o "$OUT/smk_fader_pickup_test" tests/host/smk_fader_pickup_test.c
"$OUT/smk_fader_pickup_test"
