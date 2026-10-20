#!/usr/bin/env python3
"""Mutate only Pillow inside a disposable pinned worker, restoring each probe."""
from pathlib import Path
import importlib.metadata
import json
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2]/'tools'))
import validation_inputs as identity
import PIL
import PIL._imaging

dist = importlib.metadata.distribution('pillow')
assert dist.files is None, 'proof requires the pinned distro package'
baseline = identity.runtime({})
rows = []
for path in (Path(PIL.__file__), Path(PIL._imaging.__file__)):
    original = path.read_bytes()
    before = identity.file_hash(path)
    try:
        path.write_bytes(original+b'\n# disposable package-byte probe\n')
        changed = identity.runtime({})
        assert changed != baseline
        assert changed['python_packages']['pillow'] != baseline['python_packages']['pillow']
        rows.append({'path': str(path), 'before': before, 'after': identity.file_hash(path),
                     'runtime_after': changed})
    finally:
        path.write_bytes(original)
    assert identity.runtime({}) == baseline
print(json.dumps({'distribution_files_is_none': dist.files is None,
                  'covered_files': sorted(identity.package_files(dist)),
                  'runtime_before': baseline, 'mutations': rows, 'restored': True}, indent=2))
