#!/usr/bin/env python3
"""Record the disposable worker package-byte probe and its exact command."""
from pathlib import Path
import hashlib
import json
import subprocess
import time
root = Path(__file__).resolve().parents[2]
out = Path(__file__).resolve().parent
argv = ['docker', 'run', '--rm', '--platform', 'linux/amd64', '--mount',
        'type=bind,source=' + str(root) + ',target=/review,readonly', '--entrypoint', 'python3',
        'sha256:0cffe038efa0f50556de3fb444de6386729d495f63703c8c3b40fdfd0482d4b1',
        '-B', '/review/evidence/build-time-stage3/investigation-pillow-proof.py']
start = time.monotonic()
r = subprocess.run(argv, capture_output=True, text=True)
elapsed = time.monotonic() - start
log = out / 'investigation-pillow-proof.log'
log.write_text(r.stdout + r.stderr)
d = {'argv': argv, 'returncode': r.returncode, 'seconds': elapsed, 'log': log.name,
     'log_sha256': hashlib.sha256(log.read_bytes()).hexdigest(),
     'result': json.loads(r.stdout) if r.returncode == 0 else r.stderr}
(out / 'investigation-pillow-proof.json').write_text(json.dumps(d, indent=2) + '\n')
print(json.dumps({k: d[k] for k in ('returncode', 'seconds', 'log', 'log_sha256')}, indent=2))
assert r.returncode == 0, r.stderr
print(json.dumps({k: d['result'][k] for k in ('module_path', 'distribution_files_is_none',
    'distribution_files_count', 'module_before_sha256', 'module_after_sha256',
    'package_bytes_changed', 'runtime_identity_unchanged')}, indent=2))
