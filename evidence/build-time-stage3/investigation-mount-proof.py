#!/usr/bin/env python3
"""Focused identity probe. Uses only private scratch; no build or device state."""
from pathlib import Path
import json
import subprocess
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools'))
import validation_inputs as v
repo = Path(__file__).resolve().parents[2]
root = repo / 'build/investigation-mount-proof'
root.mkdir(parents=True, exist_ok=True)
for name in ('sdk', 'toolchain', 'extra'):
    (root / name).mkdir(exist_ok=True)
extra = root / 'extra/input.txt'
extra.write_text('first\n')
commit = subprocess.check_output(['git', '-C', str(repo), 'rev-parse', 'HEAD'], text=True).strip()
config = {'model': 'elite', 'config': 'dev', 'controller': False, 'dependencies': [],
          'plan': {'base': commit, 'scope': 'shell check consumes an additional mounted input',
                   'checks': [{'reason': 'external input', 'argv': ['sh', '-c', 'test -s "$PYTHONPATH/input.txt"']}],
                   'worker': {'image': 'sha256:fixed', 'mounts': {str(root / 'extra'): True},
                              'environment': {'PYTHONPATH': str(root / 'extra')}}}}
execution = {'dependencies': {}, 'environment': {}, 'platform': ['Linux', 'x86_64', 'fixture'],
             'tools': {}, 'python_packages': {}}
def capture():
    return v.capture(repo, commit, 'dev', config, root / 'toolchain', root / 'sdk', env={}, execution=execution)
a = capture()
first = v.content(root / 'extra')
extra.write_text('second\n')
b = capture()
second = v.content(root / 'extra')
assert first != second and a['sha256'] == b['sha256']
print(json.dumps({'before_identity': a['sha256'], 'after_identity': b['sha256'],
                  'before_extra_content': first, 'after_extra_content': second}, indent=2))
