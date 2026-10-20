#!/usr/bin/env python3
"""Verify stage4 source, reports and retained artifacts without running builds or suites."""
import ast
import difflib
import hashlib
import json
from pathlib import Path
import platform
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
CANDIDATE = 'c4cec67f9c642fa6f925a06147e136b2b7231644'
HANDOFF = '7e0f567a0adec5252fbf97de0176ef8c6549afd1'
INDEPENDENT = '258144db38d331c691c3a449146ff3da8ed86676'
BASE = '78c5296e95b9a18a514bbfd126ab04ebd86df9c9'
ORIGINAL = '30c567d8588227d5285f04422130300fe83577a9'
AUDIT = '07fa87dc950908d4d720bde9c06c2478cc212b19'
IMAGE = 'sha256:0cffe038efa0f50556de3fb444de6386729d495f63703c8c3b40fdfd0482d4b1'
SCRIPT = 'ble/ctl/build_in_container.sh'
FIXTURE = 'tests/host/test_ctl_compile.py'
verified = {}

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT)

def read(path):
    return json.loads((ROOT / path).read_text())

def record(path, expected=None):
    digest = sha(ROOT / path)
    if expected is not None:
        assert digest == expected, path
    verified[str(path)] = digest
    return digest

def source_entries(ref):
    return [entry for entry in git('ls-tree', '-r', '-z', ref).split(b'\0')
            if entry and not entry.split(b'\t', 1)[1].startswith(b'evidence/')]

def check_log(row):
    assert row['returncode'] == 0, row
    record(row['log'], row['sha256'])

# Verify every tracked non-evidence source path, mode and blob, rather than a selected subset.
entries = source_entries(CANDIDATE)
head = git('rev-parse', 'HEAD').decode().strip()
assert source_entries(HANDOFF) == entries == source_entries(head)
assert not git('diff', CANDIDATE, '--', '.', ':(exclude)evidence/**')
assert not git('ls-files', '--others', '--exclude-standard', '--', '.', ':(exclude)evidence/**')
assert git('branch', '--show-current').decode().strip() == 'lane/opt-build-work-cleanup-stage4'
for ancestor, descendant in [(AUDIT, ORIGINAL), (BASE, CANDIDATE), (CANDIDATE, INDEPENDENT), (INDEPENDENT, HANDOFF)]:
    subprocess.run(['git', 'merge-base', '--is-ancestor', ancestor, descendant], cwd=ROOT, check=True)
post_candidate = git('rev-list', '--reverse', CANDIDATE + '..' + HANDOFF).decode().splitlines()
commit_changes = {}
for commit in post_candidate:
    changed = git('diff-tree', '--no-commit-id', '--name-only', '-r', commit).decode().splitlines()
    assert changed and all(p.startswith('evidence/build-work-cleanup-stage3/') for p in changed), commit
    commit_changes[commit] = changed
assert set(p for p in git('diff', '--name-only', BASE, CANDIDATE).decode().splitlines()
           if not p.startswith('evidence/')) == {SCRIPT, FIXTURE}
assert set(p for p in git('diff', '--name-only', ORIGINAL, CANDIDATE).decode().splitlines()
           if not p.startswith('evidence/')) == {SCRIPT, FIXTURE}

# Manifests preserve the original reports, including the historical failure and its disposition.
for directory in ['build-work-cleanup-repair', 'build-work-stage3', 'build-work-cleanup-stage3']:
    manifest = 'evidence/' + directory + '/evidence-sha256.json'
    record(manifest)
    for path, digest in read(manifest).items():
        resolved = path if path.startswith('evidence/') else str(Path(manifest).parent / path)
        record(resolved, digest)
stage3 = read('evidence/build-work-cleanup-stage3/result.json')
assert stage3['result'] == 'PASS' and stage3['tested_candidate'] == CANDIDATE
assert stage3['source_changed'] is False
for role, info in stage3['independent_reports'].items():
    record(info['path'], info['sha256'])
    report = read(info['path'])
    assert report['result'] == 'PASS' and report['candidate'] == CANDIDATE
    assert report['material_findings'] == []
    for row in report.get('commands', report.get('checks', [])):
        check_log(row)
for row in read('evidence/build-work-cleanup-stage3/checks.json'):
    check_log(row)

reuse = read('evidence/build-work-cleanup-repair/reuse.json')
for path, digest in reuse['source_hashes'].items():
    record(path, digest)
    assert hashlib.sha256(git('show', CANDIDATE + ':' + path)).hexdigest() == digest
for row in reuse['reused_checks']:
    check_log(row)
assert len(reuse['reused_checks']) == 6
prior = read('evidence/build-work-stage3/test/report.json')
record(prior['identity']['path'], prior['identity']['sha256'])
for row in prior['commands']:
    check_log(row)
prior_identity = read(prior['identity']['path'])
assert [c['result'] for c in prior_identity['adversarial']['cases']] == ['PASS', 'PASS']
record('evidence/build-work-stage3/test/adversarial.py', prior_identity['adversarial']['script_sha256'])
for path in ['evidence/build-work-stage3/test/report.json', 'evidence/build-work-stage3/investigation/report.json']:
    record(path)

