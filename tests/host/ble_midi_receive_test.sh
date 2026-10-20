#!/bin/sh
# Public receive tests with address/undefined sanitizers, scratch binaries only.
set -eu
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
for test in ble_midi_central ble_midi_receive; do
    ${CC:-cc} -std=c99 -g -O1 -Wall -Wextra -Werror -fsanitize=address,undefined \
        "tests/host/${test}_test.c" firmware/src/ble/ble_midi_central.c -o "$OUT/$test"
    "$OUT/$test"
done
