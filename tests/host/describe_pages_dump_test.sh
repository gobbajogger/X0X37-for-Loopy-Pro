#!/bin/sh
# Dump describe_pages for every view/part/engine/selection, plus framebuffer
# hashes of the screens that consume those pages. If DESCRIBE_GOLDEN is set,
# the dump must match that file byte-for-byte (before vs after identity).
set -eu
cd "$(dirname "$0")/../.."
OUT=${DESCRIBE_OUT:-build/host/describe-pages-dump}
mkdir -p "$OUT"
awk '/^static const char CK_FLD_N\[\] = /{print}
     /^static uint32_t ck_page_on\(void\)$/ || /^int smk_ck_smc_(turn|nav)\(/{on=1}
     on{print} on && /^}/{on=0}' firmware/src/app/main_fm1.c > "$OUT/clip_smc.inc"
test -s "$OUT/clip_smc.inc"
if [ ! -d build/gen ] || [ -z "$(ls -A build/gen 2>/dev/null || true)" ]; then
  sh host/build_host.sh --generate-only > "$OUT/generate.log" 2>&1 || { cat "$OUT/generate.log"; exit 1; }
fi
. tests/host/original_build.sh
U=$ORIGINAL_SOURCES
POSIX_DEFS=-D_POSIX_C_SOURCE=200809L
# shellcheck disable=SC2086
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -DX0X_HOST $POSIX_DEFS ${X0X_HOST_DEFS:-} ${SAN_FLAGS:-} \
    -DSMC_SOURCE_FILE=\"$PWD/firmware/src/app/smc_mixer.c\" \
    -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen -I"$OUT" \
    tests/host/describe_pages_dump_test.c $ORIGINAL_FLAGS $U -lm -o "$OUT/test"
DUMP=${DESCRIBE_DUMP:-$OUT/describe_pages.txt}
DESCRIBE_DUMP="$DUMP" "$OUT/test"
if [ -n "${DESCRIBE_GOLDEN:-}" ]; then
  cmp -s "$DESCRIBE_GOLDEN" "$DUMP"
  echo "describe_pages dump IDENTICAL to $DESCRIBE_GOLDEN"
fi
sha256sum "$DUMP"
