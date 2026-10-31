#!/bin/sh
# Scoped compiler-level regression for the mixer FX page and peer-held popup.
set -eu
cd "$(dirname "$0")/../.."
OUT="${SMC_FX_PORT_OUT:-build/host/smc-fx-page-port}"
mkdir -p "$OUT"
"${CC:-cc}" -std=c99 -Wall -Wextra -Werror -pedantic -O2 \
    ${SAN_FLAGS:-} -Ifirmware/src \
    tests/host/smc_fx_page_port_test.c firmware/src/app/smc_fx_map_tables.c \
    -o "$OUT/test"
"$OUT/test"
