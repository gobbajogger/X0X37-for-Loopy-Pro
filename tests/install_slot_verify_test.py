#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Bound post-install sample readback/CRC regressions; fake MIDI and NOR only."""
import os
import sys
import tempfile
import unittest
import zlib
from pathlib import Path
from unittest.mock import patch

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import install_test as T
from slot_mock import Flash, MockDevice, SMK_MAP
import fm1pkg_make
import sampleio as sio
import x0x_slots as xs

I = T.I
I.DELAY.update(loader=0.1, reboot=0.1)


class Keyboard(T.FakeSMK):
    def __init__(self, image, model, flash, before=SMK_MAP, after=SMK_MAP, mutate=None):
        self.flash, self.before, self.after, self.mutate = flash, before, after, mutate
        self.slot_cmds = []
        super().__init__(image, identity=f"SMK-37 {model}_9001000", name=f"SMK-37 {model} TARGET",
                         after_write=f"SMK-37 {model}_9001100")

    def boot(self, identity, name):
        super().boot(identity, name)
        post = self.upgrades == 2 and not identity.startswith("ota-")
        if post and self.mutate:
            self.mutate(self.flash)
        self.mock = MockDevice(slots=self.after if post else self.before, flash=self.flash)

    def rx(self, data):
        if data[:4] == I.ED_HDR and data[-1] == 0xF7:
            cmd = data[4]
            self.slot_cmds.append((self.upgrades == 2, cmd))
            reply = self.mock.req(cmd, list(data[5:-1]))
            self.tx(I.ED_HDR + bytes([cmd]) + bytes(reply) + b"\xF7")
            return
        super().rx(data)


class Bus:
    def __init__(self, *devices):
        self.devices = list(devices)

    def input_names(self):
        return [n for d in self.devices for n in d.input_names()]

    output_names = input_names

    def open(self, name, out):
        return next(d for d in self.devices if name in d.input_names()).open(name, out)


def image(name="PRESERVE", tail=False):
    hdr, data = sio.user_slot(name, [([1000, -1000] * 600, 60, None, None)])
    head = bytearray(hdr.ljust(xs.DATA_OFF, b"\xff"))
    if tail:
        head[xs.HDR_LEN:] = bytes(range(32))
    return bytes(head) + data


def populate(flash, images, slotmap=SMK_MAP):
    mock = MockDevice(slots=slotmap, flash=flash)
    for k, img in enumerate(images):
        if img is not None:
            mock.load(k, img[:xs.DATA_OFF], img[xs.DATA_OFF:])


