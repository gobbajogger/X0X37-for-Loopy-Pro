#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/host
${CC:-gcc} -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -Wno-error=attributes ${SAN_FLAGS:-} tests/host/adaptive_break_dsp_test.c -lm -o build/host/adaptive_break_dsp_test
build/host/adaptive_break_dsp_test
