#!/bin/sh
# Production HCI pair and per-handle credits; no device I/O.
set -eu
cd "$(dirname "$0")/../.."
OUT=${BLE_DUAL_OUT:-build/host/ble-dual-peer}
mkdir -p "$OUT"
${CC:-cc} -std=c99 -O1 -g -Wall -Wextra -Werror ${SAN_FLAGS:-} tests/host/ble_dual_peer_test.c -o "$OUT/test"
"$OUT/test"
