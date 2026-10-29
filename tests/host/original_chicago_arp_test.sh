#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Golden numeric witnesses from the original bounded static interpreter; no
# plug-in loading, proprietary executable or native target is needed to test.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?output directory};mkdir -p "$OUT";OUT=$(cd "$OUT"&&pwd)
for mode in normal sanitized;do
 flags='-O2';[ "$mode" != sanitized ]||flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
 ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -Iexperiments/original_synth -Itests/host tests/host/original_chicago_arp_control_test.c experiments/original_synth/chicago_arp_control.c -lm -o "$OUT/arp-$mode"
 "$OUT/arp-$mode" > "$OUT/arp-$mode.log"
 ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -Iexperiments/original_synth tests/host/original_chicago_program_arp_test.c experiments/original_synth/chicago_arp_control.c experiments/original_synth/chicago_program.c -lm -o "$OUT/program-arp-$mode"
 "$OUT/program-arp-$mode" > "$OUT/program-arp-$mode.log"
done
cmp "$OUT/arp-normal.log" "$OUT/arp-sanitized.log"
printf 'PASS original Chicago ARP transition/clock/native108 immediate route normal+ASan/UBSan\n'
