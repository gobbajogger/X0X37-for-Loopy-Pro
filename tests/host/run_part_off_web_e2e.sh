#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# M7a end to end firmware -> web: build the whole host app with tests/host/part_off_web_e2e.c, export the .x0x
# project for every part OFF mask, then run tests/host/part_off_web_e2e.mjs (real web parseProject) on it.
#   SAN_FLAGS="-fsanitize=address,undefined"  sanitizer build. No node: FAIL (exit 1); D6 forbids false-green NOT_RUN.
set -eu
cd "$(dirname "$0")/../.."
command -v node >/dev/null 2>&1 || { echo "  FAIL part_off_web_e2e (node not found)"; exit 1; }
OUT=${OUT:-build/host/part-off-web-e2e}
rm -rf "$OUT" && mkdir -p "$OUT/files"
sh host/build_host.sh --generate-only >/dev/null
. tests/host/original_build.sh
# shellcheck disable=SC2086
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -w -DX0X_HOST -D_POSIX_C_SOURCE=200809L ${SAN_FLAGS:-} \
    -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen \
    tests/host/part_off_web_e2e.c $ORIGINAL_FLAGS $ORIGINAL_SOURCES -lm -o "$OUT/export"
"$OUT/export" "$OUT/files" >"$OUT/export.log"
node tests/host/part_off_web_e2e.mjs "$OUT/files"
