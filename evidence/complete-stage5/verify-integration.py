"""Check the merged source and the retained contribution evidence.

Run from the combined checkout. This check does not repeat the host suites.
The startup replay needs a new run because it includes both contributions.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess

ROOT = Path.cwd()
BASE = '1313e0fe384ef5df1f2f5f5ef699902c4543bc0d'
MAPPING = '4ff0cd468e8afa746deca5840afb7f2890eedb9c'
CENTRAL = '049d0274c80af543bd4e334af92b4393015c4f85'
MERGE = 'dc4fc5fc39a2a1f910173b1a550c2acc9a1786c9'

def git(*args):
    return subprocess.check_output(['git', '-C', str(ROOT), *args], text=True).strip()

def read(name):
    return json.loads((ROOT / name).read_text())

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def paths(rev):
    return set(git('diff', '--name-only', BASE, rev).splitlines())

mapping, central = paths(MAPPING), paths(CENTRAL)
assert not mapping & central
for rev, names in [(MAPPING, mapping), (CENTRAL, central)]:
    subprocess.run(['git', '-C', str(ROOT), 'merge-base', '--is-ancestor', rev, 'HEAD'], check=True)
    for name in names:
        assert git('rev-parse', rev + ':' + name) == git('rev-parse', MERGE + ':' + name), name
assert not git('diff', MERGE, 'HEAD', '--', 'firmware', 'tests', 'tools', 'ble/ctl', 'host')
assert not git('diff', '--', 'firmware', 'tests', 'tools', 'ble/ctl', 'host')

saved = 'evidence/stop-stage2/'
allowed = {
    'firmware/src/app/smc_mixer.c', 'tests/host/smc_buttons_test.py',
    'tests/host/smc_clip_visibility_test.c', 'tests/host/smc_clip_visibility_test.sh',
    'tests/host/smc_filter_test.py', 'tests/host/smc_mapping_test.py', 'tests/run_tests.sh',
}
manifests = {}
for name in ['source-inputs.json', 'helper-inputs.json', 'stop-inputs.json']:
    rows = read(saved + name)
    different = []
    for path, expected in rows.items():
        p = Path(path)
        if not p.is_absolute():
            p = ROOT / p
        if sha(p) != expected:
            different.append(path)
    assert set(different) == (allowed if name == 'source-inputs.json' else set()), different
    manifests[name] = {'count': len(rows), 'changed': different}

checks = read(saved + 'checks.json')
for name, row in checks['logs'].items():
    p = ROOT / saved / name
    assert sha(p) == row['sha256'], name
    assert not re.search(r'ERROR: AddressSanitizer|runtime error:', p.read_text()), name
commands = []
for name, count in [('host-checks.json', 56), ('stop-checks.json', 14), ('saved-probes.json', 7)]:
    rows = read(saved + name)
    assert len(rows) == count
    assert all(row['returncode'] == row['expected'] for row in rows)
    commands.extend(rows)
assert sum(row['expected'] != 0 for row in commands) == 30

for role in ['test', 'investigation']:
    row = read('evidence/stop-stage3/' + role + '/report.json')
    assert row['result'] == 'PASS' and not row['source_repair_required'] and not row['actionable_findings']
probe = ROOT / 'evidence/stop-stage3/investigation'
for name, expected in read('evidence/stop-stage3/investigation/probe-inputs.json').items():
    assert sha(ROOT / name) == expected, name
for row in read('evidence/stop-stage3/investigation/probe-commands.json'):
    assert row['returncode'] == row['expected'] == 0
    assert not re.search(r'ERROR: AddressSanitizer|runtime error:', (probe / row['log']).read_text())
assert '16 different-handle, stop-intent and restart cases: PASS' in (probe / 'probe.log').read_text()

for name, expected in read('evidence/mapping-stage4/source-identity.json')['inputs'].items():
    assert sha(ROOT / name) == expected, name
for row in read('evidence/complete-stage5/mapping-evidence-snapshot.json'):
    assert sha(ROOT / row['saved']) == row['sha256'], row['saved']
assert read('evidence/mapping-stage3/result.json')['result'] == 'STAGE3 PASS'

records = []
for folder, model in [('mapping-stage4', 'elite'), ('mapping-stage4', 'pro'), ('stop-stage4', 'elite')]:
    directory = ROOT / 'evidence' / folder / model
    row = json.loads((directory / 'record.json').read_text())
    assert row['result'] == 'DEV CHECKED'
    for name, log in row['logs'].items():
        assert log['returncode'] == 0
        assert sha(directory / (name + '.log')) == log['sha256']
    records.append({'record': str(directory.relative_to(ROOT) / 'record.json'),
                    'commit': row['commit'], 'validation_inputs': row['validation_inputs']['sha256'],
                    'budgets': row['budgets']})

print(json.dumps({'result': 'PASS', 'merge': MERGE, 'merge_tree': git('rev-parse', MERGE + '^{tree}'),
                  'mapping_paths': len(mapping), 'central_paths': len(central), 'overlap': [],
                  'source_manifests': manifests, 'saved_logs': len(checks['logs']),
                  'saved_commands': len(commands), 'expected_nonzero': 30,
                  'independent_central_cases': 16, 'separate_records': records,
                  'host_suites_repeated': False, 'combined_replay_required': True,
                  'combined_dev_validation_required': True}, indent=2))
