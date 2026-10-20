#!/usr/bin/env python3
"""Verify retained evidence and exact cleanup candidate without repeating builds."""
import difflib
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
CANDIDATE = 'c4cec67f9c642fa6f925a06147e136b2b7231644'
BASE = '78c5296e95b9a18a514bbfd126ab04ebd86df9c9'
def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def git(*args): return subprocess.check_output(['git', *args], cwd=ROOT)
def read(path): return json.loads((ROOT / path).read_text())

reuse = read('evidence/build-work-cleanup-repair/reuse.json')
for path, digest in reuse['source_hashes'].items():
    assert sha(ROOT / path) == digest
    assert hashlib.sha256(git('show', CANDIDATE + ':' + path)).hexdigest() == digest
assert not git('diff', CANDIDATE, '--', *reuse['source_hashes'])
manifest = read('evidence/build-work-cleanup-repair/evidence-sha256.json')
for path, digest in manifest.items(): assert sha(ROOT / path) == digest, path
changed = git('diff', BASE, CANDIDATE, '--name-only').decode().splitlines()
source_delta = [p for p in changed if not p.startswith('evidence/')]
assert source_delta == ['ble/ctl/build_in_container.sh', 'tests/host/test_ctl_compile.py']
for env in reuse['new_controller_checks']:
    for path, digest in env['sources'].items(): assert sha(ROOT / path) == digest
    for check in env['checks']:
        assert check['returncode'] == 0 and sha(ROOT / check['log']) == check['sha256']
for check in reuse['reused_checks']:
    assert check['returncode'] == 0 and sha(ROOT / check['log']) == check['sha256']
for check in read('evidence/build-work-stage3/test/report.json')['commands']:
    assert check['returncode'] == 0 and sha(ROOT / check['log']) == check['sha256']
controller = read('evidence/build-work-stage2/controller-result.json')
assert controller['returncode'] == 0 and controller['matches_saved_controller']
assert sha(ROOT / 'evidence/build-work-stage2/controller.log') == controller['log_sha256']
record = Path(controller['baseline_record'])
assert sha(record) == controller['baseline_record_sha256']
baseline = json.loads(record.read_text())
assert baseline['result'] == 'DEV CHECKED'
assert sha(record.parent / 'ctl.img') == controller['controller_sha256'] == baseline['artifacts']['controller']['sha256']
assert sha(record.parent / 'controller.log') == baseline['logs']['controller']['sha256']
assert sha(ROOT.parent / 'build_work_implementation/ble/ctl/out/ctl.img') == controller['controller_sha256']
old_reuse = read('evidence/build-work-stage2/controller-reuse.json')
assert sha(old_reuse['baseline_environment']) == old_reuse['baseline_environment_sha256']
for path, digest in controller['sources'].items():
    assert sha(ROOT / path) == digest
    assert hashlib.sha256(git('show', baseline['commit'] + ':' + path)).hexdigest() == digest
current = (ROOT / 'ble/ctl/build_in_container.sh').read_text()
built = (ROOT / 'evidence/build-work-stage2/controller-built-script.sh').read_text()
parent = git('show', BASE + ':ble/ctl/build_in_container.sh').decode()
assert list(difflib.unified_diff(built.splitlines(), current.splitlines(), lineterm='')) == reuse['exact_built_to_current_delta']
assert list(difflib.unified_diff(parent.splitlines(), current.splitlines(), lineterm='')) == reuse['parent_delta']
assert built.split('CF=', 1)[1].split('# Work in /tmp:', 1)[0] == current.split('CF=', 1)[1].split('# Work in /tmp:', 1)[0]
assert built.split('L=$B/lib\n', 1)[1] == current.split('L=$B/lib\n', 1)[1]
assert not git('diff', BASE, CANDIDATE, '--', *reuse['protected_sources_unchanged'])
result = dict(result='PASS', candidate=CANDIDATE, base=BASE,
              source_hashes=reuse['source_hashes'], source_delta=source_delta,
              repair_evidence_hashes_verified=manifest,
              affected_platforms=['darwin-arm64', 'linux-amd64'],
              matching_controller_checks_reused=True, unaffected_suite_count=len(reuse['reused_checks']),
              prior_independent_quickcheck_logs_match=True,
              controller_image_sha256=controller['controller_sha256'],
              real_controller_seconds=controller['elapsed_seconds'],
              baseline_record_image_environment_sources_match=True,
              cleanup_delta_matches=True, flags_and_link_bytes_match=True,
              protected_sources_unchanged=reuse['protected_sources_unchanged'],
              limits=['No suite reruns or target builds in this identity check.',
                      'SDK/toolchain hashes use saved records; full dependencies were not rehashed.',
                      'Independent source/fixture reviews decide cleanup correctness.'])
(OUT / 'identity.json').write_text(json.dumps(result, indent=2) + '\n')
print('PASS: checked source, repair/Quickcheck logs, reused controller artifact and cleanup-only delta identities')
