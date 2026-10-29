#!/bin/sh
# Actual owned numeric reset plus existing5319 wider-entry counterexample.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?fresh output};BASE=${2:?immutable5319repo};mkdir -p "$OUT";OUT=$(cd "$OUT"&&pwd)
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
  cc -std=c99 $flags -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -frounding-math -Iexperiments/original_synth -Iexperiments/wobble tests/host/original_miami_geometry_owner_test.c experiments/original_synth/miami_geometry.c $SRC -lm -o "$exe"
  "$exe" > "$exe.log"
 done
done
for x in native-sanitized soft-normal soft-sanitized;do cmp "$OUT/owner-native-normal.log" "$OUT/owner-$x.log";done
[ "$(git -C "$BASE" rev-parse HEAD)" = 5319f3776955d7ec31e82adc8382d050c92de72c ]
[ -z "$(git -C "$BASE" status --porcelain)" ]
OLD_SRC=$(for p in $SRC;do printf '%s/%s ' "$BASE" "$p";done)
cc -std=c99 -O2 -DORIGINAL_USE_SOFT_FMA -DMIAMI_GEOMETRY_WIDER_RESET_BASELINE -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -frounding-math -Iexperiments/original_synth -Iexperiments/wobble tests/host/original_miami_geometry_owner_test.c experiments/original_synth/miami_geometry.c $OLD_SRC -lm -o "$OUT/wider-entry-counterexample"
set +e
"$OUT/wider-entry-counterexample" > "$OUT/wider-entry-counterexample.stdout" 2> "$OUT/wider-entry-counterexample.log"
status=$?
set -e
[ "$status" = 42 ]
printf '%s\n' "$status" > "$OUT/wider-entry-counterexample.exit"
cat "$OUT/owner-native-normal.log" "$OUT/wider-entry-counterexample.log"
