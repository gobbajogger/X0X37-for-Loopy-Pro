#!/usr/bin/env python3
"""Verify the exact candidate and reused logs without running target builds."""
import difflib
import hashlib
import json
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
S2 = ROOT / 'evidence/build-work-stage2'
CANDIDATE = '30c567d8588227d5285f04422130300fe83577a9'
def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def git(*args): return subprocess.check_output(['git', *args], cwd=ROOT)
assert git('rev-parse', 'HEAD').decode().strip() == CANDIDATE
sources = json.loads((S2 / 'source-files.json').read_text())
for p, h in sources.items():
    assert sha(ROOT / p) == h, p
    assert hashlib.sha256(git('show', CANDIDATE + ':' + p)).hexdigest() == h, p
checks = json.loads((S2 / 'checks.json').read_text())
assert len(checks) == 7
for c in checks:
    assert c['returncode'] == 0, c
    assert sha(ROOT / c['log']) == c['sha256'], c
controller = json.loads((S2 / 'controller-result.json').read_text())
reuse = json.loads((S2 / 'controller-reuse.json').read_text())
assert controller['returncode'] == 0 and controller['matches_saved_controller']
assert sha(S2 / 'controller.log') == controller['log_sha256']
record = Path(controller['baseline_record'])
assert sha(record) == controller['baseline_record_sha256']
baseline = json.loads(record.read_text())
assert baseline['result'] == 'DEV CHECKED'
assert sha(record.parent / 'ctl.img') == controller['controller_sha256'] == baseline['artifacts']['controller']['sha256']
assert sha(record.parent / 'controller.log') == baseline['logs']['controller']['sha256']
current_image = Path('/Users/mac/Documents/x0x37/build_work_implementation/ble/ctl/out/ctl.img')
assert sha(current_image) == controller['controller_sha256']
assert sha(reuse['baseline_environment']) == reuse['baseline_environment_sha256']
for p, h in controller['sources'].items():
    assert sha(ROOT / p) == h, p
    assert hashlib.sha256(git('show', baseline['commit'] + ':' + p)).hexdigest() == h, p
old = S2 / 'controller-built-script.sh'
new = ROOT / 'ble/ctl/build_in_container.sh'
assert sha(old) == reuse['built_script_sha256'] == controller['orchestration']['ble/ctl/build_in_container.sh']
assert sha(new) == reuse['current_script_sha256']
assert list(difflib.unified_diff(old.read_text().splitlines(), new.read_text().splitlines(), lineterm='')) == reuse['exact_delta']
assert sha(ROOT / 'ble/ctl/build_ctl.sh') == controller['orchestration']['ble/ctl/build_ctl.sh']
protected = ['tests/run_tests.sh', 'tools/build.py', 'tools/dev_validate.py', 'tools/validation_inputs.py', 'host/build_host.sh', 'BUILDING.md']
assert not git('diff', '07fa87dc950908d4d720bde9c06c2478cc212b19', CANDIDATE, '--', *protected)
assert not git('diff', CANDIDATE, '--', *sources)
result = dict(candidate=CANDIDATE, result='PASS', source_files=sources,
              reused_suites=checks, controller_sources_match=True,
              controller_image_sha256=controller['controller_sha256'],
              real_controller_seconds=controller['elapsed_seconds'],
              controller_log_matches=True, baseline_record_matches=True,
              baseline_environment_matches=True, current_image_matches=True,
              cleanup_delta_matches_exact_saved_diff=True,
              protected_sources_unchanged=protected,
              limits=['No target builds or suite reruns were performed by this identity check.',
                      'Toolchain and SDK identities are reused from the saved before/after build record; they were not rehashed by this check.',
                      'Successful controller-path equivalence also requires source review of the recorded cleanup-only delta.'])
(OUT / 'identity.json').write_text(json.dumps(result, indent=2) + '\n')
print('PASS: exact candidate, nine source hashes, seven suite logs, controller artifact and cleanup delta identities')
