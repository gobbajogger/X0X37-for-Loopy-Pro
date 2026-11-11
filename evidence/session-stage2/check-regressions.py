"""Run the new public test against the base and the candidate. No device access."""
from pathlib import Path
import json
import resource
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
BASE = '1313e0fe384ef5df1f2f5f5ef699902c4543bc0d'
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
results = {}
with tempfile.TemporaryDirectory(prefix='smc-central-regression-') as tmp:
    scratch = Path(tmp)
    for name in ('base', 'candidate'):
        root = scratch / name
        for rel in ('firmware/src/ble/ble_midi_central.c',
                    'firmware/src/ble/ble_midi_central.h',
                    'tests/host/ble_midi_signal_test.c'):
            path = root / rel
            path.parent.mkdir(parents=True, exist_ok=True)
            if name == 'base' and rel.startswith('firmware/'):
                path.write_bytes(subprocess.check_output(['git', 'show', f'{BASE}:{rel}'], cwd=ROOT))
            else:
                shutil.copyfile(ROOT / rel, path)
        cmd = ['cc', '-std=c99', '-g', '-O1', '-Wall', '-Wextra', '-Werror',
               '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
               str(root / 'tests/host/ble_midi_signal_test.c'),
               str(root / 'firmware/src/ble/ble_midi_central.c'), '-o', str(root / 'test')]
        subprocess.run(cmd, check=True)
        for case in ('signaling', 'lifecycle'):
            argv = [str(root / 'test')] + (['lifecycle'] if case == 'lifecycle' else [])
            run = subprocess.run(argv, capture_output=True, text=True)
            log = OUT / f'{name}-{case}.log'
            log.write_text(run.stdout + run.stderr)
            results[f'{name}-{case}'] = dict(returncode=run.returncode, log=str(log), compile=cmd)
            assert (run.returncode != 0) if name == 'base' else (run.returncode == 0)
(OUT / 'regressions.json').write_text(json.dumps(results, indent=2) + '\n')
print(json.dumps({k: v['returncode'] for k, v in results.items()}))
