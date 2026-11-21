#!/usr/bin/env python3
import os,subprocess,tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[2];out=root/'build/host/freeplay-state';out.mkdir(parents=True,exist_ok=True)
def run(name,source=None):
 env=dict(os.environ,SMC_UI_TEST_FILE='tests/host/ui_freeplay_state_test.c',SMC_UI_OUT=str(out/name))
 if source:env['X0X_HOST_DEFS']=env.get('X0X_HOST_DEFS','')+' -DX0X_HOST_UI_SOURCE="'+str(source)+'"'
 p=subprocess.run(['sh','tests/host/smc_ui_test.sh'],cwd=root,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=100);(out/(name+'.log')).write_text(p.stdout);return p
p=run('positive')
if p.returncode or 'PASS FREE PLAY' not in p.stdout:raise SystemExit(p.stdout[-3000:])
s=(root/'firmware/src/app/ui.c').read_text()
anchor='&& ui.freeplay[bidx()] && !ui.rec'
if s.count(anchor)!=1:raise SystemExit('missing/duplicate mode anchor')
cue='free_mode?"FREE PLAY":VIEW_N[ui.view]'
if s.count(cue)!=1:raise SystemExit('missing/duplicate cue anchor')
# Before fd29ace4 the sequencer header always used the part/view title and normal
# header font. Making free_mode false selects those same paths on this checkout.
controls = [
    ('missing-cue', s.replace(anchor, '&& 0'), 'FREE_PLAY_MODE_CUE_MISSING'),
    ('missing-label', s.replace(cue, 'VIEW_N[ui.view]'), 'FREE_PLAY_MODE_CUE_MISSING'),
    ('always-free', s.replace(anchor, '&& !ui.rec'), 'FREE_PLAY_MODE_CUE_UNEXPECTED'),
]
for name, text, marker in controls:
 with tempfile.TemporaryDirectory(prefix='freeplay-mode-negative-') as d:
  f=Path(d)/'ui.c';f.write_text(text);p=run(name,f)
  if not p.returncode or marker not in p.stdout or 'FAIL ' not in p.stdout:raise SystemExit(name+' not rejected as intended '+p.stdout[-3000:])
print('PASS actual FREE PLAY visibility/no-loss and missing-cue/wrong-state negatives')
