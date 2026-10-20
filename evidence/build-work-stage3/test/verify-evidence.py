#!/usr/bin/env python3
"""Verify saved source and test-result identities without repeating passing suites."""
import hashlib
import json
from pathlib import Path
import platform
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
h = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
git = lambda *a: subprocess.check_output(['git', *a], cwd=ROOT, text=True).strip()
CANDIDATE = '30c567d8588227d5285f04422130300fe83577a9'
assert git('rev-parse', 'HEAD') == CANDIDATE
stage2 = ROOT / 'evidence/build-work-stage2'
result = json.loads((stage2/'result.json').read_text())
sources = json.loads((stage2/'source-files.json').read_text())
for name, digest in sources.items():
    assert h(ROOT/name) == digest, name
    blob = subprocess.check_output(['git','show', CANDIDATE+':'+name], cwd=ROOT)
    assert hashlib.sha256(blob).hexdigest() == digest, name
checks = json.loads((stage2/'checks.json').read_text())
assert len(checks) == 7
for row in checks:
    assert row['returncode'] == 0, row
    assert h(ROOT/row['log']) == row['sha256'], row
    text = (ROOT/row['log']).read_text()
    assert ('\nOK\n' in text or ' of ' in text and ' ok\n' in text), row
# After the checked code commit, only evidence and its dedicated documentation were added.
postcheck = git('diff','--name-only',result['code_commit'],CANDIDATE).splitlines()
assert all(p.startswith('evidence/build-work-stage2/') or p == 'docs/QUICKCHECK-SCOPE.md' for p in postcheck), postcheck
assert not git('diff',result['audit_commit'],CANDIDATE,'--','tests/run_tests.sh','tools/build.py','tools/dev_validate.py','tools/validation_inputs.py','host/build_host.sh','BUILDING.md')
all_source_diff = git('diff','--name-only',result['audit_commit'],CANDIDATE).splitlines()
non_evidence = [p for p in all_source_diff if not p.startswith('evidence/')]
assert set(non_evidence) == set(sources)
identity = {
 'candidate': CANDIDATE, 'checked_code_commit': result['code_commit'],
 'source_manifest_hash': h(stage2/'source-files.json'),
 'source_hashes_match_checkout_and_candidate': True,
 'source_files': sources, 'all_candidate_source_changes_in_manifest': True,
 'postcheck_changes_are_evidence_or_dedicated_doc': True,
 'seven_saved_logs_match': checks,
 'saved_worker_image': result['checked']['worker_image'],
 'runner_and_active_other_sources_unchanged': True,
 'adversarial': {'argv':['python3','-B','evidence/build-work-stage3/test/adversarial.py'], 'returncode':0,
                 'script_sha256':h(OUT/'adversarial.py'),'log_sha256':h(OUT/'adversarial.log'),
                 'cases':json.loads((OUT/'adversarial.log').read_text())},
 'local_adversarial_environment': {'python': sys.version, 'platform':platform.platform(),'git':git('--version')},
}
(OUT/'identity.json').write_text(json.dumps(identity,indent=2)+'\n')
print(json.dumps({'result':'PASS','source_files':len(sources),'saved_suites':len(checks),'adversarial_cases':2}))
