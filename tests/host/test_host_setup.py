#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Check host asset setup and simulator build dispatch in disposable fixtures."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]


class HostSetupTest(unittest.TestCase):
    def setUp(self):
        self.scratch = tempfile.TemporaryDirectory(prefix="x0x-host-setup-")
        self.addCleanup(self.scratch.cleanup)
        self.root = Path(self.scratch.name)
        for directory in ("host", "tools"):
            (self.root / directory).mkdir()
        shutil.copy2(ROOT / "host/build_host.sh", self.root / "host/build_host.sh")
        generator = '''import os, pathlib, sys
name = pathlib.Path(sys.argv[0]).name
with open("generators.log", "a") as log:
    log.write(name + ":" + repr(sys.argv[1:]) + "\\n")
if os.environ.get("FAIL_GENERATOR") == name:
    sys.exit(23)
pathlib.Path(sys.argv[-1]).write_text(name)
'''
        for name in ("gen_font.py", "gen_drum_samples.py", "gen_break_bank.py"):
            (self.root / "tools" / name).write_text(generator)
        (self.root / "tools/gen_builtin_break.sh").write_text(
            'set -e\necho builtin >> generators.log\n'
            '[ "${FAIL_GENERATOR:-}" != builtin ] || exit 23\n'
            'echo builtin > "$1/x0x_break_bank.h"\n'
        )
        compiler = self.root / "compiler"
        compiler.write_text('''#!/usr/bin/env python3
import json, os, pathlib, sys
pathlib.Path("compile.json").write_text(json.dumps({
    "argv": sys.argv[1:], "marker": os.environ.get("SETUP_MARKER")}))
if os.environ.get("FAIL_COMPILE"):
    sys.exit(29)
pathlib.Path(sys.argv[sys.argv.index("-o") + 1]).write_text("simulator")
''')
        compiler.chmod(0o755)
        self.env = {k: v for k, v in os.environ.items()
                    if not k.startswith(("X0X_", "FAIL_"))}
        self.env.update(CC=str(compiler), SETUP_MARKER="inherited")

    def run_builder(self, *args, **env):
        return subprocess.run(["sh", "host/build_host.sh", *args], cwd=self.root,
                              env={**self.env, **env}, capture_output=True, text=True)

    def test_generate_only_from_empty_headers(self):
        result = self.run_builder("--generate-only")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual({p.name for p in (self.root / "build/gen").iterdir()},
                         {"felucca_font.h", "x0x_drum_samples.h", "x0x_break_bank.h"})
        self.assertFalse((self.root / "compile.json").exists())
        self.assertFalse((self.root / "build/host/x0x_host").exists())

    def test_default_build_and_environment(self):
        result = self.run_builder(X0X_HOST_DEFS="-DFIXTURE=1 -DSECOND=2",
                                  X0X_HOST_OUT="build/host/custom")
        self.assertEqual(result.returncode, 0, result.stderr)
        record = json.loads((self.root / "compile.json").read_text())
        argv = record["argv"]
        self.assertEqual(argv[:8], ["-O2", "-ffp-contract=off", "-std=c99", "-Wall",
                                  "-Wextra", "-Wno-unused-function",
                                  "-Wno-unused-parameter", "-DX0X_HOST"])
        self.assertIn("-DFIXTURE=1", argv)
        self.assertIn("-DSECOND=2", argv)
        self.assertIn("host/x0x_host.c", argv)
        self.assertEqual(argv[-1], "-lm")
        self.assertEqual(record["marker"], "inherited")
        self.assertTrue((self.root / "build/host/custom").is_file())
        self.assertIn("host: build/host/x0x_host", result.stdout)

    def test_default_output(self):
        self.assertEqual(self.run_builder().returncode, 0)
        self.assertTrue((self.root / "build/host/x0x_host").is_file())

    def test_custom_bank(self):
        result = self.run_builder("--generate-only", X0X_BREAK_BANK="bank with spaces")
        self.assertEqual(result.returncode, 0, result.stderr)
        log = (self.root / "generators.log").read_text()
        self.assertIn("gen_break_bank.py:['bank with spaces', 'build/gen/x0x_break_bank.h']", log)
        self.assertNotIn("builtin", log)
        self.assertFalse((self.root / "compile.json").exists())

    def test_existing_headers_skip_only_font_and_drums(self):
        self.assertEqual(self.run_builder("--generate-only").returncode, 0)
        (self.root / "generators.log").write_text("")
        self.assertEqual(self.run_builder("--generate-only").returncode, 0)
        self.assertEqual((self.root / "generators.log").read_text(), "builtin\n")

    def test_setup_failures_stop_both_modes(self):
        for mode in ((), ("--generate-only",)):
            for generator in ("gen_font.py", "gen_drum_samples.py", "builtin",
                              "gen_break_bank.py"):
                with self.subTest(mode=mode, generator=generator):
                    shutil.rmtree(self.root / "build", ignore_errors=True)
                    result = self.run_builder(*mode, FAIL_GENERATOR=generator,
                                              **({"X0X_BREAK_BANK": "bank"}
                                                 if generator == "gen_break_bank.py" else {}))
                    self.assertEqual(result.returncode, 23, result.stderr)
                    self.assertFalse((self.root / "compile.json").exists())

    def test_compile_failure_propagates(self):
        result = self.run_builder(FAIL_COMPILE="1")
        self.assertEqual(result.returncode, 29)
        self.assertNotIn("host: build/host/x0x_host", result.stdout)


if __name__ == "__main__":
    unittest.main()
