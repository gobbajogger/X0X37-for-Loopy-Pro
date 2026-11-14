#!/usr/bin/env python3
"""Verify the packet's relative file hashes. No network, MIDI or writes."""
import hashlib,json,pathlib
root=pathlib.Path(__file__).resolve().parent
manifest=json.loads((root/'MANIFEST.json').read_text())
for item in manifest['files']:
    relative=pathlib.PurePosixPath(item['path'])
    assert not relative.is_absolute() and '..' not in relative.parts
    raw=(root/str(relative)).read_bytes()
    assert len(raw)==item['size'],relative
    assert hashlib.sha256(raw).hexdigest()==item['sha256'],relative
print(f"PASS: {len(manifest['files'])} packet files match SHA-256 and size")
