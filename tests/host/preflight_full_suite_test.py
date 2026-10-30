#!/usr/bin/env python3
"""Offline unit checks for full-suite setup preflight path selection."""

import importlib.util
from pathlib import Path
import tempfile
import unittest

SCRIPT = Path(__file__).with_name("preflight_full_suite.py")
spec = importlib.util.spec_from_file_location("x0x37_preflight_full_suite", SCRIPT)
preflight = importlib.util.module_from_spec(spec)
spec.loader.exec_module(preflight)


class PreflightTests(unittest.TestCase):
    def test_missing_full_references_fail_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            checks = preflight.reference_checks(Path(directory), [])
            self.assertEqual(3, len(checks))
            self.assertEqual({"reference drum909", "reference drum808", "reference sequencer"},
                             {check[0] for check in checks})
            self.assertTrue(all(not result[1] for result in checks))

    def test_selected_reference_checks_only_what_is_requested(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "repo"
            root.mkdir()
            path = root.parent / "schwung-9W9/src/dsp/er99_engine.c"
            path.parent.mkdir(parents=True)
            path.write_text("/* fixture */", encoding="ascii")
            checks = preflight.reference_checks(root, ["drum909"])
            self.assertEqual(1, len(checks))
            self.assertTrue(checks[0][1])
            self.assertEqual("reference drum909", checks[0][0])

    def test_absolute_path_environment_override(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "repo"
            root.mkdir()
            ref = Path(directory) / "custom/src/dsp/er99_engine.c"
            ref.parent.mkdir(parents=True)
            ref.write_text("/* fixture */", encoding="ascii")
            checks = preflight.reference_checks(root, ["drum909"], {"NINEW9_DIR": str(ref.parents[2])})
            self.assertTrue(checks[0][1])

    def test_package_skip_is_detected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            checks = preflight.package_checks(root, [], {})
            self.assertEqual(3, len(checks))
            self.assertTrue(all(not c[1] for c in checks))
            self.assertEqual([], preflight.package_checks(root, ["fastmath"], {}))

    def test_package_inputs_are_accepted(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for filename in ("build/smk37/x0x.bin", "build/smk37/loader/ota.bin"):
                path = root / filename
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b"fixture")
            sdk = root / "sdk"
            sdk.mkdir()
            checks = preflight.package_checks(root, ["smk-package"], {"AC79_SDK": str(sdk)})
            self.assertTrue(all(c[1] for c in checks))

    def test_tb3po_reference_is_file_not_directory(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "repo"
            root.mkdir()
            path = root.parent / "schwung-tb3po/src/dsp/tb3po.c"
            path.parent.mkdir(parents=True)
            path.write_text("/* fixture */", encoding="ascii")
            checks = preflight.reference_checks(root, ["sequencer"])
            self.assertEqual(1, len(checks))
            self.assertTrue(checks[0][1])


if __name__ == "__main__":
    unittest.main()
