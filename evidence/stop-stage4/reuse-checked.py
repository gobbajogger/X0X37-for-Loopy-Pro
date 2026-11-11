"""Check retained host evidence against the complete stage3 candidate.

This check reads saved results. It does not repeat the host suites.
The host compiler and build environment were checked on the producer host.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess

ROOT = Path.cwd()
SAVED = ROOT / 'evidence/stop-stage2'
STAGE3 = ROOT / 'evidence/stop-stage3'
OUT = Path(__file__).resolve().parent

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def read(path):
    return json.loads(path.read_text())

summary = {}
for name in ['source-inputs.json', 'helper-inputs.json', 'stop-inputs.json', 'replay-inputs.json']:
    manifest = read(SAVED / name)
    for relative, expected in manifest.items():
        path = Path(relative)
        if not path.is_absolute():
            path = ROOT / path
        assert sha(path) == expected, str(path)
    summary[name] = len(manifest)

checks = read(SAVED / 'checks.json')
for name, record in checks['logs'].items():
    path = SAVED / name
    assert sha(path) == record['sha256'], name
    assert not re.search(r'ERROR: AddressSanitizer|runtime error:', path.read_text()), name
summary['logs'] = len(checks['logs'])
nonzero = 0
for name, count in [('host-checks.json', 56), ('stop-checks.json', 14), ('saved-probes.json', 7)]:
    records = read(SAVED / name)
    assert len(records) == count
    for record in records:
        assert record['returncode'] == record['expected'], record
        nonzero += record['expected'] != 0
    summary[name] = count
assert nonzero == 30
summary['expected_nonzero'] = nonzero

for role in ['test', 'investigation']:
    report = read(STAGE3 / role / 'report.json')
    assert report['result'] == 'PASS' and not report['source_repair_required']
    assert not report['actionable_findings']
    assert report['candidate'] == 'cce6ac5ccd45ebb2809afd973671af96b138748a'
probe = STAGE3 / 'investigation'
for relative, expected in read(probe / 'probe-inputs.json').items():
    assert sha(ROOT / relative) == expected, relative
for command in read(probe / 'probe-commands.json'):
    assert command['returncode'] == command['expected'] == 0
    log = (probe / command['log']).read_text()
    assert not re.search(r'ERROR: AddressSanitizer|runtime error:', log)
assert '16 different-handle, stop-intent and restart cases: PASS' in (probe / 'probe.log').read_text()
summary['independent_probe_cases'] = 16

host = read(OUT / 'reuse-host.json')
assert host['result'] == 'PASS' and all(not item['failures'] for item in host['records'])
diff = subprocess.check_output(['git', 'diff', 'cce6ac5ccd45ebb2809afd973671af96b138748a', '--',
                                'firmware', 'tests', 'tools', 'ble/ctl', 'host'], text=True)
assert not diff, diff
summary.update(result='PASS', host_suites_repeated=False, replay_repeated=False)
print(json.dumps(summary, indent=2))
