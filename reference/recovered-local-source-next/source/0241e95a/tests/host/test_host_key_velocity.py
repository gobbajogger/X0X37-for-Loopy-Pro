#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Deterministic logical-key replay on the real host app, not board/sensor emulation."""
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
BINARY = ROOT / 'build/host/x0x_host'
SCRIPT = '''wait 300
expect keyvel0 100
expect keyvel26 100
keyvel 0 8
keyvel 26 127
expect keyvel0 8
expect keyvel26 127
expect keyvel1 100
turn ALGO 4
expect part 2
tap SEQ
turn SELECT 1
turn K1 1
wav replay.wav
tapkey 0
wait 100
keyvel 0 127
tapkey 0
wait 100
wavstop
keyvel 0 100
expect keyvel0 100
'''


def main():
    results = []
    with tempfile.TemporaryDirectory(prefix='x0x-keyvel-') as tmp:
        root = Path(tmp)
        digests = []
        for i in range(2):
            out = root / str(i)
            out.mkdir()
            script = out / 'replay.x0x'
            script.write_text(SCRIPT)
            p = subprocess.run([str(BINARY), str(script), str(out)], capture_output=True, text=True)
            assert p.returncode == 0, p.stdout + p.stderr
            audio = (out / 'replay.wav').read_bytes()
            assert len(audio) > 44
            digests.append(hashlib.sha256(audio).hexdigest())
            results.append({'case': f'replay-{i}', 'exit': p.returncode,
                            'audio_sha256': digests[-1], 'audio_bytes': len(audio)})
        assert digests[0] == digests[1], digests
        for i, command in enumerate(['keyvel -1 100', 'keyvel 27 100', 'keyvel 0 0',
                                      'keyvel 0 128', 'keyvel 0 abc', 'keyvel abc 100',
                                      'keyvel 0', 'keyvel 0 100 extra']):
            script = root / 'bad.x0x'
            script.write_text(command + '\n')
            p = subprocess.run([str(BINARY), str(script), str(root)], capture_output=True, text=True)
            assert p.returncode == 2 and 'keyvel needs logical key' in p.stderr, (command, p.stdout, p.stderr)
            results.append({'case': command, 'exit': p.returncode, 'expected_rejection': True})
    print(json.dumps({'scope': 'actual host logical keys; no physical sensor/SMK scan claim',
                      'cases': results}, indent=2))


if __name__ == '__main__':
    main()
