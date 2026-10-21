#!/usr/bin/env python3
"""Exercise the real runner in a temporary checkout; no live result folders."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class RunnerSelections(unittest.TestCase):
    def invoke(self, names, jobs, env_selection=None):
        with tempfile.TemporaryDirectory(prefix="x0x-runner-selection-") as tmp:
            root = Path(tmp)
            (root / "tests/host").mkdir(parents=True)
            shutil.copy2(ROOT / "tests/run_tests.sh", root / "tests/run_tests.sh")
            shutil.copy2(ROOT / "tests/host/fastmath_test.c", root / "tests/host/fastmath_test.c")
            (root / "firmware/src/dsp").mkdir(parents=True)
            shutil.copy2(ROOT / "firmware/src/dsp/fastmath.h", root / "firmware/src/dsp/fastmath.h")
            env = os.environ.copy()
            env.pop("X0X_TESTS", None)
            env["X0X_TEST_JOBS"] = str(jobs)
            if env_selection is not None:
                env["X0X_TESTS"] = env_selection
            result = subprocess.run(["sh", "tests/run_tests.sh", *names], cwd=root,
                                    env=env, text=True, capture_output=True, timeout=30)
            output = result.stdout + result.stderr
            ran = (root / "build/host/fastmath.log").exists()
            return result.returncode, output, ran

    def test_unknown_and_mixed_fail_before_running(self):
        for jobs in (1, 4):
            for names in (["audit-nonexistent-test"], ["fastmath", "audit-nonexistent-test"]):
                with self.subTest(jobs=jobs, names=names):
                    code, out, ran = self.invoke(names, jobs)
                    self.assertEqual(code, 2, out)
                    self.assertIn("Unknown or unavailable test selection: audit-nonexistent-test", out)
                    self.assertNotIn("all tests passed", out)
                    self.assertFalse(ran)

    def test_environment_selection_is_validated(self):
        code, out, ran = self.invoke([], 4, "fastmath audit-nonexistent-test")
        self.assertEqual(code, 2, out)
        self.assertFalse(ran)

    def test_empty_explicit_selection_is_rejected(self):
        for names, env in (([""], None), ([], " \t ")):
            code, out, ran = self.invoke(names, 1, env)
            self.assertEqual(code, 2, out)
            self.assertIn("must name at least one test", out)
            self.assertFalse(ran)

    def test_valid_name_really_executes_once(self):
        for jobs in (1, 4):
            code, out, ran = self.invoke(["fastmath"], jobs)
            self.assertEqual(code, 0, out)
            self.assertTrue(ran)
            self.assertIn("executed 1 tests", out)
            self.assertEqual(out.count("ok   fastmath"), 1)


if __name__ == "__main__":
    unittest.main()
