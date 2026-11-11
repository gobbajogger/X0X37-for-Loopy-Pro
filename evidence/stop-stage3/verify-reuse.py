"""Verify saved stage2 inputs and artifacts without a repeated test or build."""
from pathlib import Path
import hashlib
import json
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
SAVED = ROOT / 'evidence/stop-stage2'

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

records = []
for name in ['source-inputs.json', 'helper-inputs.json', 'stop-inputs.json', 'replay-inputs.json']:
    inputs = json.loads((SAVED / name).read_text())
    failures = []
    for name_or_path, expected in inputs.items():
        path = Path(name_or_path)
        if not path.is_absolute():
            path = ROOT / path
        if not path.is_file() or sha(path) != expected:
            failures.append(str(path))
    records.append(dict(manifest=name, count=len(inputs), failures=failures))

checks = json.loads((SAVED / 'checks.json').read_text())
log_failures = []
for name, details in checks['logs'].items():
    if sha(SAVED / name) != details['sha256']:
        log_failures.append(name)
records.append(dict(manifest='checks.json logs', count=len(checks['logs']), failures=log_failures))

artifact_failures = []
artifacts = json.loads((SAVED / 'artifacts.json').read_text())
for name, details in artifacts.items():
    path = Path(details['path'])
    if not path.is_file() or path.stat().st_size != details['size'] or sha(path) != details['sha256']:
        artifact_failures.append(name)
records.append(dict(manifest='artifacts.json', count=len(artifacts), failures=artifact_failures))

environment = json.loads((SAVED / 'environment.json').read_text())
env_failures = []
for key in ['host_cc', 'target_cc']:
    if sha(Path(environment[key]['path'])) != environment[key]['sha256']:
        env_failures.append(key)
sdk = environment['sdk']
sdk_commit = subprocess.check_output(['git', '-C', sdk['path'], 'rev-parse', 'HEAD'], text=True).strip()
sdk_status = subprocess.check_output(['git', '-C', sdk['path'], 'status', '--porcelain'], text=True).strip()
if sdk_commit != sdk['commit'] or sdk_status != sdk['status']:
    env_failures.append('sdk')
image_id = subprocess.check_output(['docker', 'image', 'inspect', 'x0x37-validation:local', '--format', '{{.Id}}'], text=True).strip()
if image_id != environment['image_id']:
    env_failures.append('image')
records.append(dict(manifest='environment.json', count=4, failures=env_failures))

diff = subprocess.check_output(['git', 'diff', 'cce6ac5ccd45ebb2809afd973671af96b138748a', '--', 'firmware', 'tests', 'tools', 'ble/ctl', 'host'], cwd=ROOT, text=True)
records.append(dict(manifest='candidate checked paths', count=1, failures=['diff'] if diff else []))
result = dict(result='PASS' if all(not r['failures'] for r in records) else 'FAIL',
              candidate='cce6ac5ccd45ebb2809afd973671af96b138748a', records=records,
              command='python3 -B evidence/stop-stage3/verify-reuse.py',
              test_rerun=False, target_build_rerun=False,
              scope='The saved inputs, logs, artifacts and available environment match. Stage4 validation remains required.')
(OUT / 'reuse.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result, indent=2))
assert result['result'] == 'PASS'
