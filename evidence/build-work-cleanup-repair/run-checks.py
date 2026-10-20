#!/usr/bin/env python3
"""Check only controller cleanup; run the original source as a scratch negative control."""
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
BASE = '78c5296e95b9a18a514bbfd126ab04ebd86df9c9'
def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def negative():
    spec = importlib.util.spec_from_file_location('fixture', ROOT / 'tests/host/test_ctl_compile.py')
    fixture = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(fixture)
    with tempfile.TemporaryDirectory() as folder:
        scratch = Path(folder)
        script = scratch / 'ble/ctl/build_in_container.sh'
        script.parent.mkdir(parents=True)
        script.write_bytes(subprocess.check_output(['git', '-c', 'safe.directory=' + str(ROOT), 'show',
                                                   BASE + ':ble/ctl/build_in_container.sh'], cwd=ROOT))
        fixture.TOP = scratch
        names = ['test_successful_driver_child_before_later_failure',
                 'test_successful_driver_child_stopped_on_success',
                 'test_signal_after_successful_batch_stops_children']
        suite = unittest.TestSuite(fixture.ControllerCompile(name) for name in names)
        result = unittest.TextTestRunner(stream=sys.stdout, verbosity=2).run(suite)
        assert len(result.failures) == 10 and not result.errors, (len(result.failures), result.errors)
        print('PASS: original source rejected in all ten new failure/success/signal subcases; fixture-owned children stopped.')

if sys.argv[1:] == ['negative']:
    negative()
else:
    negative_only = sys.argv[1:] == ['negative-only']
    rows = [row for row in json.loads((OUT / (sys.platform + '-checks.json')).read_text())['checks']
            if row['label'] == 'suite'] if negative_only else []
    for label, argv in [('suite', [sys.executable, '-B', 'tests/host/test_ctl_compile.py', '-v']),
                        ('negative', [sys.executable, '-B', str(Path(__file__).relative_to(ROOT)), 'negative'])]:
        if negative_only and label == 'suite': continue
        start = time.monotonic()
        p = subprocess.run(argv, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=90)
        log = OUT / (sys.platform + '-' + label + '.log')
        # unittest subtest progress can end in spaces; keep committed logs whitespace-clean.
        log.write_bytes(b'\n'.join(line.rstrip() for line in p.stdout.splitlines()) + b'\n')
        rows.append(dict(label=label, argv=argv, returncode=p.returncode, seconds=time.monotonic()-start,
                         log=str(log.relative_to(ROOT)), sha256=sha(log),
                         raw_sha256=hashlib.sha256(p.stdout).hexdigest(),
                         log_normalization='Trailing line whitespace removed; terminal newline retained.'))
        print(label, p.returncode, flush=True)
    result = dict(platform=sys.platform, machine=platform.machine(), python=sys.version,
                  image=os.environ.get('CHECK_IMAGE'), base=BASE,
                  sources={p: sha(ROOT / p) for p in ['ble/ctl/build_in_container.sh', 'tests/host/test_ctl_compile.py']},
                  checks=rows)
    (OUT / (sys.platform + '-checks.json')).write_text(json.dumps(result, indent=2) + '\n')
    raise SystemExit(int(any(row['returncode'] for row in rows)))
