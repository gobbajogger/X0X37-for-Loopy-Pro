#!/bin/sh
set -eu
: "${CC:=cc}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
"$CC" -std=c99 -Wall -Wextra -Werror -O0 -Ifirmware/src \
  tests/host/smc_page_peek_state_test.c -o "$TMP/smc_page_peek"
"$TMP/smc_page_peek"
"$CC" -std=c99 -Wall -Wextra -Werror -O2 -Ifirmware/src \
  tests/host/smc_page_peek_state_test.c -o "$TMP/smc_page_peek_opt"
"$TMP/smc_page_peek_opt"
