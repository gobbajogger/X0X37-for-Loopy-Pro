#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/synth_state build/gen
[ -f build/gen/x0x_drum_tables.h ] || python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h >/dev/null
${CC:-cc} -std=c99 -O2 -Wall -Wextra -Werror -Ibuild/gen -DX0X_HOST -DBOARD_SMK37=1 ${SYNTH_STATE_CFLAGS:-} tests/host/synth_state_test.c -o build/synth_state/test
build/synth_state/test
