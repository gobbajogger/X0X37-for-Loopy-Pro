#!/usr/bin/env python3
"""Scoped scratch checks for the package-byte repair. No target builds or gates."""
from pathlib import Path
import hashlib
import json
import os
import platform
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
IMAGE = 'sha256:0cffe038efa0f50556de3fb444de6386729d495f63703c8c3b40fdfd0482d4b1'
env = dict(os.environ, CC='cc', CXX='c++', PYTHONDONTWRITEBYTECODE='1', X0X_TEST_WORKER_IMAGE=IMAGE)
docker = ['docker', 'run', '--rm', '--platform', 'linux/amd64', '--mount',
          'type=bind,source='+str(ROOT)+',target='+str(ROOT)+',readonly',
          '--tmpfs', str(ROOT/'build')+':rw,exec', '-w', str(ROOT),
          '-e', 'CC=cc', '-e', 'CXX=c++', '-e', 'PYTHONDONTWRITEBYTECODE=1',
          '--entrypoint', 'python3', IMAGE, '-B']
checks = [
    ('image', ['docker', 'image', 'inspect', IMAGE, '--format', '{{.Id}} {{.Os}}/{{.Architecture}}']),
    ('host-inputs', [sys.executable, '-B', '-m', 'unittest', 'tests.host.test_validation_inputs']),
    ('host-dev', [sys.executable, '-B', '-m', 'unittest', 'tests.host.test_dev_validate.DevelopmentValidation']),
    ('host-dev-fail-closed', [sys.executable, '-B', '-m', 'unittest',
        'tests.host.test_dev_validate.DevelopmentValidation.test_unenumerable_python_package_issues_no_record']),
    ('linux-inputs', docker+['-m', 'unittest', 'tests.host.test_validation_inputs']),
    ('linux-native-dev', docker+['-m', 'unittest', 'tests.host.test_dev_validate.NativeLinuxValidation']),
    ('linux-native-fail-closed', docker+['-m', 'unittest',
        'tests.host.test_dev_validate.NativeLinuxValidation.test_unenumerable_python_package_issues_no_record']),
    ('package-proof', docker+[str(OUT/'package-proof.py')]),
    ('worker-affected', [sys.executable, '-B', '-m', 'unittest',
        'tests.host.test_dev_validate.WorkerValidation.test_worker_provenance_and_consumer_authentication',
        'tests.host.test_dev_validate.WorkerValidation.test_installed_pillow_source_mutation_issues_no_record',
        'tests.host.test_dev_validate.WorkerValidation.test_installed_pillow_binary_mutation_issues_no_record',
        'tests.host.test_dev_validate.WorkerValidation.test_identity_required_and_transfer_does_not_need_build_dependencies']),
    ('worker-additional', [sys.executable, '-B', '-m', 'unittest',
        'tests.host.test_dev_validate.WorkerValidation.test_unenumerable_python_package_issues_no_record',
        'tests.host.test_dev_validate.WorkerValidation.test_timeout_kills_owned_child',
        'tests.host.test_dev_validate.WorkerValidation.test_signal_removes_worker_before_scratch']),
    ('diff', ['git', 'diff', '--check']),
]
results = {'host': platform.platform(), 'python': sys.executable, 'image': IMAGE,
           'environment_overrides': {k: env[k] for k in ('CC', 'CXX', 'PYTHONDONTWRITEBYTECODE', 'X0X_TEST_WORKER_IMAGE')},
           'timer': 'perf_counter around each complete subprocess.run; no firmware validation timing', 'checks': []}
if '--resume' in sys.argv:
    results['checks'] = json.loads((OUT/'checks.json').read_text())['checks']
for name, argv in checks:
    previous = next((r for r in results['checks'] if r['name'] == name and not r['returncode']), None)
    if previous and hashlib.sha256((OUT/previous['log']).read_bytes()).hexdigest() == previous['sha256']:
        print(name+': retained matching successful check', flush=True)
        continue
    start = time.perf_counter()
    with (OUT/(name+'.log')).open('w') as log:
        process = subprocess.run(argv, cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=600)
    row = {'name': name, 'argv': argv, 'returncode': process.returncode,
           'seconds': time.perf_counter()-start, 'log': name+'.log',
           'sha256': hashlib.sha256((OUT/(name+'.log')).read_bytes()).hexdigest()}
    results['checks'] = [r for r in results['checks'] if r['name'] != name] + [row]
    (OUT/'checks.json').write_text(json.dumps(results, indent=2)+'\n')
    print(name+': '+str(process.returncode)+' ('+str(round(row['seconds'], 3))+'s)', flush=True)
    if process.returncode:
        sys.exit(process.returncode)
