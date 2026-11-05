#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Chain slot = longest 909 pattern. The playback test, then two mutants:
# shortest window, and advance at the playing pattern's own end.
set -eu
cd "$(dirname "$0")/../.."
CC=${CC:-cc}
OUT=${1:-build/host}
mkdir -p "$OUT"
W="-std=c99 -O2 -ffp-contract=off -Wall -Wextra -Wdouble-promotion -Werror"
INC=""
SRC="tests/host/chain_longest_test.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c"

# shellcheck disable=SC2086
"$CC" $W $INC -o "$OUT/chain_longest_test" $SRC -lm
"$OUT/chain_longest_test"

expect_mutant() {
    name=$1
    def=$2
    needle=$3
    # shellcheck disable=SC2086
    "$CC" $W $INC -D"$def"=1 -o "$OUT/chain_longest_mutant_$name" $SRC -lm
    if "$OUT/chain_longest_mutant_$name" >"$OUT/chain_longest_mutant_$name.log" 2>&1; then
        echo "chain-longest mutant $name passed; $needle must fail"
        exit 1
    fi
    if ! grep -q "$needle" "$OUT/chain_longest_mutant_$name.log"; then
        echo "chain-longest mutant $name failed without $needle"
        cat "$OUT/chain_longest_mutant_$name.log"
        exit 1
    fi
    echo "chain-longest mutant: $name killed ($needle)"
}

expect_mutant shortest SEQ_CHAIN_WINDOW_SHORTEST "long-holds"
expect_mutant first-end SEQ_CHAIN_WINDOW_FIRST_END "short-holds"
"$OUT/chain_longest_test"
