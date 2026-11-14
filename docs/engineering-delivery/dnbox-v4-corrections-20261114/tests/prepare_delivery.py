"""Prepare the explicitly scoped public text projection and full Library packet."""
from pathlib import Path
import hashlib,json,shutil,zipfile
ROOT=Path(__file__).resolve().parents[1]
BASE=Path('/workspace/website-v4-inputs/DnBOX-v4-product-and-evidence-20261114/DnBOX-v4/site')
GIT_ROOT='docs/engineering-delivery/dnbox-v4-corrections-20261114'
def sha(b):return hashlib.sha256(b).hexdigest()
projection=[]
for p in sorted((ROOT/'site').rglob('*')):
 if not p.is_file():continue
 rel=p.relative_to(ROOT/'site');old=BASE/rel
 if (not old.exists() or p.read_bytes()!=old.read_bytes()) and p.suffix!='.wav':
  projection.append((p,'source-overlay/'+rel.as_posix()))
for n in ['README.md','REPORT.md','PLAN.md','CAPABILITY-OWNERSHIP.md']:
 projection.append((ROOT/n,n))
for p in sorted((ROOT/'tests').glob('*.py')):projection.append((p,'tests/'+p.name))
for n in ['INTAKE.md','FINAL-DISPOSITIONS.md','capture-qualification.json','current-status-receipts.json','runtime-manifest-check.json','final-candidate.json','final-all-files.sha256','delta.json','browser-trust-probe.json','after-navigation.json','history-edge-before.json','nav-fix-history-edge-after.json','nav-fix-focus-state-after.json','candidate-audio-checks.json','simplification-disposition.md']:
 projection.append((ROOT/'evidence'/n,'evidence/'+n))
for n in ['ASSESSMENT.md','CORRECTED-REQUALIFICATION.md','recordings-analysis.json']:
 projection.append((ROOT/'blind-review'/n,'blind-review/'+n))
for n in ['review.json','requalification.json','requalification.md','validator.json']:
 projection.append((ROOT/'code-review'/n,'code-review/'+n))
for d in ['recordings','recordings-extra']:
 for n in ['summary.json','all-events.json']:
  projection.append((ROOT/'evidence'/d/n,'evidence/'+d+'/'+n))
for n in ['ASSESSMENT.md']:
 projection.append((ROOT/'final-fresh-review'/'postfix'/n,'final-fresh-review/postfix/'+n))
for n in ['REPORT.md','qualification.json','manifest.sha256']:
 projection.append((ROOT/'evidence'/'pattern-continuity'/n,'evidence/pattern-continuity/'+n))
for n in ['midi-profile-source-receipts.json','capability-delivery-intake.json']:
 projection.append((ROOT/'evidence'/n,'evidence/'+n))
items=[]
for p,rel in projection:
 raw=p.read_bytes();raw.decode('utf-8')
 items.append(dict(local_path=str(p),path=GIT_ROOT+'/'+rel,mode='100644',sha256=sha(raw),git_blob=hashlib.sha1(b'blob '+str(len(raw)).encode()+b'\0'+raw).hexdigest(),bytes=len(raw)))
(ROOT/'evidence/git-payload-manifest.json').write_text(json.dumps(items,indent=2))
print(json.dumps({'text_files':len(items),'bytes':sum(i['bytes'] for i in items),'paths':[i['path'] for i in items]},indent=2))
