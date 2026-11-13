"""Synthetic format tests; these are not firmware or owner-scene results."""
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib


def pack(adpcm=b"\x17" * 32, name=b"TEST", bars=1, rate=44100):
    body = name.ljust(8, b"\0") + struct.pack("<IB3xI", len(adpcm) * 2, bars, len(adpcm)) + adpcm
    return b"X0XBRKPK" + struct.pack("<HHII", 1, 1, rate, zlib.crc32(body)) + body


class FixtureTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        path = Path(__file__).with_name("fixture.py")
        cls.mod = None
        if path.exists():
            spec = importlib.util.spec_from_file_location("dropout_fixture", path)
            cls.mod = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(cls.mod)

    def setUp(self):
        self.assertIsNotNone(self.mod, "sample-slot fixture implementation is missing")

    def test_overlay_preserves_app_and_all_other_flash_bytes(self):
        base = bytes(range(256)) * 4096
        result, report = self.mod.make_fixture(base, pack(), "TEST", 1)
        self.assertEqual(result[:0xC2000], base[:0xC2000])
        self.assertEqual(result[0xD6000:], base[0xD6000:])
        self.assertEqual(result[0xC2000:0xC2008], b"FSMP\x01\x00\x01\x00")
        self.assertEqual(result[0xC2200:0xC2220], b"\x17" * 32)
        self.assertEqual(struct.unpack_from("<5I2h4B", result, 0xC2020),
                         (0, 64, 0, 64, 65536, 960, 0, 0, 0, 127, 1))
        self.assertEqual(report["status"], "PREPARED_NOT_RUN")
        self.assertFalse(report["owner_scene_exact"])
        self.assertEqual(report["sample"]["adpcm_sha256"], hashlib.sha256(b"\x17" * 32).hexdigest())

    def test_corrupt_pack_data_rejected(self):
        bad = bytearray(pack()); bad[-1] ^= 1
        with self.assertRaisesRegex(ValueError, "CRC"):
            self.mod.make_fixture(bytes(1 << 20), bad, "TEST", 1)

    def test_second_sample_slot_cannot_silently_remain_loaded(self):
        base, _ = self.mod.make_fixture(bytes(1 << 20), pack(name=b"OTHER"), "OTHER", 2)
        with self.assertRaisesRegex(ValueError, "other sample slot"):
            self.mod.make_fixture(base, pack(), "TEST", 1)

    def test_truncated_and_trailing_pack_rejected(self):
        for bad in (pack()[:-1], pack() + b"\0"):
            with self.subTest(size=len(bad)), self.assertRaises(ValueError):
                self.mod.make_fixture(bytes(1 << 20), bad, "TEST", 1)

    def test_slot_capacity_and_flash_size_rejected(self):
        with self.assertRaisesRegex(ValueError, "capacity"):
            self.mod.make_fixture(bytes(1 << 20), pack(bytes(33000)), "TEST", 3)
        with self.assertRaisesRegex(ValueError, "1048576"):
            self.mod.make_fixture(bytes(64), pack(), "TEST", 1)

    def test_pack_bars_are_not_misreported_as_runtime_bars(self):
        _, report = self.mod.make_fixture(bytes(1 << 20), pack(bytes(44100), bars=2), "TEST", 1)
        self.assertEqual(report["sample"]["pack_bars"], 2)
        self.assertEqual(report["sample"]["user_slot_bars_inferred"], 1)

    def test_cli_hash_mismatch_does_not_create_output(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); (root / "base.bin").write_bytes(bytes(1 << 20)); (root / "pack.bin").write_bytes(pack())
            result = subprocess.run([sys.executable, str(Path(__file__).with_name("fixture.py")),
                "--flash", str(root / "base.bin"), "--pack", str(root / "pack.bin"),
                "--pack-sha256", "0" * 64, "--loop", "TEST", "--out", str(root / "out")], capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("SHA256", result.stderr)
            self.assertFalse((root / "out").exists())

    def test_cli_records_hashes_and_refuses_existing_output(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); (root / "base.bin").write_bytes(bytes(1 << 20)); (root / "pack.bin").write_bytes(pack())
            command = [sys.executable, str(Path(__file__).with_name("fixture.py")),
                "--flash", str(root / "base.bin"), "--pack", str(root / "pack.bin"),
                "--pack-sha256", hashlib.sha256(pack()).hexdigest(), "--loop", "TEST", "--out", str(root / "out")]
            first = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(first.returncode, 0, first.stderr)
            report = json.loads((root / "out" / "fixture.json").read_text())
            self.assertEqual(report["flash_sha256"], hashlib.sha256((root / "out" / "fixture.flash").read_bytes()).hexdigest())
            self.assertEqual(report["sample"]["pack_sha256"], hashlib.sha256(pack()).hexdigest())
            second = subprocess.run(command, capture_output=True, text=True)
            self.assertNotEqual(second.returncode, 0)


if __name__ == "__main__":
    unittest.main()
