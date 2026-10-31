#!/bin/sh
# SMC own pop-up: keyboard UI never changes. No device I/O.
# The legacy #737 page-peek contract is kept behind -DSMC_PAGE_PEEK=1
# (default 0: SMC shows only its own pop-up). Optional CC, SAN_FLAGS, SMC_UI_OUT and SMC_UI_SOURCE_FILE support independent replay.
set -eu
cd "$(dirname "$0")/../.."
OUT=${SMC_UI_OUT:-build/host/smc-own-popup}
mkdir -p "$OUT"
awk '/^static const char CK_FLD_N\[\] = /{print}
     /^static uint32_t ck_page_on\(void\)$/ || /^int smk_ck_smc_(turn|nav)\(/{on=1}
     on{print} on && /^}/{on=0}' firmware/src/app/main_fm1.c > "$OUT/clip_smc.inc"
test -s "$OUT/clip_smc.inc"
sh host/build_host.sh --generate-only > "$OUT/generate.log" 2>&1 || { cat "$OUT/generate.log"; exit 1; }
. tests/host/original_build.sh
U=$ORIGINAL_SOURCES
DARWIN=
[ "$(uname -s)" != Darwin ] || DARWIN=-D_DARWIN_C_SOURCE
# Linux host needs POSIX clock APIs from x0x_host.c under -std=c99.
POSIX_DEFS=-D_POSIX_C_SOURCE=200809L
SOURCE=${SMC_UI_SOURCE_FILE:-$PWD/firmware/src/app/smc_mixer.c}
# shellcheck disable=SC2086
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -DX0X_HOST $POSIX_DEFS $DARWIN ${X0X_HOST_DEFS:-} ${SAN_FLAGS:-} \
    -DSMC_SOURCE_FILE=\"$SOURCE\" -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen -I"$OUT" \
    tests/host/smc_own_popup_test.c $ORIGINAL_FLAGS $U -lm -o "$OUT/test"
"$OUT/test"
