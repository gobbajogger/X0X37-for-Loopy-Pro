#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
out=${1:-build/wobble-host}
mkdir -p "$out"
cc=${CC:-clang}
"$cc" --version > "$out/compiler.txt"
"$cc" -std=c99 -Os -Wall -Wextra -Werror -ffp-contract=off -c experiments/wobble/wobble.c -o "$out/wobble.o"
"$cc" -std=c99 -O2 -Wall -Wextra -Werror -ffp-contract=off experiments/wobble/test_wobble.c "$out/wobble.o" -lm -o "$out/test_wobble"
"$cc" -std=c99 -Os -Wall -Wextra -Werror -ffp-contract=off experiments/wobble/render.c "$out/wobble.o" -o "$out/render"
"$out/test_wobble" > "$out/tests.txt"
size -m "$out/wobble.o" > "$out/object-size.txt"
nm -u "$out/wobble.o" > "$out/undefined-symbols.txt"
cat "$out/tests.txt" "$out/object-size.txt" "$out/undefined-symbols.txt"
