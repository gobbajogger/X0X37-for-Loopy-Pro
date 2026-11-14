#!/usr/bin/env python3
"""Reconstruct an isolated V4 preview from exact V3 and reviewed text overlay.
No downloads, device actions, shared-ref changes or publication occur.
"""
import argparse, hashlib, json, pathlib, shutil
parser=argparse.ArgumentParser()
parser.add_argument('baseline',type=pathlib.Path)
parser.add_argument('overlay',type=pathlib.Path)
parser.add_argument('output',type=pathlib.Path)
args=parser.parse_args()
expected=json.loads((args.overlay.parent/'BASELINE.json').read_text())['files']
actual={p.relative_to(args.baseline).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in args.baseline.rglob('*') if p.is_file()}
assert actual==expected,'Baseline must match all 53 exact V3 files'
assert not args.output.exists(),'Choose a new isolated output directory'
shutil.copytree(args.baseline,args.output)
for p in args.overlay.iterdir():
 assert p.is_file() and p.name in {'index.html','app.js','experience.js','product.js','product.css','README.md','VERSION-HISTORY.md','MANIFEST.sha256'},'Unexpected overlay member'
 shutil.copyfile(p,args.output/p.name)
for line in (args.output/'MANIFEST.sha256').read_text().splitlines():
 digest,name=line.split(None,1)
 assert hashlib.sha256((args.output/name.strip()).read_bytes()).hexdigest()==digest,name
print('PASS: exact V3 plus reviewed V4 overlay; all resulting manifest hashes match')
