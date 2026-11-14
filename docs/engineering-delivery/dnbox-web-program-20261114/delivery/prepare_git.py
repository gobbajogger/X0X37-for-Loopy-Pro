"""List only owned website changes and source-referenced integration evidence."""
from pathlib import Path
import hashlib,json
ROOT=Path(__file__).resolve().parents[1]
PREFIX='docs/engineering-delivery/dnbox-web-program-20261114'
names=['WORKSPACE-ACCEPTANCE.md','README.md','integrated-candidate.json','integrated-all-files.sha256',
       'source-delta-check.json','learning-intake/REPORT.md',
       'learning-intake/MANIFEST.json','learning-intake/producer-source-consumption.json',
       'learning-intake/manual-images-receipts.json','learning-intake/public-links/REPORT.md',
       'learning-intake/public-links/touchosc-check.md',
       'tools-intake/INTAKE.md','tools-intake/INTEGRATION-COPY.md',
       'tools-intake/manifest.sha256','tools-intake/entry-url-receipt.json',
       'tools-intake/editor/REPORT.md','tools-intake/editor/receipts/byte-identity-check.json',
       'delivery/package.py','delivery/prepare_git.py','delivery/PUBLISHER-HANDOFF.md','delivery/COMPONENT-FINDINGS.md']
names += ['site/'+n for n in ['MANIFEST.sha256','app.js','product.js','journeys.js',
                            'provenance/integration-20261114.md']]
review=ROOT/'integration-review'
if review.exists():
    names += [p.relative_to(ROOT).as_posix() for p in sorted(review.rglob('*'))
              if p.is_file() and p.name!='source-capabilities.md' and p.suffix in ('.md','.json','.py','.cjs','.sha256')]
names += [p.relative_to(ROOT).as_posix() for p in sorted((ROOT/'history').rglob('*')) if p.is_file()]
names += ['learning-intake/manual-successor-4998572/receipt.json']
launcher=ROOT/'workspace-source'
if launcher.exists():
    names += [p.relative_to(ROOT).as_posix() for p in sorted(launcher.rglob('*')) if p.is_file() and '__pycache__' not in p.parts]
rows=[]
for name in names:
    p=ROOT/name
    raw=p.read_bytes(); raw.decode('utf-8')
    rows.append(dict(local_path=str(p),path=PREFIX+'/'+name,
                     sha256=hashlib.sha256(raw).hexdigest(),
                     git_blob=hashlib.sha1(b'blob '+str(len(raw)).encode()+b'\0'+raw).hexdigest(),
                     bytes=len(raw)))
(ROOT/'delivery/git-payload.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps({'files':len(rows),'bytes':sum(r['bytes'] for r in rows)}))
