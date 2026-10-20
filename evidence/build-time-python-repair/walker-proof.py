#!/usr/bin/env python3
"""Compare actual SDK/toolchain bytes with the frozen original walker; read only."""
from pathlib import Path
import json
import platform
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tests.host.test_validation_inputs import ContentWalk, identity

rows = []
for path in sys.argv[1:]:
    row = {'path': path}
    for name, function in [('original', ContentWalk.original), ('direct', identity.content)]:
        start = time.perf_counter()
        row[name] = function(path)
        row[name+'_seconds'] = time.perf_counter()-start
    assert row['original'] == row['direct'], path
    rows.append(row)
print(json.dumps({'host': platform.platform(), 'argv': sys.argv, 'equal': True,
                  'timer': 'perf_counter around one content(path) call, original then direct; warm caches possible; no target build',
                  'results': rows}, indent=2))
