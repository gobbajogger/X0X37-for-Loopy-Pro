#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
${CC:-cc} -std=c99 -O1 -g ${SAN_FLAGS:-} -Ibuild/gen tests/host/bass_presets_export_test.c -lm -o "$OUT/probe"
"$OUT/probe"
