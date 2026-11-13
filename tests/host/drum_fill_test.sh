#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host sequencer check for M6 drum fill. Device and native runs are not this script.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
cd "$root"
out=$(mktemp -d "${TMPDIR:-/tmp}/x0x-drum-fill.XXXXXX")
trap 'rm -rf "$out"' EXIT HUP INT TERM
src="tests/host/drum_fill_test.c firmware/src/seq/sequencer.c firmware/src/seq/pattern.c firmware/src/seq/tb3po.c"
# shellcheck disable=SC2086
"${CC:-cc}" -std=c99 -Wall -Wextra -Werror -Ifirmware/src -Ifirmware/src/seq \
    $src -o "$out/drum_fill"
"$out/drum_fill"
if "${CC:-cc}" -fsanitize=address,undefined -fno-sanitize-recover=all -std=c99 -Wall -Wextra -Werror \
    -Ifirmware/src -Ifirmware/src/seq $src -o "$out/drum_fill_san" 2>"$out/san-compile.txt"; then
    "$out/drum_fill_san"
else
    echo "drum fill sanitizer: NOT_RUN"
fi
