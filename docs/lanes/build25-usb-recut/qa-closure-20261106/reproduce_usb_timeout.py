#!/usr/bin/env python3
"""Compile isolated timeout-reset mutants; never edit production source."""
import argparse, hashlib, json, os, pathlib, shutil, subprocess

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--root',type=pathlib.Path,required=True)
p.add_argument('--out',type=pathlib.Path,required=True)
args=p.parse_args(); root=args.root.resolve(); out=args.out.resolve()
out.mkdir(parents=True,exist_ok=True)
cc=os.environ.get('CC','cc')
source=(root/'firmware/src/usb.c').read_bytes()
def sha(data): return hashlib.sha256(data).hexdigest()
def blob(data): return hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()
assert blob(source)=='b22a79215072ee178351e42f35ed2678dde9f360'
start=source.index(b'static void usb_retry(uint32_t now_ms)')
end=source.index(b'static void usb_detach(void)',start)
body=source[start:end]
line=b'    usb.timeouts = 0;\n'
assert body.count(line)==1
mutant=source[:start]+body.replace(line,b'    /* MUTANT: stale timeout count is not reset. */\n',1)+source[end:]
records=[]
for name,fixture_ref,expect in [('old-fixture-mutant','74bd41b7477f5dcb8b882a0ab36f92724e654368',0),('v2-fixture-mutant',None,1)]:
 case=out/name; (case/'src').mkdir(parents=True,exist_ok=True); (case/'hal').mkdir(exist_ok=True)
 (case/'src/usb.c').write_bytes(mutant)
 fixture={}
 for relative,dest in [('tests/host/usb_retry_test.c',case/'test.c'),('tests/host/usb_retry_fake_usb.h',case/'hal/fm1_usb.h')]:
  data=subprocess.check_output(['git','show',fixture_ref+':'+relative],cwd=root) if fixture_ref else (root/relative).read_bytes()
  dest.write_bytes(data); fixture[relative]={'git_blob':blob(data),'sha256':sha(data)}
 argv=[cc,'-std=c99','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function','-I'+str(case),'-I'+str(root/'firmware/src'),'-I'+str(root/'firmware/hal'),'-o',str(case/'test'),str(case/'test.c')]
 compiled=subprocess.run(argv,cwd=root,capture_output=True)
 (out/(name+'-compile.log')).write_bytes(compiled.stdout+compiled.stderr)
 assert compiled.returncode==0,(name,'compile',compiled.returncode)
 run=subprocess.run([str(case/'test')],cwd=root,capture_output=True)
 log=run.stdout+run.stderr; (out/(name+'.log')).write_bytes(log)
 assert run.returncode==expect,(name,run.returncode,expect,log)
 if name.startswith('v2'):
  assert b'attach after 25 ms is up with the timeout count cleared' in log
  assert b'up 0' in log and b'timeouts 3' in log and b'ticks 600000' in log
 record={'name':name,'expected_exit':expect,'actual_exit':run.returncode,'compile_exit':compiled.returncode,'fixture_inputs':fixture,'mutant_source_git_blob':blob(mutant),'mutant_source_sha256':sha(mutant),'log_sha256':sha(log),'compile_argv':argv}
 records.append(record)
 print(name+': exit '+str(run.returncode)+(' (survived, demonstrates previous coverage gap)' if expect==0 else ' (killed by dead-SIE assertion)'),flush=True)
 print(log.decode(),end='',flush=True)
assert (root/'firmware/src/usb.c').read_bytes()==source
(out/'mutant-results.json').write_text(json.dumps({'production_git_blob':blob(source),'production_sha256':sha(source),'production_unchanged':True,'mutants':records},indent=2)+'\n')
