#!/usr/bin/env python3
"""Synthetic, hardware-free tests for native package and controller integrity."""
from __future__ import annotations

import importlib.util
from pathlib import Path
import struct
import sys
import tempfile
import unittest
import zlib

TOOLS = Path(__file__).resolve().parents[2] / 'tools'
sys.path.insert(0, str(TOOLS))
import add_ctl
import fm1pkg_make as pkg
import check_native_build_output as guard


class OutputGuardTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        for name, data in {'uboot.boot': bytes(range(128)), 'cfg_tool.bin': b'CFG' * 60,
                           'cfg/eq_cfg_hw.bin': b'EQ' * 30}.items():
            path = self.root / 'cpu/wl82/tools' / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        pkg.set_board('smk37')
        pkg.SDK = self.root

    def package(self, model='elite'):
        content = pkg.ufw(pkg.flash_image(b'\xab' * 2048, pkg.KEY),
                          b'FAKE OTA PAYLOAD' * 15, guard.PRODUCTS[model])
        path = self.root / 'package.fwsc'
        path.write_bytes(content)
        return path

    def controller(self):
        buf = bytearray(128)
        struct.pack_into('<IHHIIIIIIIIIIII', buf, 0,
                         add_ctl.HDR_MAGIC, add_ctl.HDR_VERSION, add_ctl.HDR_SIZE,
                         1, len(buf), 0, add_ctl.ABI,
                         add_ctl.XIP + 80, add_ctl.XIP + 96,
                         add_ctl.RAM_LO + 16, add_ctl.RAM_LO + 256,
                         add_ctl.RAM_LO, add_ctl.RAM_LO + 512, 0, 0)
        crc = zlib.crc32(buf[:16] + b'\0' * 4 + buf[20:])
        struct.pack_into('<I', buf, 16, crc)
        path = self.root / 'synthetic-controller.img'
        path.write_bytes(buf)
        return path

    def test_both_matching_synthetic_models_accepted(self):
        for model in ('elite', 'pro'):
            with self.subTest(model=model):
                got = guard.check_firmware(self.package(model), model)
                self.assertEqual(got['model'], model)
                self.assertEqual(got['payloads'], ['flash.bin', 'ota.bin'])

    def test_wrong_model_fails(self):
        for model, other in [('elite', 'pro'), ('pro', 'elite')]:
            with self.subTest(model=model):
                with self.assertRaisesRegex(ValueError, 'wrong firmware model'):
                    guard.check_firmware(self.package(model), other)

    def test_corrupt_flash_or_loader_fails_crc(self):
        for segment in ('flash.bin', 'ota.bin'):
            with self.subTest(segment=segment):
                path = self.package()
                blob = bytearray(path.read_bytes())
                _, logical = add_ctl.logical_of(blob)
                hdr = bytearray(logical[:64]); pkg.enc(hdr, 0, len(hdr))
                count = struct.unpack_from('<H', hdr, 8)[0]
                listing = logical[64:64 + count * 80]
                index = 0 if segment == 'flash.bin' else 1
                entry = bytearray(listing[index * 80:(index + 1) * 80]); pkg.enc(entry, 0, 80)
                off, size = struct.unpack_from('<II', entry, 8)
                self.assertGreater(size, 0)
                # Converted from UFW logical offset to the fwsc inserted-marker offset.
                raw_off = off + (20 if off >= 20 * 47 else off // 47)
                blob[raw_off + 10] ^= 0x01
                path.write_bytes(blob)
                with self.assertRaisesRegex((ValueError, add_ctl.PkgError), 'CRC'):
                    guard.check_firmware(path, 'elite')

    def test_corrupt_and_truncated_package_refused(self):
        path = self.package()
        path.write_bytes(path.read_bytes()[:55])
        with self.assertRaises(add_ctl.PkgError):
            guard.check_firmware(path, 'elite')

    def test_synthetic_controller_crc_and_corruption(self):
        path = self.controller()
        guard.check_controller(path)
        invalid = bytearray(path.read_bytes())
        invalid[90] ^= 1
        path.write_bytes(invalid)
        with self.assertRaisesRegex(ValueError, 'CRC mismatch'):
            guard.check_controller(path)


if __name__ == '__main__':
    unittest.main(verbosity=2)
