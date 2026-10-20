#!/usr/bin/env python3
"""Extend existing source and real controller evidence for the bounded cleanup-only delta."""
import difflib
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
BASE = '78c5296e95b9a18a514bbfd126ab04ebd86df9c9'
CANDIDATE = '30c567d8588227d5285f04422130300fe83577a9'
def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def git(*args): return subprocess.check_output(['git', *args], cwd=ROOT)
manifest = json.loads((ROOT / 'evidence/build-work-stage2/source-files.json').read_text())
changed = ['ble/ctl/build_in_container.sh', 'tests/host/test_ctl_compile.py']
for p, h in manifest.items():
    assert hashlib.sha256(git('show', CANDIDATE + ':' + p)).hexdigest() == h
    assert git('show', BASE + ':' + p) == git('show', CANDIDATE + ':' + p)
    if p not in changed: assert sha(ROOT / p) == h, p
source_delta = [p for p in git('diff', BASE, '--name-only').decode().splitlines()
                if not p.startswith('evidence/build-work-cleanup-repair/')]
assert set(source_delta) <= set(changed), source_delta
checks = json.loads((ROOT / 'evidence/build-work-stage2/checks.json').read_text())
for row in checks:
    assert row['returncode'] == 0 and sha(ROOT / row['log']) == row['sha256']
new_checks = [json.loads((OUT / (p + '-checks.json')).read_text()) for p in ['darwin', 'linux']]
for env in new_checks:
    for p, h in env['sources'].items(): assert sha(ROOT / p) == h
    for row in env['checks']:
        assert row['returncode'] == 0 and sha(ROOT / row['log']) == row['sha256']
s3 = ROOT / 'evidence/build-work-stage3'
report = json.loads((s3 / 'test/report.json').read_text())
for row in report['commands']:
    assert row['returncode'] == 0 and sha(ROOT / row['log']) == row['sha256']
s2 = ROOT / 'evidence/build-work-stage2'
controller = json.loads((s2 / 'controller-result.json').read_text())
old_reuse = json.loads((s2 / 'controller-reuse.json').read_text())
assert controller['returncode'] == 0 and controller['matches_saved_controller']
assert sha(s2 / 'controller.log') == controller['log_sha256']
record = Path(controller['baseline_record'])
assert sha(record) == controller['baseline_record_sha256']
baseline = json.loads(record.read_text())
assert baseline['result'] == 'DEV CHECKED'
assert sha(record.parent / 'ctl.img') == controller['controller_sha256'] == baseline['artifacts']['controller']['sha256']
assert sha(record.parent / 'controller.log') == baseline['logs']['controller']['sha256']
assert sha('/Users/mac/Documents/x0x37/build_work_implementation/ble/ctl/out/ctl.img') == controller['controller_sha256']
assert sha(old_reuse['baseline_environment']) == old_reuse['baseline_environment_sha256']
for p, h in controller['sources'].items():
    assert sha(ROOT / p) == h
    assert hashlib.sha256(git('show', baseline['commit'] + ':' + p)).hexdigest() == h
assert sha(ROOT / 'ble/ctl/build_ctl.sh') == controller['orchestration']['ble/ctl/build_ctl.sh']
built = (s2 / 'controller-built-script.sh').read_text()
parent = git('show', BASE + ':ble/ctl/build_in_container.sh').decode()
current = (ROOT / 'ble/ctl/build_in_container.sh').read_text()
assert hashlib.sha256(built.encode()).hexdigest() == old_reuse['built_script_sha256']
assert hashlib.sha256(parent.encode()).hexdigest() == old_reuse['current_script_sha256']
assert list(difflib.unified_diff(built.splitlines(), parent.splitlines(), lineterm='')) == old_reuse['exact_delta']
# The repair changes only join_jobs. All invocation, diagnostic, link and output bytes stay equal.
for a, b in [(parent, current)]:
    assert a.split('join_jobs() {', 1)[0] == b.split('join_jobs() {', 1)[0]
    assert a.split('for f in src/*.c src/*.S gen/*.c; do', 1)[1] == b.split('for f in src/*.c src/*.S gen/*.c; do', 1)[1]
assert built.split('CF=', 1)[1].split('# Work in /tmp:', 1)[0] == current.split('CF=', 1)[1].split('# Work in /tmp:', 1)[0]
assert built.split('L=$B/lib\n', 1)[1] == current.split('L=$B/lib\n', 1)[1]
protected = ['tests/run_tests.sh', 'tools/build.py', 'tools/dev_validate.py', 'tools/validation_inputs.py', 'host/build_host.sh', 'BUILDING.md']
assert not git('diff', BASE, '--', *protected)
result = dict(result='PASS', source_base=BASE, source_candidate=CANDIDATE,
              source_hashes={p:sha(ROOT/p) for p in manifest}, changed_files=changed,
              reused_checks=[r for r in checks if r['argv'][-1] != 'tests/host/test_ctl_compile.py'],
              superseded_controller_fixture=checks[1], new_controller_checks=new_checks,
              stage3_quickcheck_adversarial_report='evidence/build-work-stage3/test/report.json',
              controller_result='evidence/build-work-stage2/controller-result.json',
              controller_image_sha256=controller['controller_sha256'],
              original_real_controller_seconds=controller['elapsed_seconds'],
              current_script_sha256=sha(ROOT/'ble/ctl/build_in_container.sh'),
              parent_delta=list(difflib.unified_diff(parent.splitlines(),current.splitlines(),lineterm='')),
              exact_built_to_current_delta=list(difflib.unified_diff(built.splitlines(),current.splitlines(),lineterm='')),
              successful_invocation_flags_link_outputs_unchanged=True, protected_sources_unchanged=protected,
              reason='Only joined-driver process-group cleanup changes. A normally successful driver leaves no residual group; the added kill then returns ESRCH and is ignored. Any surviving compiler children are stopped before group ownership is dropped. Compiler argv, batch bound, diagnostic and object order, link and image inputs remain unchanged.',
              limits=['No second real controller, baseline or application build; no isolated timing comparison.',
                      'SDK/toolchain identities reuse saved controller records and were not rehashed.',
                      'Stage2 checks do not replace the required fresh independent stage3 review.'])
(OUT/'reuse.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS: matching unaffected evidence, new cleanup checks, exact real controller artifact and bounded source-delta equivalence')
