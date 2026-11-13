#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Offline MBOOT adoption/head-preserving package candidate; never installs."""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
import add_ctl
import fm1_install
import fm1pkg_make as pm
import check as qualification

MODELS = {'pro': ('SMK-37 Pro', 1), 'elite': ('SMK-37 Elite', 2)}
HEAD = 0x4000
FLASH = 0x100000


def digest(data):
    return hashlib.sha256(data).hexdigest()


def pinned(data, expected, label):
    if not re.fullmatch('[0-9a-f]{64}', expected or '') or digest(data) != expected:
        raise ValueError(label + ' SHA-256 does not match frozen input')


def chipkey(head):
    """Conservative package-key check, not a read of the unit's runtime efuse."""
    raw = add_ctl._dec(head[64:96])
    if pm.crc16(raw[2:]) != struct.unpack_from('<H', raw)[0] or raw[16:].split(b'\0')[0] != b'isd_config.ini':
        raise ValueError('configuration must be record 64 with valid CRC')
    address, size = struct.unpack_from('<II', raw, 4)
    if size < 34 or address < 0xa0 or address + size > HEAD:
        raise ValueError('configuration is not wholly retained in the head')
    config = head[address:address + size]
    if pm.crc16(config) != struct.unpack_from('<H', raw, 2)[0] or pm.crc16(config[:32]) != struct.unpack_from('<H', config, 32)[0]:
        raise ValueError('configuration or key-prefix CRC mismatch')
    return pm.chipkey_decode(config[:32])


def package_entries(logical):
    damage = fm1_install.package_damage(logical)
    if damage:
        raise ValueError('package: ' + damage)
    hdr = add_ctl._dec(logical[:64])
    count = struct.unpack_from('<H', hdr, 8)[0]
    if count != 2 or struct.unpack_from('<I', hdr, 4)[0] > len(logical):
        raise ValueError('requires exactly flash.bin and ota.bin')
    entries = []
    for i in range(count):
        entry = add_ctl._dec(logical[64 + i * 80:144 + i * 80])
        typ = struct.unpack_from('<H', entry)[0]
        off, size = struct.unpack_from('<II', entry, 8)
        name = bytes(entry[64:]).split(b'\0')[0]
        if off < 0x400 or size == 0 or off + size > len(logical):
            raise ValueError('unsafe package entry bounds')
        entries.append((typ, name, off, size, i, entry))
    if {(e[0], e[1]) for e in entries} != {(0, b'flash.bin'), (100, b'ota.bin')}:
        raise ValueError('duplicate or unknown package entries')
    a, b = sorted(entries, key=lambda e: e[2])
    if a[2] + a[3] > b[2]:
        raise ValueError('overlapping package entries')
    return entries


def app_geometry(head):
    """Exact supported directory selection; no app relocation or bank migration."""
    header = add_ctl._dec(head[:32])
    if len(header) != 32 or pm.crc16(header[2:]) != struct.unpack_from('<H', header)[0]:
        raise ValueError('invalid flash header')
    directories = {}
    for offset in range(32, 0xa0, 32):
        record = add_ctl._dec(head[offset:offset+32])
        if pm.crc16(record[2:]) != struct.unpack_from('<H', record)[0]:
            raise ValueError('invalid top-level directory CRC')
        name = bytes(record[16:]).split(b'\0')[0]
        if name in (b'app_dir_head', b'app_dir_head2'):
            if name in directories:
                raise ValueError('duplicate application directory')
            directories[name] = (offset, bytes(record))
    if b'app_dir_head' not in directories:
        raise ValueError('missing application directory')
    # Capacity and allocation alignment also affect upstream lookup/request scans.
    return bytes(header[8:14]), directories


def adapt_package(raw, adopted_head, model, expected_controller):
    """Preserve application/controller/OTA bytes, replacing only the package head.

    Called only with a head returned by the verified C adoption transform below.
    This does not change installer policy or approve a package for installation.
    """
    product, logical = add_ctl.logical_of(raw)
    if model not in MODELS or not re.fullmatch(re.escape(MODELS[model][0]) + r'_\d+', product):
        raise ValueError('exact package model mismatch')
    if add_ctl.fwsc_of(logical, product) != raw:
        raise ValueError('unsupported/noncanonical FWSC interleave')
    entries = package_entries(logical)
    flash_entry = next(e for e in entries if e[0] == 0)
    ota_entry = next(e for e in entries if e[0] == 100)
    _, _, off, size, index, entry = flash_entry
    if size < HEAD:
        raise ValueError('truncated flash head')
    ota = logical[ota_entry[2]:ota_entry[2]+ota_entry[3]]
    if fm1_install.LOADER_MARK not in ota[:512]:
        raise ValueError('requires the existing head-preserving Felucca app loader')
    controller = add_ctl.controller_of(logical)
    if controller is None:
        raise ValueError('complete embedded controller is required')
    pinned(controller, expected_controller, 'controller')
    old_head = bytes(logical[off:off+HEAD])
    if len(adopted_head) != HEAD or chipkey(old_head) != chipkey(adopted_head):
        raise ValueError('package/configuration chip-key mismatch; re-encryption is forbidden')
    if app_geometry(old_head) != app_geometry(adopted_head):
        raise ValueError('application geometry/banking mismatch; relocation is forbidden')
    updated = bytearray(logical)
    updated[off:off+HEAD] = adopted_head
    struct.pack_into('<H', entry, 4, pm.crc16(updated[off:off+size]))
    pm.enc(entry, 0, 80)
    updated[64+index*80:144+index*80] = entry
    hdr = add_ctl._dec(updated[:64])
    struct.pack_into('<H', hdr, 2, pm.crc16(updated[64:224]))
    struct.pack_into('<H', hdr, 0, pm.crc16(hdr[2:]))
    pm.enc(hdr, 0, 64); updated[:64] = hdr
    package_entries(updated)
    if bytes(updated[off+HEAD:off+size]) != bytes(logical[off+HEAD:off+size]):
        raise ValueError('application/controller bytes changed')
    if updated[ota_entry[2]:ota_entry[2]+ota_entry[3]] != ota:
        raise ValueError('OTA bytes changed')
    if add_ctl.controller_of(updated) != controller or fm1_install.package_head(updated) != adopted_head:
        raise ValueError('controller/head preservation failed')
    # Prove the unchanged actual installer guard accepts the prepared head.
    with tempfile.TemporaryDirectory(prefix='mboot-head-check-') as directory:
        reference = Path(directory) / 'head.bin'
        reference.write_bytes(adopted_head)
        if not fm1_install.head_compare(updated, reference)[0]:
            raise ValueError('unchanged installer head guard refused candidate')
    return add_ctl.fwsc_of(updated, product)


