#!/usr/bin/env python3
"""Check native SMK-37 DEV build outputs before marking a build probe passed.

This is an additional build-output integrity guard, NOT an installer, release
qualification, sample preservation check or physical-device test.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import struct
import sys
import zlib

import add_ctl
import fm1pkg_make as pkg

PRODUCTS = {'elite': 'SMK-37 Elite_900', 'pro': 'SMK-37 Pro_900'}


def check_firmware(path: Path, model: str) -> dict:
    if model not in PRODUCTS:
        raise ValueError('unsupported SMK-37 model')
    raw = path.read_bytes()
    product, logical = add_ctl.logical_of(raw)
    if product != PRODUCTS[model]:
        raise ValueError(f'wrong firmware model: got {product!r}, expected {PRODUCTS[model]!r}')
    # Checks flash/app-area CRC and SMK board/slot identity using the existing
    # package implementation rather than duplicating its encrypted flash parser.
    state = add_ctl._open(logical)
    if pkg.slotmap_of(state["flash"]) != pkg.SLOTMAP["smk37"]:
        raise ValueError("SMK sample slot map missing, invalid or changed")
    if len(logical) < 0x40:
        raise ValueError('package missing UFW header')
    hdr = bytearray(logical[:0x40])
    pkg.enc(hdr, 0, len(hdr))
    count = struct.unpack_from('<H', hdr, 8)[0]
    if count != 2:
        raise ValueError(f'unexpected package file count {count}')
    start, end = 0x40, 0x40 + 0x50 * count
    if end > len(logical):
        raise ValueError('truncated package file descriptors')
    entries = logical[start:end]
    if pkg.crc16(entries) != struct.unpack_from('<H', hdr, 2)[0]:
        raise ValueError('package file-list CRC mismatch')
    names = []
    for index in range(count):
        entry = bytearray(entries[index * 0x50:(index + 1) * 0x50])
        pkg.enc(entry, 0, 0x50)
        kind, number, checksum, _flags, off, size, _padded = struct.unpack_from('<HHHHIII', entry, 0)
        name = bytes(entry[0x40:0x50]).split(b'\0', 1)[0]
        expected = (b'flash.bin', b'ota.bin')[index]
        if name != expected or kind != (0, 100)[index] or number != index:
            raise ValueError(f'unexpected firmware payload descriptor {index}')
        if size == 0 or off < 0x400 or off > len(logical) or size > len(logical) - off:
            raise ValueError(f'payload {expected.decode()} missing or truncated')
        if pkg.crc16(logical[off:off + size]) != checksum:
            raise ValueError(f'{expected.decode()} CRC mismatch')
        names.append(expected.decode())
    return {'model': model, 'product': product, 'bytes': len(raw),
            'sha256_check': 'package header, flash/app and OTA CRC verified',
            'package_crc32': f'{zlib.crc32(raw):08x}', 'payloads': names}


def check_controller(path: Path) -> dict:
    image = path.read_bytes()
    if problem := add_ctl.ctl_problem(image):
        raise ValueError(f'controller image: {problem}')
    return {'controller_bytes': len(image), 'controller_crc32': f'{zlib.crc32(image):08x}',
            'controller_header_crc': 'verified'}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('type', choices=['package', 'controller'])
    parser.add_argument('path', type=Path)
    parser.add_argument('--model', choices=sorted(PRODUCTS))
    args = parser.parse_args(argv)
    if args.type == 'package' and not args.model:
        parser.error('package requires --model')
    if args.type == 'controller' and args.model:
        parser.error('controller has no --model')
    try:
        result = (check_firmware(args.path, args.model) if args.type == 'package'
                  else check_controller(args.path))
    except (OSError, ValueError, add_ctl.PkgError, struct.error) as exc:
        print('BUILD OUTPUT INVALID: ' + str(exc), file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == '__main__':
    sys.exit(main())
