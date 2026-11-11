#!/bin/sh
# Self-contained host test: current SMC FX-caption helper, no synth headers.
set -eu
cd "$(dirname "$0")/../.."
OUT="${SMC_FX_CAPTION_OUT:-build/host/smc-fx-caption}"
mkdir -p "$OUT"
"${CC:-cc}" -std=c99 -Wall -Wextra -Werror -pedantic -O2 \
    ${SAN_FLAGS:-} -Ifirmware/src tests/host/smc_fx_caption_test.c firmware/src/app/smc_fx_map_tables.c \
    -o "$OUT/test"
"$OUT/test"
