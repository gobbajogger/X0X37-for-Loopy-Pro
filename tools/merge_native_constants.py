#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Coalesce audited value-only native LUTs without changing compiler output.

The compiler keeps static const definitions and constant folding. Before the
native link, identical, whole readonly tables receive ELF SHF_MERGE metadata.
No section payload, symbol, relocation or instruction bytes are modified.
Do not add address-identity-bearing objects, strings, pointer tables or writable
objects to this allowlist. This is intentionally not a general ELF optimizer.
"""
from pathlib import Path
import collections
import hashlib
import struct

TABLES = {
    'IMA_ROUND_CORR': (64, '1d9b12c62c87407a8f43f24dd8e8dd69436df5b9c35a24f0f8dfb4130594af2d'),
    'fx_beats': (52, 'd28c773f2033dcf08b986c902d51330fa2e1850c180a9b970344edf64ff8554d'),
    'mix_send_values': (512, '2e7800667d4ae3e49ac70e26b3c2631385158fa9aa31755904a651566fb1dc7d'),
    'original_pitch44100': (512, '77c39cdfb56f1cd4da2bfbe83d1c7902a0a8078e0b66f98045a49d0e9dc20926'),
    'mod_linear': (256, 'a7d0f57804777b16ea8d658964f84e29a9846084af4e9c145116ba9ba4daa903'),
    'original_sound_unit_value': (512, '2e7800667d4ae3e49ac70e26b3c2631385158fa9aa31755904a651566fb1dc7d'),
    'original_drive_table': (256, 'a7d0f57804777b16ea8d658964f84e29a9846084af4e9c145116ba9ba4daa903'),
    'miami_decimator_wet': (256, 'a7d0f57804777b16ea8d658964f84e29a9846084af4e9c145116ba9ba4daa903'),
    'original_miami_voice_compile.base': (40, '0192a1c6d0a48f1cef5d37d48feca0cb97c3336d00d11414bc339cc9e6f36e9b'),
    'original_chicago_voice_compile.base': (40, '0192a1c6d0a48f1cef5d37d48feca0cb97c3336d00d11414bc339cc9e6f36e9b'),
    'chi_bpmdly_beats.beats': (52, 'd28c773f2033dcf08b986c902d51330fa2e1850c180a9b970344edf64ff8554d'),
    'original_chicago_arp_base': (40, '0192a1c6d0a48f1cef5d37d48feca0cb97c3336d00d11414bc339cc9e6f36e9b'),
}


def _tables(path):
    data = bytearray(path.read_bytes())
    if len(data) < 52 or data[:6] != b'\x7fELF\x01\x01':
        raise ValueError(f'{path}: expected ELF32 little endian')
    if data[6] != 1 or struct.unpack_from('<I', data, 20)[0] != 1 or struct.unpack_from('<H', data, 40)[0] != 52:
        raise ValueError(f'{path}: invalid ELF version/header size')
    if struct.unpack_from('<I', data, 28)[0] or struct.unpack_from('<H', data, 44)[0]:
        raise ValueError(f'{path}: unexpected program headers')
    if struct.unpack_from('<H', data, 18)[0] != 241:
        raise ValueError(f'{path}: expected pi32v2 object')
    if struct.unpack_from('<H', data, 16)[0] != 1:
        raise ValueError(f'{path}: expected relocatable object')
    offset = struct.unpack_from('<I', data, 32)[0]
    stride, count, strings_index = struct.unpack_from('<HHH', data, 46)
    if offset < 52 or offset % 4 or stride != 40 or not count or strings_index >= count or offset + count * stride > len(data):
        raise ValueError(f'{path}: invalid section directory')
    headers = [struct.unpack_from('<10I', data, offset + i * stride) for i in range(count)]
    if any(headers[0]):
        raise ValueError(f'{path}: invalid null section')
    occupied = [(0, 52), (offset, offset + count * stride)]
    for h in headers[1:]:
        alignment = h[8]
        if alignment and alignment & (alignment - 1):
            raise ValueError(f'{path}: non-power-of-two alignment')
        if h[1] != 8 and h[5]:  # NOBITS has no file payload
            lo, hi = h[4], h[4] + h[5]
            if hi > len(data) or any(lo < b and a < hi for a, b in occupied):
                raise ValueError(f'{path}: overlapping/out-of-bounds section')
            occupied.append((lo, hi))
    string_header = headers[strings_index]
    start, size = string_header[4:6]
    if string_header[1] != 3 or not size or start + size > len(data) or data[start] != 0:
        raise ValueError(f'{path}: invalid section names')
    names = data[start:start + size]
    tables = []
    for index, header in enumerate(headers):
        if header[0] >= len(names):
            raise ValueError(f'{path}: invalid section name offset')
        end = names.find(0, header[0])
        if end < 0:
            raise ValueError(f'{path}: unterminated section name')
        name = bytes(names[header[0]:end]).decode('ascii')
        if not name.startswith('.rodata.') or name[8:] not in TABLES:
            continue
        expected, digest = TABLES[name[8:]]
        if header[1] != 1 or header[2] not in (2, 18) or header[5] != expected or header[9] not in (0, expected):
            raise ValueError(f'{path}: incompatible audited table {name}')
        if header[3] != 0 or expected % max(1, header[8]) or header[4] + expected > len(data) or header[8] not in (1, 2, 4, 8, 16) or header[4] % max(1, header[8]):
            raise ValueError(f'{path}: invalid table bounds/alignment {name}')
        if any(h[1] in (4, 9) and h[7] == index for h in headers):
            raise ValueError(f'{path}: table contains relocations {name}')
        payload = bytes(data[header[4]:header[4] + expected])
        if hashlib.sha256(payload).hexdigest() != digest:
            raise ValueError(f'{path}: audited table bytes changed {name}')
        tables.append((offset + index * stride, name, payload))
    return data, tables


def merge_constants(objects):
    """Validate all objects first, then mark only byte-identical table groups."""
    inputs = [(Path(p), *_tables(Path(p))) for p in objects]
    groups = collections.Counter(payload for _, _, tables in inputs for _, _, payload in tables)
    changed = []
    for path, data, tables in inputs:
        marked = []
        for offset, name, payload in tables:
            if groups[payload] < 2:
                continue
            struct.pack_into('<I', data, offset + 8, 18)  # SHF_ALLOC | SHF_MERGE
            struct.pack_into('<I', data, offset + 36, len(payload))
            marked.append(name)
        if marked:
            path.write_bytes(data)
            changed.append((str(path), marked))
    return changed
