#!/usr/bin/env python3
"""Verify final committed tooling source and preserved firmware/generator identities."""
import ast,hashlib,json,re,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];E=Path(__file__).resolve().parent
BASE='1165396199210889fa460d26cbdd0b66183267e1';CANDIDATE='e7e63bdca31e238c07b724fcc271788c877918b8'
def git(*args):return subprocess.check_output(['git',*args],cwd=ROOT)
def raw(path,rev=CANDIDATE):return git('show',rev+':'+path)
def sha(data):return hashlib.sha256(data).hexdigest()
files=git('ls-tree','-r','--full-tree',CANDIDATE).decode().splitlines()
source={line.split('\t',1)[1]:line.split('\t',1)[0] for line in files if not line.split('\t',1)[1].startswith('evidence/')}
assert git('rev-parse',CANDIDATE+':firmware')==git('rev-parse',BASE+':firmware')
unchanged=['tools/build.py','tools/gen_font.py','tools/gen_drum_samples.py','tools/gen_break_bank.py','tools/gen_builtin_break.sh','tools/gen_builtin_break.c','tools/fm1_install.py','tools/require_gate_pass.sh','web/fm1pkg.js','web/fm1ota.js','web/x0x_breaks.js','web/x0x_installer.html','web/x0x_breaks.html','tools/upload_breaks.py','tools/sampleio.py']
assert all(raw(p)==raw(p,BASE) for p in unchanged)
assert 'tools/asset_cache.py' not in source and not any(p.startswith('web/emu/') for p in source)
pat=re.compile(r'web/emu|build/emu|emu[_-](?:ref|test|slots)|X0X_WEB|emscripten|\bemcc\b|EMSDK|EMCC_|EM_CONFIG|LLVM_ROOT|BINARYEN_ROOT|\.wasm|browser.{0,45}(?:emulat|version)|emulat.{0,45}browser',re.I)
findings=[];scanned=0
for p,entry in source.items():
 if p.startswith(('docs/audit/','docs/rules/')) or entry.startswith('120000'):continue
 try:text=raw(p).decode()
 except UnicodeError:continue
 scanned+=1
 for n,line in enumerate(text.splitlines(),1):
  if pat.search(line):findings.append({'path':p,'line':n,'text':line})
assert not findings,findings
accepted=json.loads((ROOT.parent/'host_setup_stage4/evidence/host-setup-stage4/result.json').read_text())['accepted_source_sha256']
assert all(sha(raw(p))==value for p,value in accepted.items())
quickfiles=git('diff','--name-only','07fa87d','c4cec67','--','.',':!evidence').decode().splitlines()
assert all(raw(p)==raw(p,'c4cec67') for p in quickfiles if p!='tests/host/test_ctl_compile.py')
fixture=ast.parse(raw('tests/host/test_ctl_compile.py')); removed=[]
for node in ast.walk(fixture):
 if isinstance(node,ast.Call):
  for keyword in list(node.keywords):
   if keyword.arg=='preexec_fn':
    removed.append(ast.dump(keyword.value,include_attributes=False));node.keywords.remove(keyword)
expected=ast.dump(ast.parse('lambda: signal.signal(signal.SIGINT, signal.SIG_DFL)',mode='eval').body,include_attributes=False)
assert removed==[expected]
assert ast.dump(fixture,include_attributes=False)==ast.dump(ast.parse(raw('tests/host/test_ctl_compile.py','c4cec67')),include_attributes=False)
for p in ['tests/host/smk_clip_chord_display_test.sh','tests/host/smk_clip_knob_test.sh','tests/host/test_runner_admission.py']:
 assert raw(p)==raw(p,'fe7f040')
rows=json.loads((ROOT.parent/'build_speed_consumer_audit/evidence/cache-consumer-audit/saved-header-equality.json').read_text())['rows']
outputs={}
for row in rows:
 value=sha((ROOT.parent/'build_speed_consumer_audit/build/cache-consumer-baseline'/row['name']).read_bytes());assert value==row['baseline_sha256']==row['saved_package_generation_sha256'];outputs[row['name']]=value
result={'candidate':git('rev-parse',CANDIDATE).decode().strip(),'base':BASE,'result':'PASS','firmware_tree':git('rev-parse',BASE+':firmware').decode().strip(),'source_manifest':source,'unchanged_generators_guards_and_web_tools':{p:sha(raw(p)) for p in unchanged},'all_host_accepted_source_hashes_match':True,'Quickcheck_controller_source':'Eight files exactly match acceptedc4cec67; fixture AST differs only by the verified child SIGINT reset. All assertions and production compiler source match.','three_runner_extraction_fixture_sources_match':True,'maintained_semantic_scan':{'files':scanned,'findings':findings,'excluded':'Historical evidence and docs/audit, retired docs/rules, and symlink pointers. No history rewritten.'},'retained_generated_outputs':outputs,'output_scope':'Existing saved original generation matches1165396 package-generation hashes; no new generation or target artifact validation claimed.','source_worktree_changes':git('diff','--name-only',CANDIDATE,'--','.',':!evidence').decode().splitlines(),'limits':['No target/controller/application build, full gate, install, device or external push.','No cache source/routing or repaired-cache speedup accepted.','Native target artifact equality and end-to-end build speed remain unmeasured; old9a1d6fc cannot prove combined1165396 artifacts.']}
(E/'final-identity.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='source_manifest'},indent=2))
