#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host proof for Chicago Arp Switch/Mode/Range/Speed on native303 live path.
set -e
cd "$(dirname "$0")/../.."
OUT=build/chicago_arp
mkdir -p "$OUT"
CC="${CC:-cc}"
CFLAGS="-std=c99 -O2 -ffp-contract=off -Wall -Wextra -Wdouble-promotion"
$CC $CFLAGS -I firmware/src -I firmware/src/app -I firmware/src/dsp \
  tests/host/chicago_arp_test.c firmware/src/app/chicago_param_tables.c \
  firmware/src/app/smk_clip_arp_share.c -lm -o "$OUT/chicago_arp"
"$OUT/chicago_arp"