class SlotVerification(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="x0x-slot-verify-")
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.saved = self.root / "before.x0xslots"
        self.images = [image(tail=True), None, image("OTHER")]

    def device(self, model="Pro", after=SMK_MAP, mutate=None, images=None):
        raw = bytearray(T.package(f"SMK-37 {model}_9001100"))
        blob = fm1pkg_make.slotmap_blob(after)
        raw[0x1A00:0x1A00 + len(blob)] = blob
        self.package = self.root / "checked.fwsc"
        self.package.write_bytes(raw)
        flash = Flash()
        populate(flash, self.images if images is None else images)
        return Keyboard(I.logical_image(raw), model, flash, after=after, mutate=mutate)

    def run_install(self, device, extra=()):
        return T.cli([str(self.package), "--yes", "--port", "TARGET", "--backup", str(self.saved),
                      "--verify-slots", *extra], device)

    def assert_backup(self):
        self.assertEqual(xs.read_archive(self.saved.read_bytes())[1], self.images)

    def test_pro_and_elite_complete_image_crcs_and_empty_slots(self):
        for model in ("Pro", "Elite"):
            with self.subTest(model=model):
                dev = self.device(model)
                rc, out, err = self.run_install(dev)
                self.assertEqual(rc, 0, err)
                self.assert_backup()
                self.assertEqual(dev.upgrades, 2)
                self.assertEqual(dev.name, "Felucca")
                self.assertIn(f"CRC32 {zlib.crc32(self.images[0]):08x} (backup slot 1): OK", out)
                self.assertIn("slot 2 read back: empty, expected empty: OK", out)
                self.assertIn("all 3 sample slots compared: OK", out)
                self.assertEqual({cmd for post, cmd in dev.slot_cmds if post}, {xs.CMD_INFO, xs.CMD_READ})

    def test_reserved_header_mismatch_keeps_backup_and_never_rewrites(self):
        def corrupt(flash):
            flash.mem[SMK_MAP[0][0] + 500] ^= 1
        dev = self.device(mutate=corrupt)
        rc, out, err = self.run_install(dev)
        self.assertEqual(rc, 7, err)
        self.assertIn("slot 1 read back: CRC32", out)
        self.assertIn("MISMATCH", out)
        self.assertIn("full slot readback/CRC comparison failed", err)
        self.assertIn("No automatic rewrite was attempted", err)
        self.assertIn(str(self.saved), err)
        self.assert_backup()
        self.assertEqual({cmd for post, cmd in dev.slot_cmds if post}, {xs.CMD_INFO, xs.CMD_READ})

    def test_unexpected_sample_in_empty_slot_fails_without_erase(self):
        dev = self.device(mutate=lambda f: populate(f, [None, image("UNEXPECTED"), None]))
        rc, out, err = self.run_install(dev)
        self.assertEqual(rc, 7, err)
        self.assertIn("expected empty: MISMATCH", out)
        self.assert_backup()
        self.assertEqual({cmd for post, cmd in dev.slot_cmds if post}, {xs.CMD_INFO, xs.CMD_READ})

    def test_post_verify_bystander_or_replacement_is_never_read(self):
        original = I.Updater.verify
        for replace in (False, True):
            with self.subTest(replace=replace):
                dev = self.device()
                other = T.FakeSMK(dev.image, identity=dev.after_write, name="Felucca OTHER")
                bus = Bus(dev)

                def verified(up, product, step):
                    result = original(up, product, step)
                    bus.devices = [other] if replace else [other, dev]
                    return result

                with patch.object(I.Updater, "verify", verified):
                    rc, out, err = self.run_install(bus)
                self.assertEqual(rc, 6, err)
                self.assertEqual(other.sent, [])
                self.assertEqual([cmd for post, cmd in dev.slot_cmds if post], [])
                self.assert_backup()

    def test_migration_then_all_slots_compare_with_original_sources(self):
        moved = [(0xC2000, 0x14000), (0xD7000, 0x1D000), (0xB0000, 0xB000)]
        self.images = [image(tail=True), image("MOVED", tail=True), None]
        dev = self.device(after=moved)
        rc, out, err = self.run_install(dev)
        self.assertEqual(rc, 0, err)
        self.assertIn("slot 2 written back to slot 2 and read back: ok", out)
        self.assertIn("(backup slot 2): OK", out)
        self.assertIn("all 3 sample slots compared: OK", out)
        self.assert_backup()
        self.assertEqual(sum(cmd == xs.CMD_BEGIN for _, cmd in dev.slot_cmds), 1)

    def test_unexpected_post_install_map_fails_without_slot_writes(self):
        dev = self.device()
        dev.after = SMK_MAP[:2]
        rc, out, err = self.run_install(dev)
        self.assertEqual(rc, 7, err)
        self.assertIn("slot map differs from the checked package", err)
        self.assert_backup()
        self.assertEqual([cmd for post, cmd in dev.slot_cmds if post], [xs.CMD_INFO])

    def test_option_implies_backup_and_refuses_unverifiable_targets(self):
        dev = self.device()
        old = Path.cwd()
        try:
            os.chdir(self.root)
            rc, out, err = T.cli([str(self.package), "--yes", "--verify-slots"], dev)
        finally:
            os.chdir(old)
        self.assertEqual(rc, 0, err)
        backups = list(self.root.glob("x0x-slots-before-install-*.x0xslots"))
        self.assertEqual(len(backups), 1)
        self.assertEqual(xs.read_archive(backups[0].read_bytes())[1], self.images)
        dev = self.device()
        self.package.write_bytes(T.package("SMK-37 Pro_016", marker=False))
        rc, out, err = self.run_install(dev, ["--force"])
        self.assertEqual(rc, 7, err)
        self.assertIn("needs a package with X0X sample slots", err)
        self.assertEqual(dev.upgrades, 0)
        self.assertFalse(self.saved.exists())
        dev = self.device()
        dev.boot("ota-SMK-37 Pro_900", "Felucca TARGET")
        rc, out, err = self.run_install(dev)
        self.assertEqual(rc, 7, err)
        self.assertIn("need the running firmware", err)
        self.assertEqual(dev.upgrades, 0)
        with self.assertRaises(SystemExit) as cm:
            self.run_install(self.device(), ["--no-slots"])
        self.assertEqual(cm.exception.code, 2)


if __name__ == "__main__":
    unittest.main(verbosity=2)
