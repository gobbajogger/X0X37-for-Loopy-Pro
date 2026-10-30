#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
SRC=${SRC:-firmware/src}
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
awk '/^static const char CK_FLD_N\[\] = /{print}
     /^static uint32_t ck_page_on\(void\)$/ || /^int smk_ck_smc_turn\(int n, int delta\)$/{on=1}
     on{print} on && /^}/{on=0}' "$SRC/app/main_fm1.c" > "$OUT/clip_turn.inc"
awk '/^#define SMC_Q /{on=1} /^static int smc_same\(/{on=0}
     /^static void smc_solo_publish\(/ || /^static void smc_solo_press\(/{skip=1}
     skip && /^}/{skip=0; next} on && !skip{print}' "$SRC/app/smc_mixer.c" > "$OUT/smc_input.inc"
awk '/^static void smc_knob\(/ || /^static void smc_guarded_button\(/ || /^static uint8_t smc_message\(/ || /^static void smc_mmc_drain_stop\(/ || /^static void smc_spill_apply\(/ || /^static void smc_poll_peer\(void\)/ || /^void smc_poll\(void\)/{on=1}
     on{print} on && /^}/{on=0}' "$SRC/app/smc_mixer.c" > "$OUT/smc_turn.inc"
for f in clip_turn smc_input smc_turn; do test -s "$OUT/$f.inc"; done
"${CC:-cc}" -std=c99 -Wall -Wextra -Werror -I"$SRC" -I"$OUT" -Itests/host \
    -o "$OUT/smc_clip_visibility_test" tests/host/smc_clip_visibility_test.c
"$OUT/smc_clip_visibility_test"
