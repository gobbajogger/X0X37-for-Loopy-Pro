#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# M7a end to end firmware -> web: build the whole host app with tests/host/part_off_web_e2e.c, export the .x0x
# project for every part OFF mask (and cross-check web-built word 28 via project_file_validate), then run
# tests/host/part_off_web_e2e.mjs (real web parseProject + agreement with fw_validate.txt) on it.
#   SAN_FLAGS="-fsanitize=address,undefined"  sanitizer build. No node: FAIL (exit 1); D6 forbids false-green NOT_RUN.
#   PART_OFF_WEB_E2E_FORCE_NOT_RUN=<reason>  print NOT_RUN for a required check and exit 1.
#     run_tests.sh counts a command as ok when it exits 0, and it does not read inner NOT_RUN lines.
#     This script has no allowed skip. A NOT_RUN line must not exit 0.
set -eu
cd "$(dirname "$0")/../.."
if [ -n "${PART_OFF_WEB_E2E_FORCE_NOT_RUN:-}" ]; then
    echo "  NOT_RUN part_off_web_e2e (${PART_OFF_WEB_E2E_FORCE_NOT_RUN})"
    exit 1
fi
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
cat "$OUT/export.log"
# C harness prints PASS/FAIL and exits non-zero on its cross-checks; keep that failure.
grep -q '^part_off_web_e2e_fw: ok' "$OUT/export.log" || { echo "  FAIL part_off_web_e2e (firmware validate cross-check)"; exit 1; }
if grep -q '  NOT_RUN ' "$OUT/export.log"; then
    echo "  FAIL part_off_web_e2e (firmware check NOT_RUN)"
    exit 1
fi
node tests/host/part_off_web_e2e.mjs "$OUT/files" >"$OUT/node.log"
cat "$OUT/node.log"
if grep -q '  NOT_RUN ' "$OUT/node.log"; then
    echo "  FAIL part_off_web_e2e (web check NOT_RUN)"
    exit 1
fi
