#!/bin/sh
# Current-production SMC mapped-page logic, no firmware hardware or old snapshot.
set -eu
cd "$(dirname "$0")/../.."
: "${CC:=cc}"
SOURCE=${SMC_PAGE_PEEK_SOURCE:-firmware/src/app/smc_mixer.c}
OUT=${SMC_PAGE_PEEK_OUT:-build/host/smc-page-peek}
mkdir -p "$OUT"
python3 tests/host/extract_smc_page_peek.py --source "$SOURCE" --output "$OUT/smc_page_peek_production.inc"
for OPT in 0 2; do
    "$CC" -std=c99 -Wall -Wextra -Werror -O"$OPT" \
      -Ifirmware/src -I"$OUT" tests/host/smc_page_peek_production_test.c \
      -o "$OUT/test-O$OPT"
    "$OUT/test-O$OPT"
done
