#!/usr/bin/env python3
"""Run isolated mocked launcher branches. No real MIDI, pip, or installation."""
from pathlib import Path
import json, os, shutil, subprocess, sys, tempfile
source=Path(sys.argv[1]).resolve()
manifest=json.loads((source/'MANIFEST.json').read_text())
runtime=manifest['runtime_commit']
manual=manifest.get('post_install_watch')=='manual-only'
with tempfile.TemporaryDirectory(prefix='forward-review-') as tmp:
    base=Path(tmp); root=base/'kit with spaces'
    shutil.copytree(source,root,symlinks=True)
    (root/'.venv/bin').mkdir(parents=True)
    python=root/'.venv/bin/python3'
    python.write_text('#!'+sys.executable+'\n'+'''import json,os,sys
from pathlib import Path
with open(os.environ['EVENTS'],'a') as f: f.write(json.dumps({'argv':sys.argv[1:],'lock':os.environ.get('X0X_HW_LOCK'),'queue':os.environ.get('X0X_QUEUE_FILE'),'head':os.environ.get('X0X_HEAD_REF')})+'\\n')
a=sys.argv[1:]
if a[:1]==['-c']: sys.exit(0)
if a and a[0].endswith('/fm1_install.py'):
    print('MOCK native installer')
    sys.exit(int(os.environ.get('INSTALL_RC','0')))
if a[:1]==['-B'] and a[1].endswith('/smc_connection_watch.py'):
    print('MOCK read-only capture')
    if os.environ.get('WRITE_SUMMARY','1')=='1':
        p=Path(a[a.index('--output')+1]); p.write_text('{"mock":true}\\n'); p.with_name(p.name+'.summary.txt').write_text('MOCK SUMMARY\\n')
    sys.exit(int(os.environ.get('WATCH_RC','0')))
if a[:1]==['-'] and len(a)==2:
    sys.stdin.read(); print('MOCK CoreMIDI preflight'); sys.exit(0)
if a[:1]==['-']:
    os.execv('''+repr(sys.executable)+''', ['''+repr(sys.executable)+''', *a])
raise SystemExit('Unexpected mock Python argv '+repr(a))
''')
    python.chmod(0o755)
    bins=base/'bin'; bins.mkdir()
    (bins/'uname').write_text('#!/bin/sh\necho Darwin\n'); (bins/'uname').chmod(0o755)
    realtee=shutil.which('tee')
    (bins/'tee').write_text('#!'+sys.executable+'\n'+'''import os,sys
a=sys.argv[1:]; fail=os.environ.get('FAIL_TEE','')
if (fail=='install_append' and '-a' in a and any(x.endswith('/install.log') for x in a)) or (fail=='watch_append' and '-a' in a and any(x.endswith('/connection-watch.log') for x in a)) or (fail=='install_initial' and '-a' not in a and any(x.endswith('/install.log') for x in a)) or (fail=='watch_initial' and '-a' not in a and any(x.endswith('/connection-watch.log') for x in a)):
    sys.stdin.read(); sys.exit(1)
os.execv('''+repr(realtee)+''',['''+repr(realtee)+''', *a])
'''); (bins/'tee').chmod(0o755)
    events=base/'events'; queue=base/'private-fixture-queue.md'
    # A visibly nonempty private TEST fixture, never a replacement for the owner's queue.
    queue.write_text('- [ ] '+runtime+' PRIVATE TEST FIXTURE ONLY\n')
    env={**os.environ,'PATH':str(bins)+':'+os.environ['PATH'],'EVENTS':str(events),'X0X_QUEUE_FILE':str(queue),'X0X_HW_LOCK':'owner-lock-sentinel','X0X_HEAD_REF':'owner-head-sentinel'}
    def run(name,args=(),updates=None,want=0):
        events.unlink(missing_ok=True)
        p=subprocess.run(['bash',str(root/'Install.command'),*args],env={**env,**(updates or {})},text=True,input='',capture_output=True,timeout=15)
        calls=[json.loads(x) for x in events.read_text().splitlines()] if events.exists() else []
        assert p.returncode==want,(name,p.returncode,want,p.stdout,p.stderr)
        for c in calls:
            assert c['lock']=='owner-lock-sentinel' and c['head']=='owner-head-sentinel'
        print('PASS',name,'exit',p.returncode)
        return calls,p.stdout+p.stderr
    c,o=run('offline verification opens no MIDI or dependency environment',('--verify-only',)); assert c==[]
    c,o=run('explicit unresolved nonempty queue veto',('--verify-only',),updates={'X0X_QUEUE_FILE':str(base/'bad-queue')},want=0) if False else (None,None)
    bad=base/'bad-queue'; bad.write_text('- [ ] pending PRIVATE TEST FIXTURE unresolved work\n')
    c,o=run('native present-queue veto',('--verify-only',),updates={'X0X_QUEUE_FILE':str(bad)},want=3); assert not c and 'open queue line with no commit' in o
    c,o=run('absent queue disclosed, no invented wrapper veto',('--verify-only',),updates={'X0X_QUEUE_FILE':str(base/'ABSENT')}); assert not c and ('NOT CHECKED' in o or 'not checked' in o)
    c,o=run('unsafe bypass argument refused',('--no-slots',),want=2); assert not c
    c,o=run('relative head-ref refused',('--head-ref','relative.bin'),want=2); assert not c
    for code in (1,7,8,9,10,11):
        c,o=run('native failure '+str(code)+' skips capture',updates={'INSTALL_RC':str(code)},want=code); assert not any('smc_connection_watch.py' in ' '.join(x['argv']) for x in c)
    c,o=run('successful exact guarded native invocation',('--port','Elite port name','--head-ref','/tmp/genuine head.bin'))
    inst=next(x['argv'] for x in c if x['argv'][0].endswith('fm1_install.py'))
    watch=next((x['argv'] for x in c if 'smc_connection_watch.py' in ' '.join(x['argv'])), None)
    for arg in ('--commit','--dev-record','--ctl','--backup','--verify-slots','--config','--port','--head-ref'): assert arg in inst
    assert inst[inst.index('--commit')+1]==runtime
    assert inst[inst.index('--config')+1]=='dev'
    assert inst[inst.index('--port')+1]=='Elite port name'
    assert inst[inst.index('--head-ref')+1]=='/tmp/genuine head.bin'
    assert inst[inst.index('--backup')+1].endswith('slots-before-install.x0xslots')
    assert not set(inst)&{'--yes','--force','--no-ctl','--no-slots'}
    if manual:
        assert watch is None and 'No automatic connection watcher or polling was started' in o
        if manifest.get('host_tools'):
            assert '/host-tools/tools/fm1_install.py' in inst[0]
        for failure in ('install_append', 'install_initial'):
            run('manual logging failure preserves native8 '+failure,updates={'INSTALL_RC':'8','FAIL_TEE':failure},want=8)
            calls,_=run('manual success logging failure is74 '+failure,updates={'FAIL_TEE':failure},want=74)
            assert not any('smc_connection_watch.py' in ' '.join(x['argv']) for x in calls)
        calls,_=run('manual mode never invokes watcher even with watcher failure settings',updates={'WATCH_RC':'130','WRITE_SUMMARY':'0'})
        assert not any('smc_connection_watch.py' in ' '.join(x['argv']) for x in calls)
        print('PASS all manual-only launcher contracts; no automatic watcher or device access')
        sys.exit(0)
    assert watch[watch.index('--seconds')+1]=='45' and watch[watch.index('--interval')+1]=='0.25'
    assert watch[watch.index('--port')+1]=='Elite port name'
    run('collector nonzero becomes incomplete20',updates={'WATCH_RC':'3'},want=20)
    run('collector interrupt remains130',updates={'WATCH_RC':'130'},want=130)
    c,o=run('collector early failure gets fallback report',updates={'WATCH_RC':'2','WRITE_SUMMARY':'0'},want=20); assert 'did not produce its normal report' in o
    run('collector silent missing report is incomplete20',updates={'WRITE_SUMMARY':'0'},want=20)
    run('install append-log failure preserves native8',updates={'INSTALL_RC':'8','FAIL_TEE':'install_append'},want=8)
    run('install initial-log failure preserves native8',updates={'INSTALL_RC':'8','FAIL_TEE':'install_initial'},want=8)
    run('install append-log failure after success is74',updates={'FAIL_TEE':'install_append'},want=74)
    run('install initial-log failure after success is74',updates={'FAIL_TEE':'install_initial'},want=74)
    run('watch append-log failure preserves incomplete20',updates={'WATCH_RC':'3','FAIL_TEE':'watch_append'},want=20)
    run('watch append-log failure after success is74',updates={'FAIL_TEE':'watch_append'},want=74)
    run('watch initial-log failure after success is74',updates={'FAIL_TEE':'watch_initial'},want=74)
    print('PASS all isolated launcher contracts; no device, dependency download, or firmware write')
