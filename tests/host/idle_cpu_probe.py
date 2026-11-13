#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Box-only silence/restart capture. This script does not qualify target CPU.

Run on the immutable baseline first, then the candidate with --baseline DIR
and --check-idle-paths to run the 808 dispatch and 909 history fixtures.
Each benchmark pass renders exactly 441000 frames, or ten seconds of audio.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import wave


def build_fixture(root, out, source, name, include_host=False):
    binary = out / name
    with tempfile.TemporaryDirectory(prefix='x0x-idle-compiler-') as temporary:
        entry = root / source
        if include_host:
            entry = out / (name + '-entry.c')
            entry.write_text('#define main simulator_main\n#include ' +
                             json.dumps(str(root / 'host/x0x_host.c')) +
                             '\n#undef main\n#include ' +
                             json.dumps(str(root / source)) + '\n')
        wrapper = Path(temporary) / 'compiler'
        wrapper.write_text('#!' + sys.executable + '\n'
                           'import json,os,shlex,sys\n'
                           'compiler=shlex.split(os.environ["IDLE_PROBE_CC"])\n'
                           'args=[os.environ["IDLE_PROBE_ENTRY"] if x == "host/x0x_host.c" else x for x in sys.argv[1:]]\n'
                           'command=compiler+args\n'
                           'with open(os.environ["IDLE_PROBE_COMMAND"],"w") as log:\n'
                           '    json.dump(command,log,indent=2)\n'
                           'os.execvp(compiler[0],command)\n')
        wrapper.chmod(0o755)
        env = dict(os.environ, CC=str(wrapper),
                   IDLE_PROBE_CC=os.environ.get('CC', 'cc'),
                   IDLE_PROBE_ENTRY=str(entry),
                   IDLE_PROBE_COMMAND=str(out / (name + '-command.json')),
                   X0X_HOST_OUT=str(binary), X0X_DEV_WOBBLE='0')
        with (out / (name + '-build.log')).open('w') as log:
            subprocess.run(['sh', 'host/build_host.sh'], cwd=root, env=env,
                           stdout=log, stderr=subprocess.STDOUT, check=True)
    return binary


def check_idle_paths(root, out):
    fixture = out / 'idle-fixture.wav'
    with wave.open(str(fixture), 'wb') as wav:
        wav.setparams((1, 2, 22050, 0, 'NONE', 'not compressed'))
        wav.writeframes(b''.join(struct.pack('<h', int(10000 * math.sin(
            2 * math.pi * 173 * i / 22050))) for i in range(44100)))
    results = []
    for name, source, arguments, include_host in (
        ('idle-core2', 'tests/host/idle_core2_test.c', [str(fixture)], False),
        ('idle-drums', 'tests/host/idle_drums_test.c', [], True),
    ):
        binary = build_fixture(root, out, source, name, include_host)
        command = [str(binary), *arguments]
        with (out / (name + '-run.log')).open('w') as log:
            result = subprocess.run(command, cwd=root, stdout=log,
                                    stderr=subprocess.STDOUT)
        results.append({'fixture': source, 'command': command,
                        'returncode': result.returncode})
        (out / 'idle-path-results.json').write_text(json.dumps(results, indent=2) + '\n')
        result.check_returncode()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--baseline', type=Path)
    parser.add_argument('--rounds', type=int, default=5)
    parser.add_argument('--check-idle-paths', action='store_true',
                        help='candidate only: build and run 808 dispatch and 909 history checks')
    args = parser.parse_args()
    if args.rounds < 1:
        parser.error('--rounds must be positive')
    root = Path(__file__).resolve().parents[2]
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    # Preserve the actual production host source closure and floating flags.
    # Its only replacement is the simulator's main-entry fixture.
    binary = build_fixture(root, out, 'tests/host/idle_cpu_probe.c', 'idle-probe')
    if args.check_idle_paths:
        check_idle_paths(root, out)
    benchmark = []
    for _ in range(args.rounds):
        result = subprocess.check_output([str(binary), 'bench'], cwd=root, text=True)
        benchmark.append(json.loads(result.splitlines()[-1]))
    hashes = {}
    # The adoption block in setup_idle precedes these intervals; the matrix
    # still straddles 512 subsequent silent samples and partial-block seams.
    for part in range(5):
        for idle in (0, 1, 255, 256, 511, 512, 513, 1023, 1024, 441000):
            for offset in (0, 1, 255, 256, 511):
                label = f'p{part}-idle{idle}-offset{offset}'
                case = out / label
                case.mkdir()
                subprocess.run([str(binary), 'restart', str(part), str(idle),
                                str(offset), str(case)], cwd=root, check=True)
                for file in sorted(case.iterdir()):
                    hashes[f'{label}/{file.name}'] = {
                        'sha256': hashlib.sha256(file.read_bytes()).hexdigest(),
                        'bytes': file.stat().st_size}
    (out / 'benchmark.json').write_text(json.dumps(benchmark, indent=2) + '\n')
    (out / 'hashes.json').write_text(json.dumps(hashes, indent=2) + '\n')
    if args.baseline:
        expected = json.loads((args.baseline / 'hashes.json').read_text())
        differences = sorted(k for k in expected.keys() | hashes.keys()
                             if expected.get(k) != hashes.get(k))
        if differences:
            raise SystemExit('FAIL restart audio/state mismatch: ' + ', '.join(differences))
        print(f'PASS {len(hashes)} per-part idle/restart audio and state files bit-identical')
    print('Host timing captured; target idle render load <3 percent remains NOT_RUN')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
