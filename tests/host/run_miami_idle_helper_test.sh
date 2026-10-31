#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Exact original Miami idle math and zero-store source, no private headers.
set -eu
cd "$(dirname "$0")/../.."
out="build/host/miami-idle-helper"
if [ "$#" -gt 0 ]; then out="$1"; fi
mkdir -p "$out"
for mode in normal sanitized; do
    flags='-Os'
    if [ "$mode" = sanitized ]; then
        flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
    fi
    cc -std=c99 $flags -Wall -Wextra -Werror -fno-fast-math -ffp-contract=off \
      -Iexperiments/original_synth tests/host/miami_idle_helper_smoke_test.c -o "$out/$mode"
    ASAN_OPTIONS=detect_leaks=0 "$out/$mode"
done
