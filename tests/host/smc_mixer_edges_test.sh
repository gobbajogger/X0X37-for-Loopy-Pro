#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
OUT="${SMC_MIXER_EDGES_OUT:-build/host/smc-mixer-edges}"
mkdir -p "$OUT"
"${CC:-cc}" -std=c99 -O2 -Wall -Wextra -Werror -pedantic ${SAN_FLAGS:-} \
    -Ifirmware/src tests/host/smc_mixer_edges_test.c -o "$OUT/test"
"$OUT/test"
