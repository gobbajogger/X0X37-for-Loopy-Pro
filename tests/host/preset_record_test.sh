#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host tests for the versioned preset record. No device build, no flash.
set -eu
# Do not pipe tests into tee: POSIX sh would return tee's status and could
# falsely mark a failing test binary as PASS. Capture first, print after success.
cd "$(dirname "$0")/../.."
OUT=${1:-build/preset-record}
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)
CC="${CC:-cc}"
CFLAGS="-std=c99 -O2 -Wall -Wextra -Werror -fno-fast-math -ffp-contract=off -Iexperiments/original_synth"
SRC="tests/host/preset_record_test.c firmware/src/app/preset_record.c firmware/src/app/original_sound.c experiments/original_synth/miami_program.c experiments/original_synth/chicago_program.c experiments/original_synth/fused_reference.c"

# First run may print factory_table_crc; a pin is passed when known.
PIN=""
if [ -n "${PRESET_FACTORY_TABLE_CRC:-}" ]; then
    PIN="-DPRESET_FACTORY_TABLE_CRC=${PRESET_FACTORY_TABLE_CRC}"
fi

echo "== switch OFF (default) =="
$CC $CFLAGS $PIN -o "$OUT/preset_record_off" $SRC -lm
"$OUT/preset_record_off" > "$OUT/off.log"
cat "$OUT/off.log"

# Pin the factory CRC from the first run if the caller did not supply it.
if [ -z "$PIN" ]; then
    GOT=$(sed -n 's/^factory_table_crc=//p' "$OUT/off.log")
    PIN="-DPRESET_FACTORY_TABLE_CRC=${GOT}"
    echo "== re-run OFF with pin $GOT =="
    $CC $CFLAGS $PIN -o "$OUT/preset_record_off" $SRC -lm
    "$OUT/preset_record_off" > "$OUT/off-pinned.log"
    cat "$OUT/off-pinned.log"
    cmp "$OUT/off.log" "$OUT/off-pinned.log"
fi

echo "== switch ON =="
$CC $CFLAGS $PIN -DDNBOX_FACTORY_PRESET_EXPORT=1 -o "$OUT/preset_record_on" $SRC -lm
"$OUT/preset_record_on" > "$OUT/on.log"
cat "$OUT/on.log"

echo "== sanitized OFF =="
$CC $CFLAGS $PIN -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all \
    -o "$OUT/preset_record_asan" $SRC -lm
"$OUT/preset_record_asan" > "$OUT/asan.log"
cat "$OUT/asan.log"
cmp "$OUT/off-pinned.log" "$OUT/asan.log" 2>/dev/null || cmp "$OUT/off.log" "$OUT/asan.log"

# HOST ONLY object size (not device flash).
$CC -std=c99 -Os -Wall -Wextra -Werror -c firmware/src/app/preset_record.c -o "$OUT/preset_record.o"
echo "HOST ONLY size:"
size "$OUT/preset_record.o"
echo "HOST ONLY size -A:"
size -A "$OUT/preset_record.o"

echo PASS preset-record
