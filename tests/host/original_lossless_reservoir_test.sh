#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Local decoder proof and alternating host timings. No full-port/native gate.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?output directory};mkdir -p "$OUT";OUT=$(cd "$OUT"&&pwd)
python3 - <<'PY'
import hashlib,pathlib
p=pathlib.Path('tests/host/fixtures/original_lossless_second_baseline.c')
assert hashlib.sha256(p.read_bytes()).hexdigest()=='69b1985bcda35bf3bbd001b0d9b5e9c7b8711ef25c911cfb3ee00ae68ee98e8b', 'baseline differs from348ae21'
PY
for mode in normal sanitized;do
 flags='-O2';[ "$mode" != sanitized ]||flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
 ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic -Iexperiments/original_synth \
  -Doriginal_lossless_lookup=baseline_lossless_lookup \
  -Doriginal_lossless_second_lookup=baseline_lossless_second_lookup \
  -Doriginal_lossless_second16_lookup=baseline_lossless_second16_lookup \
  -c tests/host/fixtures/original_lossless_second_baseline.c -o "$OUT/baseline-$mode.o"
 ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic -Iexperiments/original_synth \
  -c experiments/original_synth/lossless_table.c -o "$OUT/candidate-$mode.o"
 ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic -Iexperiments/original_synth \
  tests/host/original_lossless_reservoir_test.c "$OUT/baseline-$mode.o" "$OUT/candidate-$mode.o" -o "$OUT/decoder-$mode"
 "$OUT/decoder-$mode" > "$OUT/decoder-$mode.log"
done
cmp "$OUT/decoder-normal.log" "$OUT/decoder-sanitized.log"
cat "$OUT/decoder-normal.log"
"$OUT/decoder-normal" --benchmark > "$OUT/benchmark.log"
python3 - "$OUT" <<'PY'
import json,pathlib,statistics,sys
out=pathlib.Path(sys.argv[1]);groups={}
for line in (out/'benchmark.log').read_text().splitlines():
    tag,name,run,count,old,new=line.split();assert tag=='TIMING'
    groups.setdefault(name,[]).append((int(count),int(old),int(new)))
rows=[]
for name,runs in groups.items():
    old=[t[1]/t[0] for t in runs];new=[t[2]/t[0] for t in runs]
    row={'workload':name,'alternating_rounds':len(runs),'lookups_per_round':runs[0][0],
         'baseline_median_ns':statistics.median(old),'candidate_median_ns':statistics.median(new),
         'baseline_min_ns':min(old),'candidate_min_ns':min(new),
         'baseline_max_ns':max(old),'candidate_max_ns':max(new),
         'median_paired_speedup':statistics.median(t[1]/t[2] for t in runs)}
    rows.append(row);print(json.dumps(row,sort_keys=True))
(out/'timing-summary.json').write_text(json.dumps({'scope':'host decoder only; no device latency or complete-engine speedup claim','workloads':rows},indent=2)+'\n')
PY
