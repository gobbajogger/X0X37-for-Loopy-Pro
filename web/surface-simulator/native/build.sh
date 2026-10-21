#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Build only the host simulator adapter; never writes firmware or repository build folders.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [ "$#" -gt 0 ]; then REPO=$1
elif [ -d "$HERE/../../../firmware/src" ]; then REPO="$HERE/../../.."
else REPO="$HERE/../x0x-simulator"
fi
REPO=$(CDPATH= cd -- "$REPO" && pwd)
OUT="$HERE/build"
mkdir -p "$OUT/gen"
python3 "$REPO/tools/gen_font.py" "$OUT/gen/felucca_font.h"
python3 "$REPO/tools/gen_drum_samples.py" "$OUT/gen/x0x_drum_samples.h"
${CC:-cc} -O2 -ffp-contract=off -I"$REPO/firmware/src" -I"$OUT/gen" -o "$OUT/gen_builtin_break" "$REPO/tools/gen_builtin_break.c" "$REPO/firmware/src/dsp/drum909.c" -lm
"$OUT/gen_builtin_break" "$OUT/gen/x0x_break_bank.h"
set --
for f in dsp/drum909 dsp/drum808 dsp/bass303 dsp/breaks dsp/fxbus dsp/master seq/sequencer seq/tb3po seq/pattern seq/motion app/engine; do set -- "$@" "$REPO/firmware/src/$f.c"; done
${CC:-cc} -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -fPIC -shared -DX0X_HOST -I"$REPO" -I"$REPO/firmware/src" -I"$REPO/firmware/src/dsp" -I"$OUT/gen" -o "$OUT/core.so" "$HERE/core.c" "$@" -lm
python3 - "$REPO" "$OUT" <<'PY'
import hashlib,json,pathlib,subprocess,sys
r,o=map(pathlib.Path,sys.argv[1:]); paths=[r/'host/x0x_host.c',*sorted((r/'firmware/src').rglob('*.c')),*sorted((r/'firmware/src').rglob('*.h'))]
(o/'source-manifest.json').write_text(json.dumps({'revision':subprocess.check_output(['git','-C',str(r),'rev-parse','HEAD'],text=True).strip(),'files':{str(p.relative_to(r)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}},indent=2)+'\n')
PY
