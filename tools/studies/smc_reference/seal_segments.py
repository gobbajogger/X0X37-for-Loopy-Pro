#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""Build ABI4 segment-validation fixtures from a resolved ELF. NEVER an installer.

Keeps the existing fixed layout. Does not seal app data, create a bundle, grant
flash write permission or remove the app's study-only activation interlock.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib

RANGES = ((0x84000, 0x17000), (0xAD000, 0x2000), (0xFE000, 0x1000))
BASE = 0x01FFC000
HEADER = 128


def crc(data):
    return zlib.crc32(data) & 0xffffffff


def extract(elf, model):
    def read(fmt, off):
        size = struct.calcsize(fmt)
        if off < 0 or off + size > len(elf):
            raise ValueError('truncated ELF structure')
        return struct.unpack_from(fmt, elf, off)
    def span(off, size):
        if off < 0 or size < 0 or off + size > len(elf):
            raise ValueError('truncated ELF bytes')
        return elf[off:off + size]
    if elf[:7] != b'\x7fELF\x01\x01\x01':
        raise ValueError('32-bit little-endian ELF required')
    if read('<HH', 16) != (2, 241):
        raise ValueError('resolved pi32v2 executable required')
    phoff, shoff = read('<II', 28)
    phsize, phnum, shsize, shnum, shstr = read('<HHHHH', 42)
    if phsize != 32 or shsize != 40 or not phnum or not shnum or shstr >= shnum:
        raise ValueError('unsupported ELF tables')
    ph = [read('<8I', phoff + i * phsize) for i in range(phnum)]
    sh = [read('<10I', shoff + i * shsize) for i in range(shnum)]
    names = span(sh[shstr][4], sh[shstr][5])
    def string(table, off):
        if off >= len(table) or b'\0' not in table[off:]:
            raise ValueError('bad string offset')
        return table[off:table.index(0, off)].decode('ascii')
    sections = {}
    for s in sh:
        name = string(names, s[0])
        if name in sections:
            raise ValueError('duplicate section name')
        sections[name] = s
        if s[2] & 2 and s[5]:
            vma, size = s[3], s[5]
            if name in ('.ramtext', '.data', '.bss', '.heap'):
                valid = 0x01C01800 <= vma and vma + size <= 0x01C07F00
            elif name == '.highcpu':
                valid = 0x01C7D400 <= vma and vma + size <= 0x01C7F000
            elif name in ('.ctl_header', '.text'):
                valid = BASE + RANGES[0][0] <= vma and vma + size <= BASE + sum(RANGES[0])
            elif name == '.rodata':
                valid = BASE + RANGES[1][0] <= vma and vma + size <= BASE + sum(RANGES[1])
            else:
                valid = False
            if not valid:
                raise ValueError('allocated VMA outside admitted section region: ' + name)
    symbols = {}
    for s in sh:
        if s[1] != 2:
            continue
        if s[9] != 16 or s[5] % 16 or s[6] >= len(sh):
            raise ValueError('bad symbol table')
        strs = span(sh[s[6]][4], sh[s[6]][5])
        for off in range(s[4], s[4] + s[5], 16):
            name, value, _, _, _, index = read('<IIIBBH', off)
            if index and name:
                symbols[string(strs, name)] = value
    images = [bytearray() for _ in RANGES]
    occupied = [set() for _ in RANGES]
    allowed = {'.ctl_header', '.text', '.rodata', '.ramtext', '.data'}
    for name, s in sections.items():
        _, typ, flags, vma, off, size, *_ = s
        if typ != 1 or not flags & 2 or not size:
            continue
        if name not in allowed:
            raise ValueError('unexpected load section: ' + name)
        matches = [p for p in ph if p[0] == 1 and p[1] <= off and off + size <= p[1] + p[4]]
        if len(matches) != 1:
            raise ValueError('ambiguous/missing load address: ' + name)
        lma = matches[0][3] + off - matches[0][1]
        slots = [(i, lma - BASE - start) for i, (start, cap) in enumerate(RANGES)
                 if BASE + start <= lma and lma + size <= BASE + start + cap]
        if len(slots) != 1:
            raise ValueError('load outside fixed segments: ' + name)
        i, rel = slots[0]
        # Instructions may reside in primary XIP or execute in low RAM only.
        if flags & 4 and not ((i == 0 and vma == lma) or
                              (name == '.ramtext' and 0x01C01800 <= vma and vma + size <= 0x01C07F00)):
            raise ValueError('executable secondary/high memory')
        offsets = set(range(rel, rel + size))
        if occupied[i] & offsets:
            raise ValueError('overlapping load sections')
        occupied[i] |= offsets
        images[i].extend(b'\xff' * max(0, rel + size - len(images[i])))
        images[i][rel:rel + size] = span(off, size)
    if set(allowed) - set(sections) or sections['.ctl_header'][3:6:2] != (BASE + RANGES[0][0], HEADER):
        raise ValueError('missing sections or wrong manifest placeholder')
    for image in images:
        if not image:
            raise ValueError('missing segment')
        image.extend(b'\xff' * (-len(image) % 4))
    def sym(name):
        if name not in symbols:
            raise ValueError('missing symbol ' + name)
        return symbols[name]
    primary = BASE + RANGES[0][0]
    text = sections['.text']
    executable = text[3] + text[5] - primary
    init, poll = sym('ctl_entry_init'), sym('ctl_entry_poll')
    if not all(not x & 1 and primary + HEADER <= x < primary + executable for x in (init, poll)):
        raise ValueError('bootstrap entry outside verified executable region')
    rlo, rhi = sym('__ctl_ram_lo'), sym('__ctl_ram_hi')
    slo, shi = sym('__ctl_isr_stack_lo'), sym('__ctl_isr_stack_hi')
    dlo = sym('__ctl_highcpu_start')
    dhi = (sym('__ctl_highcpu_end') + 15) & ~15
    if not (rlo == 0x01C01800 <= slo < shi <= rhi <= 0x01C07F00 and
            not (slo | shi | rhi) & 15 and dlo == 0x01C7D400 <= dhi <= 0x01C7F000):
        raise ValueError('RAM/stack admission bounds')
    if len(images[0]) >= 0x7004 and images[0][0x7000:0x7004] == b'X0BC':
        raise ValueError('legacy ABI2 location accidentally begins with valid magic')
    old = struct.unpack_from('<IHHIIIIIIIIIIII3I', images[0])
    if old[:3] != (0x43423058, 2, 68) or old[6] != 4:
        raise ValueError('unexpected diagnostic controller header')
    if old[7:13] != (init, poll, slo, shi, rlo, rhi):
        raise ValueError('header/symbol ownership mismatch')
    irq_lo, irq_hi = old[13:15]
    if irq_lo & sum(1 << b for b in (1,11,16,20,24)) or irq_hi & sum(1 << b for b in (5,30,31)):
        raise ValueError('reserved IRQ ownership requested')
    image_id = crc(b''.join(images)) or 1
    h = bytearray(HEADER)
    struct.pack_into('<IHH', h, 0, 0x53423058, 1, HEADER)
    fields = [4, crc(model.encode('ascii')), 0x534D4302, 1, image_id, init, poll,
              slo, shi, rlo, rhi, dlo, dhi, irq_lo, irq_hi]
    struct.pack_into('<15I', h, 8, *fields)
    for i, (image, (off, _)) in enumerate(zip(images, RANGES)):
        struct.pack_into('<4I', h, 68 + i * 16, off, len(image), executable if i == 0 else 0,
                         crc(image[HEADER:] if i == 0 else image))
    struct.pack_into('<I', h, 116, crc(h))
    images[0][:HEADER] = h
    return images, {'model': model, 'model_crc32': fields[1], 'image_id': image_id,
                    'ram_lo': rlo, 'ram_hi': rhi, 'high_lo': dlo, 'high_hi': dhi,
                    'primary_executable_bytes': executable}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--elf', type=Path, required=True)
    p.add_argument('--model', choices=('SMK-37 Elite',), required=True)
    p.add_argument('--out', type=Path, required=True)
    a = p.parse_args()
    if a.out.exists():
        p.error('output must be a new directory')
    try:
        images, meta = extract(a.elf.read_bytes(), a.model)
    except (ValueError, UnicodeError) as e:
        p.error(str(e))
    a.out.mkdir(parents=True)
    meta.update(installable=False, bundle_generated=False, elf_sha256=hashlib.sha256(a.elf.read_bytes()).hexdigest())
    meta['segments'] = []
    for i, image in enumerate(images):
        name = f'segment-{i}-NOT-INSTALLABLE.fixture'
        (a.out / name).write_bytes(image)
        meta['segments'].append(dict(file=name, offset=RANGES[i][0], size=len(image), sha256=hashlib.sha256(image).hexdigest()))
    (a.out / 'NON-INSTALLABLE.txt').write_text('Manifest validation fixture only. No all-piece updater, app admission, live-memory or hardware approval. NEVER INSTALL.\n')
    (a.out / 'manifest.json').write_text(json.dumps(meta, indent=2) + '\n')
    print(json.dumps(meta, indent=2))


if __name__ == '__main__':
    main()
