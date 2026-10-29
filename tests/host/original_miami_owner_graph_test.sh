#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Actual selected graph integration; explicit snapshots, no constructor claim.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?output directory};mkdir -p "$OUT";OUT=$(cd "$OUT"&&pwd)
SRC=$(python3 - <<'PY'
from pathlib import Path
paths=['tests/host/original_chicago_selected_graph_test.sh','tests/host/original_miami_graph_test.sh']
s=set()
for p in paths:s.update(Path(p).read_text().split("SRC='")[1].split("'")[0].split())
s.update('experiments/original_synth/'+p+'.c'for p in ['miami_voice_owner','miami_note_context','miami_control_owner','miami_voice_snapshot','chicago_voice_snapshot','state_codec','program_envelope','engine_events','chicago_key_admission'])
print(' '.join(sorted(s)))
PY
)
for arithmetic in native soft;do
 for mode in normal sanitized;do
  flags='-O2';[ "$mode" != sanitized ]||flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
  [ "$arithmetic" != soft ]||flags="$flags -DORIGINAL_USE_SOFT_FMA"
  exe="$OUT/miami-$arithmetic-$mode"
  ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -frounding-math -Iexperiments/original_synth -Iexperiments/wobble tests/host/original_miami_owner_graph_test.c $SRC -lm -o "$exe"
  "$exe" > "$exe.log"
 done
done
for x in native-sanitized soft-normal soft-sanitized;do cmp "$OUT/miami-native-normal.log" "$OUT/miami-$x.log";done
cat "$OUT/miami-native-normal.log"
