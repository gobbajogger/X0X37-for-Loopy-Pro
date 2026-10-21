#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Installer and sample preservation regressions; mocked MIDI/NOR only.

Run: python3 tests/installer_safety_test.py
"""
import contextlib
import io
import math
import struct
import sys
import tempfile
import unittest
import wave
from pathlib import Path
from unittest.mock import patch

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import install_test as T
from slot_mock import MockDevice, SMK_MAP
import fm1_sample_upload as upload
import sampleio as sio
import x0x_slots as xs

I = T.I
I.DELAY.update(loader=0.1, reboot=0.1)


class Keyboards:
    def __init__(self, *devices):
        self.devices = list(devices)

    def input_names(self):
        return [n for d in self.devices for n in d.input_names()]

    output_names = input_names

    def open(self, name, out):
        for d in self.devices:
            if name in d.input_names():
                return d.open(name, out)
        raise IOError("port is gone")


class InstallerSafety(unittest.TestCase):
    def test_bystanders_never_receive_update(self):
        for model in ("Pro", "Elite"):
            for loader in (False, True):
                for reverse in (False, True):
                    with self.subTest(model=model, loader=loader, reverse=reverse):
                        product = f"SMK-37 {model}_900"
                        raw = T.package(product)
                        image = I.logical_image(raw)
                        target = T.FakeSMK(image, identity=f"SMK-37 {model}_016", name=f"SMK-37 {model} TARGET", after_write=product)
                        other = T.FakeSMK(image, identity=("ota-" if loader else "") + product,
                                          name="Felucca Update OTHER" if loader else "SMK-37 OTHER")
                        devs = [other, target] if reverse else [target, other]
                        rc, _, err = T.cli([T.pkgfile("safe.fwsc", raw), "--yes", "--port", "TARGET"], Keyboards(*devs))
                        self.assertEqual(rc, 6, err)
                        self.assertIn("cannot bind", err)
                        self.assertEqual((target.upgrades, other.upgrades), (0, 0))

    def test_duplicate_port_names_are_ambiguous(self):
        raw = T.package()
        a, b = [T.FakeSMK(I.logical_image(raw)) for _ in range(2)]
        rc, _, err = T.cli([T.pkgfile("duplicates.fwsc", raw), "--yes"], Keyboards(a, b))
        self.assertEqual(rc, 6, err)
        self.assertEqual((a.upgrades, b.upgrades), (0, 0))

    def test_loader_arriving_during_transition_is_ambiguous(self):
        raw = T.package()
        image = I.logical_image(raw)
        target = T.FakeSMK(image)
        other = T.FakeSMK(image, identity="ota-SMK-37 Pro_900", name="Felucca Update OTHER")
        backend = Keyboards(target)
        up = I.Updater(backend)
        selected = up.find()

        def step(phase, *_):
            if phase == "loader":
                backend.devices.append(other)

        with self.assertRaises(I.InstallError) as cm:
            up.install(selected, image, "SMK-37 Pro_900", step)
        self.assertEqual(cm.exception.code, "ambiguous")
        self.assertEqual((target.upgrades, other.upgrades), (1, 0))

    def test_loader_model_is_rechecked(self):
        class WrongLoader(T.FakeSMK):
            def boot(self, identity, name):
                super().boot("ota-FM-1_900" if identity.startswith("ota-") else identity, name)
        raw = T.package()
        d = WrongLoader(I.logical_image(raw))
        rc, _, err = T.cli([T.pkgfile("wrong-loader.fwsc", raw), "--yes"], d)
        self.assertEqual(rc, 6, err)
        self.assertEqual(d.upgrades, 1)

    def test_single_pro_and_elite_tolerate_reenumeration(self):
        for model in ("Pro", "Elite"):
            product = f"SMK-37 {model}_900"
            raw = T.package(product)
            d = T.FakeSMK(I.logical_image(raw), identity=f"SMK-37 {model}_016", name=f"SMK-37 {model} TARGET", after_write=product)
            rc, _, err = T.cli([T.pkgfile("single.fwsc", raw), "--yes", "--port", "TARGET"], d)
            self.assertEqual(rc, 0, err)
            self.assertEqual(d.upgrades, 2)
            self.assertEqual(d.name, "Felucca")

    def test_restore_keeps_verified_port(self):
        image = I.logical_image(T.package())
        d = T.FakeSMK(image)
        up = I.Updater(d)
        dev = up.find()
        up.bind(dev)
        dev.link.close()
        up.verified = True
        d.boot("SMK-37 Pro_900", "Felucca OTHER")
        with self.assertRaises(I.InstallError) as cm:
            up.wait_for(lambda i: not i.loader, 0.1)
        self.assertEqual(cm.exception.code, "ambiguous")
        self.assertEqual(d.upgrades, 0)

    def test_unknown_layout_blocks_update_before_writes(self):
        class MissingLayout(T.FakeSMK):
            malformed = None

            def rx(self, data):
                if data == I.ED_HDR + bytes([15, 0xF7]):
                    if self.malformed is not None:
                        self.tx(I.ED_HDR + bytes([15]) + bytes(self.malformed) + bytes([0xF7]))
                    return
                super().rx(data)

        raw = T.package("SMK-37 Pro_016", marker=False)
        for reply in (None, [], [0, 44], [3, 44]):
            d = MissingLayout(I.logical_image(raw), identity="SMK-37 Pro_900", after_write="SMK-37 Pro_016")
            d.malformed = reply
            rc, _, err = T.cli([T.pkgfile("stock.fwsc", raw), "--yes", "--force"], d)
            self.assertEqual(rc, 7, err)
            self.assertIn("preservation is unknown", err)
            self.assertEqual(d.upgrades, 0)
        d = MissingLayout(I.logical_image(raw), identity="SMK-37 Pro_900", after_write="SMK-37 Pro_016")
        rc, _, err = T.cli([T.pkgfile("opt-out.fwsc", raw), "--yes", "--force", "--no-slots"], d)
        self.assertEqual(rc, 0, err)
        self.assertEqual(d.upgrades, 2)


class SampleSafety(unittest.TestCase):
    @staticmethod
    def image(tail=False):
        h, d = sio.user_slot("SAFE", [([100, -100] * 100, 60, None, None)])
        image = bytearray(h.ljust(xs.DATA_OFF, b"\xff") + d)
        if tail:
            image[xs.HDR_LEN:xs.DATA_OFF] = bytes(range(32))
        return bytes(image)

    def test_full_header_roundtrip_and_legacy_compatibility(self):
        for full, tail in ((True, True), (False, False)):
            with self.subTest(full=full, tail=tail):
                src = MockDevice(count=1)
                img = self.image(tail)
                src.load(0, img[:512], img[512:])
                lay, images = xs.backup(src)
                meta, images = xs.read_archive(xs.make_archive(lay, images))
                dest = MockDevice(count=1, full_header=full)
                self.assertEqual(xs.restore(dest, meta, images), [])
                self.assertEqual(dest.image(0), img)
                self.assertEqual(9 in dest.log, tail)

    def test_legacy_tail_refusal_precedes_erase(self):
        dest = MockDevice(count=1, full_header=False)
        old = self.image(False)
        dest.load(0, old[:512], old[512:])
        before = bytes(dest.fl.mem)
        with self.assertRaises(xs.SlotError):
            xs.write_slot(dest, 0, self.image(True))
        self.assertEqual(bytes(dest.fl.mem), before)
        self.assertEqual(dest.log, [9])

    def test_sample_upload_uses_selected_capacity_before_erase(self):
        with tempfile.TemporaryDirectory(prefix="x0x-upload-safe-") as td:
            wav = Path(td) / "five-seconds.wav"
            with wave.open(str(wav), "wb") as f:
                f.setparams((1, 2, 22050, 0, "NONE", "not compressed"))
                f.writeframes(struct.pack("<110250h", *[int(10000 * math.sin(i * 0.07)) for i in range(110250)]))
            device = MockDevice(slots=SMK_MAP)
            old = self.image()
            device.load(2, old[:512], old[512:])
            before = bytes(device.fl.mem)
            args = ["fm1_sample_upload.py", "--no-trim", "load", "3", "LONG", str(wav)]
            with patch.object(upload, "Link", return_value=device), patch.object(sys, "argv", args), contextlib.redirect_stdout(io.StringIO()):
                with self.assertRaises(SystemExit) as cm:
                    upload.main()
            self.assertIn("nothing was erased", str(cm.exception))
            self.assertEqual(device.log, [xs.CMD_INFO])
            self.assertEqual(bytes(device.fl.mem), before)
            with wave.open(str(wav), "wb") as f:
                f.setparams((1, 2, 22050, 0, "NONE", "not compressed"))
                f.writeframes(struct.pack("<176400h", *[int(10000 * math.sin(i * 0.07)) for i in range(176400)]))
            args = ["fm1_sample_upload.py", "--no-trim", "load", "2", "LONG", str(wav)]
            with patch.object(upload, "Link", return_value=device), patch.object(sys, "argv", args), contextlib.redirect_stdout(io.StringIO()):
                upload.main()
            self.assertEqual(device.data_len(1), 88200)
            self.assertEqual(device.image(2), old)

    def test_malformed_extended_layout_is_rejected(self):
        reply = MockDevice(slots=SMK_MAP).req(xs.CMD_INFO, [])
        for bad in ([0, 80], [2, 0], reply[:-1], reply + [0]):
            with self.assertRaises(xs.SlotError):
                xs.parse_info(bad)


if __name__ == "__main__":
    unittest.main(verbosity=2)
