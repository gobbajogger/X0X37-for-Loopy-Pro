#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?output directory};mkdir -p "$OUT";OUT=$(cd "$OUT"&&pwd)
python3 - "$OUT" <<'PY'
import pathlib,re,subprocess,sys,json,hashlib
out=pathlib.Path(sys.argv[1]);base='737bd7f26ee683a807544fee22156d71011981d5'
sources={name:subprocess.check_output(['git','show',base+':experiments/original_synth/'+name+'.c'],text=True) for name in ('chicago_output','master','chicago_program')}
names=[]
for src in sources.values():names+=re.findall(r'^(?:int|void|const char \*)\s*(original_\w+)\(',src,re.M)
metadata=[]
for name,src in sources.items():
 src=''.join('#define '+symbol+' baseline_'+symbol+'\n' for symbol in sorted(set(names)))+src
 (out/('baseline_'+name+'.c')).write_text(src)
 metadata.append({'unit':name,'sha256':hashlib.sha256(src.encode()).hexdigest()})
(out/'fenv_contract.h').write_text('#pragma STDC FENV_ACCESS ON\n')
(out/'BASELINE.json').write_text(json.dumps({'source_commit':base,'scope':'exact R3 output, master and Chicago program source; exported names renamed only for differential linking','renamed_functions':names,'sources':metadata},indent=2)+'\n')
PY
SRC='experiments/original_synth/chicago_program.c experiments/original_synth/chicago_output.c experiments/original_synth/chicago_drive.c experiments/original_synth/master.c experiments/original_synth/fused_reference.c experiments/wobble/miami_fx_conversion.c'
for mode in normal sanitized;do
 flags='-O2';[ "$mode" != sanitized ]||flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
 ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -frounding-math -include "$OUT/fenv_contract.h" -Iexperiments/original_synth -Iexperiments/wobble tests/host/original_binary32_classify_test.c "$OUT/baseline_chicago_output.c" "$OUT/baseline_master.c" "$OUT/baseline_chicago_program.c" $SRC -lm -o "$OUT/classify-$mode"
 "$OUT/classify-$mode" > "$OUT/classify-$mode.log"
done
cmp "$OUT/classify-normal.log" "$OUT/classify-sanitized.log"
printf 'PASS bounded classifier native-link dependency correction, host producer proof only; new source PENDING_REVIEW\n'
