#!/usr/bin/env python3
"""Verify final independent evidence without rerunning unaffected checks."""
from pathlib import Path
import hashlib
import json
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
CANDIDATE = 'cadeae04bdc9160cfe15a74427829165e36ba60b'
def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()
inputs = json.loads((OUT/'inputs.json').read_text())
for path, expected in inputs['source_hashes'].items():
    assert sha(ROOT/path) == expected, path
    source = subprocess.check_output(['git', '-C', str(ROOT), 'show', CANDIDATE+':'+path])
    assert hashlib.sha256(source).hexdigest() == expected, path
for path, expected in inputs['unchanged_consumers_and_generators'].items():
    assert sha(ROOT/path) == expected, path
for check in json.loads((OUT/'checks.json').read_text())['checks']:
    assert check['returncode'] == 0 and sha(OUT/check['log']) == check['sha256'], check
test = json.loads((OUT/'independent-test/test-result.json').read_text())
investigation = json.loads((OUT/'independent-investigation/investigation-result.json').read_text())
assert test['status'] == investigation['status'] == 'PASS'
assert test['candidate'] == investigation['candidate'] == CANDIDATE
assert not test['source_changed'] and investigation['source_unchanged']
assert not test['failures'] and not test['blocked_tool_execution'] and not investigation['findings']
for row in test['commands']:
    assert row['returncode'] == 0 and sha(OUT/'independent-test'/row['log']) == row['sha256']
result = {'workflow': 'build-time-20261021', 'marker': 'build-time-20261021-stage2-python-manifest-repair',
          'status': 'CHECKED SOURCE REPAIR AND INDEPENDENT TEST/INVESTIGATION PASS',
          'checked_source_commit': CANDIDATE,
          'source_files': inputs['source_hashes'], 'production_runner_unchanged': True,
          'implementation': 'result.json', 'implementation_checks': 'checks.json',
          'independent_test': 'independent-test/test-result.json',
          'independent_investigation': 'independent-investigation/investigation-result.json',
          'independent_outcome': {'test': 'PASS:9 host and14 pinned-Linux cases',
                                  'investigation': 'PASS:10 fresh edge scopes, matching retained evidence, no material findings'},
          'remaining': ['Controlled target validation and exact package/controller artifact comparisons with matching saved audit inputs.',
                        'End-to-end command timing with exact timer scope; no second complete old validation merely to recreate baseline timings.',
                        'A supplied-evidence decision in classic LoopyX0X37 before another bounded execution task.'],
          'classic_handoff': {'status': 'PROMPT READY; OWNER LAUNCH NEEDED', 'prompt': 'classic-handoff.txt',
                               'project_id': 'g-p-ad9d6d4ab04c8191af6aa22fd752707e',
                               'project_url': 'https://chatgpt.com/g/g-p-ad9d6d4ab04c8191af6aa22fd752707e/project',
                               'originating_classic_thread': None, 'codex_successor_created': False,
                               'source_of_updated_routing': '01a15b05-73df-7fb1-a9f7-6bb1484cfd02',
                               'confirmed_by_coordinator': '01a15aec-08a1-7cb3-b395-f17ad2cf9add'},
          'limits': ['No real target build, target artifact equality or end-to-end speed measurement is claimed.',
                     'No release/full gate, device install/test, firmware integration or external push.',
                     'Manifestless package coverage follows safely declared installed module roots; it does not enumerate arbitrary installation files or a full system library closure.',
                     'Every additional consumed worker mount must be declared in plan.dependencies.',
                     'Foreign AGENTS.md/docs/SKILLS.md and discovery-link rollout edits remain outside these commits.'],
          'evidence_sha256': {str(p.relative_to(OUT)): sha(p) for p in sorted(OUT.rglob('*'))
                              if p.is_file() and p.name != 'final-result.json'}}
(OUT/'final-result.json').write_text(json.dumps(result, indent=2)+'\n')
print('Final source, current command logs and independent evidence verified.')
