#!/usr/bin/env python3
"""Independent complete-candidate source identity and contract audit."""
import ast,hashlib,json,re,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];E=Path(__file__).resolve().parent
BASE='1165396199210889fa460d26cbdd0b66183267e1';CANDIDATE=sys.argv[1] if len(sys.argv)>1 else 'c0d2ab2f3248ce86a1e6421be926ce5297768272'
def git(*a):return subprocess.check_output(['git',*a],cwd=ROOT)
def blob(c,p):return git('show',c+':'+p)
def file(p):return (ROOT/p).read_bytes()
def sha(b):return hashlib.sha256(b).hexdigest()
assert git('rev-parse','HEAD').decode().strip()==CANDIDATE
assert not git('diff','f53309c','c0d2ab2')
assert {p for p in git('diff','--name-only','c0d2ab2',CANDIDATE).decode().splitlines() if not p.startswith('evidence/')} == {'tests/host/test_ctl_compile.py','tests/run_tests.sh'}
assert not git('diff',BASE,CANDIDATE,'--','firmware','assets')
unchanged=['tools/build.py','tools/fm1_install.py','tools/require_gate_pass.sh','tools/fm1pkg_make.py',
 'web/fm1pkg.js','web/fm1ota.js','web/x0x_breaks.js','web/x0x_installer.html','web/x0x_breaks.html',
 'web/test_breaks.mjs','web/test_breaks_page.mjs','web/test_install_slots.mjs','tools/upload_breaks.py',
 'tests/host/run_drum909.sh','tests/host/run_drum909_adpcm.sh']
assert all(file(p)==blob(BASE,p) for p in unchanged)
assert not (ROOT/'tools/asset_cache.py').exists()
contributor={}
for c,paths in {
 '46a3b3e':['host/build_host.sh','tests/host/smc_drum_mutes_test.sh','tests/host/smc_performance_abort_test.sh','tests/host/smc_reachability_test.sh','tests/host/test_host_setup.py','tests/host/ui_feedback_test.sh'],
 'fe7f040':['tests/host/smk_clip_knob_test.sh','tests/host/smk_clip_chord_display_test.sh','tests/host/test_runner_admission.py'],
 'c4cec67':['ble/ctl/build_ctl.sh','ble/ctl/build_in_container.sh','docs/QUICKCHECK-SCOPE.md','tests/host/test_portable_rules.py','tests/host/test_quickcheck_plan.py','tools/install_gate.sh','tools/quickcheck.sh','tools/quickcheck_plan.py'],
 'cadeae0':['tests/host/test_dev_validate.py']}.items():
 for p in paths:assert file(p)==blob(c,p),(c,p)
 contributor[c]=paths
oldfixture=blob('c4cec67','tests/host/test_ctl_compile.py').decode()
expectedfixture=oldfixture.replace("                p = subprocess.Popen(['sh', str(TOP / 'ble/ctl/build_in_container.sh')],\n                                     env=self.env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)\n                try:", "                # An outer asynchronous shell can inherit ignored SIGINT. The\n                # tested child needs the normal disposition before its shell exec.\n                p = subprocess.Popen(['sh', str(TOP / 'ble/ctl/build_in_container.sh')],\n                                     env=self.env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,\n                                     preexec_fn=lambda: signal.signal(signal.SIGINT, signal.SIG_DFL))\n                try:")
assert file('tests/host/test_ctl_compile.py').decode()==expectedfixture
runner=file('tests/run_tests.sh').decode();normal=runner
for n,p in [('quickcheck-plan','test_quickcheck_plan.py'),('ctl-compile','test_ctl_compile.py'),('host-setup','test_host_setup.py'),('runner-admission','test_runner_admission.py')]:
 line=f'run {n} python3 tests/host/{p}\n';assert normal.count(line)==1;normal=normal.replace(line,'')
