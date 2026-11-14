#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Actual UI renderer and production button filter/clip transport; no target SDK.
set -eu
cd "$(dirname "$0")/../.."
OUT=${OUT:-build/host/clip-question-hint}
mkdir -p "$OUT"
python3 - "$OUT" <<'PY'
from pathlib import Path
import hashlib, json, re, sys
root = Path.cwd()
out = Path(sys.argv[1]).resolve()
def function(text, name):
    hit = re.search(r"^[^\n;]*\b" + name + r"\([^;]*?\)\s*(?:/\*[^\n]*\*/)?\n\{.*?^\}", text, re.M | re.S)
    if not hit:
        raise SystemExit('missing production function: ' + name)
    return hit.group()
def replace_once(text, before, after):
    if text.count(before) != 1:
        raise SystemExit('ambiguous fixture replacement: ' + before[:50])
    return text.replace(before, after)
host = (root/'host/x0x_host.c').read_text()
plat = (root/'firmware/src/app/plat_fm1.c').read_text()
main = (root/'firmware/src/app/main_fm1.c').read_text()
panel_source = (root/'firmware/src/app/panel.c').read_text()
smk_input = (root/'firmware/hal/smk_input.h').read_text()
# Use the actual SMK default panel map and the production octave cutoff.
# Calibrated per-device tables remain outside this host fixture.
buttons = re.search(r'static const panel_t PANEL_DEFAULT = \{\s*PANEL_MAGIC,\s*\{([^}]+)\}', panel_source)
cutoff = re.search(r'FM1_BTN_OCT_UP = (\d+)', smk_input)
if not buttons or not cutoff:
    raise SystemExit('missing production SMK panel mapping or octave cutoff')
declarations = ('static struct { uint32_t buttons; } fm1_in;\n'
                'static struct { uint8_t btn[NB]; } panel = {{'+buttons.group(1)+'}};\n'
                '#define FM1_BTN_OCT_UP '+cutoff.group(1)+'u\n')
host = replace_once(host, 'uint32_t plat_buttons(void) { return held_btn; }',
                    declarations + function(plat, 'plat_buttons'))
# Preserve all original relative includes after putting the host translation unit in OUT.
host = re.sub(r'(?m)^#include "(\.\./[^"]+)"',
              lambda h: '#include "'+str((root/'host'/h.group(1)).resolve())+'"', host)
(out/'clip_question_host.inc').write_text(host)
fixture = (root/'tests/host/smc_ui_test.c').read_text()
fixture = replace_once(fixture, '#include "../../host/x0x_host.c"',
                       '#include "'+str(out/'clip_question_host.inc')+'"')
for name in ['smk_clip_transport_button', 'smk_clip_history_button']:
    original = re.search(r'(?m)^uint8_t '+name+r'\([^\n{]+\)\s*\{[^\n]+\}', fixture)
    if not original:
        raise SystemExit('missing fixture substitute: '+name)
    declaration = original.group().split('{', 1)[0].rstrip() + ';'
    fixture = replace_once(fixture, original.group(), declaration)
(out/'clip_question_fixture.inc').write_text(fixture)
(out/'clip_question_dispatch.inc').write_text('\n'.join(function(main, name) for name in
    ['smk_clip_history_button','smk_clip_transport_button'])+'\n')
manifest = {'scope':'Production physical-button filter and clip transport/history replace only named fixture substitutes; hardware mapping/IRQ/MIDI sinks remain host substitutes.',
            'inputs':{},'generated':{}}
for path in ['host/x0x_host.c','tests/host/smc_ui_test.c','firmware/src/app/ui.c',
             'firmware/src/app/plat_fm1.c','firmware/src/app/main_fm1.c',
             'firmware/src/app/panel.c','firmware/hal/smk_input.h']:
    manifest['inputs'][path] = hashlib.sha256((root/path).read_bytes()).hexdigest()
for path in out.glob('clip_question_*.inc'):
    manifest['generated'][path.name] = hashlib.sha256(path.read_bytes()).hexdigest()
(out/'input-generation.json').write_text(json.dumps(manifest,indent=2)+'\n')
PY
awk '/^static const char CK_FLD_N\[\] = /{print}
     /^static uint32_t ck_page_on\(void\)$/ || /^int smk_ck_smc_(turn|nav)\(/{on=1}
     on{print} on && /^}/{on=0}' firmware/src/app/main_fm1.c > "$OUT/clip_smc.inc"
test -s "$OUT/clip_smc.inc"
sh host/build_host.sh --generate-only > "$OUT/generate.log" 2>&1 || { cat "$OUT/generate.log"; exit 1; }
. tests/host/original_build.sh
result=0
for MODEL in 'SMK-37 Elite' 'SMK-37 Pro'; do
    case "$MODEL" in *Elite) MODEL_OUT="$OUT/elite";; *) MODEL_OUT="$OUT/pro";; esac
    mkdir -p "$MODEL_OUT"
    # Same production source/flags for both model identities; host substitutes remain explicit.
    # shellcheck disable=SC2086
    ${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
        -DX0X_HOST -D_POSIX_C_SOURCE=200809L "-DSMK_MODEL=\"$MODEL\"" ${SAN_FLAGS:-} \
        -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen -I"$OUT" \
        tests/host/clip_question_hint_test.c $ORIGINAL_FLAGS $ORIGINAL_SOURCES -lm -o "$MODEL_OUT/test"
    SMC_UI_SHOTS="$MODEL_OUT" "$MODEL_OUT/test" || result=1
done
exit "$result"
