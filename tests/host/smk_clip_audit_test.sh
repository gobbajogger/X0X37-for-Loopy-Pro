#!/bin/sh
# Production controls/APC regression; separate extracted fixtures per selected test.
set -eu
cd "$(dirname "$0")/../.."
case "${1:-}" in
    settings) probe=smk_clip_settings_dirty ;;
    ingress) probe=smc_host_ingress ;;
    *) echo "usage: $0 settings|ingress" >&2; exit 2 ;;
esac
OUT="${OUT:-build/host/$probe}"
export OUT
sh tests/host/smk_clip_knob_test.sh --extract-only
${CC:-cc} -std=gnu99 -O1 -w -I"${SRC:-firmware/src}" -Ifirmware/hal -I"$OUT" \
    -o "$OUT/$probe" "tests/host/${probe}_test.c"
"$OUT/$probe"
