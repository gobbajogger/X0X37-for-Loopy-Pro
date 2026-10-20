#!/usr/bin/env python3
"""Check new dispatch rejection and caller forwarding/cleanup in disposable fixtures.

This uses the candidate shell scripts with controlled generators/compiler. It
does not compile firmware or substitute for the retained real probe logs.
"""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
CALLERS = ("smc_reachability", "smc_drum_mutes", "smc_performance_abort", "ui_feedback")


class Supplement(unittest.TestCase):
    def setUp(self):
        self.scratch = tempfile.TemporaryDirectory(prefix="x0x-stage3-callers-")
        self.addCleanup(self.scratch.cleanup)
        self.root = Path(self.scratch.name)
        for name in ("host", "tests/host", "tools", "tmp", "source/firmware/src/app"):
            (self.root / name).mkdir(parents=True, exist_ok=True)
        for name in ("host/build_host.sh", *(f"tests/host/{c}_test.sh" for c in CALLERS)):
            shutil.copy2(ROOT / name, self.root / name)
        (self.root / "source/firmware/src/app/main_fm1.c").write_text("/* source override fixture */\n")
        generator = '''import os, pathlib, sys
if os.environ.get("FAIL_SETUP"):
    print("fixture setup failure", file=sys.stderr)
    sys.exit(23)
pathlib.Path(sys.argv[-1]).write_text("fixture header")
'''
        for name in ("gen_font.py", "gen_drum_samples.py", "gen_break_bank.py"):
            (self.root / "tools" / name).write_text(generator)
        (self.root / "tools/gen_builtin_break.sh").write_text(
            'echo fixture > "$1/x0x_break_bank.h"\n')
        compiler = self.root / "compiler"
        compiler.write_text('''#!/usr/bin/env python3
import json, os, pathlib, sys
with open("compile.jsonl", "a") as out:
    out.write(json.dumps({"argv":sys.argv[1:],"marker":os.environ.get("SETUP_MARKER")})+"\\n")
if os.environ.get("FAIL_COMPILE"):
    sys.exit(29)
target=pathlib.Path(sys.argv[sys.argv.index("-o")+1])
target.write_text("#!/bin/sh\\nprintf '%s\\\\n' \\\"$@\\\" > run-args.txt\\nexit ${FAIL_RUN:-0}\\n")
target.chmod(0o755)
''')
        compiler.chmod(0o755)
        self.env = {k: v for k, v in os.environ.items()
                    if not k.startswith(("X0X_", "SMC_", "FAIL_"))}
        self.env.update(CC=str(compiler), TMPDIR=str(self.root / "tmp"),
                        SETUP_MARKER="inherited", X0X_HOST_DEFS="-DFIXTURE=1",
                        X0X_BREAK_BANK="bank with spaces",
                        SMC_SOURCE_ROOT=str(self.root / "source"),
                        SMC_SOURCE="app/alternate_mixer.c",
                        SMC_SOURCE_FILE="app/alternate_performance.c")

    def run_script(self, script, *args, **env):
        return subprocess.run(["sh", script, *args], cwd=self.root,
                              env={**self.env, **env}, capture_output=True, text=True)

    def assert_clean(self):
        if (self.root / "compile.jsonl").exists():
            for line in (self.root / "compile.jsonl").read_text().splitlines():
                argv = json.loads(line)["argv"]
                self.assertFalse(Path(argv[argv.index("-o") + 1]).parent.exists())
        self.assertEqual(list((self.root / "tmp").iterdir()), [])

    def test_unknown_mode_stops_before_setup(self):
        result = self.run_script("host/build_host.sh", "--invalid")
        self.assertEqual(result.returncode, 2)
        self.assertIn("usage:", result.stderr)
        self.assertFalse((self.root / "build").exists())

    def test_caller_configurations_and_cleanup(self):
        for caller in CALLERS:
            for failure in (None, "FAIL_SETUP", "FAIL_COMPILE", "FAIL_RUN"):
                with self.subTest(caller=caller, failure=failure):
                    shutil.rmtree(self.root / "build", ignore_errors=True)
                    (self.root / "compile.jsonl").unlink(missing_ok=True)
                    (self.root / "run-args.txt").unlink(missing_ok=True)
                    args = ("alpha", "two words") if caller == "ui_feedback" else ()
                    result = self.run_script(f"tests/host/{caller}_test.sh", *args,
                                             **({failure: "37" if failure == "FAIL_RUN" else "1"}
                                                if failure else {}))
                    expected = {None: 0, "FAIL_SETUP": 1, "FAIL_COMPILE": 29, "FAIL_RUN": 37}[failure]
                    self.assertEqual(result.returncode, expected, result.stderr + result.stdout)
                    if failure == "FAIL_SETUP":
                        self.assertIn("fixture setup failure", result.stdout)
                        self.assertFalse((self.root / "compile.jsonl").exists())
                    else:
                        lines = (self.root / "compile.jsonl").read_text().splitlines()
                        self.assertEqual(len(lines), 1, "Setup must not compile a simulator")
                        record = json.loads(lines[0]); argv = record["argv"]
                        self.assertEqual(record["marker"], "inherited")
                        self.assertIn(f"tests/host/{caller}_test.c", argv)
                        if caller == "smc_reachability":
                            self.assertIn(f'-DSMC_SOURCE_FILE="{self.root}/source/firmware/src/app/smc_mixer.c"', argv)
                        elif caller == "smc_drum_mutes":
                            self.assertIn('-DSMC_MIXER_SOURCE="app/alternate_mixer.c"', argv)
                        elif caller == "smc_performance_abort":
                            self.assertIn('-DSMC_SOURCE_FILE="app/alternate_performance.c"', argv)
                        if caller == "ui_feedback":
                            self.assertNotIn("-DFIXTURE=1", argv)
                            if os.uname().sysname == "Darwin":
                                self.assertIn("-D_DARWIN_C_SOURCE", argv)
                            if failure != "FAIL_COMPILE":
                                self.assertEqual((self.root / "run-args.txt").read_text().splitlines(), list(args))
                        else:
                            self.assertIn("-DFIXTURE=1", argv)
                    self.assert_clean()


if __name__ == "__main__":
    unittest.main()
