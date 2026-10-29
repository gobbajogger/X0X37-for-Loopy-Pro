#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Actual-source held-note/manual/autosave regression; SAN_FLAGS enables sanitizers.
set -eu
cd "$(dirname "$0")/../.."
OUT=${SAVE_NOTE_OUT:-build/host/save-note-quiet}
mkdir -p "$OUT"
sh host/build_host.sh --generate-only >/dev/null
. tests/host/original_build.sh
U=$ORIGINAL_SOURCES
# shellcheck disable=SC2086
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    ${SAN_FLAGS:-} -DX0X_HOST -DBOARD_SMK37=1   \
    -DX0X_BASS_PRESET_PREVIEW=1 -DX0X_BASS_TAGGED_SOUND=1 -DX0X_BASS_NOTE_SOUND=0 \
    -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen tests/host/save_note_quiet_test.c $ORIGINAL_FLAGS $U -lm -o "$OUT/test"
"$OUT/test"
