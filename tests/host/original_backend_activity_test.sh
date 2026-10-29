#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Bounded activity regression: immutable baseline must expose false-active;
# fixed/software-FMA normal and ASan+UBSan must pass with identical audio and
# untouched raw-key metadata. Dedicated Chicago FX remains a host projection.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?output directory}
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)
BASE=77fb447fffee2128ff20f72f6a07b4b8735c8caa
git show "$BASE:firmware/src/dsp/original_chicago_backend.c" > "$OUT/baseline_chicago.c"
git show "$BASE:firmware/src/dsp/original_miami_backend.c" > "$OUT/baseline_miami.c"
SRC=$(python3 - <<'PY'
from pathlib import Path
sources=set()
for path in ['tests/host/original_chicago_selected_graph_test.sh','tests/host/original_miami_graph_test.sh']:
    sources.update(Path(path).read_text().split("SRC='")[1].split("'")[0].split())
sources.update('experiments/original_synth/'+name+'.c' for name in [
    'miami_voice_owner','miami_note_context','miami_control_owner','engine_events','chicago_key_admission'])
print(' '.join(sorted(sources)))
PY
)
printf '%s\n' "$SRC" > "$OUT/sources.txt"
for mode in baseline normal sanitized; do
    flags='-O2'
    backends='firmware/src/dsp/original_chicago_backend.c firmware/src/dsp/original_miami_backend.c'
    [ "$mode" != sanitized ] || flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
    if [ "$mode" = baseline ]; then
        backends="$OUT/baseline_chicago.c $OUT/baseline_miami.c"
    fi
    ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic \
        -Wno-misleading-indentation -Wno-array-parameter -Wno-type-limits \
        -fno-fast-math -ffp-contract=off -frounding-math -DORIGINAL_USE_SOFT_FMA \
        -Ifirmware/src/dsp -Iexperiments/original_synth -Iexperiments/wobble \
        tests/host/original_backend_activity_test.c $backends $SRC -lm \
        -o "$OUT/$mode" > "$OUT/$mode.compile.log" 2>&1
    if [ "$mode" = baseline ]; then
        set +e
        "$OUT/$mode" > "$OUT/$mode.log" 2>&1
        result=$?
        set -e
        [ "$result" = 1 ] || { cat "$OUT/$mode.log"; echo "Baseline regression exit: expected1, got$result"; exit 1; }
    else
        ASAN_OPTIONS=detect_leaks=0 "$OUT/$mode" > "$OUT/$mode.log" 2>&1
    fi
done
cmp "$OUT/normal.log" "$OUT/sanitized.log"
python3 - "$OUT" <<'PY'
from pathlib import Path
import re,sys
p=Path(sys.argv[1])
pattern=r'RESULT cases=(\d+) failures=(\d+) stereo_frames=(\d+) audio=([0-9a-f]+) raw_keys=([0-9a-f]+)'
baseline=re.search(pattern,(p/'baseline.log').read_text())
current=re.search(pattern,(p/'normal.log').read_text())
assert baseline and current
assert baseline[1]=='65' and baseline[2]=='123',baseline[0]
assert current[1]=='65' and current[2]=='0',current[0]
assert baseline.groups()[2:]==current.groups()[2:]
print('PASS: frozen baseline reproduces123 false-active/idle observations;65 real lifecycle cases pass.')
print(current[0])
print('Original audio and raw key-state hashes match baseline. No raw DSP state was cleared for activity.')
PY
