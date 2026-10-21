#!/bin/sh
# Source-only fixture check; all compilation stays in a temporary directory.
set -eu
HERE=$(CDPATH= cd "$(dirname "$0")" && pwd)
REF=${1:-"$HERE/../../reference-smc-integration"}
REF=$(CDPATH= cd "$REF" && pwd)
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
mkdir -p "$OUT/tests/host"
cp "$REF/tests/host/smc_clip_visibility_test.sh" "$OUT/tests/host/"
cp "$HERE/smc_clip_visibility_test.c" "$OUT/tests/host/"
SRC="$REF/firmware/src" sh "$OUT/tests/host/smc_clip_visibility_test.sh"
