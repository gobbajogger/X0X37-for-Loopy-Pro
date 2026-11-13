#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Replay reviewed native MIDI software checks in a disposable local clone.

Never invokes the hardware launcher, imports mido, rebuilds firmware, or fetches.
Optional private inputs stay local and are never included in the result receipt.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
BASE = '54f2ffbc3124f7449a69067dc8215a2d91a2e675'
TREE = '3b5cadafba33f4c1b116660bc9d19a27a892337c'
PATCH_SHA = '584ea440bc642b5e741d12f69c135a0049056f71cb620b900535930e6d4ea01a'
UPDATER_SHA = '7e5056036aeae47cf2ce2175de4eef7c255b897349eff8153ef7753c8f775dfa'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--private-evidence', type=Path,
                        help='optional original qualified local preparation directory')
    parser.add_argument('--result', type=Path, required=True,
                        help='new JSON result; contains no private paths or command output')
    args = parser.parse_args()
    if args.result.exists():
        parser.error('--result must be a new path')
    repo = Path(subprocess.check_output(
        ['git', '-C', str(HERE), 'rev-parse', '--show-toplevel'], text=True).strip())
    patch = HERE / 'reviewed-workflow.patch'
    if hashlib.sha256(patch.read_bytes()).hexdigest() != PATCH_SHA:
        raise ValueError('reviewed patch changed')
    env = dict(os.environ, PYTHONDONTWRITEBYTECODE='1')
    rows = []
    with tempfile.TemporaryDirectory(prefix='native-midi-replay-') as name:
        root = Path(name) / 'source'
        subprocess.run(['git', 'clone', '--quiet', '--shared', '--no-checkout',
                        str(repo), str(root)], check=True, env=env)
        subprocess.run(['git', '-C', str(root), 'checkout', '--quiet', '--detach', BASE],
                       check=True, env=env)
        actual = subprocess.check_output(['git', '-C', str(root), 'rev-parse', 'HEAD^{tree}'],
                                         text=True).strip()
        if actual != TREE:
            raise ValueError('replay base tree differs')
        for extra in (['--check'], []):
            subprocess.run(['git', '-C', str(root), 'apply', *extra, str(patch)], check=True)
        if hashlib.sha256((root/'tools/fm1_install.py').read_bytes()).hexdigest() != UPDATER_SHA:
            raise ValueError('replayed updater differs from hardware-tested bytes')
        checks = [
            ('transport_boundaries', [sys.executable, '-B', str(HERE/'tests/transport_test.py'), str(root)]),
            *[(test, [sys.executable, '-B', test]) for test in (
                'tests/install_test.py', 'tests/install_ctl_test.py',
                'tests/install_slots_test.py', 'tests/x0x_slots_test.py',
                'tests/host/test_dev_validate.py', 'tests/host/test_validation_inputs.py')],
            *[(test, ['bash', test]) for test in (
                'tests/host/test_install_head.sh', 'tests/host/test_install_gate_guard.sh',
                'tests/host/test_queue_guard.sh')],
        ]
        if args.private_evidence:
            private = args.private_evidence.resolve(strict=True)
            # Only read originals. Tests mutate their own temporary copies.
            for target in ('inputs', 'review'):
                (root/target).symlink_to(private/target, target_is_directory=True)
            (root/'prep-evidence/accepted-independent-qa.json').symlink_to(
                private/'prep-evidence/accepted-independent-qa.json')
            checks += [
                ('private_head_boundaries', [sys.executable, '-B',
                    str(HERE/'tests/head_boundary_test.py'), str(root),
                    str(private/'inputs/elite-dev.fwsc'), str(private/'inputs/reference-flash.bin')]),
                ('private_build_mapping', [sys.executable, '-B',
                    str(HERE/'tests/build_mapping_test.py'), str(root)]),
            ]
        for label, command in checks:
            result = subprocess.run(command, cwd=root, env=env, capture_output=True)
            rows.append({'check': label, 'exit': result.returncode,
                         'output_sha256': hashlib.sha256(result.stdout+result.stderr).hexdigest()})
            print(f'{label}: exit {result.returncode}', flush=True)
            if result.returncode:
                # Output remains local; do not place paths/private output in public receipts.
                sys.stderr.buffer.write(result.stdout+result.stderr)
    receipt = {'base_commit': BASE, 'base_tree': TREE, 'patch_sha256': PATCH_SHA,
               'replayed_updater_sha256': UPDATER_SHA, 'checks': rows,
               'private_fixture_checks': 'RUN' if args.private_evidence else 'NOT_RUN',
               'hardware_io': False, 'firmware_build': False,
               'limitations': ['install_test package/Node parity may skip without a built fixture',
                              'Darwin skips Linux-only dev_validate runner cases',
                              'host simulations do not establish physical-device continuity']}
    with args.result.open('x') as output:
        json.dump(receipt, output, indent=2)
        output.write('\n')
    return int(any(row['exit'] for row in rows))


if __name__ == '__main__':
    sys.exit(main())
