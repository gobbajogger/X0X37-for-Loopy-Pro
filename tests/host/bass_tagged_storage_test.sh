#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
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
 p=Path('tests/host/fixtures/bass_identity')/('project-v%d-sound10.json'%stream)
 d=json.loads(p.read_text());b=zlib.decompress(base64.b64decode(d['zlib_base64']))
 assert d['synthetic'] and not d['contains_user_data'] and hashlib.sha256(b).hexdigest()==d['sha256']
 (out/('v%d.bin'%stream)).write_bytes(b)
PY
U="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/app/engine.c"
for WBL in 0 1; do
 ${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter ${SAN_FLAGS:-} ${BASS_NOTE_FLAGS:-} \
 -DX0X_BASS_TAGGED_SOUND=1 -DX0X_BASS_PRESET_PREVIEW=1 -DX0X_HOST -D_DARWIN_C_SOURCE -D_POSIX_C_SOURCE=200809L -DX0X_WOBBLE=$WBL -DX0X_DEV_WOBBLE=$WBL \
 -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen tests/host/bass_tagged_storage_test.c $U -lm -o "$OUT/probe-$WBL"
 BASS_TAG_FIXTURE_OUT="${BASS_TAG_OUTPUT_DIR:-$OUT}/v6-sound11-wbl$WBL.bin" "$OUT/probe-$WBL" "$OUT/v3.bin" "$OUT/v6.bin"
done

node tests/host/bass_tagged_browser_test.mjs "${BASS_TAG_OUTPUT_DIR:-$OUT}/v6-sound11-wbl0.bin" "${BASS_TAG_OUTPUT_DIR:-$OUT}/v6-sound11-wbl1.bin"

# The ordinary Getting Started scenario still validates unflagged Chicago.
# Exercise the same sequence, motion and SAVE assertions with tagged WBL1
# preview enabled: SOUND > TONE, including the audio-boundary bank commit.
X0X_HOST_DEFS="-DX0X_BASS_PRESET_PREVIEW=1 -DX0X_BASS_TAGGED_SOUND=1 -DX0X_BASS_NOTE_SOUND=0" \
 X0X_HOST_OUT="$OUT/tutorial-host" sh host/build_host.sh >"$OUT/tutorial-build.log" 2>&1 || {
 cat "$OUT/tutorial-build.log"
 exit 1
}
mkdir -p "$OUT/tutorial"
X0X_HOST_LOAD=0 "$OUT/tutorial-host" tests/host/fixtures/getting_started_bass_preview.x0x "$OUT/tutorial"
