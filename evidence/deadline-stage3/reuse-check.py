"""Check retained stage2 inputs and artifacts. Do not run builds or replay."""
from pathlib import Path
import hashlib
import json
import shutil
import subprocess

R = Path(__file__).resolve().parents[2]
E = Path(__file__).resolve().parent
P = R.parent / 'smc_central_abort_deadline_repair_20261021'
X = R.parent / 'smc_working_source_regression_evidence_20261021'
D = P / 'evidence/deadline-stage2'

def digest(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def git(*args, repo=R):
    return subprocess.check_output(['git', '-C', str(repo), *args], text=True).strip()

def load(p):
    return json.loads(Path(p).read_text())

source = load(D / 'source-inputs.json')
mismatches = []
for name, wanted in source.items():
    for root in (R, P):
        if digest(root / name) != wanted:
            mismatches.append(str(root / name))
assert not mismatches, mismatches
candidate = 'f080d1a5f31073dde600238f780acf37c12b2738'
assert not git('diff', candidate, 'HEAD', '--', 'firmware', 'tests', 'tools', 'host', 'ble', 'build.sh')
assert not git('diff', '--', 'firmware', 'tests', 'tools', 'host', 'ble', 'build.sh')

replay_inputs = load(D / 'replay-inputs.json')
verified_replay = {}
for name, wanted in replay_inputs.items():
    path = X / name.split('/', 1)[1] if name.startswith('external/') else P / name
    assert digest(path) == wanted, path
    verified_replay[name] = {'path': str(path), 'sha256': wanted}
capture = load(P / 'cloud-handoff/2026-10-20/smc_filter_fix/evidence/smc-raw-ble-retry.json')
payloads = [bytes.fromhex(row['hex']) for row in capture]
binary = b''.join(len(p).to_bytes(2, 'little') + p for p in payloads if len(p) <= 61)
assert binary == (P / 'build/deadline-stage2/payloads.bin').read_bytes()
assert len([p for p in payloads if len(p) <= 61]) == 1268
assert len([p for p in payloads if len(p) > 61]) == 3

# The saved harness has one absolute include. Compare that source with this lane.
assert (P / 'host/x0x_host.c').read_bytes() == (R / 'host/x0x_host.c').read_bytes()
saved = (D / 'replay.c').read_text()
import importlib.util
spec = importlib.util.spec_from_file_location('saved_harness', X / 'compare_startup.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
for name in ('ble_central_midi', 'ble_central_link_notify', 'ble_central_service', 'plat_ble_smc_send'):
    assert module.extract((R / 'firmware/src/ble.c').read_text(), name) in saved, name

environment = load(D / 'environment.json')
cc = Path(shutil.which('cc')).resolve()
host_version = subprocess.check_output(['cc', '--version'], text=True)
assert host_version == environment['cc']
target_cc = Path('/Users/mac/.jieli/toolchain/pi32v2/bin/clang')
assert digest(target_cc) == environment['compiler_sha256']
assert git('rev-parse', 'HEAD', repo=Path('/Users/mac/fw-AC79_AIoT_SDK')) == environment['sdk']['commit']
assert git('status', '--porcelain', repo=Path('/Users/mac/fw-AC79_AIoT_SDK')) == environment['sdk']['status']
image = subprocess.check_output(['docker', 'image', 'inspect', 'x0x37-validation:local', '--format', '{{.Id}}'], text=True).strip()
assert image == environment['image_id']

checks = load(D / 'checks.json')
artifacts = {}
for model, row in checks['builds'].items():
    artifacts[model] = {}
    for kind, item in row['artifacts'].items():
        p = Path(item['path'])
        assert p.stat().st_size == item['size'] and digest(p) == item['sha256'], p
        artifacts[model][kind] = item

logs = {}
for p in sorted(D.glob('*.log')):
    text = p.read_text()
    logs[p.name] = {'sha256': digest(p), 'bytes': p.stat().st_size,
                    'sanitizer_error': any(t in text for t in ('ERROR: AddressSanitizer', 'runtime error:', 'UndefinedBehaviorSanitizer:'))}
assert not any(v['sanitizer_error'] for v in logs.values())
host = load(D / 'host-checks.json')
assert len(host) == 56 and all(row['result'] == 'PASS' for row in host)
for row in host:
    assert (D / row['log']).is_file()

record = dict(candidate=candidate, candidate_tree=git('rev-parse', candidate + '^{tree}'),
              lane_base=git('rev-parse', 'HEAD'), source_input_count=len(source),
              source_inputs=source, source_mismatches=mismatches,
              replay_inputs=verified_replay, replay_result=load(D / 'replay.json'),
              generated_payloads={'included': 1268, 'excluded': 3, 'resegmented': False},
              generated_headers={str(p.relative_to(P)): digest(p) for p in sorted((P / 'build/gen').rglob('*')) if p.is_file()},
              compiler={'host_path': str(cc), 'host_sha256': digest(cc),
                        'host_version': host_version, 'target_path': str(target_cc),
                        'target_sha256': digest(target_cc)},
              environment=environment, image_id=image, stage2_artifacts=artifacts,
              stage2_logs=logs, stage2_host_commands=56,
              replay_run_in_stage3=False, target_build_in_stage3=False,
              result='PASS',
              setup_notes=['The first identity check compared the host compiler with the target compiler hash. The corrected check compares each compiler with its recorded identity.'],
              limit='Stage2 build evidence is not a stage4 validated install record.',
              physical_result='NOT TESTED', original_input_failure='UNRESOLVED')
(E / 'input-review.json').write_text(json.dumps(record, indent=2) + '\n')
print(json.dumps({'result': 'PASS', 'source_inputs': len(source), 'replay_inputs': len(verified_replay),
                  'stage2_logs': len(logs), 'artifacts': 6, 'new_target_builds': 0, 'new_replays': 0}))
