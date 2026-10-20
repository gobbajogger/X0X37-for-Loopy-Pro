#!/usr/bin/env python3
"""Verify candidate/reused evidence and write the independent test result."""
from pathlib import Path
import hashlib
import json
import subprocess

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
CANDIDATE = 'cadeae04bdc9160cfe15a74427829165e36ba60b'
IMAGE = 'sha256:0cffe038efa0f50556de3fb444de6386729d495f63703c8c3b40fdfd0482d4b1'
def sha(data):
    return hashlib.sha256(data).hexdigest()
def git(*argv):
    return subprocess.check_output(['git', '-C', str(ROOT), *argv])
inputs = json.loads((OUT.parent/'inputs.json').read_text())
source = {}
for path, expected in {**inputs['source_hashes'], **inputs['unchanged_consumers_and_generators']}.items():
    committed = sha(git('show', CANDIDATE+':'+path))
    working = sha((ROOT/path).read_bytes())
    assert expected == committed == working, path
    source[path] = {'sha256': expected, 'commit_matches': True, 'working_matches': True}
retained = []
checks = json.loads((OUT.parent/'checks.json').read_text())
for row in checks['checks']:
    path = OUT.parent/row['log']
    assert row['returncode'] == 0, row
    assert sha(path.read_bytes()) == row['sha256'], path
    retained.append({'name': row['name'], 'log': str(path), 'sha256': row['sha256'],
                     'returncode': row['returncode'], 'argv': row['argv']})
assert (OUT/'image.log').read_text().strip() == IMAGE+' linux/amd64'
host = ['python3', '-B', '-m', 'unittest', '-v',
        'tests.host.test_validation_inputs.PackageBytes', 'tests.host.test_validation_inputs.ContentWalk']
linux = ['docker', 'run', '--rm', '--platform', 'linux/amd64', '--mount',
         'type=bind,source='+str(ROOT)+',target='+str(ROOT)+',readonly',
         '--tmpfs', str(ROOT/'build')+':rw,exec', '-w', str(ROOT),
         '-e', 'CC=cc', '-e', 'CXX=c++', '-e', 'PYTHONDONTWRITEBYTECODE=1',
         '--entrypoint', 'python3', IMAGE, '-B', str(OUT/'linux-probe.py')]
assert 'Ran 9 tests' in (OUT/'host.log').read_text() and '\nOK\n' in (OUT/'host.log').read_text()
assert 'Ran 14 tests' in (OUT/'linux.log').read_text() and '\nOK\n' in (OUT/'linux.log').read_text()
result = {
    'workflow': 'build-time-20261021', 'scope': 'Independent Test in the current bounded execution task',
    'status': 'PASS', 'candidate': CANDIDATE, 'source_changed': False,
    'source_hashes': source,
    'commands': [
        {'argv': host, 'cwd': str(ROOT), 'returncode': 0, 'log': 'host.log',
         'sha256': sha((OUT/'host.log').read_bytes()), 'cases': 9,
         'risk': 'Missing/empty manifests, ordinary declared files, unsafe metadata, optional absent packages and exact content-walker behavior.'},
        {'argv': linux, 'cwd': str(ROOT), 'returncode': 0, 'log': 'linux.log',
         'sha256': sha((OUT/'linux.log').read_bytes()), 'cases': 14,
         'risk': 'The same package/walker fixtures on actual Linux, direct native success/queue guards, dependency mutation, fail-closed metadata and installed Pillow source/binary mutation.'},
        {'argv': ['docker', 'image', 'inspect', IMAGE, '--format', '{{.Id}} {{.Os}}/{{.Architecture}}'],
         'returncode': 0, 'log': 'image.log', 'sha256': sha((OUT/'image.log').read_bytes())}
    ],
    'independent_installed_mutations': [
        {'path': '/usr/lib/python3/dist-packages/PIL/__init__.py',
         'before': 'dd9f25c2ad1d6a7444ed64056716bbbccbdf4a3bff0b8f90ef7154aff807b786',
         'after': 'd52193b03a6b73fc236a265e399bb71df05d169c846c87c4dd02535b4555fe17',
         'returncode': 3, 'record_issued': False},
        {'path': '/usr/lib/python3/dist-packages/PIL/_imaging.cpython-311-x86_64-linux-gnu.so',
         'before': '0aced06a23f7166ee001346828b253c82d3e0afd05cbc220546775c9822ab2aa',
         'after': '2247c153d10c26de5181c07bcde0f932b50cb9ff9f726f9872c3dffe0388019e',
         'returncode': 3, 'record_issued': False}
    ],
    'retained_checked_evidence': retained,
    'reuse_scope': 'Checked source hashes and all retained log hashes match. Reuse the current repair checks for unaffected cases and actual LinuxWorker orchestration/cleanup. Independent execution reran the new package cases and real Linux-native source/binary mutation refusal. No old pre-repair result establishes the missing-manifest fix.',
    'failures': [], 'blocked_tool_execution': [],
    'environment': {'image': IMAGE, 'platform': 'linux/amd64', 'CC': 'cc', 'CXX': 'c++',
                    'source_mount': 'same absolute path, read only',
                    'scratch': 'same build/ path on executable container-local tmpfs',
                    'HOME_changed': False},
    'limits': ['Independent new mutation tests exercise actual Linux-native execution in the pinned Docker image; LinuxWorker orchestration cases are retained matching current-repair evidence.',
               'No target firmware/controller build, artifact equality, end-to-end timing, formal gate, install, firmware integration or push.',
               'Extra consumed worker mounts still require explicit plan.dependencies declarations.',
               'Unrelated AGENTS.md/docs/SKILLS.md and skill-discovery rollout changes were preserved and not included in this source scope.'],
    'routing': 'Complete this requested independent scope here. No successor Codex chat was created under the current routing rules.'
}
(OUT/'test-result.json').write_text(json.dumps(result, indent=2)+'\n')
print('PASS: independent results, candidate source hashes and all retained current-repair logs match.')
