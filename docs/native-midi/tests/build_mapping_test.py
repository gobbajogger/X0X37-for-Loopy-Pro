#!/usr/bin/env python3
"""Independent original-build adapter tests, using only genuine retrieved log bytes."""
import copy, hashlib, importlib.util, json, os, shutil, subprocess, sys, tempfile
from pathlib import Path
if len(sys.argv) != 2:
    raise SystemExit('usage: build_mapping_test.py REPLAY_ROOT')
ROOT=Path(sys.argv[1]).resolve()
DESCRIPTOR=ROOT/'prep-evidence/accepted-build-map.json'
ACCEPTED='54f2ffbc3124f7449a69067dc8215a2d91a2e675'
base=json.loads(DESCRIPTOR.read_text());logs=(DESCRIPTOR.parent/base['build_logs']).resolve()
expected_logs=['elite-build.log','pro-build.log','controller-build.log']
missing=[str(logs/n) for n in expected_logs if not (logs/n).is_file()]
if missing:
    print('PENDING genuine original logs: '+', '.join(missing));sys.exit(2)
rows=[]
def test(name, cond, detail=''):
    rows.append({'name':name,'pass':bool(cond),'detail':detail});print(('PASS ' if cond else 'FAIL ')+name+(' '+detail if detail else ''))
with tempfile.TemporaryDirectory(prefix='accepted-build-review-') as td:
    td=Path(td);(td/'tools').mkdir();(td/'review').mkdir();(td/'evidence').mkdir()
    shutil.copy2(ROOT/'tools/accepted_build_mapping.py',td/'tools/accepted_build_mapping.py')
    shutil.copy2(ROOT/'tools/fm1_install.py',td/'tools/fm1_install.py')
    for n in ['head-guard-review.json','head-boundary-test.py','head-boundary-test.log','head-boundary-results.json','head-guard-review.md']:
        shutil.copy2(ROOT/'review'/n,td/'review'/n)
    shutil.copytree(logs,td/'evidence/logs')
    review=json.loads((DESCRIPTOR.parent/base['accepted_review']).read_text())
    review_path=td/'evidence/qa.json';review_path.write_text(json.dumps(review))
    descriptor=dict(base,build_logs='logs',accepted_review='qa.json');record=td/'evidence/map.json'
    record.write_text(json.dumps(descriptor))
    spec=importlib.util.spec_from_file_location('review_adapter',td/'tools/accepted_build_mapping.py');mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
    pkg=ROOT/'inputs/elite-dev.fwsc';ctl=ROOT/'inputs/ctl.img'
    def verify(d=None,commit=ACCEPTED,package=pkg,controller=ctl):
        record.write_text(json.dumps(descriptor if d is None else d))
        return mod.verify(ROOT,commit,record,package,controller)
    def refuse(name,**kwargs):
        try:verify(**kwargs)
        except (ValueError,OSError,KeyError,TypeError,subprocess.SubprocessError) as e:test(name,True,str(e))
        else:test(name,False,'unexpected acceptance')
    actual=verify();test('authentic exact mapping accepted',actual['result']=='ORIGINAL BUILD MAPPED' and actual['original_build_status']=='BUILD_SUCCEEDED_UNREVIEWED' and actual['accepted_commit']==ACCEPTED)
    test('mapping preserves original distinct schema',actual['schema']=='x0x37.accepted-build-map.v1' and actual.get('result')!='DEV CHECKED')
    for field,val in [('schema',2),('config','rel'),('model','pro'),('controller',False),('accepted_commit','dc4669083218f17f8cfb69069e7f32911a232b52'),('built_commit',ACCEPTED)]:
        refuse('descriptor '+field+' mutation',d=dict(descriptor,**{field:val}))
    refuse('wrong requested source',commit='dc4669083218f17f8cfb69069e7f32911a232b52')
    refuse('unknown requested source',commit='0'*40)
    refuse('controller omitted',controller=None)
    badpkg=td/'bad.fwsc';b=bytearray(pkg.read_bytes());b[-1]^=1;badpkg.write_bytes(b)
    refuse('changed actual package byte',package=badpkg)
    badctl=td/'bad-ctl.img';b=bytearray(ctl.read_bytes());b[-1]^=1;badctl.write_bytes(b)
    refuse('changed actual controller byte',controller=badctl)
    for name in expected_logs:
        path=td/'evidence/logs'/name;original=path.read_bytes();path.write_bytes(original+b'\n')
        refuse('original log corruption '+name);path.write_bytes(original)
        path.unlink();refuse('original log missing '+name);path.write_bytes(original)
    for field,val in [('id',5638784847),('commit_id',ACCEPTED),('body',review['body']+' altered'),('html_url','https://example.invalid/fake-review')]:
        changed=dict(review,**{field:val});review_path.write_text(json.dumps(changed));refuse('independent review '+field+' mutation')
    review_path.write_text(json.dumps(review))
    # Native review/log inputs live only in the temporary QA copy, never in the live preparation tree.
    for name in ['head-guard-review.json','head-boundary-test.log','head-boundary-results.json']:
        path=td/'review'/name;original=path.read_bytes();path.write_bytes(original+b'\n');refuse('native QA corruption '+name);path.write_bytes(original)
    path=td/'tools/fm1_install.py';original=path.read_bytes();path.write_bytes(original+b'\n');refuse('native updater differs from reviewed bytes');path.write_bytes(original)
    test('positive recovers after all corruptions restored',verify()['result']=='ORIGINAL BUILD MAPPED')
