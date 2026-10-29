#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Focused firmware adapter differential: exact pre-FX graph; named FX projection.
# Check the two new units with strict warnings. The unchanged compact original
# units need GCC's misleading-indentation/array-parameter diagnostics disabled.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?output directory}
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)
SRC=$(python3 - <<'PY'
from pathlib import Path
paths=['tests/host/original_chicago_selected_graph_test.sh','tests/host/original_miami_graph_test.sh']
sources=set()
for path in paths:
    sources.update(Path(path).read_text().split("SRC='")[1].split("'")[0].split())
sources.update('experiments/original_synth/'+name+'.c' for name in [
    'miami_voice_owner','miami_note_context','miami_control_owner','engine_events','chicago_key_admission'])
sources.add('firmware/src/dsp/original_chicago_backend.c')
print(' '.join(sorted(sources)))
PY
)
for arithmetic in native soft; do
    for mode in normal sanitized; do
        flags='-O2'
        [ "$mode" != sanitized ] || flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
        [ "$arithmetic" != soft ] || flags="$flags -DORIGINAL_USE_SOFT_FMA"
        exe="$OUT/backend-$arithmetic-$mode"
        ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic \
            -fno-fast-math -ffp-contract=off -frounding-math \
            -Ifirmware/src/dsp -Iexperiments/original_synth -Iexperiments/wobble \
            -fsyntax-only tests/host/original_chicago_backend_test.c \
            firmware/src/dsp/original_chicago_backend.c
        ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic \
            -Wno-misleading-indentation -Wno-array-parameter \
            -fno-fast-math -ffp-contract=off -frounding-math \
            -Ifirmware/src/dsp -Iexperiments/original_synth -Iexperiments/wobble \
            tests/host/original_chicago_backend_test.c $SRC -lm -o "$exe"
        "$exe" > "$exe.log"
    done
done
for variant in native-sanitized soft-normal soft-sanitized; do
    cmp "$OUT/backend-native-normal.log" "$OUT/backend-$variant.log"
done
cat "$OUT/backend-native-normal.log"
