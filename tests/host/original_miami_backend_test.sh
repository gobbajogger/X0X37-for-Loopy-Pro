#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Adapter wiring check against the independent direct selected owner graph.
# Four strict arithmetic/sanitizer variants; no firmware activation or build.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?output directory}
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)
# The direct reference uses existing engine_events, whose generic dispatcher
# also links Chicago callbacks. These host-only dependencies are not required
# by the Miami firmware adapter, which has a Miami-only event dispatcher.
SRC=$(python3 - <<'PY'
from pathlib import Path
sources = set()
for path in ['tests/host/original_chicago_selected_graph_test.sh',
             'tests/host/original_miami_graph_test.sh']:
    sources.update(Path(path).read_text().split("SRC='")[1].split("'")[0].split())
sources.update('experiments/original_synth/' + name + '.c' for name in
               ['miami_voice_owner', 'miami_note_context', 'miami_control_owner',
                'engine_events', 'chicago_key_admission'])
print(' '.join(sorted(sources)))
PY
)
printf '%s\n' "$SRC" > "$OUT/reference-sources.txt"
for arithmetic in native soft; do
    for mode in normal sanitized; do
        flags='-O2'
        [ "$mode" != sanitized ] || flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
        [ "$arithmetic" != soft ] || flags="$flags -DORIGINAL_USE_SOFT_FMA"
        exe="$OUT/$arithmetic-$mode"
        # Three existing source warning classes stay visible on GCC13; all
        # other warnings are errors. No new adapter warning is suppressed.
        ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic \
            -Wno-error=misleading-indentation -Wno-error=type-limits -Wno-error=array-parameter \
            -fno-fast-math -ffp-contract=off -frounding-math \
            -Iexperiments/original_synth -Iexperiments/wobble \
            tests/host/original_miami_backend_test.c firmware/src/dsp/original_miami_backend.c \
            $SRC -lm -o "$exe" > "$exe.compile.log" 2>&1
        "$exe" > "$exe.log" 2>&1
    done
done
for variant in native-sanitized soft-normal soft-sanitized; do
    cmp "$OUT/native-normal.log" "$OUT/$variant.log"
done
cat "$OUT/native-normal.log"
