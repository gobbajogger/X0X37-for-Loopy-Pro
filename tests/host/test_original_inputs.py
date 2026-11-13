#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exercise the input gate with synthetic local files, without private data."""
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("check_original_inputs", ROOT / "tools/check_original_inputs.py")
checker = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(checker)


class OriginalInputsTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.manifest = self.root / checker.MANIFEST_PATH
        self.manifest.parent.mkdir(parents=True)
        self.data = {"excluded_data_inputs": []}
        for i in range(12):
            path = f"fixture/input-{i}.h"
            payload = f"synthetic verification fixture {i}\n".encode()
            local = self.root / path
            local.parent.mkdir(exist_ok=True)
            local.write_bytes(payload)
            self.data["excluded_data_inputs"].append({
                "path": path, "bytes": len(payload),
                "sha256": hashlib.sha256(payload).hexdigest(),
                "required_by_active_original_profile": i < 10})
        self.save_manifest()

    def save_manifest(self):
        raw = json.dumps(self.data).encode()
        self.manifest.write_bytes(raw)
        self.pin = hashlib.sha256(raw).hexdigest()

    def check(self, scope="active", pin=None):
        return checker.verify(self.root, scope=scope,
                              expected_manifest_sha256=pin or self.pin)

    def test_active_and_all_pass_without_mutation(self):
        before = {p.relative_to(self.root): p.read_bytes() for p in self.root.rglob("*") if p.is_file()}
        for scope, count in (("active", 10), ("all", 12)):
            report = self.check(scope)
            self.assertTrue(report["ok"], report["errors"])
            self.assertEqual(len(report["inputs"]), count)
        after = {p.relative_to(self.root): p.read_bytes() for p in self.root.rglob("*") if p.is_file()}
        self.assertEqual(before, after)

    def test_missing_active_fails_and_missing_alternate_is_scoped(self):
        (self.root / "fixture/input-11.h").unlink()
        self.assertTrue(self.check()["ok"])
        self.assertFalse(self.check("all")["ok"])
        (self.root / "fixture/input-0.h").unlink()
        report = self.check()
        self.assertFalse(report["ok"])
        self.assertIn("fixture/input-0.h", " ".join(report["errors"]))

    def test_same_length_corruption_and_length_change_fail(self):
        path = self.root / "fixture/input-0.h"
        payload = path.read_bytes()
        path.write_bytes(b"X" + payload[1:])
        self.assertFalse(self.check()["ok"])
        path.write_bytes(payload + b"X")
        self.assertFalse(self.check()["ok"])

    def test_manifest_pin_mismatch_fails_before_input_reads(self):
        report = self.check(pin="0" * 64)
        self.assertFalse(report["ok"])
        self.assertIn("manifest SHA256 mismatch", report["errors"])
        self.assertEqual(report["inputs"], [])

    def test_empty_manifest_cannot_pass(self):
        self.data["excluded_data_inputs"] = []
        self.save_manifest()
        self.assertFalse(self.check()["ok"])

    def test_duplicate_and_escaping_paths_fail(self):
        for name in ("fixture/input-1.h", "../escape.h", "/absolute.h"):
            with self.subTest(name=name):
                self.data["excluded_data_inputs"][0]["path"] = name
                self.save_manifest()
                self.assertFalse(self.check()["ok"])

    def test_incorrect_active_count_and_flag_type_fail(self):
        for flag in (False, 1):
            with self.subTest(flag=flag):
                self.data["excluded_data_inputs"][0]["required_by_active_original_profile"] = flag
                self.save_manifest()
                self.assertFalse(self.check()["ok"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
