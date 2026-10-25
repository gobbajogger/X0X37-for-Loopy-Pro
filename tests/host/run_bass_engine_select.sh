#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -e
cd "$(dirname "$0")/../.."
mkdir -p build/host
${CC:-cc} -std=c99 -O2 -Wall -Wextra -Werror -Wno-unused-function ${SAN_FLAGS:-} \
  -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/seq \
  tests/host/bass_engine_select_test.c -o build/host/bass_engine_select_test
build/host/bass_engine_select_test
# Also prove the WBL1-available path compiles and selects when the DEV fixture flag is on
# (no DSP linked — availability only). Operator builds leave the flag off.
${CC:-cc} -std=c99 -O2 -Wall -Wextra -Werror -Wno-unused-function ${SAN_FLAGS:-} \
  -DX0X_DEV_WOBBLE=1 -DX0X_DEV_PADCAL=1 \
  -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/seq \
  tests/host/bass_engine_select_test.c -o build/host/bass_engine_select_test_wbl1
build/host/bass_engine_select_test_wbl1
