#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/wobble-combined
for mode in normal sanitized; do
 flags="-O2"
 if [ "$mode" = sanitized ]; then flags="-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all"; fi
 ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -ffp-contract=off -DX0X_SYNTH_HOST_TEST -Ifirmware/src tests/host/wobble_backend_test.c experiments/wobble/wobble.c firmware/src/dsp/bass303.c -o build/wobble-combined/$mode
 build/wobble-combined/$mode
done
# Product path: WBL1 allowed without PADCAL / with VERSION. Off still excludes backend.
if printf '#include "dsp/wobble_backend.h"\n' | ${CC:-cc} -x c -E -Ifirmware/src -DX0X_WOBBLE=0 - >/dev/null 2>&1; then exit 1; fi
if ! printf '#include "dsp/wobble_config.h"\n' | ${CC:-cc} -x c -E -Ifirmware/src -DX0X_WOBBLE=1 -DX0X_DEV_PADCAL=0 - >/dev/null 2>&1; then exit 1; fi
if ! printf '#include "dsp/wobble_config.h"\n' | ${CC:-cc} -x c -E -Ifirmware/src -DX0X_WOBBLE=1 -DX0X_VERSION=1 - >/dev/null 2>&1; then exit 1; fi
echo 'PASS default/backend and product WBL1 enable'
