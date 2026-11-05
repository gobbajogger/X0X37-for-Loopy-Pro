#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Cooperative USB retry against a counted reset/attach model, and the old blocking retry.
set -e
cd "$(dirname "$0")/../.."
OUT=build/host/usb-retry
mkdir -p "$OUT/src" "$OUT/hal"
cp firmware/src/usb.c "$OUT/src/usb.c"
cp tests/host/usb_retry_fake_usb.h "$OUT/hal/fm1_usb.h"
${CC:-cc} -std=c99 -O2 -Wall -Wextra -Werror -Wno-unused-function \
    -I"$OUT" -Ifirmware/src -Ifirmware/hal \
    -o "$OUT/usb_retry_test" tests/host/usb_retry_test.c
"$OUT/usb_retry_test"
