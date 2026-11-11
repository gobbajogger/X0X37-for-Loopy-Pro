"""Verify the stage3 records and save the source-repair disposition."""
from pathlib import Path
import hashlib
import json
import subprocess

OUT = Path(__file__).resolve().parent
ROOT = OUT.parents[1]

def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

test = json.loads((OUT / 'test/report.json').read_text())
investigation = json.loads((OUT / 'investigation/report.json').read_text())
assert test['result'] == investigation['result'] == 'FAIL'
assert test['candidate'] == investigation['candidate'] == '05eff604cb3f136db71727a2aeeea785cab73b99'
assert test['new_failure']['id'] == investigation['findings'][0]['id'] == 'CENTRAL-ABORT-02'
for name, digest in json.loads((OUT / 'test/sha256.json').read_text()).items():
    assert sha(OUT / 'test' / name) == digest, name
for name, digest in investigation['owned_evidence_sha256'].items():
    assert sha(OUT / 'investigation' / name) == digest, name
for record in json.loads((OUT / 'test/commands.json').read_text()):
    assert record['returncode'] == record['expected'], record['name']
deadline = json.loads((OUT / 'investigation/deadline-stop-record.json').read_text())
assert len(deadline) == 13
assert deadline[0]['returncode'] == 0
assert all(row['returncode'] == -6 for row in deadline[1:])
for name, digest in json.loads((OUT / 'input-review.json').read_text())['source_inputs'].items():
    assert sha(ROOT / name) == digest, name
subprocess.run(['git', 'diff', '--check'], cwd=ROOT, check=True)
assert not subprocess.check_output(['git', 'diff', '--name-only'], cwd=ROOT, text=True).strip()
record = {
    'stage': 3,
    'marker': 'smc-central-abort-20261021-stage3-after-checked-repair',
    'owner_thread': '01a15a92-6343-7f30-8495-f968228e2d2c',
    'source_owner': '01a15a8a-6061-7a83-9716-5766833e15a5',
    'candidate': test['candidate'],
    'candidate_tree': test['candidate_tree'],
    'result': 'SOURCE REPAIR REQUIRED',
    'source_repair_required': True,
    'next_stage': 2,
    'helpers': {
        'test': {'owner': '/root/test', 'result': 'FAIL', 'report': 'test/report.json'},
        'investigation': {'owner': '/root/investigation', 'result': 'FAIL', 'report': 'investigation/report.json'}
    },
    'passing_checks': ['Public ASan/UBSan central, receive, signaling and abort tests.', 'Old and extended DIAG decoder tests.', 'Public overflow/timeout and both saved independent probes pass on the candidate. The matching 28d0fc2 controls fail functionally.', 'Additional event-order and lifecycle checks.'],
    'new_failure': investigation['findings'][0],
    'negative_control_limit': 'The original expected 28d0fc2 failures are separate from the unexpected deadline failures on 05eff60.',
    'reuse': {'record': 'input-review.json', 'source_inputs': 412, 'replay': 'The exact stage2 replay inputs match. Stage3 does not repeat that replay.', 'builds': 'The saved stage2 artifacts match. Stage3 runs no target build. Stage4 validated install records do not exist for this contribution.'},
    'setup_notes': ['The first input review stopped because the stage2 owner added a handoff-only commit. The source comparison then passed.', 'The investigation extra probe first failed to compile because the nested public test redefined main. The owned include resolves that setup error.'],
    'production_edits': False,
    'tracked_test_edits': False,
    'device_write': False,
    'controller_write': False,
    'target_build': False,
    'release_build': False,
    'full_gate': False,
    'external_push': False,
    'physical_result': 'NOT TESTED',
    'original_input_failure': 'UNRESOLVED',
    'mapping_stage4': '01a15a87-298e-7d21-b004-b78d5beafd89',
    'integration': 'Mapping stage4 retains selection of the sole stage5 integrator for both validated contributions.',
    'authorization': 'Owner request Now read rules and fix; workflow17 source-repair loop; workflow18 automatic launch, recovery, startup checks and completion notices; owner-decisions14 complete validated DEV install with notice.'
}
(OUT / 'checks.json').write_text(json.dumps(record, indent=2) + '\n')
manifest = {str(p.relative_to(OUT)): sha(p) for p in sorted(OUT.rglob('*')) if p.is_file() and p.name not in ('evidence-sha256.json', 'successor.json', 'successor-startup.json')}
(OUT / 'evidence-sha256.json').write_text(json.dumps(manifest, indent=2) + '\n')
print('Both independent failure reports and all evidence hashes agree.')
print('No production source or tracked test changed.')
