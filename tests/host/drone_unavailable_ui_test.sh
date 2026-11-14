#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Focused actual UI/engine tests. Optional baseline uses an immutable pre-change
# UI against the same current engine; no source or shared fixture is modified.
set -eu
cd "$(dirname "$0")/../.."
OUT=${OUT:-build/host/drone-unavailable-ui}
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)
awk '/^static const char CK_FLD_N\[\] = /{print}
     /^static uint32_t ck_page_on\(void\)$/ || /^int smk_ck_smc_(turn|nav)\(/{on=1}
     on{print} on && /^}/{on=0}' firmware/src/app/main_fm1.c > "$OUT/clip_smc.inc"
test -s "$OUT/clip_smc.inc"
sh host/build_host.sh --generate-only > "$OUT/generate.log" 2>&1 || { cat "$OUT/generate.log"; exit 1; }
. tests/host/original_build.sh
python3 - "$OUT" <<'PY'
from pathlib import Path
import hashlib,json,os,sys
root=Path.cwd();out=Path(sys.argv[1])
inputs=['tests/host/drone_unavailable_ui_test.c','tests/host/drone_unavailable_ui_test.sh',
        'tests/host/smc_ui_test.c','host/x0x_host.c','tests/host/original_build.sh',
        'firmware/src/app/ui.c','firmware/src/app/engine.c','firmware/src/app/original_presets.inc',
        'firmware/src/app/original_runtime.c','firmware/src/app/original_runtime.h',
        'firmware/src/app/bass_identity.h','firmware/src/app/main_fm1.c','build/gen/felucca_font.h']
manifest={'scope':'Focused production UI/engine host tests; no firmware emulator/native/device acceptance.',
          'inputs':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in inputs},
          'scenarios':os.getenv('DRONE_UI_SCENARIOS','restore selection pending pattern pending-pattern locks available').split()}
baseline=os.getenv('DRONE_UI_BASELINE_SOURCE')
if baseline:
    source=Path(baseline).resolve();data=source.read_bytes()
    expected='8778a5fce5d635927e6116c295fa2c5999d9a0a35664b2422f3602aaa774f2c4'
    if hashlib.sha256(data).hexdigest()!=expected:raise SystemExit('wrong immutable pre-change UI source')
    host=(root/'host/x0x_host.c').read_text()
    import re
    host=re.sub(r'(?m)^#include "(\.\./[^"]+)"',lambda m:'#include "'+str((root/'host'/m[1]).resolve())+'"',host)
    old='#include "'+str(root/'firmware/src/app/ui.c')+'"'
    if host.count(old)!=1:raise SystemExit('ambiguous host UI include')
    host=host.replace(old,'#include "'+str(source)+'"')
    (out/'baseline_host.inc').write_text(host)
    fixture=(root/'tests/host/smc_ui_test.c').read_text()
    old='#include "../../host/x0x_host.c"'
    if fixture.count(old)!=1:raise SystemExit('ambiguous SMC host include')
    (out/'baseline_fixture.inc').write_text(fixture.replace(old,'#include "'+str(out/'baseline_host.inc')+'"'))
    manifest['baseline']={'ui_sha256':expected,'git_blob':'0694f20d1e7106d2479aa3846873426af7d114c9',
                          'method':'Only UI include changes; engine, input fixture, fonts and all other source remain identical.'}
