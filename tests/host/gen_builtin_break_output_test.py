#!/usr/bin/env python3
"""Check that built-in 909 BREAK generation rejects truncated or failed output."""
# SPDX-License-Identifier: GPL-3.0-only

import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


class BuiltinBreakOutputTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.scratch = tempfile.TemporaryDirectory(prefix="x0x37-break-output-")
        cls.dir = Path(cls.scratch.name)
        cls.generator = cls.dir / "gen_builtin_break"
        sample_header = cls.dir / "x0x_drum_samples.h"
        subprocess.run(
            [sys.executable, "tools/gen_drum_samples.py", str(sample_header)],
            cwd=ROOT, check=True, capture_output=True, text=True,
        )
        flags = ["-O1", "-g", "-ffp-contract=off", "-w"]
        if os.environ.get("X0X_GEN_SANITIZE") == "1":
            flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        subprocess.run(
            [os.environ.get("CC", "cc"), *flags, "-Ifirmware/src", f"-I{cls.dir}",
             "-o", str(cls.generator), "tools/gen_builtin_break.c",
             "firmware/src/dsp/drum909.c", "-lm"],
            cwd=ROOT, check=True, capture_output=True, text=True,
        )

    @classmethod
    def tearDownClass(cls):
        cls.scratch.cleanup()

    def generate(self, destination):
        return subprocess.run(
            [str(self.generator), str(destination)],
            cwd=ROOT, capture_output=True, text=True,
            env={**os.environ, "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
                 "UBSAN_OPTIONS": "halt_on_error=1"},
        )

    def test_writable_output_is_deterministic(self):
        first, second = self.dir / "first.h", self.dir / "second.h"
        a, b = self.generate(first), self.generate(second)
        self.assertEqual(a.returncode, 0, a.stderr)
        self.assertEqual(b.returncode, 0, b.stderr)
        data = first.read_bytes()
        self.assertEqual(data, second.read_bytes())
        self.assertIn(b"#define X0X_NBREAKS 2", data)
        self.assertIn("builtin break:", a.stdout)

    def test_open_failure_is_reported(self):
        result = self.generate(self.dir / "missing-parent" / "missing.h")
        self.assertNotEqual(result.returncode, 0)
        self.assertNotIn("builtin break:", result.stdout)

    @unittest.skipUnless(Path("/dev/full").exists(), "requires Linux /dev/full")
    def test_write_failure_is_reported(self):
        result = self.generate("/dev/full")
        self.assertNotEqual(result.returncode, 0,
                            "write error lost: fflush/fclose ENOSPC was ignored")
        self.assertIn("output write or close failed", result.stderr)
        self.assertNotIn("builtin break:", result.stdout)


if __name__ == "__main__":
    unittest.main(verbosity=2)
