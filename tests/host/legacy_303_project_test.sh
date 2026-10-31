#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Old bass303 projects load safely as the Chicago default (no crash, no save).
set -eu
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
sh host/build_host.sh --generate-only >"$OUT/assets.log" 2>&1 || { cat "$OUT/assets.log"; exit 1; }
python3 - "$OUT" <<'PY'
import base64,json,sys,zlib,hashlib
from pathlib import Path
out=Path(sys.argv[1])
for stream in (3,6):
    d=json.loads((Path('tests/host/fixtures/bass_identity')/('project-v%d-sound10.json'%stream)).read_text())
    b=zlib.decompress(base64.b64decode(d['zlib_base64']))
    assert hashlib.sha256(b).hexdigest()==d['sha256']
    (out/('v%d.bin'%stream)).write_bytes(b)
PY
${CC:-cc} -std=c99 -O1 -g -w -ffp-contract=off -fno-fast-math -D_POSIX_C_SOURCE=200809L -DX0X_HOST \
  -DX0X_ORIGINAL_SYNTH=1 -DX0X_WOBBLE=0 -DX0X_DEV_WOBBLE=1 -DORIGINAL_USE_SOFT_FMA -DMIAMI_FX_NO_HOST_FENV \
  -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen -Iexperiments/original_synth -Iexperiments/wobble \
  tests/host/legacy_303_project_test.c -o "$OUT/probe" -lm $(python3 - <<'PY'
import importlib.util
spec=importlib.util.spec_from_file_location('build','tools/build.py')
b=importlib.util.module_from_spec(spec); spec.loader.exec_module(b)
print(' '.join(['firmware/src/dsp/drum909.c','firmware/src/dsp/drum808.c','firmware/src/dsp/x0x_param.c','firmware/src/dsp/breaks.c','firmware/src/dsp/fxbus.c','firmware/src/dsp/master.c','firmware/src/seq/sequencer.c','firmware/src/seq/tb3po.c','firmware/src/seq/pattern.c','firmware/src/seq/motion.c','firmware/src/app/engine.c','firmware/src/app/factory_banks.c',*b.ORIGINAL_SYNTH_UNITS,*b.ORIGINAL_SYNTH_BACKENDS]))
PY
) > "$OUT/cc.log" 2>&1 || { cat "$OUT/cc.log"; exit 1; }
"$OUT/probe" "$OUT/v3.bin" "$OUT/v6.bin"
