#!/usr/bin/env python3
"""Run only the assigned scratch orchestration and affected compatibility checks."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time
ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
CHECKS = [
    ['python3', '-B', 'tests/host/test_quickcheck_plan.py'],
    ['python3', '-B', 'tests/host/test_ctl_compile.py'],
    ['bash', 'tests/host/test_quickcheck_ste.sh'],
    ['bash', 'tests/host/test_quickcheck_rules.sh'],
    ['bash', 'tests/host/test_quickcheck_devbug.sh'],
    ['bash', 'tests/host/test_rules_guard_hook.sh'],
    ['python3', '-B', 'tests/host/test_portable_rules.py'],
]
requested = set(sys.argv[1:])
rows = json.loads((OUT / 'checks.json').read_text()) if requested and (OUT / 'checks.json').exists() else []
for argv in CHECKS:
    if requested and Path(argv[-1]).stem not in requested:
        continue
    rows = [r for r in rows if r['argv'] != argv]
    name = Path(argv[-1]).stem
    start = time.monotonic()
    p = subprocess.run(argv, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    log = OUT / (name + '.log')
    log.write_bytes(p.stdout)
    row = {'argv': argv, 'returncode': p.returncode, 'elapsed_seconds': time.monotonic()-start,
           'log': str(log.relative_to(ROOT)), 'sha256': hashlib.sha256(p.stdout).hexdigest()}
    rows.append(row)
    print(json.dumps(row), flush=True)
(OUT / 'checks.json').write_text(json.dumps(rows, indent=2) + '\n')
raise SystemExit(int(any(r['returncode'] != 0 for r in rows)))
