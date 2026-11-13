#!/usr/bin/env python3
"""JSON schema tests for release records."""

from __future__ import annotations

import copy
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from validate_release_status import (  # noqa: E402
    load_json,
    validate_instance,
)

SCHEMA = load_json(ROOT / "schema" / "release-record.schema.json")
RECORD_941 = load_json(ROOT / "records" / "0.0.26-94197fc8.json")
RECORD_600 = load_json(ROOT / "records" / "0.0.26-600b3b45.json")
RECORD_025 = load_json(ROOT / "records" / "0.0.25.1-79280e76.json")


class TestSchema(unittest.TestCase):
    def test_seed_94197fc8_matches_schema(self):
        errors = validate_instance(RECORD_941, SCHEMA, SCHEMA)
        self.assertEqual(errors, [], msg=errors)

    def test_seed_600b_matches_schema(self):
        errors = validate_instance(RECORD_600, SCHEMA, SCHEMA)
        self.assertEqual(errors, [], msg=errors)

    def test_seed_0251_matches_schema(self):
        errors = validate_instance(RECORD_025, SCHEMA, SCHEMA)
        self.assertEqual(errors, [], msg=errors)

    def test_missing_required_field_fails(self):
        rec = copy.deepcopy(RECORD_941)
        del rec["source_sha"]
        errors = validate_instance(rec, SCHEMA, SCHEMA)
        self.assertTrue(errors, "a missing required field must fail")
        self.assertTrue(
            any("source_sha" in e for e in errors),
            errors,
        )

    def test_source_sha_not_40_hex_fails(self):
        for bad in ("94197fc8", "94197FC839B00AFF47B98CA02F3C6396E4A6EABF", "zzzz", "x" * 40):
            rec = copy.deepcopy(RECORD_941)
            rec["source_sha"] = bad
            errors = validate_instance(rec, SCHEMA, SCHEMA)
            self.assertTrue(errors, f"source SHA {bad!r} must fail")

    def test_sha256_not_64_hex_fails(self):
        for bad in ("95b3195f", "G" * 64, "95B3195F4767EF5DF33B365197B01203873E5EEC3D7FA497C73E5E509484A4A6"):
            rec = copy.deepcopy(RECORD_941)
            rec["models"]["elite"]["image_sha256"] = bad
            errors = validate_instance(rec, SCHEMA, SCHEMA)
            self.assertTrue(errors, f"SHA-256 {bad!r} must fail")

    def test_missing_states_fails(self):
        rec = copy.deepcopy(RECORD_941)
        del rec["states"]
        errors = validate_instance(rec, SCHEMA, SCHEMA)
        self.assertTrue(any("states" in e for e in errors), errors)


if __name__ == "__main__":
    unittest.main()
