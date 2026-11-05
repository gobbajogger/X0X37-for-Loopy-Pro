#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=build/host/usb_retry_cooperative
mkdir -p "$OUT/src" "$OUT/hal"
cp "${SRC:-firmware/src}/usb.c" "$OUT/src/usb.c"
cp tests/host/smk_clip_usb_clear_fake_usb.h "$OUT/hal/fm1_usb.h"
${CC:-cc} -std=gnu99 -O1 -Wall -Wextra -Werror -Wno-unused-function \
    -ffunction-sections -fdata-sections -Wl,--gc-sections -I"$OUT" \
    -o "$OUT/test" tests/host/usb_retry_cooperative_test.c
"$OUT/test"
