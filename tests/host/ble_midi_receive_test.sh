#!/bin/sh
# Public receive tests with fail-fast address/undefined sanitizers, scratch binaries only.
set -eu
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
for test in ble_midi_central ble_midi_receive ble_midi_signal ble_midi_abort; do
    ${CC:-cc} -std=c99 -g -O1 -Wall -Wextra -Werror -fsanitize=address,undefined \
        -fno-sanitize-recover=all \
        "tests/host/${test}_test.c" firmware/src/ble/ble_midi_central.c -o "$OUT/$test"
    "$OUT/$test"
done
python3 -B tests/host/ble_midi_deadline_test.py
python3 -B tests/host/ble_midi_stop_test.py
