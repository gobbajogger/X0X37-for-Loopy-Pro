#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Inspect genuine host-suite results without hiding failures or inventing clears.

A later git checkout is not evidence of the revision that ran an old transcript.
Use --run-record from run_host_with_evidence.py for source-bound comparisons.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import subprocess

# The runner uses precisely two leading spaces; nested failure diagnostics are
# indented more deeply, and must not become additional test results.
STATUS = re.compile(r'^ {2}(ok|FAIL|not-run) +([a-zA-Z0-9_][a-zA-Z0-9_.+-]*)\b')
ERROR = re.compile(
    r'(?:fatal error:|error:|\bFAIL\b|AssertionError|assertion failed|Traceback|'
    r'ModuleNotFoundError|undefined reference|ld:|No such file or directory|'
    r'command not found|AddressSanitizer|UndefinedBehaviorSanitizer)', re.I)
RUN_SCHEMA = 'x0x37-host-run-record-v2'
REPORT_SCHEMA = 'x0x37-host-failure-summary-v3'


def parse_status(lines):
    states = {}
    for line in lines:
        match = STATUS.match(line)
        if not match:
            continue
        name = match.group(2)
        if name in states:
            # Never overwrite a prior FAIL with an appended OK.
            raise ValueError('Duplicate host test result for ' + name)
        states[name] = match.group(1)
    return states


def read_transcript(path):
    return path.read_text(encoding='utf-8', errors='replace').splitlines()


def status_from_results(directory):
    states = {}
    if not directory.is_dir():
        return states
    for item in sorted(directory.iterdir()):
        if not item.is_file() or item.name.endswith(('.fail', '.time')):
            continue
        found = parse_status(read_transcript(item))
        for name, state in found.items():
            if name in states:
                raise ValueError('Duplicate host test result for ' + name)
            states[name] = state
    return states


def describe_failure(logdir, name):
    path = logdir / (name + '.log')
    if not path.is_file():
        return {'log': str(path), 'error': 'LOG MISSING', 'lines': []}
    lines = read_transcript(path)
    matches = [line.strip() for line in lines if ERROR.search(line)]
    diagnosis = matches[0] if matches else lines[-1].strip() if lines else 'EMPTY LOG'
    return {'log': str(path), 'error': diagnosis[:500],
            'lines': matches[:12] if matches else lines[-8:]}


def plan_hash(states):
    """Same-name test selection, including not-run entries, for comparison."""
    return hashlib.sha256(('\n'.join(sorted(states)) + '\n').encode('utf-8')).hexdigest()


def git_head(root=None):
    try:
        return subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root,
                                       stderr=subprocess.DEVNULL, text=True).strip()
    except (OSError, subprocess.CalledProcessError):
        return None


def assert_clean_source(root=None):
    """Fail closed on staged, tracked or non-ignored untracked source edits."""
    try:
        result = subprocess.run(
            ['git', 'status', '--porcelain=v1', '--untracked-files=all',
             '--ignore-submodules=none'],
            cwd=root if root is not None else Path.cwd(),
            capture_output=True, text=True, check=True)
    except (OSError, subprocess.CalledProcessError) as exc:
        raise ValueError('Cannot verify source cleanliness: ' + str(exc)) from exc
    if result.stdout:
        raise ValueError('Source worktree/index is dirty: ' +
                         result.stdout.splitlines()[0])


def validate_run_record(record, states, transcript):
    if transcript is None:
        raise ValueError('A run record requires the exact full transcript')
    if record.get('schema') != RUN_SCHEMA:
        raise ValueError('Unexpected run-record schema')
    head = record.get('head', '')
    if not re.fullmatch('[0-9a-f]{40}', head):
        raise ValueError('Invalid run-record source commit')
    root = record.get('root')
    if not isinstance(root, str) or not Path(root).is_absolute() or not Path(root).is_dir():
        raise ValueError('Run record has no valid source root')
    # Check the repository that ran the suite, not the current directory.
    if git_head(root) != head:
        raise ValueError('Run record was created at a different Git HEAD')
    if record.get('source_clean_before_after') is not True:
        raise ValueError('Missing clean-source pre/post-run proof')
    assert_clean_source(root)
    actual_sha = hashlib.sha256(transcript.read_bytes()).hexdigest()
    if record.get('transcript_sha256') != actual_sha:
        raise ValueError('Run-record transcript SHA-256 mismatch')
    if record.get('selected_tests') != sorted(states):
        raise ValueError('Run-record selected tests differ from observed results')
    if record.get('plan_sha256') != plan_hash(states):
        raise ValueError('Run-record test-plan SHA-256 mismatch')
    command = record.get('command')
    if not isinstance(command, list) or command[:2] != ['sh', 'tests/run_tests.sh']:
        raise ValueError('Run record does not identify the actual host runner')
    rc = record.get('exit_code')
    if type(rc) is not int or (rc == 0) != ('FAIL' not in states.values()):
        raise ValueError('Run-record exit status does not match host test failures')
    return actual_sha


