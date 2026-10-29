#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Summarize an actual X0X host-suite run without hiding or reclassifying FAILs."""
import argparse
import json
from collections import Counter
from pathlib import Path
import re
import subprocess

# The suite emits exactly two leading spaces. An indented compiler/assertion
# tail in serial or .results mode is diagnostic text, never another test result.
STATUS = re.compile(r'^ {2}(ok|FAIL|not-run) +([a-zA-Z0-9_][a-zA-Z0-9_.+-]*)\b')
ERROR = re.compile(r'(?:fatal error:|error:|\bFAIL\b|AssertionError|assertion failed|Traceback|ModuleNotFoundError|undefined reference|ld:|No such file or directory|command not found|AddressSanitizer|UndefinedBehaviorSanitizer)', re.I)


def parse_status(lines):
    states = {}
    for line in lines:
        match = STATUS.match(line)
        if match:
            states[match.group(2)] = match.group(1)
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
        states.update(parse_status(read_transcript(item)))
    return states


def describe_failure(logdir, name):
    path = logdir / (name + '.log')
    if not path.is_file():
        return {'log': str(path), 'error': 'LOG MISSING', 'lines': []}
    lines = read_transcript(path)
    matches = [line.strip() for line in lines if ERROR.search(line)]
    # Preserve an actual compiler/assertion line. Never infer success from missing errors.
    diagnosis = (matches[0] if matches else (lines[-1].strip() if lines else 'EMPTY LOG'))
    return {'log': str(path), 'error': diagnosis[:500], 'lines': matches[:12] if matches else lines[-8:]}


def build_summary(logdir, transcript=None, baseline=None):
    if transcript is not None:
        lines = read_transcript(transcript)
        states = parse_status(lines)
        if not states:
            raise ValueError('Transcript contains no X0X test status lines')
        status_source = str(transcript)
    else:
        states = status_from_results(logdir / '.results')
        if not states:
            raise ValueError('No suite results found. Supply --transcript from an actual run')
        status_source = str(logdir / '.results')
    counts = Counter(states.values())
    failed_names = sorted(name for name, state in states.items() if state == 'FAIL')
    failed = {name: describe_failure(logdir, name) for name in failed_names}
    try:
        sha = subprocess.check_output(['git', 'rev-parse', 'HEAD'], stderr=subprocess.DEVNULL, text=True).strip()
    except (OSError, subprocess.CalledProcessError):
        sha = None
    report = {
        'schema': 'x0x37-host-failure-summary-v1',
        'head': sha,
        'status_source': status_source,
        'counts': {key: counts.get(key, 0) for key in ('ok', 'FAIL', 'not-run')},
        'failures': failed,
    }
    if baseline:
        prior = json.loads(baseline.read_text(encoding='utf-8'))
        old = set(prior.get('failures', {}))
        current = set(failed_names)
        report['comparison'] = {
            'new_failures': sorted(current - old),
            'preexisting_failures': sorted(current & old),
            'cleared_failures': sorted(old - current),
        }
    return report


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--logdir', type=Path, default=Path('build/host'))
    ap.add_argument('--transcript', type=Path, help='full terminal transcript from tests/run_tests.sh; required for serial mode')
    ap.add_argument('--baseline-report', type=Path, help='JSON report from the identical suite against the original base')
    ap.add_argument('--output', type=Path, default=Path('build/host/failure-summary.json'))
    args = ap.parse_args()
    try:
        report = build_summary(args.logdir, args.transcript, args.baseline_report)
    except (ValueError, OSError, json.JSONDecodeError) as exc:
        ap.error(str(exc))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    print('HOST TEST SUMMARY: ' + ', '.join(f'{k}={v}' for k, v in report['counts'].items()))
    if report.get('comparison'):
        for key, names in report['comparison'].items():
            print(f'{key}: {len(names)} ' + ' '.join(names))
    for name, result in report['failures'].items():
        print(f'FAIL {name}: {result["error"]}')
    print(f'Report: {args.output}')
    # Diagnostics should not conceal an upstream failed suite.
    return 1 if report['failures'] else 0


if __name__ == '__main__':
    raise SystemExit(main())
