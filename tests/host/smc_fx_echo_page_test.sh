#!/bin/sh
# Third SMC FX SELECT page (ROOM / TIGHT / ECHO) and shared drum suits.
set -eu
cd "$(dirname "$0")/../.."
OUT="${SMC_FX_ECHO_OUT:-build/host/smc-fx-echo-page}"
mkdir -p "$OUT"
"${CC:-cc}" -std=c99 -Wall -Wextra -Werror -pedantic -O2 \
    ${SAN_FLAGS:-} -Ifirmware/src \
    tests/host/smc_fx_echo_page_test.c firmware/src/app/smc_fx_map_tables.c \
    -o "$OUT/test"
"$OUT/test"
