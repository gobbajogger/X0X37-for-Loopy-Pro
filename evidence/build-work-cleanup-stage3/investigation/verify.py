#!/usr/bin/env python3
"""Independently verify the cleanup-only source and retained evidence identities."""
import difflib
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
CANDIDATE = 'c4cec67f9c642fa6f925a06147e136b2b7231644'
BASE = '78c5296e95b9a18a514bbfd126ab04ebd86df9c9'
SCRIPT = 'ble/ctl/build_in_container.sh'
FIXTURE = 'tests/host/test_ctl_compile.py'
def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def git(*args): return subprocess.check_output(['git', *args], cwd=ROOT)
def read(p): return json.loads((ROOT / p).read_text())

assert git('rev-parse', 'HEAD').decode().strip() == CANDIDATE
changed = git('diff', '--name-only', BASE, CANDIDATE).decode().splitlines()
assert sorted(p for p in changed if not p.startswith('evidence/')) == sorted([SCRIPT, FIXTURE])
repair = ROOT / 'evidence/build-work-cleanup-repair'
reuse = read('evidence/build-work-cleanup-repair/reuse.json')
verified_logs = {}
for p in [SCRIPT, FIXTURE]:
    assert (ROOT / p).read_bytes() == git('show', CANDIDATE + ':' + p)
    assert sha(ROOT / p) == reuse['source_hashes'][p]
parent = git('show', BASE + ':' + SCRIPT).decode()
current = (ROOT / SCRIPT).read_text()
assert parent.split('join_jobs() {', 1)[0] == current.split('join_jobs() {', 1)[0]
assert parent.split('for f in src/*.c src/*.S gen/*.c; do', 1)[1] == current.split('for f in src/*.c src/*.S gen/*.c; do', 1)[1]
assert list(difflib.unified_diff(parent.splitlines(), current.splitlines(), lineterm='')) == reuse['parent_delta']
for platform in ['darwin', 'linux']:
    record = read('evidence/build-work-cleanup-repair/' + platform + '-checks.json')
    assert record['base'] == BASE
    for p, h in record['sources'].items(): assert sha(ROOT / p) == h
    for row in record['checks']:
        assert row['returncode'] == 0
        assert sha(ROOT / row['log']) == row['sha256']
        log = (ROOT / row['log']).read_text()
        if row['label'] == 'suite': assert 'Ran 7 tests' in log and '\nOK\n' in log
        else:
            assert 'FAILED (failures=10)' in log
            assert log.count('\nFAIL: ') == 10
            assert 'original source rejected in all ten' in log
        verified_logs[row['log']] = row['sha256']
for row in reuse['reused_checks']:
    assert row['returncode'] == 0 and sha(ROOT / row['log']) == row['sha256']
    verified_logs[row['log']] = row['sha256']
prior_test = read('evidence/build-work-stage3/test/report.json')
for row in prior_test['commands']:
    assert row['returncode'] == 0 and sha(ROOT / row['log']) == row['sha256']
    verified_logs[row['log']] = row['sha256']
for p, h in reuse['source_hashes'].items(): assert sha(ROOT / p) == h
controller = read('evidence/build-work-stage2/controller-result.json')
old_reuse = read('evidence/build-work-stage2/controller-reuse.json')
assert controller['returncode'] == 0 and controller['matches_saved_controller']
assert sha(ROOT / 'evidence/build-work-stage2/controller.log') == controller['log_sha256']
baseline_path = Path(controller['baseline_record'])
assert sha(baseline_path) == controller['baseline_record_sha256']
baseline = json.loads(baseline_path.read_text())
assert baseline['result'] == 'DEV CHECKED'
assert sha(baseline_path.parent / 'ctl.img') == controller['controller_sha256'] == baseline['artifacts']['controller']['sha256']
assert sha(baseline_path.parent / 'controller.log') == baseline['logs']['controller']['sha256']
assert sha('/Users/mac/Documents/x0x37/build_work_implementation/ble/ctl/out/ctl.img') == controller['controller_sha256']
assert sha(old_reuse['baseline_environment']) == old_reuse['baseline_environment_sha256']
for p, h in controller['sources'].items():
    assert sha(ROOT / p) == h
    assert hashlib.sha256(git('show', baseline['commit'] + ':' + p)).hexdigest() == h
built = (ROOT / 'evidence/build-work-stage2/controller-built-script.sh').read_text()
assert hashlib.sha256(built.encode()).hexdigest() == old_reuse['built_script_sha256']
assert list(difflib.unified_diff(built.splitlines(), current.splitlines(), lineterm='')) == reuse['exact_built_to_current_delta']
assert built.split('CF=', 1)[1].split('# Work in /tmp:', 1)[0] == current.split('CF=', 1)[1].split('# Work in /tmp:', 1)[0]
assert built.split('L=$B/lib\n', 1)[1] == current.split('L=$B/lib\n', 1)[1]
for p in reuse['protected_sources_unchanged']: assert not git('diff', BASE, CANDIDATE, '--', p)
result = dict(result='PASS', candidate=CANDIDATE, base=BASE,
              source_hashes={p:sha(ROOT / p) for p in [SCRIPT, FIXTURE]},
              changed_non_evidence=[p for p in changed if not p.startswith('evidence/')],
              checked_log_hashes=verified_logs, controller_image=controller['controller_sha256'],
              real_controller_seconds=controller['elapsed_seconds'],
              compiler_flags_byte_equal=True, link_post_link_byte_equal=True,
              exact_delta_verified=True, retained_artifacts_and_baseline_verified=True,
              sdk_toolchain_hashes_reused_not_rehashed=True)
(OUT / 'identity.json').write_text(json.dumps(result, indent=2) + '\n')
print('PASS: cleanup-only source, platform check logs, unaffected evidence, exact retained image and successful input bytes match.')
