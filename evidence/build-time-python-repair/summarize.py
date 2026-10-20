#!/usr/bin/env python3
"""Save checked source identities, scoped results and retained-evidence limits."""
from pathlib import Path
import ast
import hashlib
import json
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
BASE = '6d690e347b2c66d331f0b6da58a390b627ee84b7'
def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def git(*args):
    return subprocess.check_output(['git', '-C', str(ROOT), *args], text=True).strip()
checks = json.loads((OUT/'checks.json').read_text())
for check in checks['checks']:
    assert check['returncode'] == 0, check
    assert sha(OUT/check['log']) == check['sha256'], check
def function(source, name):
    return ast.dump(next(n for n in ast.parse(source).body if isinstance(n, ast.FunctionDef) and n.name == name))
guards = ['tools/dev_validate.py', 'tools/require_gate_pass.sh', 'tools/fm1_install.py',
          'tools/build.py', 'tools/gen_font.py', 'ble/ctl/build_ctl.sh', 'ble/ctl/build_in_container.sh']
unchanged = {}
for path in guards:
    before = subprocess.check_output(['git', '-C', str(ROOT), 'show', BASE+':'+path])
    assert before == (ROOT/path).read_bytes(), path
    unchanged[path] = sha(ROOT/path)
assert function((ROOT/'tools/dev_validate.py').read_text(), 'verify') == function(
    subprocess.check_output(['git', '-C', str(ROOT), 'show', '6affed8:tools/dev_validate.py'], text=True), 'verify')
old_checks = json.loads((ROOT/'evidence/build-time-stage2/checks.json').read_text())
retained = []
for row in old_checks:
    path = ROOT/'evidence/build-time-stage2'/(row['name']+'.log')
    assert sha(path) == row['log_sha256'], path
    retained.append({'log': str(path.relative_to(ROOT)), 'sha256': sha(path),
                     'returncode': row['returncode']})
files = ['tools/validation_inputs.py', 'tests/host/test_validation_inputs.py',
         'tests/host/test_dev_validate.py', 'BUILDING.md']
inputs = {'base': BASE, 'source_hashes': {p: sha(ROOT/p) for p in files},
          'unchanged_consumers_and_generators': unchanged, 'verify_AST_matches_audit': True,
          'host_python': {'path': sys.executable, 'sha256': sha(sys.executable)},
          'worker_image': checks['image'], 'worker_runtime_and_pillow_byte_proof': 'package-proof.log',
          'dependency_identities_and_walk_timer': 'walker-proof.json',
          'sdk_commit': subprocess.check_output(['git', '-C', '/Users/mac/fw-AC79_AIoT_SDK', 'rev-parse', 'HEAD'], text=True).strip(),
          'sdk_tracked_status': subprocess.check_output(['git', '-C', '/Users/mac/fw-AC79_AIoT_SDK', 'status', '--porcelain', '--untracked-files=no'], text=True).strip()}
(OUT/'inputs.json').write_text(json.dumps(inputs, indent=2)+'\n')
result = {'workflow': 'build-time-20261021', 'marker': 'build-time-20261021-stage2-python-manifest-repair',
          'stage': 2, 'status': 'CHECKED REPAIR', 'source_chat': '01a15ae0-185b-7b40-b1e8-9a19c0eca02d',
          'lane': str(ROOT), 'branch': git('branch', '--show-current'), 'base': BASE,
          'fixes': ['I2: Hash installed declared modules when distro metadata omits or empties the manifest; include source, extensions and package data without importing modules.',
                    'Refuse absent/unsafe/unresolvable installed-module coverage, empty fingerprints and missing declared manifest files. Preserve optional absent distributions as null.',
                    'Use the audited direct directory traversal while preserving byte/mode/exclusion/symlink identities. Rehash bytes every capture; no mtime cache.'],
          'inputs': 'inputs.json', 'checks': 'checks.json',
          'coverage': {'input_cases_host': 14, 'input_cases_linux': 14,
                       'host_dev_cases': '14 original cases plus 1 new fail-closed consumer case',
                       'linux_native_dev_cases': '14 original cases plus 1 new fail-closed consumer case, no platform mocking',
                       'real_worker_cases': '7: actual provenance, source mutation, binary mutation, transferred consumer authentication, fail closed, timeout cleanup and signal cleanup',
                       'actual_package_proof': 'Pinned distro Pillow source and compiled _imaging bytes each invalidate runtime; restored bytes restore exact runtime identity.',
                       'walker_equivalence': 'Frozen original algorithm versus direct walker on scratch aliases/modes/bytes/exclusions/cycle/dangling/FIFO cases and actual SDK/toolchain.'},
          'setup_failures': ['Initial Docker socket access required approval, which was granted. No denied attempt is counted as passing.',
                             'Linux shared-mount scratch files triggered Git dubious ownership and unsupported FIFO creation (linux-inputs-attempt1.log; setup-attempt.json).',
                             'Default Linux tmpfs was noexec, so fake emcc was not discoverable (linux-inputs-attempt2.log; setup-attempt2.json).',
                             'Controlled fixture retry uses read-only same-path source and exec-enabled Linux scratch tmpfs at build/. No HOME or global Git setting changed.'],
          'reused_checks': {'logs': retained,
                            'scope': 'Original checks are retained historical coverage for unchanged plan/record/queue/installer/worker lifecycle code. Changed package/runtime and direct-walker behavior is covered by the new checks. Original native/input results are superseded by current execution; no original run proves I2.'},
          'I1_declaration_contract': 'Every additional consumed worker mount must be listed in plan.dependencies. Mount path/access metadata alone does not hash all mounted bytes.',
          'authorization': {'owner_request_source_chat': '01a15a99-75d7-7921-9d65-f4b120566904',
                            'owner_quote': 'I grant you full access for this project indefinitely for development purposes',
                            'quote_source_chat': '01a15ac7-17e4-7633-b9a5-6652fa0aefed',
                            'optional_walker_guidance': '/Users/mac/Documents/x0x37/build_time_stage3/evidence/build-time-stage3/repair-handoff-supplement.json'},
          'limits': ['No target build, exact firmware/controller comparison, end-to-end timing, full/release gate, device install, firmware integration or external push.',
                     'SDK traversal timers are one sequential local probe with possible warm-cache effects, not full validation speed.',
                     'Linux fixture tmpfs, scratch paths and per-worker HOSTNAME are recorded bookkeeping; real comparison inputs must still match.',
                     'Fresh independent stage3 must test the complete checked repair before stage4.'],
          'routing_update': {'source_chat': '01a15b05-73df-7fb1-a9f7-6bb1484cfd02',
                             'current_rule': '/Users/mac/Documents/x0x37/.agents/skills/x0x37-task/references/routing.md',
                             'disposition': 'Complete the requested independent test and investigation scopes in this bounded execution task; do not launch an automatic Codex stage chain. Save the remaining validation handoff for classic ChatGPT in LoopyX0X37.'},
          'next_stage': 3, 'next_owner': 'Separate independent test and investigation subagents in this bounded execution task.'}
(OUT/'result.json').write_text(json.dumps(result, indent=2)+'\n')
print('Checked commands, input hashes, unchanged guards and retained log hashes verified.')
