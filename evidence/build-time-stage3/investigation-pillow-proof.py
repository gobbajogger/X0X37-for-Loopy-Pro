#!/usr/bin/env python3
"""Run only in a disposable Docker container: mutates its Pillow init file."""
from pathlib import Path
import hashlib
import importlib.metadata
import json
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools'))
import validation_inputs as v
import PIL
p = Path(PIL.__file__)
d = importlib.metadata.distribution('pillow')
a = v.runtime({})
before = hashlib.sha256(p.read_bytes()).hexdigest()
p.write_bytes(p.read_bytes() + b'\n# Disposable stage3 installed-byte coverage probe.\n')
after = hashlib.sha256(p.read_bytes()).hexdigest()
b = v.runtime({})
result = {'distribution_files_is_none': d.files is None,
          'distribution_files_count': len(d.files or []), 'module_path': str(p),
          'module_before_sha256': before, 'module_after_sha256': after,
          'runtime_before': a, 'runtime_after': b,
          'package_bytes_changed': before != after, 'runtime_identity_unchanged': a == b}
assert before != after
assert a == b
print(json.dumps(result, indent=2))
