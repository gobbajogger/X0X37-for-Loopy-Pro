#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-build/host/juno-stock-bank}
mkdir -p "$OUT"
${CC:-cc} -std=c11 -O2 -Wall -Wextra -Werror -pedantic -ffp-contract=off -DX0X_HOST=1 \
    tests/host/juno_stock_bank_test.c firmware/src/dsp/juno6.c -lm -o "$OUT/juno-stock-bank-test"
"$OUT/juno-stock-bank-test"