def compare_baseline(report, baseline):
    old_states = baseline.get('statuses')
    if not isinstance(old_states, dict):
        return {'status': 'UNVERIFIED_BASELINE', 'cleared_failures': [],
                'reason': 'Baseline lacks per-test execution statuses'}
    old_failed = {n for n, v in old_states.items() if v == 'FAIL'}
    current = report['statuses']
    unresolved_skipped = sorted(n for n in old_failed if current.get(n) == 'not-run')
    unresolved_unselected = sorted(n for n in old_failed if n not in current)
    if not (report['source_verified'] and baseline.get('source_verified') is True
            and re.fullmatch('[0-9a-f]{40}', baseline.get('head') or '')
            and re.fullmatch('[0-9a-f]{64}', baseline.get('transcript_sha256') or '')
            and baseline.get('source_clean_before_after') is True):
        return {'status': 'UNVERIFIED_PROVENANCE', 'cleared_failures': [],
                'unresolved_not_run': unresolved_skipped,
                'unresolved_not_selected': unresolved_unselected}
    if baseline.get('plan_sha256') != report['plan_sha256'] or set(old_states) != set(current):
        return {'status': 'INCOMPARABLE_TEST_SELECTION', 'cleared_failures': [],
                'unresolved_not_run': unresolved_skipped,
                'unresolved_not_selected': unresolved_unselected,
                'baseline_only': sorted(set(old_states) - set(current)),
                'current_only': sorted(set(current) - set(old_states))}
    new_failed = {n for n, v in current.items() if v == 'FAIL'}
    return {'status': 'COMPARABLE',
            'new_failures': sorted(new_failed - old_failed),
            'preexisting_failures': sorted(new_failed & old_failed),
            'cleared_failures': sorted(n for n in old_failed if current[n] == 'ok'),
            'unresolved_not_run': unresolved_skipped,
            'unresolved_not_selected': unresolved_unselected}


def build_summary(logdir, transcript=None, baseline=None, run_record=None):
    if transcript is not None:
        states = parse_status(read_transcript(transcript))
        if not states:
            raise ValueError('Transcript contains no X0X test status lines')
        source = str(transcript)
    else:
        states = status_from_results(logdir / '.results')
        if not states:
            raise ValueError('No suite results found. Supply --transcript from an actual run')
        source = str(logdir / '.results')
    sha = validate_run_record(run_record, states, transcript) if run_record is not None else None
    counts = Counter(states.values())
    failures = {n: describe_failure(logdir, n) for n in sorted(states)
                if states[n] == 'FAIL'}
    report = {
        'schema': REPORT_SCHEMA,
        'head': run_record['head'] if sha is not None else None,
        'source_verified': sha is not None,
        'source_clean_before_after': sha is not None,
        'transcript_sha256': sha,
        'plan_sha256': plan_hash(states),
        'status_source': source,
        'statuses': states,
        'counts': {k: counts.get(k, 0) for k in ('ok', 'FAIL', 'not-run')},
        'failures': failures,
    }
    if baseline is not None:
        report['comparison'] = compare_baseline(report, json.loads(baseline.read_text(encoding='utf-8')))
    return report


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--logdir', type=Path, default=Path('build/host'))
    ap.add_argument('--transcript', type=Path)
    ap.add_argument('--run-record', type=Path,
                    help='record captured with run_host_with_evidence.py at the actual tested HEAD')
    ap.add_argument('--baseline-report', type=Path)
    ap.add_argument('--output', type=Path, default=Path('build/host/failure-summary.json'))
    args = ap.parse_args()
    try:
        record = json.loads(args.run_record.read_text(encoding='utf-8')) if args.run_record else None
        result = build_summary(args.logdir, args.transcript, args.baseline_report, record)
    except (ValueError, OSError, json.JSONDecodeError) as exc:
        ap.error(str(exc))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    print('HOST TEST SUMMARY: ' + ', '.join(f'{k}={v}' for k, v in result['counts'].items()))
    if 'comparison' in result:
        comp = result['comparison']
        print('comparison: ' + comp['status'])
        for key in ('new_failures', 'preexisting_failures', 'cleared_failures',
                    'unresolved_not_run', 'unresolved_not_selected'):
            if key in comp:
                print(f'{key}: {len(comp[key])} ' + ' '.join(comp[key]))
    for name, error in result['failures'].items():
        print(f'FAIL {name}: {error["error"]}')
    print('Report: ' + str(args.output))
    return 1 if result['failures'] or (
        result.get('comparison', {}).get('status', 'COMPARABLE') != 'COMPARABLE') else 0


if __name__ == '__main__':
    raise SystemExit(main())
