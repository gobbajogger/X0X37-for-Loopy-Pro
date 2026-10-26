"""Compare the candidate with the saved stage2 inputs. No device access occurs."""
from pathlib import Path
import hashlib
import json
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
PRIOR = ROOT.parent / 'smc_central_abort_repair_20261021'
CANDIDATE = '05eff604cb3f136db71727a2aeeea785cab73b99'

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT, text=True).strip()

assert git('rev-parse', 'HEAD') == CANDIDATE
assert git('rev-parse', 'HEAD^{tree}') == 'a00e1710ab67f800dea2bf4b97fbb9bd34fde384'
assert not git('status', '--porcelain', '--untracked-files=no')
prior_head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=PRIOR, text=True).strip()
prior_changes = subprocess.check_output(['git', 'diff', '--name-only', CANDIDATE, prior_head], cwd=PRIOR, text=True).splitlines()
assert all(name.startswith('evidence/abort-stage2/') for name in prior_changes), prior_changes
assert not subprocess.check_output(['git', 'status', '--porcelain', '--untracked-files=no'], cwd=PRIOR, text=True).strip()
selected = git('ls-files', 'firmware', 'host', 'tools', 'tests', 'build.sh', 'ble/ctl').splitlines()
inputs = {}
for name in selected:
    a, b = ROOT / name, PRIOR / name
    assert sha(a) == sha(b), name
    inputs[name] = sha(a)

record = json.loads((ROOT / 'evidence/abort-stage2/replay.json').read_text())
external = ROOT.parent / 'smc_working_source_regression_evidence_20261021'
assert sha(external / 'compare_startup.py') == record['harness_sha256']
assert sha(external / 'working-source-store.bin') == record['store_sha256']
capture = 'cloud-handoff/2026-10-20/smc_filter_fix/evidence/smc-raw-ble-retry.json'
assert sha(ROOT / capture) == sha(PRIOR / capture) == record['capture_sha256']
generated = json.loads((ROOT / 'evidence/abort-stage2/replay-inputs.json').read_text())
for name, expected in generated.items():
    assert sha(PRIOR / name) == expected, name
    if (ROOT / name).exists():
        assert sha(ROOT / name) == expected, name
assert subprocess.check_output(['cc', '--version'], text=True) == json.loads((ROOT / 'evidence/abort-stage2/environment.json').read_text())['cc']

artifacts = {}
checks = json.loads((ROOT / 'evidence/abort-stage2/checks.json').read_text())
for model, build in checks['builds'].items():
    artifacts[model] = {}
    for kind, info in build['artifacts'].items():
        p = Path(info['path'])
        assert p.stat().st_size == info['size'] and sha(p) == info['sha256']
        artifacts[model][kind] = info

result = {
    'candidate': CANDIDATE,
    'tree': git('rev-parse', 'HEAD^{tree}'),
    'source_inputs': inputs,
    'source_input_count': len(inputs),
    'tracked_edits': False,
    'prior_lane_head': prior_head,
    'prior_lane_later_files': prior_changes,
    'setup_notes': ['The first review stopped because the stage2 owner committed the stage3 startup record. Only three handoff evidence files changed. The source inputs still match.'],
    'reused_replay': record,
    'reuse_reason': 'The complete source, test, build and controller inputs match the stage2 lane. The capture, harness, store, generated inputs and compiler also match.',
    'replay_run_in_stage3': False,
    'stage2_build_artifacts': artifacts,
    'stage2_build_limit': 'These are stage2 DEV build checks. Stage4 must issue new DEV CHECKED records for both complete package and controller sets.',
    'physical_result': 'NOT TESTED',
    'original_input_failure': 'UNRESOLVED'
}
(OUT / 'input-review.json').write_text(json.dumps(result, indent=2) + '\n')
print('The candidate matches all %d selected stage2 source inputs.' % len(inputs))
print('The exact replay inputs and saved build artifacts match.')
