#!/usr/bin/env python3
"""Replay only the new F2 navigation fixtures; preserve source and record exit."""
import argparse, hashlib, json, os, pathlib, subprocess
p=argparse.ArgumentParser()
p.add_argument('--source',type=pathlib.Path,required=True)
p.add_argument('--case',choices=['step','live'],required=True)
p.add_argument('--sanitize',action='store_true')
a=p.parse_args(); root=pathlib.Path(__file__).resolve().parent; src=a.source.resolve()
expected={'firmware/src/app/ui.c':'eb911f58db83e417c67dd71576331803fa6c829e','firmware/src/app/engine.c':'dddc38dcc3e4ac28fc995fb1e4438703942c5570'}
verified={}
for path,want in expected.items():
 b=(src/path).read_bytes();got=hashlib.sha1(b'blob '+str(len(b)).encode()+b'\0'+b).hexdigest()
 verified[path]=got
 if got!=want: p.error('Source differs from baseline at '+path+'; select affected owner candidate deliberately and adapt the source gate before replay.')
mode='sanitized' if a.sanitize else 'normal';out=root/('replay-'+a.case+'-'+mode);out.mkdir(exist_ok=True)
fixture=(root/('step-navigation.c' if a.case=='step' else 'live-navigation.c')).read_text()
old='/workspace/f2-15-16-qualification/gobbajogger-X0X37-for-Loopy-Pro-94197fc'
fixture=fixture.replace(old,str(src)).replace('#include "physical-input.inc"','#include "'+str(root/'physical-input.inc')+'"')
generated=out/'fixture.c';generated.write_text(fixture)
env=os.environ.copy();env['SMC_UI_TEST_FILE']=str(generated);env['SMC_UI_OUT']=str(out)
if a.case=='live':env['X0X_HOST_DEFS']='-DX0X_LIVE_KEY_TEST=1'
if a.sanitize:env['SAN_FLAGS']='-fsanitize=address,undefined -fno-omit-frame-pointer'
cmd=['sh','tests/host/smc_ui_test.sh']
with (out/'run.log').open('w') as log: result=subprocess.run(cmd,cwd=src,env=env,stdout=log,stderr=subprocess.STDOUT)
receipt={'command':cmd,'cwd':str(src),'environment':{k:env[k] for k in ['SMC_UI_TEST_FILE','SMC_UI_OUT','X0X_HOST_DEFS','SAN_FLAGS'] if k in env},'source_blobs':verified,'fixture_sha256':hashlib.sha256(fixture.encode()).hexdigest(),'exit_code':result.returncode}
(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps(receipt,indent=2));raise SystemExit(result.returncode)
