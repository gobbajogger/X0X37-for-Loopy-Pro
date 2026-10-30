#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Synthetic unit fixtures + exact existing complete package; no device operations."""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tools/mboot'))
import prepare as p


def synthetic_snapshot(package, key=None):
    product, logical = p.add_ctl.logical_of(package)
    state = p.add_ctl._open(logical)
    image = bytearray(b'\xff' * p.FLASH)
    image[:len(state['flash'])] = state['flash']
    config = bytearray(p.pm.key_blob(state['key'] if key is None else key))
    for name, value in [('OSC', b'OSC0'), ('OSC_FREQ', struct.pack('<I', 24000000)),
                        ('SYS_CLK', struct.pack('<I', 360000000)),
                        ('HSB_DIV', struct.pack('<I', 1)), ('LSB_DIV', struct.pack('<I', 2))]:
        config += bytes([len(value)]) + name.encode() + b'\0' + value
    config += b'\0'
    image[64:96] = p.add_ctl._dec(p.pm.entry(p.pm.crc16(config), 0x38d0, len(config), 2, 0x80, 0, 'isd_config.ini'))
    image[32:64] = p.add_ctl._dec(p.pm.entry(0xffff, 0xa0, 0x3830, 0, 0, 0, 'uboot.boot'))
    image[0x38d0:0x38d0+len(config)] = config
    image[0xc1000:0xc1224] = b'\x53' * 0x224
    image[0xc1224:0xc122c] = b'trimdat\0'
    image[0xfff00:] = b'\xa7' * 256
    return bytes(image)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--package', required=True, type=Path)
    ap.add_argument('--package-sha256', required=True)
    ap.add_argument('--controller-sha256', required=True)
    ap.add_argument('--upstream', required=True, type=Path)
    a = ap.parse_args()
    raw = a.package.read_bytes(); p.pinned(raw, a.package_sha256, 'fixture package')
    up = a.upstream.resolve(); boot = (up / 'build/smk37/uboot.boot').read_bytes()
    image = synthetic_snapshot(raw)
    pins = dict(model='elite', snapshot_sha256=p.digest(image), package_sha256=a.package_sha256,
                container_sha256=p.digest(boot), controller_sha256=a.controller_sha256)
    head, updated, report = p.candidate(image, raw, boot, pins, up)
    assert report['application_controller_ota_unchanged'] and not report['device_access']
    assert head[:32] == image[:32] and head[64:0xa0] == image[64:0xa0]
    assert head[0x38d0:] == image[0x38d0:p.HEAD]
    old = p.add_ctl._open(p.add_ctl.logical_of(raw)[1])
    new = p.add_ctl._open(p.add_ctl.logical_of(updated)[1])
    assert old['flash'][p.HEAD:] == new['flash'][p.HEAD:]
    assert p.add_ctl.controller_of(p.add_ctl.logical_of(updated)[1]) == p.add_ctl.controller_of(p.add_ctl.logical_of(raw)[1])
    checks = 1
    def refused(call):
        nonlocal checks
        try:
            call()
        except (ValueError, p.add_ctl.PkgError):
            checks += 1
        else:
            raise AssertionError('unsafe input was accepted')
    for field in ('snapshot_sha256', 'package_sha256', 'container_sha256', 'controller_sha256'):
        bad = dict(pins); bad[field] = '0' * 64
        refused(lambda: p.candidate(image, raw, boot, bad, up))
    bad = dict(pins, model='pro'); refused(lambda: p.candidate(image, raw, boot, bad, up))
    bad_image = synthetic_snapshot(raw, old['key'] ^ 1)
    bad = dict(pins, snapshot_sha256=p.digest(bad_image))
    refused(lambda: p.candidate(bad_image, raw, boot, bad, up))
    for address, bank in ((0x8000, 0xffffffff), (0x4000, 0xc0000)):
        changed = bytearray(image)
        record = p.add_ctl._dec(changed[96:128])
        struct.pack_into('<II', record, 4, address, bank)
        struct.pack_into('<H', record, 0, p.pm.crc16(record[2:]))
        changed[96:128] = p.add_ctl._dec(record)
        bad = dict(pins, snapshot_sha256=p.digest(changed))
        refused(lambda: p.candidate(changed, raw, boot, bad, up))
    changed = bytearray(image)
    changed[128:160] = p.add_ctl._dec(p.pm.entry(0xffff, 0x8000, 0xc0000, 0x81, 0xff, 0, 'app_dir_head2'))
    bad = dict(pins, snapshot_sha256=p.digest(changed))
    refused(lambda: p.candidate(changed, raw, boot, bad, up))
    bare = p.add_ctl.add(raw, None); bad = dict(pins, package_sha256=p.digest(bare))
    refused(lambda: p.candidate(image, bare, boot, bad, up))
    malformed = bytearray(raw); malformed[2048] ^= 1
    bad = dict(pins, package_sha256=p.digest(malformed))
    refused(lambda: p.candidate(image, malformed, boot, bad, up))
    truncated = image[:-1]; bad = dict(pins, snapshot_sha256=p.digest(truncated))
    refused(lambda: p.candidate(truncated, raw, boot, bad, up))
    # Match actual unchanged installer behavior with a different reference head.
    with tempfile.TemporaryDirectory(prefix='mboot-caller-test-') as directory:
        work = Path(directory)
        ref = work / 'head'; ref.write_bytes(head)
        logical = p.add_ctl.logical_of(updated)[1]
        assert p.fm1_install.head_compare(logical, ref)[0]
        broken = bytearray(head); broken[0xa0] ^= 1; ref.write_bytes(broken)
        assert not p.fm1_install.head_compare(logical, ref)[0]
        (work / 'snapshot').write_bytes(image); (work / 'pins.json').write_text(json.dumps(pins))
        command = [sys.executable, '-B', str(ROOT / 'tools/mboot/prepare.py'),
                   '--snapshot', str(work / 'snapshot'), '--pins', str(work / 'pins.json'),
                   '--package', str(a.package.resolve()), '--container', str(up / 'build/smk37/uboot.boot'),
                   '--upstream', str(up)]
        subprocess.run(command, check=True, stdout=subprocess.DEVNULL)
        assert set(x.name for x in work.iterdir()) == {'head', 'snapshot', 'pins.json'}
        destination = work / 'private-candidate'
        subprocess.run(command + ['--candidate-dir', str(destination)], check=True, stdout=subprocess.DEVNULL)
        assert (destination / 'app-package.NOT-INSTALLABLE-candidate').read_bytes() == updated
        assert (destination / 'adoption-head.PRIVATE-candidate').read_bytes() == head
        assert destination.stat().st_mode & 0o777 == 0o700
        for path in destination.iterdir():
            assert path.stat().st_mode & 0o777 == 0o600
        assert subprocess.run(command + ['--candidate-dir', str(destination)], stdout=subprocess.DEVNULL,
                              stderr=subprocess.DEVNULL).returncode != 0
        checks += 3
    assert p.digest(a.package.read_bytes()) == a.package_sha256
    print('PASS %d offline caller/package groups; original package unchanged; synthetic snapshot deleted' % checks)


if __name__ == '__main__':
    main()
