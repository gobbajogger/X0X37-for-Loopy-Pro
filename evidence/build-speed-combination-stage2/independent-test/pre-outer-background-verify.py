#!/usr/bin/env python3
"""Independent identity checks and a small new registration boundary probe."""
import hashlib,importlib.util,json,os,re,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];OUT=Path(__file__).resolve().parent
CANDIDATE='c0d2ab2f3248ce86a1e6421be926ce5297768272';BASE='1165396199210889fa460d26cbdd0b66183267e1'
def git(*args):return subprocess.check_output(['git',*args],cwd=ROOT)
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def read(path):return json.loads(path.read_text())
assert git('rev-parse','HEAD').decode().strip()==CANDIDATE
checkpoint=read(OUT.parent/'checkpoint.json')
for path,want in checkpoint['source_hashes'].items():
 f=ROOT/path
 assert (sha(f) if f.is_file() else None)==want,path
 assert (git('show',CANDIDATE+':'+path) if want is not None else b'')==(f.read_bytes() if f.is_file() else b''),path
assert not git('diff',BASE,CANDIDATE,'--','firmware')
# Owned source is committed. The two tracked rollout changes are excluded from this review's source snapshot.
assert set(git('diff','--name-only').decode().splitlines()) <= {'AGENTS.md','docs/SKILLS.md'}
controller=read(ROOT.parent/'build_work_cleanup_stage4/evidence/build-work-cleanup-stage4/identity.json')
for path,want in controller['source_hashes'].items():assert sha(ROOT/path)==want,path
proof=controller['successful_controller']
for path,want in proof['sources'].items():assert sha(ROOT/path)==want,path
assert controller['compiler_flags_equal'] and controller['link_post_link_equal'] and controller['exact_cleanup_delta_verified']
image=ROOT.parent/'smc_complete_stage4_20261021/evidence/complete-stage4/elite/ctl.img'
assert sha(image)==proof['controller_sha256']
assert sha(Path(proof['baseline_record']))==proof['baseline_record_sha256']
assert not (ROOT/'tools/asset_cache.py').exists()
for path in ['tools/build.py','tests/host/run_drum909.sh','tests/host/run_drum909_adpcm.sh']:
 assert (ROOT/path).read_bytes()==git('show',BASE+':'+path),path
for path in ['host/build_host.sh','tests/host/smk_clip_chord_display_test.sh']:
 assert [x for x in (ROOT/path).read_text().splitlines() if 'gen_' in x]==[x for x in git('show',BASE+':'+path).decode().splitlines() if 'gen_' in x]
# Match saved check output/hash assertions. Prior failed registration remains failure evidence only.
local=read(OUT.parent/'controller-quickcheck-identity.json')
for path,want in local['logs_sha256'].items():assert sha(OUT.parent/path)==want,path
for name,count in [('quickcheck-plan',12),('host-setup',7),('validation-inputs',13),('dev-validate',15)]:
 log=(OUT.parent/f'registered-{name}.log').read_text();assert f'Ran {count} tests' in log and '\nOK\n' in log,name
assert '13 cases passed' in (OUT.parent/'registered-runner-admission.log').read_text()
assert '  ok   ctl-compile' in (OUT.parent/'registered-ctl-final.log').read_text()
assert 'all tests passed' in (OUT.parent/'registered-native-fixtures.log').read_text()
assert 'Ran 1 test' in (OUT.parent/'native-worker.log').read_text() and '\nOK\n' in (OUT.parent/'native-worker.log').read_text()
assert read(OUT.parent/'native-cleanup.json')['result']=='PASS'
# All new direct paths are visible to the real Quickcheck parser, including the final native consumer registration.
spec=importlib.util.spec_from_file_location('plan',ROOT/'tools/quickcheck_plan.py');plan=importlib.util.module_from_spec(spec);spec.loader.exec_module(plan)
selections={}
for name,path in {'quickcheck-plan':'test_quickcheck_plan.py','ctl-compile':'test_ctl_compile.py','host-setup':'test_host_setup.py','runner-admission':'test_runner_admission.py','dev-validate':'test_dev_validate.py'}.items():
 selected=plan.select(ROOT,['tests/host/'+path],False);assert name in selected,(name,selected);selections[name]=selected
