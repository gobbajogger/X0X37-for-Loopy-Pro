#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/x0x-ble-peer-name-bounds.XXXXXX")
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
awk '/^void plat_ble_peers\(/ || /^unsigned plat_ble_peer_names\(/ || /^const char \*plat_ble_name\(char \*names\)/{on=1} on{print} on && /^}/{on=0}' firmware/src/ble.c > "$OUT/ble_peer_names.inc"
test -s "$OUT/ble_peer_names.inc"
# SAN_FLAGS optionally enables the caller's address/undefined sanitizers.
# shellcheck disable=SC2086
${CC:-cc} -std=c11 -Wall -Wextra -Werror -DX0X_HOST ${SAN_FLAGS:-} -I"$OUT" \
    tests/host/ble_peer_name_bounds_test.c -o "$OUT/test"
"$OUT/test"
