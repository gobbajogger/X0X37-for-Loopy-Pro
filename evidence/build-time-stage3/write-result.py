#!/usr/bin/env python3
"""Save the failed stage3 disposition without changing candidate source."""
from pathlib import Path
import hashlib,json,subprocess
root=Path(__file__).resolve().parents[2]
out=Path(__file__).resolve().parent
candidate='6d690e347b2c66d331f0b6da58a390b627ee84b7'
inputs=json.loads((out/'inputs.json').read_text())
for name,expected in inputs['source_files'].items():
    assert hashlib.sha256((root/name).read_bytes()).hexdigest()==expected,name
proof=json.loads((out/'root-pillow-proof.json').read_text())
assert proof['exit_code']==0
assert proof['result']['package_bytes_changed'] and proof['result']['runtime_identity_unchanged']
font=json.loads((out/'font-check.json').read_text())
assert font['equal']
result={'workflow':'build-time-20261021','stage':3,'status':'FAILED: MATERIAL PROVENANCE DEFECT; REPAIR REQUIRED',
 'source_chat':'01a15ad7-8be3-7301-aabe-e4533790c4a2','candidate':candidate,
 'branch':subprocess.check_output(['git','branch','--show-current'],cwd=root,text=True).strip(),
 'source_unchanged':True,'input_identities':'inputs.json','reused_checks':'reuse-verification.json',
 'independent_scopes':{'test':'test-result.json','investigation':'investigation-result.json'},
 'findings':[
  {'id':'I2','severity':'material','title':'Installed Python distribution without a file manifest hashes no package bytes',
   'evidence':['root-pillow-proof.json','root-pillow-proof.log','investigation-pillow-proof.py','font-check.json'],
   'observed':'Pinned Linux image Pillow9.4.0 has dist.files=None. Changing installed PIL/__init__.py changes its bytes while complete before/after runtime snapshots stay equal. Pillow fingerprint is SHA256 of an empty JSON dictionary.',
   'origin':'The algorithm predates the stage2 refactor. It is still material to this required actual worker package-byte coverage.',
   'disposition':'Do not advance to stage4. Launch one fresh stage2 to hash installed package bytes when metadata manifests are missing/empty and fail closed when coverage cannot be established. Then require fresh independent stage3 for affected checks.'},
  {'id':'I1','severity':'documented coverage limit','title':'Additional worker mounts require explicit dependency declarations',
   'evidence':['investigation-mount-proof.json','investigation-mount-proof.py','BUILDING.md:223'],
   'disposition':'Not a demonstrated defect for a conforming plan. Every consumed additional worker mount must appear in plan.dependencies. No claim of automatic mounted-byte coverage.'}],
 'confirmed':['Six stage2 source hashes and five retained log hashes match; retained commands returned zero.','Record verifier AST and installer/gate-guard source are unchanged from audit.','Scoped real LinuxWorker font generation matches audit Linux header589ca6d26e0520b1c5d7653e516664b4e72373b66434727a195a6c904ea55d44.','Pinned image remains sha256:0cffe038efa0f50556de3fb444de6386729d495f63703c8c3b40fdfd0482d4b1 Linux/amd64.'],
 'limits':['Supplemental test execution was awaiting tool approval and then aborted; pending scopes are not passing checks. See test-result.json.','No real firmware or controller target build, end-to-end speed measurement, stage4 artifact equality, gate, device test, install, merge or external push occurred.','Immutable image ID covers initial image bytes, but cannot replace before/after capture of installed package bytes in a running writable worker.','Full identities include per-run scratch paths and HOSTNAME. Future baseline matching must distinguish recorded bookkeeping from real build inputs.'],
 'next_stage':2,'next_owner':'Exactly one fresh package-manifest repair stage2.',
 'handoff':'repair-stage2-prompt.txt','authorization':inputs['authorization'],
 'latest_owner_guidance':{'source_chat':'01a15a99-75d7-7921-9d65-f4b120566904','instruction':'Use matching saved baselines and artifacts. Do not run a second complete old Linux validation solely to recreate timings. Keep provenance and native runner/record/installer correctness.'}}
(out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
successor={'workflow':result['workflow'],'next_stage':2,'marker':'build-time-20261021-stage2-python-manifest-repair','status':'pending','source_chat':result['source_chat'],'checked_candidate':candidate,'prompt':str(out/'repair-stage2-prompt.txt'),'attempts':[]}
p=out/'successor.json'
if not p.exists():p.write_text(json.dumps(successor,indent=2)+'\n')
print(result['status'])
