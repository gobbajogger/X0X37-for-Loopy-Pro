#!/usr/bin/env python3
"""Verify committed stage2 records against their fixture assertions without running tests."""
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[3]
STAGE2 = ROOT / 'evidence/runner-speed-stage2'
OUT = Path(__file__).resolve().parent
assert subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip() == 'fe7f0401e79cd5dd4a76307811d2ee0d12446263'
spec = importlib.util.spec_from_file_location('admission', ROOT / 'tests/host/test_runner_admission.py')
fixture = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fixture)
rows = json.loads((STAGE2 / 'runner.json').read_text())
assert len(rows) == 17
verified = []
for index, saved in enumerate(rows):
    row = dict(saved)
    row['trace'] = (STAGE2 / f'runner-{index:02d}.log').read_text()
    if row['version'] == 'candidate':
        fixture.check(row, row['selected'], fail=row['fail'], setup=row['setup'])
    else:
        assert row['rc'] == 0
    verified.append({'index': index, 'version': row['version'], 'jobs': row['jobs'],
                     'selected': row['selected'], 'fail': row['fail'], 'setup': row['setup'],
                     'holds': row['holds'], 'result': 'PASS',
                     'log': f'../../runner-speed-stage2/runner-{index:02d}.log'})
probes = json.loads((STAGE2 / 'probes.json').read_text())
by_label = {row['label']: row for row in probes}
expected = [('baseline-knob', 0), ('baseline-chord_display', 0),
            ('candidate-knob', 0), ('candidate-chord_display', 0),
            ('source-override', 0), ('unknown-argument', 2),
            ('extra-argument', 2), ('override-sentinel', 0)]
incs = ['apc_a', 'apc_b', 'ck_a', 'ck_b', 'autosave', 'padled']
expected += [('missing-' + name, 1) for name in incs]
for label, rc in expected:
    assert by_label[label]['rc'] == rc
    assert (STAGE2 / (label + '.log')).is_file()
for version, count in [('baseline', 3), ('candidate', 2)]:
    row = by_label[version + '-counts']
    assert row['compile'] == row['executions'] == count
    assert sum(line.startswith('[') for line in row['details']) == count
    assert sum(line.startswith('run ') for line in row['details']) == count
    assert 'PASS 0 fail' in (STAGE2 / (version + '-knob.log')).read_text()
    assert '7049 checks, 7050 captured chord draws passed' in (STAGE2 / (version + '-chord_display.log')).read_text()
assert set(by_label['extraction-sha256']['files']) == set(incs)
for name in incs:
    assert f'marker for {name}.inc not found' in (STAGE2 / ('missing-' + name + '.log')).read_text()
gen = (STAGE2 / 'gen-order-linux.log').read_text()
assert 'ok   jobs=8: run_tests.sh passed' in gen
assert 'ok   jobs=1: run_tests.sh passed' in gen
assert 'gen order: all passed' in gen
result = {'status': 'PASS', 'candidate_assertions_rechecked': 13,
          'baseline_success_records': 4, 'runner_cases': verified,
          'probe_summary_records_verified': len(probes),
          'evidence_sha256': {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                              for path in sorted(STAGE2.iterdir()) if path.is_file()}}
(OUT / 'saved-assertions.json').write_text(json.dumps(result, indent=2) + '\n')
print('PASS: 13 candidate records, 4 baseline records, 17 probe records and final Linux generation-order log verified.')