normal=normal.replace('run dev-validate python3 tests/host/test_dev_validate.py DevelopmentValidation\n','')
normal=normal.replace('    ctl-compile) TEST_CLASS=S ;;   # signal fixtures need foreground SIGINT disposition\n','')
normal=normal.replace('[ "$1" != rec-live ] && [ "$1" != ctl-compile ]','[ "$1" != rec-live ]')
assert normal.encode()==blob('fe7f040','tests/run_tests.sh')
for p in ['tools/validation_inputs.py','tools/dev_validate.py']:
 old=blob('cadeae0',p).decode();new=file(p).decode()
 if p.endswith('validation_inputs.py'):
  expected=old.replace("'make', 'node', 'emcc', 'cc'","'make', 'node', 'cc'")
  a=expected.index("    emcc = shutil.which('emcc'");b=expected.index('    # Include installed Python dependency contents',a);expected=expected[:a]+expected[b:]
  expected=expected.replace("('X0X_DEV_', 'FELUCCA_', 'EMCC_')","('X0X_DEV_', 'FELUCCA_')").replace("'X0X_VERSION', 'EMCC_CFLAGS', 'EM_CONFIG', 'LLVM_ROOT', 'BINARYEN_ROOT',","'X0X_VERSION',")
 else:
  expected=old.replace("'X0X_VERSION', 'EMSDK', 'EM_CONFIG', 'LLVM_ROOT', 'BINARYEN_ROOT',","'X0X_VERSION',").replace("if k in keys or k.startswith('EMCC_')","if k in keys")
 assert new==expected,p
old=blob(BASE,'ble/ctl/build_in_container.sh').decode();new=file('ble/ctl/build_in_container.sh').decode()
assert old[old.index('CF="'):old.index('# Work in /tmp')]==new[new.index('CF="'):new.index('# Work in /tmp')]
assert old[old.index('L=$B/lib'):]==new[new.index('L=$B/lib'):]
# Check literal declarations retained and retired app unavailable.
oldnames=set(re.findall(r'(?:^|[\s;&|])run "?([A-Za-z0-9_.-]+)"?\s+',blob(BASE,'tests/run_tests.sh').decode().replace('\\\n',' ')))
newnames=set(re.findall(r'(?:^|[\s;&|])run "?([A-Za-z0-9_.-]+)"?\s+',runner.replace('\\\n',' ')))
assert newnames==(oldnames-{'emu','emu-slots'})|{'quickcheck-plan','ctl-compile','host-setup','runner-admission','dev-validate'}
assert not (ROOT/'web/emu').exists()
assert not (ROOT/'tests/host/emu_test.mjs').exists()
changed=git('diff','--name-only',BASE,CANDIDATE).decode().splitlines();prod=[p for p in changed if not p.startswith('evidence/')]
manifest={p:sha(file(p)) if (ROOT/p).is_file() else None for p in prod}
result={'candidate':CANDIDATE,'candidate_tree':git('rev-parse',CANDIDATE+'^{tree}').decode().strip(),'result':'PASS independent source assertions','production_paths':len(prod),'production_sha256':manifest,'firmware_tree':git('rev-parse',CANDIDATE+':firmware').decode().strip(),'preserved_base_files':unchanged,'exact_contributor_files':contributor,'contracts':['All five registration additions are literal and present; no prior maintained declaration lost.','Combined runner differs from accepted scheduler only by five literal fixture declarations; signal behavior is normalized only within the controller fixture child launch.','Native runtime/worker difference from accepted source is exactly the retired compiler/backend/environment removal.','Compiler flags and complete link/post-link section match original serial controller source; fixture change is exactly one subprocess SIGINT default reset/comment, with all assertions preserved.','Firmware/assets and installer/package/uploader/reference sources remain original; failed cache absent.'],'metadata_repair':'Original f53309c reproduced RULES FAIL due missing Trigger/Check. Amended c0d2ab2 has the exact same Git tree.'}
(E/'source-audit.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['production_sha256','exact_contributor_files','preserved_base_files']},indent=2))
