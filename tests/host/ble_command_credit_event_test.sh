#!/bin/sh
# Production HCI command-credit parsing; synthetic events, no device access.
set -eu
cd "$(dirname "$0")/../.."
OUT=${BLE_CREDIT_OUT:-build/host/ble-command-credit}
mkdir -p "$OUT"
${CC:-cc} -std=c99 -O1 -g -Wall -Wextra -Werror ${SAN_FLAGS:-} \
    tests/host/ble_command_credit_event_test.c -o "$OUT/test"
"$OUT/test"
