#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
OUT="${SMC_FX_TWO_PAGE_OUT:-build/host/smc-fx-two-page-strip}"
mkdir -p "$OUT"
"${CC:-cc}" -std=c99 -O2 -Wall -Wextra -Werror -pedantic \
  ${SAN_FLAGS:-} -Ifirmware/src \
  tests/host/smc_fx_two_page_strip_test.c \
  firmware/src/app/smc_fx_map_tables.c -o "$OUT/test"
"$OUT/test"
