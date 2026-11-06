#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT HUP INT TERM
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Ifirmware/src -Ifirmware/src/seq \
 tests/host/seq_mute_carry_test.c firmware/src/seq/sequencer.c \
 firmware/src/seq/pattern.c firmware/src/seq/tb3po.c -o "$out/test"
"$out/test"
