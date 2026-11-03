#!/usr/bin/env python3
"""Timer-free QA2 consumer diagnostic; never a firmware qualification run."""
import argparse
import csv
import hashlib
import importlib.util
import json
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

PIN = '8be16486e33f65b31c76710e1d825b6aa701c2c9'
PATHS = ('tests/host/audio_stress_test.sh', 'tools/audio_stress_report.py',
         'tests/host/host_failure_summary.py', 'tests/run_tests.sh')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--repo', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    args = ap.parse_args()
    blobs = {}
    sources = {}
    for path in PATHS:
        data = subprocess.check_output(['git', 'show', f'{PIN}:{path}'], cwd=args.repo)
        sources[path] = data
        blobs[path] = hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
    wrapper = sources[PATHS[0]].decode()
    lines = [line.strip() for line in wrapper.splitlines()
             if line.strip().startswith('python3 tools/audio_stress_report.py ')]
    assert len(lines) == 1, 'Reconcile changed wrapper before reusing this diagnostic'
    assert '--require-host-limits' not in shlex.split(lines[0])
    results = []
    with tempfile.TemporaryDirectory(prefix='qa2-consumer-') as folder:
        root = Path(folder)
        reporter = root / 'report.py'
        reporter.write_bytes(sources[PATHS[1]])
        summary_file = root / 'summary.py'
        summary_file.write_bytes(sources[PATHS[2]])
        spec = importlib.util.spec_from_file_location('pinned_summary', summary_file)
        summary = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(summary)
        for name, peak in [('within', 1000000), ('miami_over', 2397367)]:
            row = dict(block=0, cpu_ns=3000000, wall_ns=3000000,
                       chicago_cpu_ns=100000, miami_cpu_ns=peak, ui_cpu_ns=100000)
            csv_file, pcm, output = root/'blocks.csv', root/'audio.s32', root/'report.json'
            with csv_file.open('w', newline='') as stream:
                writer = csv.DictWriter(stream, fieldnames=list(row))
                writer.writeheader()
                writer.writerow(row)
            pcm.write_bytes(bytes(512 * 2 * 4))
            for require in (False, True):
                mapping = {'tools/audio_stress_report.py': str(reporter),
                           '$OUT/$workload/blocks.csv': str(csv_file),
                           '$OUT/$workload/audio.s32': str(pcm),
                           '$OUT/$workload/report.json': str(output)}
                command = [mapping.get(token, token) for token in shlex.split(lines[0])]
                command[0] = sys.executable
                if require:
                    command.append('--require-host-limits')
                proc = subprocess.run(command, capture_output=True, text=True)
                report = json.loads(output.read_text())
                expected_pass = name == 'within'
                assert report['host_timing_within_requested_limits'] == expected_pass
                assert proc.returncode == int(require and not expected_pass), proc.stderr
                assert report['target_acceptance'] == 'NOT_RUN'
                # Model the runner's exit-status classification, then exercise
                # its actual summary parser. This is NOT an actual suite log.
                status = 'ok' if proc.returncode == 0 else 'FAIL'
                synthetic = [proc.stdout.rstrip(), f'  {status} audio-stress']
                parsed = summary.parse_status(synthetic)
                assert parsed == {'audio-stress': status}
                results.append(dict(case=name, synthetic=True, require_host_limits=require,
                                    reporter_exit=proc.returncode, summary_status=parsed,
                                    timing_pass=expected_pass, target_acceptance='NOT_RUN'))
    result = dict(source_commit=PIN, source_blobs=blobs, wrapper_command=lines[0],
                  cases=results, assertions='PASS', actual_wrapper_executed=False,
                  full_suite='NOT_RUN', benchmark='NOT_RUN', hardware='NOT_RUN',
                  conclusion='Functional suite status does not establish timing acceptance.')
    args.out.write_text(json.dumps(result, indent=2) + '\n')
    print('PASS: four synthetic reporter-to-summary cases; no benchmark or firmware run')


if __name__ == '__main__':
    main()