(out/'inputs.json').write_text(json.dumps(manifest,indent=2)+'\n')
PY
result=0
for MODEL in 'SMK-37 Elite' 'SMK-37 Pro'; do
    case "$MODEL" in *Elite) MODEL_DIR=elite;; *) MODEL_DIR=pro;; esac
    for MODE in normal sanitized; do
        DIR="$OUT/$MODEL_DIR/$MODE"
        mkdir -p "$DIR"
        FLAGS=
        [ "$MODE" != sanitized ] || FLAGS='-fsanitize=address,undefined -fno-sanitize-recover=all'
        # shellcheck disable=SC2086
        ${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
            -DX0X_HOST -D_POSIX_C_SOURCE=200809L "-DSMK_MODEL=\"$MODEL\"" $FLAGS ${SAN_FLAGS:-} \
            -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen -I"$OUT" \
            tests/host/drone_unavailable_ui_test.c $ORIGINAL_FLAGS $ORIGINAL_SOURCES -lm \
            -o "$DIR/test" > "$DIR/compile.log" 2>&1 || { cat "$DIR/compile.log"; exit 1; }
        for SCENARIO in ${DRONE_UI_SCENARIOS:-restore selection pending pattern pending-pattern locks available}; do
            SMC_UI_SHOTS="$DIR" "$DIR/test" "$SCENARIO" > "$DIR/$SCENARIO.log" 2>&1 || result=1
            cat "$DIR/$SCENARIO.log"
        done
    done
    if [ -n "${DRONE_UI_BASELINE_SOURCE:-}" ]; then
        DIR="$OUT/$MODEL_DIR/baseline"
        mkdir -p "$DIR"
        # shellcheck disable=SC2086
        ${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
            -DX0X_HOST -D_POSIX_C_SOURCE=200809L "-DSMK_MODEL=\"$MODEL\"" \
            "-DDRONE_UI_FIXTURE_FILE=\"$OUT/baseline_fixture.inc\"" \
            -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen -I"$OUT" \
            tests/host/drone_unavailable_ui_test.c $ORIGINAL_FLAGS $ORIGINAL_SOURCES -lm \
            -o "$DIR/test" > "$DIR/compile.log" 2>&1 || { cat "$DIR/compile.log"; exit 1; }
        SMC_UI_SHOTS="$DIR" "$DIR/test" baseline > "$DIR/captures.log" 2>&1 || result=1
        cat "$DIR/captures.log"
        # A passing new implementation alone cannot show that the assertions
        # detect the removed UI behavior. Keep exact pre-change expected-red
        # controls for terminal presentation, engine detents, pending cues and
        # archived PAT SOUND refusal/OFF; production engine stays unchanged.
        for SCENARIO in restore selection pending pattern; do
            if "$DIR/test" "$SCENARIO" > "$DIR/negative-$SCENARIO.log" 2>&1; then
                printf '%s\n' "FAIL: pre-change UI unexpectedly passed $SCENARIO" >&2
                result=1
            else
                printf '%s\n' "Expected-red pre-change UI: $MODEL $SCENARIO"
                cat "$DIR/negative-$SCENARIO.log"
                if ! rg -q '^FAIL .*drone_unavailable_ui_test[.]c:' "$DIR/negative-$SCENARIO.log"; then
                    printf '%s\n' "FAIL: pre-change $SCENARIO did not reach a contract assertion" >&2
                    result=1
                fi
            fi
        done
    fi
done
if [ -n "${DRONE_UI_BASELINE_SOURCE:-}" ]; then
    python3 - "$OUT" <<'PY' || result=1
from pathlib import Path
import hashlib,json,struct,sys
out=Path(sys.argv[1]);results=[]
for model in ['elite','pro']:
    for bank in ['CHICAGO','MIAMI','JUNO']:
        for voice in ['A','B']:
            name=f'sound-{bank}-{voice}.rgb565'
            before=(out/model/'baseline'/name).read_bytes();after=(out/model/'normal'/name).read_bytes()
            a=struct.unpack('<57600H',before);b=struct.unpack('<57600H',after)
            # SOUND is the selected row. Its caption excludes read-only STATUS
            # from KNOBS 5-7, and the old STATUS adjustment bar disappears.
            # Every other pixel must remain identical.
            caption_y=int((out/model/'normal'/name.replace('.rgb565','.caption-y')).read_text())
            old_y=int((out/model/'baseline'/name.replace('.rgb565','.caption-y')).read_text())
            if caption_y!=old_y:raise SystemExit(f'{model}/{name}: selected row moved')
            bar_y=caption_y+45
            caption_changes=bar_changes=outside=0
            for i,(x,y) in enumerate(zip(a,b)):
                if x==y:continue
                xx,yy=i%240,i//240
                if 110<=xx<234 and caption_y<=yy<caption_y+14:caption_changes+=1
                elif 130<=xx<172 and yy==bar_y:bar_changes+=1
                else:outside+=1
            if outside:raise SystemExit(f'{model}/{name}: {outside} changed pixels outside caption/status bar')
            if not caption_changes:raise SystemExit(f'{model}/{name}: missing expected caption correction')
            if bar_changes!=42:raise SystemExit(f'{model}/{name}: status bar removal was not exactly42pixels')
            if any(b[bar_y*240+x]!=b[bar_y*240+129] for x in range(130,172)):
                raise SystemExit(f'{model}/{name}: status bar did not return to its surrounding face')
            results.append({'model':model,'frame':name,'before_sha256':hashlib.sha256(before).hexdigest(),
                            'after_sha256':hashlib.sha256(after).hexdigest(),'caption_changes':caption_changes,
                            'status_bar_changes':bar_changes,'outside_caption_status_bar_changes':outside})
(out/'equivalent-state-frames.json').write_text(json.dumps(results,indent=2)+'\n')
print('Equivalent available-engine before/after host frames: PASS (caption/status-bar correction only; both model identities)')
PY
fi
exit "$result"
