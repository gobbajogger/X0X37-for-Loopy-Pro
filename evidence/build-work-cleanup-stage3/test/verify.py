#!/usr/bin/env python3
"""Independently verify immutable affected-check evidence without rerunning suites."""
import ast
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
CANDIDATE = 'c4cec67f9c642fa6f925a06147e136b2b7231644'
BASE = '78c5296e95b9a18a514bbfd126ab04ebd86df9c9'
ORIGINAL = '30c567d8588227d5285f04422130300fe83577a9'
def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def git(*args): return subprocess.check_output(['git', *args], cwd=ROOT)
def read(p): return json.loads((ROOT / p).read_text())
def check_log(row):
    assert row['returncode'] == 0, row
    assert sha(ROOT / row['log']) == row['sha256'], row

assert git('rev-parse', 'HEAD').decode().strip() == CANDIDATE
repair = 'evidence/build-work-cleanup-repair/'
reuse = read(repair + 'reuse.json')
manifest = read(repair + 'evidence-sha256.json')
for p, digest in manifest.items(): assert sha(ROOT / p) == digest, p
for p, digest in reuse['source_hashes'].items():
    assert sha(ROOT / p) == digest
    assert hashlib.sha256(git('show', CANDIDATE + ':' + p)).hexdigest() == digest
delta = git('diff', '--name-only', BASE, CANDIDATE).decode().splitlines()
assert {p for p in delta if not p.startswith('evidence/')} == set(reuse['changed_files'])
original_delta = git('diff', '--name-only', ORIGINAL, CANDIDATE).decode().splitlines()
assert {p for p in original_delta if not p.startswith('evidence/')} == set(reuse['changed_files'])
assert len(reuse['reused_checks']) == 6
for row in reuse['reused_checks']: check_log(row)
old_report = read('evidence/build-work-stage3/test/report.json')
for row in old_report['commands']: check_log(row)
old_identity = read(old_report['identity']['path'])
assert sha(ROOT / old_report['identity']['path']) == old_report['identity']['sha256']
for name in ('script_sha256', 'log_sha256'):
    suffix = 'py' if name == 'script_sha256' else 'log'
    assert sha(ROOT / ('evidence/build-work-stage3/test/adversarial.' + suffix)) == old_identity['adversarial'][name]
assert [c['result'] for c in old_identity['adversarial']['cases']] == ['PASS', 'PASS']

environments = []
for platform in ('darwin', 'linux'):
    env = read(repair + platform + '-checks.json')
    assert env in reuse['new_controller_checks']
    assert env['base'] == BASE
    for p, digest in env['sources'].items(): assert sha(ROOT / p) == digest
    if platform == 'linux':
        assert env['image'] == 'sha256:0cffe038efa0f50556de3fb444de6386729d495f63703c8c3b40fdfd0482d4b1'
        assert env['machine'] == 'x86_64'
    for row in env['checks']:
        check_log(row)
        log = (ROOT / row['log']).read_text()
        if row['label'] == 'suite':
            assert len(re.findall(r'^test_.* \.\.\. ok$', log, re.M)) == 7
            assert '\nRan 7 tests in ' in log and '\nOK\n' in log
        else:
            assert len(re.findall(r'^FAIL: ', log, re.M)) == 10
            assert len(re.findall(r'^AssertionError:.*active at scratch cleanup:', log, re.M)) == 4
            assert len(re.findall(r'^AssertionError:.*still active:', log, re.M)) == 6
            assert 'FAILED (failures=10)' in log
            assert 'PASS: original source rejected in all ten new failure/success/signal subcases' in log
            for jobs in (1, 2):
                for signal in ('SIGHUP: 1', 'SIGINT: 2', 'SIGTERM: 15'):
                    assert '(jobs=%s, signal=<Signals.%s>)' % (jobs, signal) in log
    environments.append(env)
initial = read(repair + 'initial-linux-checks.json')
assert initial['checks'][0]['sha256'] == environments[1]['checks'][0]['sha256']
assert initial['checks'][1]['returncode'] == 1
assert 'detected dubious ownership' in (ROOT / (repair + 'initial-linux-negative.log')).read_text()
runner = (ROOT / (repair + 'run-checks.py')).read_text()
assert "BASE = '" + BASE + "'" in runner
assert 'len(result.failures) == 10 and not result.errors' in runner
fixture = (ROOT / 'tests/host/test_ctl_compile.py').read_text()
tree = ast.parse(fixture)
methods = [n.name for n in ast.walk(tree) if isinstance(n, ast.FunctionDef) and n.name.startswith('test_')]
assert len(methods) == 7
script = (ROOT / 'ble/ctl/build_in_container.sh').read_text()
parent = git('show', BASE + ':ble/ctl/build_in_container.sh').decode()
assert script.split('join_jobs() {')[0] == parent.split('join_jobs() {')[0]
assert script.split('for f in src/*.c src/*.S gen/*.c; do')[1] == parent.split('for f in src/*.c src/*.S gen/*.c; do')[1]
controller = read('evidence/build-work-stage2/controller-result.json')
check_log(dict(returncode=controller['returncode'], log='evidence/build-work-stage2/controller.log', sha256=controller['log_sha256']))
assert controller['controller_sha256'] == reuse['controller_image_sha256']
result = dict(result='PASS', candidate=CANDIDATE, original_candidate=ORIGINAL, parent=BASE,
              source_hashes=reuse['source_hashes'], repair_evidence_hashes=manifest,
              six_unaffected_suites=reuse['reused_checks'], controller_checks=environments,
              preserved_independent_quickcheck_report=old_report['commands'],
              controller_result_sha256=sha(ROOT / 'evidence/build-work-stage2/controller-result.json'),
              checks={'source_and_exact_candidate_blobs': True, 'only_two_source_files_changed': True,
                      'all_other_tracked_source_identical_to_original': True,
                      'seven_methods_pass_both_platforms': True, 'ten_expected_parent_failures_each_platform': True,
                      'negative_failures_are_child_state_assertions': True,
                      'linux_success_suite_reused_after_failed_negative_read': True})
(OUT / 'identity.json').write_text(json.dumps(result, indent=2) + '\n')
print('PASS: exact candidate/source, saved checks/logs, seven controller methods and ten parent rejections per platform; six suites and independent Quickcheck evidence retained.')
