#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/host
"${CC:-cc}" -std=c99 -O2 -ffp-contract=off -Wall -Wextra -Werror \
 tests/host/vca_sustain_test.c experiments/original_synth/fused_reference.c \
 -lm -o build/host/vca_sustain_test
build/host/vca_sustain_test
