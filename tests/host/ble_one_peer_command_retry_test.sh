#!/bin/sh
# Bounded HCI command retry and credit ownership in A's single-peer recovery.
set -eu
cd "$(dirname "$0")/../.."
OUT=${BLE_ONE_PEER_OUT:-build/host/ble-one-peer-command}
mkdir -p "$OUT"
${CC:-cc} -std=c99 -O1 -g -Wall -Wextra -Werror ${SAN_FLAGS:-} \
    tests/host/ble_one_peer_command_retry_test.c -o "$OUT/test"
"$OUT/test"
