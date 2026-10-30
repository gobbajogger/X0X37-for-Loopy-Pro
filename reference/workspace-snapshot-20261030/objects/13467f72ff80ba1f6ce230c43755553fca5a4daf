#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
awk '/^static void smc_select_part\(int part\)/{on=1} on{print} on && /^}/{exit}' firmware/src/app/smc_mixer.c > "$OUT/smc_select_part.inc"
test -s "$OUT/smc_select_part.inc"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -I"$OUT" -o "$OUT/smc_part_select_test" tests/host/smc_part_select_test.c
"$OUT/smc_part_select_test"
