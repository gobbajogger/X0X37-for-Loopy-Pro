#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read-only ELF RAM-load compression size/roundtrip probe; no firmware output."""
import argparse
import importlib.util
import hashlib
import json
import pathlib
import struct

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--repo', required=True, type=pathlib.Path)
p.add_argument('elf', type=pathlib.Path)
a = p.parse_args()
pins = json.loads((pathlib.Path(__file__).parent / 'input-hashes.json').read_text())
if hashlib.sha256((a.repo / 'tools/lz4blk.py').read_bytes()).hexdigest() != pins['host_codec_sha256']:
    raise SystemExit('Pinned host codec hash mismatch')
spec = importlib.util.spec_from_file_location('study_lz4', a.repo / 'tools/lz4blk.py')
codec = importlib.util.module_from_spec(spec)
spec.loader.exec_module(codec)
b = a.elf.read_bytes()
if b[:6] != b'\x7fELF\x01\x01':
    raise SystemExit('Expected 32-bit little-endian ELF')
header = struct.unpack_from('<HHIIIIIHHHHHH', b, 16)
shoff, shsize, shnum, names_index = header[5], header[10], header[11], header[12]
sections = [struct.unpack_from('<IIIIIIIIII', b, shoff + i * shsize) for i in range(shnum)]
names_header = sections[names_index]
names = b[names_header[4]:names_header[4] + names_header[5]]
result = {}
for section in sections:
    name = names[section[0]:].split(b'\0', 1)[0].decode()
    if name in ('.ramtext', '.data'):
        raw = b[section[4]:section[4] + section[5]]
        packed = codec.compress(raw)
        assert codec.decompress(packed) == raw
        result[name] = {'raw': len(raw), 'compressed': len(packed),
                        'saved_before_decoder_and_metadata': len(raw) - len(packed)}
if len(result) != 2:
    raise SystemExit('Expected both .ramtext and .data')
print(json.dumps(result, indent=2))
