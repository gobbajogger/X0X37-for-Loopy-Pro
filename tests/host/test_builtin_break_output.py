#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exercise real generator output admission, including buffered close failures."""
import base64
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import break_pack as bp


class BuiltinBreakOutput(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix="x0x-break-output-")
        cls.root = Path(cls.temp.name)
        gen = cls.root / "gen"
        gen.mkdir()
        subprocess.run(["python3", str(ROOT / "tools/gen_drum_samples.py"),
                        str(gen / "x0x_drum_samples.h")], check=True,
                       stdout=subprocess.DEVNULL)
        cls.binary = cls.root / "generator"
        subprocess.run(shlex.split(os.environ.get("CC", "cc")) + [
            "-O2", "-ffp-contract=off", "-w", "-I" + str(ROOT / "firmware/src"),
            "-I" + str(gen), "-o", str(cls.binary),
            str(ROOT / "tools/gen_builtin_break.c"),
            str(ROOT / "firmware/src/dsp/drum909.c"),
            str(ROOT / "firmware/src/dsp/x0x_param.c"), "-lm"], check=True)

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def invoke(self, *args):
        return subprocess.run([str(self.binary), *map(str, args)],
                              capture_output=True, text=True)

    def assert_failure(self, result):
        self.assertNotEqual(result.returncode, 0)
        self.assertNotIn("builtin break: A", result.stdout)

    def test_success_bytes_are_deterministic(self):
        dirs = [self.root / "normal-a", self.root / "normal-b"]
        for directory in dirs:
            directory.mkdir()
            result = self.invoke(directory / "bank.h", directory)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("builtin break: A", result.stdout)
        names = ["bank.h", "909GR.wav", "909FL.wav", "909GR.adpcm",
                 "909FL.adpcm", "pack.txt"]
        for name in names:
            self.assertEqual((dirs[0] / name).read_bytes(), (dirs[1] / name).read_bytes())
            self.assertGreater((dirs[0] / name).stat().st_size, 0)

    def test_current_factory_set_is_preserved(self):
        directory = self.root / "stock-identity"
        directory.mkdir()
        result = self.invoke(directory / "bank.h", directory)
        self.assertEqual(result.returncode, 0, result.stderr)
        header = (directory / "bank.h").read_text()
        self.assertIn("#define X0X_NBREAKS 2", header)
        for name in ["909 GR", "909 FL"]:
            self.assertIn('"' + name + '"', header)
        self.assertEqual(len([line for line in (directory / "pack.txt").read_text().splitlines()
                             if line and not line.startswith("#")]), 2)
        self.assertEqual([loop["name"] for loop in bp.loops_from_export(directory)],
                         ["909 GR", "909 FL"])

    def website_pack(self):
        # Decode only the generated string-literal expression; never execute JS.
        text = (ROOT / "web/factory_break_pack.js").read_text()
        text = re.sub(r"(?m)^//[^\n]*\n", "", text).strip()
        match = re.fullmatch(
            r'export const FACTORY_BREAK_PACK = Uint8Array.from\(atob\(\s*'
            r'((?:"[A-Za-z0-9+/=]+"\s*\+\s*)*"[A-Za-z0-9+/=]+")'
            r'\s*\), \(c\) => c.charCodeAt\(0\)\);', text)
        self.assertIsNotNone(match, "expected generated FACTORY_BREAK_PACK literals")
        return base64.b64decode("".join(re.findall(r'"([A-Za-z0-9+/=]+)"',
                                                  match.group(1))), validate=True)

    def test_website_factory_pack_matches_designated_export(self):
        # The website bank is the designated stock export, not the 909 generator.
        stock = ROOT / "assets/factory-breaks"
        pack = self.website_pack()
        parsed = bp.parse_pack(pack)
        self.assertIsNotNone(parsed)
        rate, loops = parsed
        self.assertEqual(rate, 44100)
        self.assertEqual([loop["name"] for loop in loops], ["CRAMEN", "TASTY", "HOTPANT"])
        self.assertEqual(loops, bp.loops_from_export(stock))
        self.assertEqual(pack, (stock / "factory-break-pack.bin").read_bytes())
        provenance = json.loads((stock / "provenance.json").read_text())
        bank = provenance["bank"]
        self.assertEqual(len(loops), bank["count"])
        self.assertEqual(rate, bank["rate_hz"])
        self.assertEqual(len(pack), bank["bytes"])
        self.assertEqual(hashlib.sha256(pack).hexdigest(), bank["sha256"])
        self.assertEqual(f"{struct.unpack_from('<I', pack, 16)[0]:08x}",
                         bank["crc32_body_hex"])
        self.assertEqual(len(loops), len(provenance["loops"]))
        for loop, source in zip(loops, provenance["loops"]):
            with self.subTest(name=loop["name"]):
                self.assertEqual(loop["name"], source["name"])
                self.assertEqual(loop["nsamples"], source["prepared_frames"])
                self.assertEqual(loop["bars"], source["bars"])
                self.assertEqual(len(loop["adpcm"]), source["adpcm_bytes"])
                self.assertEqual(hashlib.sha256(loop["adpcm"]).hexdigest(),
                                 source["adpcm_sha256"])

    def test_website_pack_rejects_invalid_data(self):
        pack = self.website_pack()
        self.assertIsNotNone(bp.parse_pack(pack), "positive control must parse")
        corrupt = bytearray(pack)
        corrupt[-1] ^= 1
        invalid = {
            "bad magic": b"!" + pack[1:],
            "unsupported version": pack[:8] + b"\x02\x00" + pack[10:],
            "zero loops": pack[:10] + b"\x00\x00" + pack[12:],
            "invalid rate": pack[:12] + b"\x00" * 4 + pack[16:],
            "truncated header": pack[:19],
            "truncated loop metadata": pack[:39],
            "truncated payload": pack[:-1],
            "checksum corruption": corrupt,
            "trailing data": pack + b"\x00",
        }
        for reason, data in invalid.items():
            with self.subTest(reason=reason):
                self.assertIsNone(bp.parse_pack(data), reason)

    def test_missing_header_parent(self):
        self.assert_failure(self.invoke(self.root / "missing" / "bank.h"))

    def test_missing_export_directory(self):
        self.assert_failure(self.invoke(self.root / "missing-export.h", self.root / "absent"))

    def test_export_path_too_long(self):
        self.assert_failure(self.invoke(self.root / "long-export.h", "x" * 512))

    def test_header_write_failure(self):
        if not Path("/dev/full").exists():
            self.fail("/dev/full unavailable: required output-failure proof cannot run")
        self.assert_failure(self.invoke("/dev/full"))

    def test_each_export_write_and_open_failure(self):
        if not Path("/dev/full").exists():
            self.fail("/dev/full unavailable: required output-failure proof cannot run")
        for name in ["909GR.wav", "909FL.wav", "909GR.adpcm", "909FL.adpcm", "pack.txt"]:
            for failure in ["write", "open"]:
                with self.subTest(name=name, failure=failure):
                    with tempfile.TemporaryDirectory(dir=self.root) as tmp:
                        directory = Path(tmp)
                        target = directory / name
                        if failure == "write":
                            target.symlink_to("/dev/full")
                        else:
                            target.mkdir()
                        self.assert_failure(self.invoke(directory / "bank.h", directory))


if __name__ == "__main__":
    unittest.main()