source=(ROOT/'tests/run_tests.sh').read_text()
start=source.index('tests_body() {');end=source.rindex('if [ "$JOBS" -le 1 ]; then\n    tests_body')
body='tests_body() {\nrun ctl-compile python3 signal-fixture.py\n}\n'
script=source[:start]+body+source[end:]
results=[]
for label,jobs,negative in [('serial',1,False),('parallel',4,False),('original-parallel-class',4,True)]:
 with tempfile.TemporaryDirectory(prefix='combined-registration-') as tmp:
  root=Path(tmp);(root/'tests').mkdir();(root/'bin').mkdir()
  active=script.replace('ctl-compile) TEST_CLASS=S','ctl-compile) TEST_CLASS=P') if negative else script
  (root/'tests/run_tests.sh').write_text(active)
  (root/'signal-fixture.py').write_text('import signal\nassert signal.getsignal(signal.SIGINT) != signal.SIG_IGN, "inherited ignored SIGINT"\nprint("SIGINT usable")\n')
  (root/'gate.lock').touch()
  (root/'bin/flock').write_text('#!/bin/sh\necho unexpected >> flock.log\nexit 97\n');(root/'bin/flock').chmod(0o755)
  env={k:v for k,v in os.environ.items() if not k.startswith(('X0X_','GIT_'))}
  env.update(PATH=str(root/'bin')+os.pathsep+os.environ['PATH'],X0X_TEST_JOBS=str(jobs),X0X_GATE_LOCK=str(root/'gate.lock'))
  p=subprocess.run(['sh','tests/run_tests.sh','ctl-compile'],cwd=root,env=env,text=True,capture_output=True,timeout=10)
  fixturelog=(root/'build/host/ctl-compile.log').read_text()
  assert not (root/'flock.log').exists(),label
  assert p.returncode==(1 if negative else 0),(label,p.stdout,p.stderr,fixturelog)
  assert ('inherited ignored SIGINT' in fixturelog) if negative else ('SIGINT usable' in fixturelog)
  results.append({'label':label,'jobs':jobs,'status':p.returncode,'expected_negative':negative,'installer_lock_invocations':0,'stdout':p.stdout,'fixture_log':fixturelog})
# Record all candidate non-evidence tracked blobs/modes, independent of foreign rollout working files.
entries=[x for x in git('ls-tree','-r','-z',CANDIDATE).split(b'\0') if x and not x.split(b'\t',1)[1].startswith(b'evidence/')]
result={'result':'PASS','candidate':CANDIDATE,'base':BASE,'source_manifest_entries':len(entries),'source_tree_identity_sha256':hashlib.sha256(b'\0'.join(entries)+b'\0').hexdigest(),'checkpoint_source_hashes':len(checkpoint['source_hashes']),'firmware_tree':git('rev-parse',CANDIDATE+':firmware').decode().strip(),'quickcheck_selection':selections,'registration_boundary_probes':results,'controller_image_sha256':sha(image),'recorded_evidence_reused':['Combined Quickcheck12, controller7, host7, runner13, native-input13 and native-DEV15 fixtures; final real worker1 fixture.','Contributor independent/acceptance source hashes, controller successful image/input/flags/link proof and accepted native package/walker/consumer scopes.'],'limits':['No behavior suites repeated; new scratch checks cover only newly registered controller foreground signal and installer-lock bypass.','Historical recorded environment/tool identity and current source/log hashes support reuse; no complete original inherited environment snapshot or installed-byte snapshot was recorded for all host fixture runs.','SDK/toolchain byte identities are retained evidence; no full dependency rehash or target build.','No target artifact equality, whole-build timing, gate, install, device or push claim. Cache and new cache consumer routing remain excluded.']}
(OUT/'verification.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