# Exercise genuine integration entry points without a hardware path.
cmd=[sys.executable,'-B',str(ROOT/'tools/dev_validate.py'),'verify','--repo',str(ROOT),'--commit',ACCEPTED,'--record',str(DESCRIPTOR),'--fwsc',str(ROOT/'inputs/elite-dev.fwsc'),'--ctl',str(ROOT/'inputs/ctl.img')]
r=subprocess.run(cmd,capture_output=True,text=True);test('existing native consumer maps original evidence',r.returncode==0 and r.stdout.startswith('ORIGINAL BUILD MAPPED '),r.stdout+r.stderr)
env=dict(os.environ,X0X_REPO=str(ROOT),PYTHONDONTWRITEBYTECODE='1')
cmd=['bash',str(ROOT/'tools/require_gate_pass.sh'),ACCEPTED,'--fwsc',str(ROOT/'inputs/elite-dev.fwsc'),'--ctl',str(ROOT/'inputs/ctl.img'),'--dev-record',str(DESCRIPTOR)]
r=subprocess.run(cmd+['--config','dev'],capture_output=True,text=True,env=env,cwd=ROOT);test('native shell consumer accepts dev mapping and keeps queue path',r.returncode==0 and 'DEV OK ' in r.stdout and 'ORIGINAL BUILD MAPPED ' in r.stdout,r.stdout+r.stderr)
r=subprocess.run(cmd+['--config','rel'],capture_output=True,text=True,env=env,cwd=ROOT);test('release config still refused',r.returncode==3 and 'development evidence cannot authorize a release install' in r.stdout,r.stdout+r.stderr)
result={'adapter_sha256':hashlib.sha256((ROOT/'tools/accepted_build_mapping.py').read_bytes()).hexdigest(),'dev_validate_sha256':hashlib.sha256((ROOT/'tools/dev_validate.py').read_bytes()).hexdigest(),'descriptor_sha256':hashlib.sha256(DESCRIPTOR.read_bytes()).hexdigest(),'total':len(rows),'passed':sum(x['pass'] for x in rows),'tests':rows,'hardware_io':False,'genuine_logs_only':True}
print(f"{result['passed']}/{result['total']} passed")
sys.exit(0 if result['passed']==result['total'] else 1)
