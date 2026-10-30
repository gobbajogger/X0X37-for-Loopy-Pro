#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Execute the real host suite and bind transcript to its exact starting HEAD.

The suite's original exit code is preserved. The record describes that one run,
not a historical transcript reinterpreted from a later checkout.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from host_failure_summary import (RUN_SCHEMA, assert_clean_source, parse_status,
                                  plan_hash)  # noqa: E402


def checked_head(root):
    head = subprocess.check_output(['git', 'rev-parse', 'HEAD'],
                                   cwd=root, text=True).strip()
    if len(head) != 40:
        raise ValueError('Invalid source Git HEAD')
    return head


def capture(root, destination, jobs, selection):
    if destination.resolve().is_relative_to((root / 'firmware').resolve()):
        raise ValueError('Output must not be inside firmware source')
    head = checked_head(root)
    assert_clean_source(root)
    destination.mkdir(parents=True, exist_ok=True)
    transcript = destination / 'full-host.log'
    command = ['sh', 'tests/run_tests.sh']
    env = os.environ.copy()
    env['X0X_TEST_JOBS'] = str(jobs)
    if selection:
        env['X0X_TESTS'] = selection
    else:
        env.pop('X0X_TESTS', None)
    with transcript.open('wb') as output:
        rc = subprocess.run(command, cwd=root, env=env, stdout=output,
                            stderr=subprocess.STDOUT, check=False).returncode
    if checked_head(root) != head:
        raise ValueError('Repository HEAD changed during host run')
    assert_clean_source(root)
    statuses = parse_status(transcript.read_text(encoding='utf-8', errors='replace').splitlines())
    if not statuses:
        raise ValueError('Host run generated no test statuses; inspect full-host.log')
    record = {
        'schema': RUN_SCHEMA,
        'head': head,
        'root': str(root),
        'source_clean_before_after': True,
        'command': command,
        'runner_env': {'X0X_TEST_JOBS': str(jobs), 'X0X_TESTS': selection},
        'exit_code': rc,
        'transcript_sha256': hashlib.sha256(transcript.read_bytes()).hexdigest(),
        'selected_tests': sorted(statuses),
        'plan_sha256': plan_hash(statuses),
    }
    (destination / 'run-record.json').write_text(json.dumps(record, indent=2) + '\n')
    return rc


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--root', type=Path, default=Path.cwd())
    ap.add_argument('--outdir', type=Path, default=Path('build/host/evidence'))
    ap.add_argument('--jobs', type=int, default=8)
    ap.add_argument('--tests', default='')
    args = ap.parse_args()
    if args.jobs < 1 or args.jobs > 64:
        ap.error('--jobs must be 1..64')
    try:
        rc = capture(args.root.resolve(), args.outdir.resolve(), args.jobs, args.tests)
    except (ValueError, OSError, subprocess.CalledProcessError) as exc:
        ap.error(str(exc))
    print('Host run exit code:', rc)
    print('Evidence:', args.outdir.resolve())
    return rc


if __name__ == '__main__':
    raise SystemExit(main())
