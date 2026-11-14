#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?fresh evidence directory required}
[ ! -e "$OUT" ] || { printf '%s\n' 'Output exists; preserve it and choose a fresh lane.' >&2; exit 2; }
mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
python3 tools/check_original_inputs.py --json > "$OUT/inputs.json"
python3 - "$OUT" <<'PY'
from pathlib import Path
import json,re,subprocess,sys,os,hashlib,shlex
out=Path(sys.argv[1])
# The explicit real-engine source closure is independent of other test runners.
src=set(['experiments/original_synth/chicago_allocator.c',
    'experiments/original_synth/chicago_arp_control.c',
    'experiments/original_synth/chicago_control_owner.c',
    'experiments/original_synth/chicago_drive.c',
    'experiments/original_synth/chicago_dynamics.c',
    'experiments/original_synth/chicago_engine_output.c',
    'experiments/original_synth/chicago_full_mg.c',
    'experiments/original_synth/chicago_fx2_compact.c',
    'experiments/original_synth/chicago_fx2_wrapper.c',
    'experiments/original_synth/chicago_global_mg.c',
    'experiments/original_synth/chicago_global_stage.c',
    'experiments/original_synth/chicago_key_admission.c',
    'experiments/original_synth/chicago_modulation.c',
    'experiments/original_synth/chicago_mp4.c',
    'experiments/original_synth/chicago_note_context.c',
    'experiments/original_synth/chicago_output.c',
    'experiments/original_synth/chicago_packed.c',
    'experiments/original_synth/chicago_program.c',
    'experiments/original_synth/chicago_program_output.c',
    'experiments/original_synth/chicago_scheduler.c',
    'experiments/original_synth/chicago_selector.c',
    'experiments/original_synth/chicago_smallfx.c',
    'experiments/original_synth/chicago_tuning.c',
    'experiments/original_synth/chicago_voice.c',
    'experiments/original_synth/chicago_voice_owner.c',
    'experiments/original_synth/control_math.c',
    'experiments/original_synth/engine_events.c',
    'experiments/original_synth/fixed_fx.c',
    'experiments/original_synth/fused_reference.c',
    'experiments/original_synth/lossless_table.c',
    'experiments/original_synth/master.c',
    'experiments/original_synth/miami_control_owner.c',
    'experiments/original_synth/miami_fx_chain.c',
    'experiments/original_synth/miami_ms20.c',
    'experiments/original_synth/miami_note_context.c',
    'experiments/original_synth/miami_output.c',
    'experiments/original_synth/miami_pitch.c',
    'experiments/original_synth/miami_program.c',
    'experiments/original_synth/miami_selected_voice.c',
    'experiments/original_synth/miami_voice_owner.c',
    'experiments/original_synth/oscillator_motion.c',
    'experiments/original_synth/voice_gain.c',
    'experiments/wobble/miami_decimator_adapter.c',
    'experiments/wobble/miami_decimator_component.c',
    'experiments/wobble/miami_fx_conversion.c',
    'firmware/src/dsp/original_chicago_backend.c',
    'firmware/src/dsp/original_miami_backend.c'])
backend='firmware/src/dsp/original_miami_backend.c'
names=sorted(set(re.findall(r'\b(original_miami_backend_\w+)\s*\(',Path(backend).read_text())))
rename=['-D'+n+'=reference_'+n[len('original_'):] for n in names]
rename+=['-Doriginal_miami_output_process=reference_miami_output_process']
# The reference TU needs its own name for the exported shared pitch table.
# Keep the pinned header and every expression unchanged; the production TU
# defines/undefines this name internally, so a compiler -D cannot rename it.
reference_source=Path(backend).read_text()
original_backend_sha256=hashlib.sha256(Path(backend).read_bytes()).hexdigest()
pitch_alias='extern const int32_t original_pitch44100[128] __attribute__((alias("original_pitch44100_local")));'
pitch_alias_replacements=reference_source.count(pitch_alias)
if pitch_alias_replacements>1:raise SystemExit('Ambiguous shared pitch alias')
if pitch_alias_replacements:
 reference_source=reference_source.replace(pitch_alias,
  'extern const int32_t reference_original_pitch44100[128] __attribute__((alias("original_pitch44100_local")));\n#define original_pitch44100 reference_original_pitch44100')