def candidate(snapshot, raw_package, container, pins, upstream):
    if set(pins) != {'model', 'snapshot_sha256', 'package_sha256', 'container_sha256', 'controller_sha256'}:
        raise ValueError('pins must contain only the five documented identity fields')
    if pins.get('model') not in MODELS:
        raise ValueError('model must be pro or elite')
    for data, field in ((snapshot, 'snapshot_sha256'), (raw_package, 'package_sha256'),
                        (container, 'container_sha256')):
        pinned(data, pins.get(field), field)
    if len(snapshot) != FLASH:
        raise ValueError('requires a complete frozen 1 MiB snapshot')
    qualification.verify_upstream(upstream)
    # Compile only the portable core and adoption wrapper; no platform/NOR/device
    # backend, installer or target execution enters this host process.
    with tempfile.TemporaryDirectory(prefix='mboot-adoption-caller-') as directory:
        library = Path(directory) / 'adoption.so'
        subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                        '-shared', '-fPIC', '-I' + str(upstream / 'include'),
                        str(HERE / 'adoption.c'), str(upstream / 'src/core.c'),
                        '-o', str(library)], check=True)
        api = ctypes.CDLL(str(library)).x0x_mboot_prepare
        pointer = ctypes.POINTER(ctypes.c_uint8)
        api.argtypes = [ctypes.c_uint] + [item for _ in range(5) for item in (pointer, ctypes.c_size_t)]
        api.restype = ctypes.c_int
        def buffer(data):
            return (ctypes.c_uint8 * len(data)).from_buffer_copy(data)
        current, approved = buffer(snapshot), buffer(snapshot)
        boot, approved_boot = buffer(container), buffer(container)
        output = (ctypes.c_uint8 * FLASH)()
        status = api(MODELS[pins['model']][1], current, FLASH, approved, FLASH,
                     boot, len(container), approved_boot, len(container), output, FLASH)
        if status:
            raise ValueError('adoption transform refused input (status %d)' % status)
        transformed = bytes(output)
    adopted_head = transformed[:HEAD]
    if transformed[0x38d0:] != snapshot[0x38d0:]:
        raise ValueError('retained data changed')
    updated = adapt_package(raw_package, adopted_head, pins['model'], pins.get('controller_sha256'))
    report = dict(pins, status='OFFLINE_CANDIDATE_NOT_INSTALLABLE', upstream_commit=qualification.PIN,
                  adopted_head_sha256=digest(adopted_head), candidate_package_sha256=digest(updated),
                  application_controller_ota_unchanged=True, device_access=False,
                  limits=['Snapshot/model provenance is supplied by the caller, not established by a hash.',
                          'Configuration prefix key matching is not runtime efuse/hardware validation.',
                          'Needs independent review and actual owner-reviewed unit/model identity before any device stage.',
                          'No device writer or install command is provided.'])
    return adopted_head, updated, report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot', required=True, type=Path)
    parser.add_argument('--package', required=True, type=Path)
    parser.add_argument('--container', required=True, type=Path)
    parser.add_argument('--pins', required=True, type=Path,
                        help='JSON: model and snapshot/package/container/controller SHA-256 pins')
    parser.add_argument('--upstream', required=True, type=Path)
    parser.add_argument('--candidate-dir', type=Path,
                        help='Optional new private output directory; default is report-only')
    args = parser.parse_args()
    head, package, report = candidate(args.snapshot.read_bytes(), args.package.read_bytes(),
                                     args.container.read_bytes(), json.loads(args.pins.read_text()),
                                     args.upstream.resolve())
    if args.candidate_dir:
        args.candidate_dir.mkdir(mode=0o700, parents=False, exist_ok=False)
        # Heads may contain private unit configuration: never publish this folder.
        for name, data in [('adoption-head.PRIVATE-candidate', head),
                           ('app-package.NOT-INSTALLABLE-candidate', package),
                           ('report.json', (json.dumps(report, indent=2)+'\n').encode())]:
            path = args.candidate_dir / name
            with path.open('xb') as out:
                os.chmod(path, 0o600); out.write(data)
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
