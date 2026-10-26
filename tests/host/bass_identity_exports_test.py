#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Run identity proposal against independent actual exports; never rewrite inputs."""
import base64
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[2]
FIXTURES = Path(__file__).resolve().parent / 'fixtures' / 'bass_identity'
PINS = {
    3: ('d633833ac369f8466bc2563791614206070e87d1', '2c47e1459cdef8db8eec73b96ce58d3a9c67b8c0abaeb3065a46141c2f911d3a'),
    6: ('6ddea31d6040d6735989e64adbe11630fc1ee7ec', 'c169f42d80c362a1f0a62ffd66baeb7aa6af41c3254a9f702280f67c987331df'),
}

def digest(b):
    return hashlib.sha256(b).hexdigest()

files = []
fixture_hashes = {p: digest(p.read_bytes()) for p in FIXTURES.glob('*.json')}
with tempfile.TemporaryDirectory(prefix='bass-identity-exports-') as tmp:
    folder = Path(tmp)
    for stream, (commit, sha) in PINS.items():
        doc = json.loads((FIXTURES / ('project-v%d-sound10.json' % stream)).read_text())
        b = zlib.decompress(base64.b64decode(doc['zlib_base64']))
        assert doc['synthetic'] is True and doc['contains_user_data'] is False
        assert doc['source_commit'] == commit and digest(b) == sha == doc['sha256']
        assert doc['project_validate_stage'] == 0 and doc['project_file_validate'] == 0
        assert struct.unpack_from('<8I', b) == tuple(doc['header_words_le'])
        assert struct.unpack_from('<I', b, 20)[0] == stream
        assert struct.unpack_from('<I', b, 24)[0] == 10
        assert zlib.crc32(b[32:]) == struct.unpack_from('<I', b, 12)[0]
        p = folder / ('actual-v%d-sound10.bin' % stream)
        p.write_bytes(b)
        files.append(p)
        # Both actual fixtures use canonical zero-valued valid sound cells. Those
        # bytes fit both engine surfaces and still cannot determine engine identity.
        sound_start = 32 + 40   # actual rev10 settings are 40B, immediately before sound
        row_a = sound_start + (2 * 12) * 32
        assert not any(b[row_a:row_a + 64])
        # Independent legacy ambiguity: preserved Chicago Cutoff48 occupies the
        # new Miami CarrierWave index. Recompute only the canonical outer CRC.
        mutated = bytearray(b)
        mutated[row_a] = 48
        struct.pack_into('<I', mutated, 12, zlib.crc32(mutated[32:]))
        q = folder / ('synthetic-legacy-carrier48-v%d.bin' % stream)
        q.write_bytes(mutated)
        files.append(q)
        print('FIXTURE actual-stream%d/10 source=%s sha256=%s; variant=synthetic Carrier48, no engine inferred' % (stream, commit, sha), flush=True)
    originals = {p: digest(p.read_bytes()) for p in files}
    result = subprocess.run(['sh', 'tests/host/bass_identity_test.sh'] + [str(p) for p in files], cwd=ROOT, env=os.environ)
    assert all(digest(p.read_bytes()) == sha for p, sha in originals.items()), 'export bytes changed'
    assert all(digest(p.read_bytes()) == sha for p, sha in fixture_hashes.items()), 'producer fixture changed'
    if result.returncode:
        raise SystemExit(result.returncode)
    print(json.dumps({'actual_export_fixtures': 2, 'synthetic_legacy_variants': 2,
                      'export_bytes_preserved': True, 'fixture_json_preserved': True,
                      'legacy_identity': 'CHOICE_REQUIRED for every voice, both build modes',
                      'production_integration': False}))