# Match saved executed platform checks; negative rc0 means the original was rejected.
platforms = []
for name, machine in [('darwin', 'arm64'), ('linux', 'x86_64')]:
    env = read('evidence/build-work-cleanup-repair/' + name + '-checks.json')
    assert env in reuse['new_controller_checks'] and env['base'] == BASE
    assert env['machine'] == machine
    assert env['image'] == (IMAGE if name == 'linux' else None)
    for path, digest in env['sources'].items():
        record(path, digest)
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
            assert 'PASS: original source rejected in all ten' in log
    platforms.append(env)
fixture = ast.parse((ROOT / FIXTURE).read_text())
assert len([n for n in ast.walk(fixture) if isinstance(n, ast.FunctionDef) and n.name.startswith('test_')]) == 7

# Recheck retained successful controller artifacts and saved dependency identities only.
controller = read('evidence/build-work-stage2/controller-result.json')
old_reuse = read('evidence/build-work-stage2/controller-reuse.json')
record('evidence/build-work-stage2/controller-result.json')
record('evidence/build-work-stage2/controller-reuse.json')
assert controller['returncode'] == 0 and controller['matches_saved_controller']
assert controller['worker_image'] == IMAGE and controller['jobs'] == 4
check_log(dict(returncode=0, log='evidence/build-work-stage2/controller.log', sha256=controller['log_sha256']))
baseline_path = Path(controller['baseline_record'])
record(baseline_path, controller['baseline_record_sha256'])
baseline = json.loads(baseline_path.read_text())
assert baseline['result'] == 'DEV CHECKED'
assert controller['dependencies'] == {name: baseline['validation_inputs']['inputs']['dependencies'][name]
                                      for name in ('toolchain', 'sdk')}
record(baseline_path.parent / 'ctl.img', controller['controller_sha256'])
assert controller['controller_sha256'] == baseline['artifacts']['controller']['sha256'] == reuse['controller_image_sha256']
record(baseline_path.parent / 'controller.log', baseline['logs']['controller']['sha256'])
built_root = ROOT.parent / 'build_work_implementation'
record(built_root / 'ble/ctl/out/ctl.img', controller['controller_sha256'])
record(built_root / 'ble/ctl/gen/rf_tables.c', controller['generated_rf_tables_sha256'])
record(old_reuse['baseline_environment'], old_reuse['baseline_environment_sha256'])
environment = json.loads(Path(old_reuse['baseline_environment']).read_text())
assert environment['image_id'] == IMAGE and environment['image_platform'] == 'linux/amd64'
assert environment['configuration']['config'] == 'dev'
for path, digest in controller['sources'].items():
    record(path, digest)
    assert hashlib.sha256(git('show', baseline['commit'] + ':' + path)).hexdigest() == digest
record('ble/ctl/build_ctl.sh', controller['orchestration']['ble/ctl/build_ctl.sh'])
built = (ROOT / 'evidence/build-work-stage2/controller-built-script.sh').read_text()
parent = git('show', BASE + ':' + SCRIPT).decode()
current = (ROOT / SCRIPT).read_text()
assert hashlib.sha256(built.encode()).hexdigest() == old_reuse['built_script_sha256'] == controller['orchestration'][SCRIPT]
assert list(difflib.unified_diff(built.splitlines(), current.splitlines(), lineterm='')) == reuse['exact_built_to_current_delta']
assert list(difflib.unified_diff(parent.splitlines(), current.splitlines(), lineterm='')) == reuse['parent_delta']
assert parent.split('join_jobs() {', 1)[0] == current.split('join_jobs() {', 1)[0]
assert parent.split('for f in src/*.c src/*.S gen/*.c; do', 1)[1] == current.split('for f in src/*.c src/*.S gen/*.c; do', 1)[1]
assert built.split('CF=', 1)[1].split('# Work in /tmp:', 1)[0] == current.split('CF=', 1)[1].split('# Work in /tmp:', 1)[0]
assert built.split('L=$B/lib\n', 1)[1] == current.split('L=$B/lib\n', 1)[1]
for path in reuse['protected_sources_unchanged']:
    assert not git('diff', BASE, CANDIDATE, '--', path)

result = dict(result='PASS', candidate=CANDIDATE, handoff=HANDOFF,
              independent_evidence_commit=INDEPENDENT, head_at_verification=head,
              source_tree_identity_sha256=hashlib.sha256(b'\0'.join(entries) + b'\0').hexdigest(),
              source_tree_identity_format='git ls-tree -r -z entries excluding evidence/; original order; terminal NUL',
              source_entry_count=len(entries), all_non_evidence_source_equal=True,
              post_candidate_evidence_only_commits=commit_changes,
              source_hashes=reuse['source_hashes'], verified_file_sha256=verified,
              reused_platform_checks=platforms, reused_unaffected_suites=reuse['reused_checks'],
              reused_quickcheck_commands=prior['commands'], successful_controller=controller,
              controller_baseline_config=environment['configuration'],
              dependencies_reused_without_full_rehash=controller['dependencies'],
              exact_cleanup_delta_verified=True, compiler_flags_equal=True, link_post_link_equal=True,
              source_findings=[], new_behavior_probes_required=False,
              verifier_environment=dict(python=sys.version, host=platform.platform()))
(OUT / 'identity.json').write_text(json.dumps(result, indent=2) + '\n')
print('PASS: all non-evidence source matches tested candidate; three stage3 commits are evidence only.')
print('PASS: independent reports/manifests, two platform suites and negative controls, six suites and Quickcheck evidence match.')
print('PASS: retained controller image, generated tables, baseline/environment/input identities and cleanup-only delta match.')
print('No behavior suites, target builds or full dependency hashing ran.')
