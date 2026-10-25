# SPDX-License-Identifier: GPL-3.0-only
"""Verify existing private renders and host-only experiment boundaries; no AU execution."""
import json,pathlib,numpy as np,subprocess,tempfile
root=pathlib.Path(__file__).resolve().parents[3];checks={}
trials=json.loads((root/'evidence/manifest.json').read_text())+json.loads((root/'evidence/slide-holdout-manifest.json').read_text())
receipts=json.loads((root/'evidence/chicago-readbacks.json').read_text())+json.loads((root/'evidence/slide-holdout-readbacks.json').read_text())
checks['receipt_count']=len(trials)==len(receipts)==52
checks['public_readbacks']=all(t['id']==r['id'] and r['rate']==44100 and r['version']==196870 and r['freshInstance'] and all(abs(p['readback']-t['chicago'][p['id']])<=1e-6 for p in r['parameters']) for t,r in zip(trials,receipts))
checks['reference_integrity']=all((lambda x:len(x)==t['frames']*2 and np.isfinite(x).all())(np.fromfile(root/'audio'/f"{t['id']}.au.f32",dtype='<f4')) for t in trials)
checks['fresh_reference_repeats']=all(np.array_equal(np.fromfile(root/'audio'/f"{t['id']}.au.f32",dtype='<f4'),np.fromfile(root/'audio'/f"{t['id'][:-1]}1.au.f32",dtype='<f4')) for t in trials if t['id'].endswith('r0'))
checks['all_audio_finite']=all(np.isfinite(np.fromfile(p,dtype='<f4')).all() for p in (root/'audio').glob('*.f32'))
lookup=json.loads((root/'evidence/experimental-cutoff-lookup.json').read_text());e=next(x for x in lookup['entries'] if x['chicago_cutoff']==.6)
checks['one_usable_spectral_anchor']=sum(x['usable_exploratory_anchor'] for x in lookup['entries'])==1
for note in [48,60]:
 raw=np.fromfile(root/'audio'/f'grid-n{note}-w0-c104.dsp.f32',dtype='<f4');out=np.fromfile(root/'audio'/f'experimental-anchor-n{note}.f32',dtype='<f4')
 checks[f'explicit_gain_{note}']=np.array_equal(out,(raw.astype(float)*10**(e['gain_db']/20)).astype('f4'))
with tempfile.TemporaryDirectory() as td:
 output=pathlib.Path(td)/'rejected.f32'
 for cut,note in [('.2','48'),('.8','48'),('.6','49')]:
  result=subprocess.run(['python3',str(root/'repo/tools/audio303/calibrated_render.py'),'--cutoff',cut,'--note',note,str(output)],capture_output=True)
  checks[f'reject_{cut}_{note}']=result.returncode==2 and not output.exists()
 for t in json.loads((root/'evidence/manifest.json').read_text()):
  if t['id'] not in ['cut0.6-n48-r0','overlap_glide_mid-r0','accent_repeat-r0']:continue
  args=[t['frames'],t['note'],t['velocity'],*t['production'],t['sequence'],t['accent'],0,0]
  subprocess.run([str(root/'production303'),str(output),*map(str,args)],check=True)
  checks[f'default_invariance_{t["name"]}']=output.read_bytes()==(root/'audio'/f'{t["id"]}.dsp.f32').read_bytes()
slide=json.loads((root/'evidence/experimental-slide-calibration.json').read_text())
checks['slide_holdout_improved']=slide['holdout_experimental_rmse_cents']<slide['holdout_baseline_rmse_cents']
checks['firmware_source_unchanged']=subprocess.run(['git','-C',str(root/'repo'),'diff','--quiet','HEAD','--','firmware/src']).returncode==0
(root/'evidence/validation.json').write_text(json.dumps(checks,indent=2));print(json.dumps(checks,indent=2));assert all(checks.values())