reference_backend=out/'reference_miami_backend.c'
reference_backend.write_text(reference_source)
(out/'reference-source.json').write_text(json.dumps({'source':backend,
 'source_sha256':original_backend_sha256,
 'reference_sha256':hashlib.sha256(reference_backend.read_bytes()).hexdigest(),
 'shared_pitch_alias_replacements':pitch_alias_replacements},indent=2)+'\n')
cc=os.environ.get('CC','cc')
flags=['-std=c99','-Os','-fno-builtin','-U_FORTIFY_SOURCE','-D_FORTIFY_SOURCE=0','-DORIGINAL_USE_SOFT_FMA','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-Wno-array-parameter','-pedantic','-fno-fast-math','-ffp-contract=off','-frounding-math','-Ifirmware/src/dsp','-Iexperiments/original_synth','-Iexperiments/wobble']
dependency_cmd=[cc,*flags,'-MM','tests/host/original_idle_test.c',*sorted(src)]
(out/'dependency-command.json').write_text(json.dumps(dependency_cmd,indent=2)+'\n')
deps=subprocess.run(dependency_cmd,text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
(out/'dependencies.txt').write_text(deps.stdout);(out/'dependencies.log').write_text(deps.stderr)
deps.check_returncode()
paths=set(src)|{'tests/host/original_idle_test.sh','tools/check_original_inputs.py',
    'experiments/original_synth/EXTERNAL-INPUTS.json'}
for line in deps.stdout.replace('\\\n',' ').splitlines():
    if ':' in line:paths.update(shlex.split(line.split(':',1)[1]))
def closure():return {p:hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in sorted(paths)}
before=closure();(out/'SOURCE-BEFORE.json').write_text(json.dumps(before,indent=2)+'\n')
commands=[];stage='compile';status='FAIL';code=1
wrapped=['miami_decimator_advance_zero','original_chicago_drive_process','original_chicago_fx2_wrapper_process',
    'original_fixed_fx_process','miami_decimator_frame','original_master_process']
try:
    for mode in ['normal','sanitized']:
        extra=[] if mode=='normal' else ['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all']
        obj=out/(mode+'-reference.o');exe=out/mode
        for cmd in [[cc,*flags,*extra,*rename,'-c',str(reference_backend),'-o',str(obj)],
                    [cc,*flags,*extra,'tests/host/original_idle_test.c',*sorted(src),str(obj),
                     *['-Wl,--wrap='+name for name in wrapped],'-lm','-o',str(exe)]]:
            stage=mode+' compile';commands.append(cmd)
            (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
            with (out/(mode+'.compile.log')).open('a') as log:
                p=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT)
            code=p.returncode
            if code:raise SystemExit(code)
        stage=mode+' execution'
        with (out/(mode+'.log')).open('w') as log,(out/(mode+'.diagnostics.log')).open('w') as err:
            p=subprocess.run([str(exe)],stdout=log,stderr=err,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
        code=p.returncode
        if code:raise SystemExit(code)
    stage='normal/sanitized exact log comparison'
    assert (out/'normal.log').read_bytes()==(out/'sanitized.log').read_bytes()
    status='PASS';code=0
finally:
    after=closure();(out/'SOURCE-AFTER.json').write_text(json.dumps(after,indent=2)+'\n')
    same=before==after
    (out/'RESULT.json').write_text(json.dumps({'status':status if same else 'FAIL',
        'stage':stage,'exit_code':code,'source_unchanged':same,'input_paths':len(before)},indent=2)+'\n')
    if not same:raise SystemExit('Source changed during execution')
print((out/'normal.log').read_text(),end='')
PY
