"""Package owned web source, accepted capture chain and scoped intake evidence."""
from pathlib import Path
import hashlib, io, json, tarfile

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT.parent / 'website-v4-corrections-20261114'
OUT = ROOT / 'delivery' / 'DnBOX-web-learning-reviewed-20261114.tar.xz'

def included(path, prefix):
    rel = path.relative_to(prefix)
    if not path.is_file() or '__pycache__' in rel.parts:
        return False
    if path.suffix in ('.fwsc', '.wasm') or path.name.startswith('engine-'):
        return False
    if prefix == BASE:
        if 'code-review' in rel.parts and 'assets' in rel.parts:
            return False
        if path.name == 'git-readback-content.json':
            return False
        if 'first-pass' in rel.parts and path.suffix in ('.wav', '.f32le'):
            return False
    else:
        if rel.parts[0] in ('intake', 'delivery', 'design-continuation', 'private-integration-intake'):
            return False
        if 'tools-intake' in rel.parts:
            # Preserve contracts/reports/identity receipts, not component implementation copies.
            if any(s.startswith('source-') for s in rel.parts):
                return False
            if path.name.startswith('candidate-'):
                return False
    return True

files = []
for prefix, name in [(BASE, 'accepted-website'), (ROOT, 'integrated-web-learning')]:
    for p in sorted(prefix.rglob('*')):
        if included(p, prefix):
            files.append((p, name + '/' + p.relative_to(prefix).as_posix()))
for name in ['package.py','prepare_git.py','PUBLISHER-HANDOFF.md','COMPONENT-FINDINGS.md','git-payload.json','git-preservation-receipt.json']:
    p = ROOT/'delivery'/name
    files.append((p,'integrated-web-learning/delivery/'+name))
assert len({name for _, name in files}) == len(files)
rows = [{'path': name, 'bytes': p.stat().st_size,
         'sha256': hashlib.sha256(p.read_bytes()).hexdigest()} for p, name in files]
manifest = json.dumps({'files': rows, 'exclusions': [
    'Firmware/SDK/WASM binaries, firmware source excerpts and unowned component implementation copies',
    'Downloaded skill scaffolding and cache files',
    'Diagnostic first-pass raw PCM/WAV duplicates; their JSON/screens remain',
    'Canonical tracker-history copies; intake reports retain exact source references'
]}, indent=2).encode() + b'\n'
with tarfile.open(OUT, 'w:xz', preset=8) as tar:
    for p, name in files:
        info = tar.gettarinfo(str(p), arcname=name)
        info.mtime = 0
        info.uid = info.gid = 0
        info.uname = info.gname = ''
        with p.open('rb') as f:
            tar.addfile(info, f)
    info = tarfile.TarInfo('PACKET-MANIFEST.json')
    info.size = len(manifest)
    info.mtime = 0
    tar.addfile(info, io.BytesIO(manifest))
with tarfile.open(OUT, 'r:xz') as tar:
    actual = {m.name for m in tar.getmembers() if m.isfile()}
    assert actual == {r['path'] for r in rows} | {'PACKET-MANIFEST.json'}
    for row in rows:
        raw = tar.extractfile(row['path']).read()
        assert len(raw) == row['bytes']
        assert hashlib.sha256(raw).hexdigest() == row['sha256'], row['path']
receipt = {'path': str(OUT), 'bytes': OUT.stat().st_size,
           'sha256': hashlib.sha256(OUT.read_bytes()).hexdigest(),
           'files': len(rows), 'every_archived_byte_verified': True,
           'base_manifest': 'a82e16b041a9be6cf290546f0336657bce5913e3afb5fefbcce843ce367d4a2f',
           'integrated_manifest': hashlib.sha256((ROOT/'site/MANIFEST.sha256').read_bytes()).hexdigest()}
(ROOT/'delivery/archive-receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
print(json.dumps(receipt))
