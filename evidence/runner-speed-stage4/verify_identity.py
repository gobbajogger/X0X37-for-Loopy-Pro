#!/usr/bin/env python3
"""Check source and saved evidence identity without running tests or builds.

Run from any directory. Only identity.json beside this script is written.
"""
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
CANDIDATE = 'fe7f0401e79cd5dd4a76307811d2ee0d12446263'
INPUT = '7f8dff3d69880e26b3a56be3f21f5a8af41ae6b8'
BASE = '07fa87dc950908d4d720bde9c06c2478cc212b19'
commands = []


def run(argv):
    p = subprocess.run(argv, cwd=ROOT, text=True, stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT, timeout=30)
    commands.append({'argv': argv, 'rc': p.returncode, 'output': p.stdout.strip()})
    assert p.returncode == 0, commands[-1]
    return p.stdout.strip()


def read(path):
    return json.loads((ROOT / path).read_text())


def sha(path):
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


run(['git', 'merge-base', '--is-ancestor', INPUT, 'HEAD'])
changed = run(['git', 'diff', '--name-only', CANDIDATE, '--', '.',
               ':(exclude)evidence/**'])
assert not changed, changed
production_files = run(['git', 'diff', '--name-only', BASE, CANDIDATE,
                        '--', '.', ':(exclude)evidence/**']).splitlines()
expected_files = ['tests/host/smk_clip_chord_display_test.sh',
                  'tests/host/smk_clip_knob_test.sh',
                  'tests/host/test_runner_admission.py', 'tests/run_tests.sh']
assert sorted(production_files) == expected_files
inputs = read('evidence/runner-speed-stage2/inputs.json')['sha256']
assert len(inputs) == 417
assert all(sha(path) == digest for path, digest in inputs.items())
source = read('evidence/runner-speed-stage3/review.json')['source_sha256']
assert all(sha(path) == digest for path, digest in source.items())
saved = read('evidence/runner-speed-stage3/test/saved-assertions.json')
assert saved['status'] == 'PASS'
assert all(sha(path) == digest for path, digest in saved['evidence_sha256'].items())
stage3_files = run(['git', 'ls-tree', '-r', '--name-only', INPUT,
                    '--', 'evidence/runner-speed-stage3']).splitlines()
for path in stage3_files:
    expected = subprocess.check_output(['git', 'show', INPUT + ':' + path], cwd=ROOT)
    assert (ROOT / path).read_bytes() == expected, path
stage3 = read('evidence/runner-speed-stage3/identity.json')
references = {}
for name, expected in read('evidence/runner-speed-stage2/reference-pins.json').items():
    path = '/Users/mac/Documents/x0x37/validation_refs/' + name
    head = run(['git', '-C', path, 'rev-parse', 'HEAD'])
    status = run(['git', '-C', path, 'status', '--porcelain', '--untracked-files=no'])
    assert head == expected and not status
    references[name] = {'head': head, 'tracked_status': status}
for tool in stage3['tools']:
    output = run(tool['argv'])
    assert output == tool['output'], {'expected': tool, 'actual': output}
unchanged_trees = {}
for path in ['firmware', 'host', 'tools', 'assets']:
    base_tree = run(['git', 'rev-parse', BASE + ':' + path])
    candidate_tree = run(['git', 'rev-parse', CANDIDATE + ':' + path])
    assert base_tree == candidate_tree
    unchanged_trees[path] = candidate_tree
assert read('evidence/runner-speed-stage3/result.json')['state'] == 'checked'
assert read('evidence/runner-speed-stage3/test/report.json')['status'] == 'PASS'
investigation = read('evidence/runner-speed-stage3/investigation/report.json')
assert investigation['result'] == 'pass' and not investigation['findings']
result = {
    'status': 'PASS', 'candidate': CANDIDATE, 'base': BASE,
    'input_evidence_commit': INPUT, 'production_files': production_files,
    'source_sha256': source, 'matching_saved_source_hashes': len(inputs),
    'matching_stage2_evidence_hashes': len(saved['evidence_sha256']),
    'stage3_evidence_sha256': {p: sha(p) for p in stage3_files},
    'unchanged_production_source': True, 'unchanged_trees': unchanged_trees,
    'reference_pins': references, 'tools': stage3['tools'],
    'commands': commands,
    'method': 'Compare unchanged source, saved evidence, clean reference pins, '
              'host versions and immutable Linux image. No behavior test or build execution.',
    'legacy_checker_note': 'Stage3 source_audit.py and verify_saved_evidence.py '
                           'require HEAD equal to the production commit. They were '
                           'not rerun on this evidence descendant. Their committed '
                           'outputs and all recorded inputs are unchanged.'
}
(OUT / 'identity.json').write_text(json.dumps(result, indent=2) + '\n')
print('PASS: 417 source hashes, saved stage2/3 evidence, four reference pins, '
      'host tools, Linux image and unchanged production trees match.')
