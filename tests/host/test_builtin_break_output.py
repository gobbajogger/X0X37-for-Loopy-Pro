#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exercise real generator output admission, including buffered close failures."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


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
            self.assertIn(name, (ROOT / "web/factory_break_pack.js").read_text().splitlines()[1])
        self.assertEqual(len([line for line in (directory / "pack.txt").read_text().splitlines()
                             if line and not line.startswith("#")]), 2)

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
