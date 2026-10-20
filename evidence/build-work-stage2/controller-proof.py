#!/usr/bin/env python3
"""Compare one parallel controller build against the saved checked image."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
BASE = Path('/Users/mac/Documents/x0x37/smc_complete_stage4_20261021/evidence/complete-stage4/elite')
SOURCE = Path('/Users/mac/Documents/x0x37/smc_complete_stage4_20261021')
IMAGE = 'sha256:0cffe038efa0f50556de3fb444de6386729d495f63703c8c3b40fdfd0482d4b1'
TC = Path('/Users/mac/.jieli/toolchain')
SDK = Path('/Users/mac/fw-AC79_AIoT_SDK')
SPEC = importlib.util.spec_from_file_location('identity', ROOT / 'tools/validation_inputs.py')
IDENTITY = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(IDENTITY)

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def run(argv):
    return subprocess.check_output(argv, cwd=ROOT, text=True).strip()

record = json.loads((BASE / 'record.json').read_text())
assert record['result'] == 'DEV CHECKED'
assert sha(BASE / 'controller.log') == record['logs']['controller']['sha256']
assert sha(BASE / 'ctl.img') == record['artifacts']['controller']['sha256']
# Only the orchestration scripts may differ from the checked controller inputs.
paths = run(['git', 'ls-tree', '-r', '--name-only', record['commit'], '--', 'ble/ctl', 'firmware/hal/x0x_ble_abi.h']).splitlines()
sources = {}
for path in paths:
    if path in ('ble/ctl/build_in_container.sh', 'ble/ctl/build_ctl.sh'):
        continue
    original = subprocess.check_output(['git', 'show', record['commit'] + ':' + path], cwd=ROOT)
    assert (ROOT / path).read_bytes() == original, path
    assert (SOURCE / path).read_bytes() == original, path
    sources[path] = sha(ROOT / path)
deps = {'toolchain': IDENTITY.content(TC), 'sdk': IDENTITY.content(SDK)}
for name, identity in deps.items():
    assert identity == record['validation_inputs']['inputs']['dependencies'][name], name
assert run(['docker', 'image', 'inspect', IMAGE, '--format', '{{.Id}} {{.Os}}/{{.Architecture}}']) == IMAGE + ' linux/amd64'
proof = {'baseline_record': str(BASE / 'record.json'), 'baseline_record_sha256': sha(BASE / 'record.json'),
         'baseline_controller_sha256': sha(BASE / 'ctl.img'), 'sources': sources,
         'dependencies': deps, 'worker_image': IMAGE, 'jobs': 4,
         'orchestration': {p: sha(ROOT / p) for p in ('ble/ctl/build_ctl.sh', 'ble/ctl/build_in_container.sh')},
         'limits': 'One new controller build. No new old baseline, application build, gate or device check. Other project lanes may use CPU/I/O concurrently. No isolated speedup claim.'}
(OUT / 'controller-before.json').write_text(json.dumps(proof, indent=2) + '\n')
argv = ['docker', 'run', '--rm', '--platform', 'linux/amd64',
        '-v', str(ROOT) + ':' + str(ROOT), '-v', str(TC) + ':' + str(TC) + ':ro',
        '-v', str(SDK) + ':' + str(SDK) + ':ro', '-w', str(ROOT),
        '-e', 'CTL_NATIVE=1', '-e', 'CTL_NO_CACHE=1', '-e', 'X0X_JOBS=4',
        '-e', 'JIELI_TOOLCHAIN=' + str(TC), '-e', 'SDK_LOCAL=' + str(SDK),
        '-e', 'CC=cc', '-e', 'CXX=c++', IMAGE, 'sh', 'ble/ctl/build_ctl.sh']
start = time.monotonic()
p = subprocess.run(argv, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
elapsed = time.monotonic() - start
(OUT / 'controller.log').write_bytes(p.stdout)
proof.update(argv=argv, elapsed_seconds=elapsed, returncode=p.returncode,
             log_sha256=sha(OUT / 'controller.log'))
assert p.returncode == 0, p.stdout.decode(errors='replace')
proof['controller_sha256'] = sha(ROOT / 'ble/ctl/out/ctl.img')
proof['matches_saved_controller'] = proof['controller_sha256'] == proof['baseline_controller_sha256']
assert proof['matches_saved_controller']
assert deps == {'toolchain': IDENTITY.content(TC), 'sdk': IDENTITY.content(SDK)}
for path, digest in sources.items():
    assert sha(ROOT / path) == digest, path
# Fetched SDK files are independently checked by fetch_sdk.sh; generated tables
# can be checked against its pinned source generator and the baseline checkout.
proof['generated_rf_tables_sha256'] = sha(ROOT / 'ble/ctl/gen/rf_tables.c')
(OUT / 'controller-result.json').write_text(json.dumps(proof, indent=2) + '\n')
print(json.dumps({k: proof[k] for k in ('returncode', 'elapsed_seconds', 'controller_sha256', 'matches_saved_controller')}, indent=2))
