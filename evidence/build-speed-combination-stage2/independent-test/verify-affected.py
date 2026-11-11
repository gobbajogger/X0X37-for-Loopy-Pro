#!/usr/bin/env python3
"""Verify the checked fixture-only outer-background repair; reuse unaffected tests."""
import ast,hashlib,json,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];OUT=Path(__file__).resolve().parent
BEFORE='c0d2ab2f3248ce86a1e6421be926ce5297768272';AFTER='e7e63bdca31e238c07b724fcc271788c877918b8'
def git(*args):return subprocess.check_output(['git',*args],cwd=ROOT)
def sha(raw):return hashlib.sha256(raw).hexdigest()
assert git('rev-parse','HEAD').decode().strip()==AFTER
changed=[p for p in git('diff','--name-only',BEFORE,AFTER).decode().splitlines() if not p.startswith('evidence/')]
assert changed==['tests/host/test_ctl_compile.py','tests/run_tests.sh'],changed
before=ast.parse(git('show',BEFORE+':tests/host/test_ctl_compile.py'));current=ast.parse((ROOT/'tests/host/test_ctl_compile.py').read_text())
resets=[]
for n in ast.walk(current):
 if isinstance(n,ast.Call) and isinstance(n.func,ast.Attribute) and n.func.attr=='Popen':
  reset=[k for k in n.keywords if k.arg=='preexec_fn']
  if reset:
   resets+=reset;n.keywords=[k for k in n.keywords if k.arg!='preexec_fn']
assert len(resets)==1
assert ast.dump(resets[0].value,include_attributes=False)==ast.dump(ast.parse('lambda: signal.signal(signal.SIGINT, signal.SIG_DFL)',mode='eval').body,include_attributes=False)
assert ast.dump(before,include_attributes=False)==ast.dump(current,include_attributes=False),'Fixture behavior/assertions changed beyond child SIGINT reset'
runner=(ROOT/'tests/run_tests.sh').read_text();accepted=git('show','fe7f040:tests/run_tests.sh').decode()
registrations={'quickcheck-plan':'python3 tests/host/test_quickcheck_plan.py','ctl-compile':'python3 tests/host/test_ctl_compile.py','host-setup':'python3 tests/host/test_host_setup.py','runner-admission':'python3 tests/host/test_runner_admission.py','dev-validate':'python3 tests/host/test_dev_validate.py DevelopmentValidation'}
stripped=runner
for name,command in registrations.items():
 line=f'run {name} {command}\n';assert stripped.count(line)==1,(name,line);stripped=stripped.replace(line,'')
assert stripped==accepted,'Accepted runner differs beyond five registrations'
assert not git('diff',BEFORE,AFTER,'--','ble/ctl','tools','host','firmware')
assert not git('diff','1165396',AFTER,'--','firmware')
# Bind the affected executed proof to this checked commit and read its outcomes.
report=ROOT/'evidence/build-speed-combination-stage2/signal-fixture-repair.json';r=json.loads(report.read_text())
assert report.read_bytes()==git('show',AFTER+':'+str(report.relative_to(ROOT)))
for name,want in r['logs_sha256'].items():
 p=report.parent/name;assert sha(p.read_bytes())==want;assert p.read_bytes()==git('show',AFTER+':'+str(p.relative_to(ROOT)))
assert 'all tests passed' in (report.parent/'registered-ctl-outer-final.log').read_text()
methods=(report.parent/'registered-ctl-outer-methods.log').read_text();assert 'Ran 7 tests' in methods and '\nOK\n' in methods
# Verify all earlier source/evidence assertions not affected by these two source edits.
prior=json.loads((OUT/'pre-outer-background-verification.json').read_text());checkpoint=json.loads((report.parent/'checkpoint.json').read_text())
for name,want in checkpoint['source_hashes'].items():
 path=ROOT/name
 if name not in changed:assert (sha(path.read_bytes()) if path.is_file() else None)==want,name
entries=[x for x in git('ls-tree','-r','-z',AFTER).split(b'\0') if x and not x.split(b'\t',1)[1].startswith(b'evidence/')]
result={'result':'PASS','candidate':AFTER,'superseded_candidate':BEFORE,'tree':git('rev-parse',AFTER+'^{tree}').decode().strip(),'source_tree_identity_sha256':sha(b'\0'.join(entries)+b'\0'),'source_entries':len(entries),'production_delta':changed,'assertion_preservation':'Whole controller fixture AST equals previous checked module after removing only the new preexec_fn keyword. All old-source negative controls and signal/cleanup/status assertions are unchanged.','signal_reset':'Exactly one tested controller-shell Popen resets SIGINT to SIG_DFL before exec. It does not reset the suite process or production compiler.','runner':'Exactly accepted fe7f040 runner plus five literal declarations; classification/lock/scheduler functions fully restored, ctl-compile defaults to parallelP with no installer lock.','affected_execution':'Root actual seven-method fake compiler suite PASS through general runner jobs4/classP under asynchronous Bash; exact committed logs/hash records verified.','reused':'All other native/source/generator/suite evidence from prior Test remains valid; old-source compiler negatives still fail before signal delivery at unchanged child-state assertions. No unrelated suite or build repeated.','logs_sha256':r['logs_sha256'],'limits':['New source changed fixture normalization, not production controller behavior.','Outer-background full seven-method execution is macOS host fake-compiler coverage; no new pinned-Linux suite or target build is claimed.','Prior source/timing/dependency/environment limitations remain. Cache source and consumer routing remain excluded.']}
(OUT/'affected-verification.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
