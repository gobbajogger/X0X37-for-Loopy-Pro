#!/bin/sh
# Clocked Equal profile with canonical mode dispatch, not a chord renderer.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?fresh output};BASE=${2:?immutable4e69770repo};mkdir -p "$OUT";OUT=$(cd "$OUT"&&pwd)
SRC=$(python3 - <<'PY'
from pathlib import Path
s=set()
for p in ['tests/host/original_chicago_selected_graph_test.sh','tests/host/original_miami_graph_test.sh']:s.update(Path(p).read_text().split("SRC='")[1].split("'")[0].split())
s.update('experiments/original_synth/'+p+'.c'for p in ['miami_voice_owner','miami_note_context','miami_control_owner','miami_voice_snapshot','chicago_voice_snapshot','state_codec','program_envelope','engine_events','chicago_key_admission'])
print(' '.join(sorted(s)))
PY
)
for arithmetic in native soft;do
 for mode in normal sanitized;do
  flags='-O2';[ "$mode" != sanitized ]||flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
  [ "$arithmetic" != soft ]||flags="$flags -DORIGINAL_USE_SOFT_FMA"
  exe="$OUT/owner-$arithmetic-$mode"
  cc -std=c99 $flags -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -frounding-math -Iexperiments/original_synth -Iexperiments/wobble tests/host/original_miami_clocked_graph_test.c experiments/original_synth/miami_geometry.c $SRC -lm -o "$exe"
  "$exe" > "$exe.log"
 done
done
for x in native-sanitized soft-normal soft-sanitized;do cmp "$OUT/owner-native-normal.log" "$OUT/owner-$x.log";done
[ "$(git -C "$BASE" rev-parse HEAD)" = 4e697701f7fef4981a81ca04159e2b595aa810e9 ]
[ -z "$(git -C "$BASE" status --porcelain)" ]
OLD_SRC=$(for p in $SRC;do printf '%s/%s ' "$BASE" "$p";done)
cc -std=c99 -O2 -DORIGINAL_USE_SOFT_FMA -DMIAMI_CLOCKED_BASELINE -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -frounding-math -Iexperiments/original_synth -Iexperiments/wobble tests/host/original_miami_clocked_graph_test.c "$BASE/experiments/original_synth/miami_geometry.c" $OLD_SRC -lm -o "$OUT/selected-binder-counterexample"
set +e
"$OUT/selected-binder-counterexample" > "$OUT/selected-binder-counterexample.stdout" 2> "$OUT/selected-binder-counterexample.log"
status=$?
set -e
[ "$status" = 42 ]
printf '%s\n' "$status" > "$OUT/selected-binder-counterexample.exit"
cat "$OUT/owner-native-normal.log" "$OUT/selected-binder-counterexample.log"
