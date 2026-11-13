#!/usr/bin/env python3
"""Catalog generator tests."""

from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / "tools"
sys.path.insert(0, str(TOOLS))

from generate_catalog import (  # noqa: E402
    build_catalog,
    main as catalog_main,
    render_html,
    write_catalog,
)
from validate_release_status import load_records  # noqa: E402

RECORDS = ROOT / "records"
SCRIPT = TOOLS / "generate_catalog.py"
COMMITTED_JSON = ROOT / "catalog" / "catalog.json"
COMMITTED_HTML = ROOT / "catalog" / "index.html"

ELITE_025 = "5e24e0d4afccc30906b334fbe73bcad3621df8df3c325790b7af2f94accffb1a"
PRO_025 = "6f135a3d03bb1a2a4d09d63040c719313612b85590a6af69b293a928e22285bf"
ELITE_941 = "95b3195f4767ef5df33b365197b01203873e5eec3d7fa497c73e5e509484a4a6"
SRC_025 = "79280e763341d03ba43266d3e25dad66f12de860"


class TestGenerateCatalog(unittest.TestCase):
    def setUp(self):
        self.records = load_records(RECORDS)
        self.catalog = build_catalog(self.records)

    def test_all_records_present_withdrawn_shown(self):
        ids = [r["catalog_id"] for r in self.catalog["records"]]
        self.assertEqual(
            ids,
            ["nightly-79280e76-dev", "nightly-94197fc8-dev", "nightly-600b3b45-dev"],
        )
        withdrawn = [r for r in self.catalog["records"] if r["withdrawn"]]
        self.assertEqual(len(withdrawn), 1)
        self.assertEqual(withdrawn[0]["catalog_id"], "nightly-600b3b45-dev")
        self.assertTrue(withdrawn[0]["withdraw_ref"])
        self.assertTrue(withdrawn[0]["withdraw_reason"])

    def test_exact_hashes_and_source(self):
        by = {r["catalog_id"]: r for r in self.catalog["records"]}
        r = by["nightly-79280e76-dev"]
        self.assertEqual(r["source_sha"], SRC_025)
        self.assertEqual(r["models"]["elite"]["image_sha256"], ELITE_025)
        self.assertEqual(r["models"]["pro"]["image_sha256"], PRO_025)
        self.assertEqual(r["models"]["elite"]["image_size"], 651721)
        self.assertEqual(by["nightly-94197fc8-dev"]["models"]["elite"]["image_sha256"], ELITE_941)
        self.assertNotEqual(ELITE_025, ELITE_941)

    def test_identity_confidence_and_elite_never_implies_pro(self):
        self.assertTrue(self.catalog["elite_never_implies_pro"])
        self.assertFalse(self.catalog["hardware_qualified"])
        self.assertFalse(self.catalog["promotion_performed"])
        self.assertEqual(self.catalog["automatic_promotion"], "not-performed")
        by = {r["catalog_id"]: r for r in self.catalog["records"]}
        r = by["nightly-79280e76-dev"]
        self.assertEqual(r["identity_confidence"]["model"], "reported")
        self.assertEqual(r["identity_confidence"]["installed_hash"], "absent")
        self.assertTrue(r["elite_never_implies_pro"])
        self.assertFalse(r["hardware_qualified"])
        self.assertFalse(r["models"]["elite"]["install_verified"])
        self.assertFalse(r["models"]["pro"]["install_verified"])
        self.assertFalse(r["models"]["pro"]["owner_tested"])
        self.assertTrue(r["models"]["elite"]["owner_tested"])
        self.assertFalse(r["models"]["elite"]["owner_hardware_approval"])
        self.assertFalse(r["models"]["pro"]["owner_hardware_approval"])

    def test_owner_reported_events_include_sources_and_quotes(self):
        by = {r["catalog_id"]: r for r in self.catalog["records"]}
        ev = by["nightly-79280e76-dev"]["owner_reported_events"]
        self.assertTrue(ev)
        self.assertTrue(ev[0]["quoted"])
        self.assertIn("owner", ev[0]["source"].lower())
        self.assertEqual(ev[0]["model"], "Elite")
        self.assertIsNone(ev[0]["installed_hash"])
        # 94197fc8 has both a relayed unquoted event and a later quoted event.
        ev2 = by["nightly-94197fc8-dev"]["owner_reported_events"]
        self.assertGreaterEqual(len(ev2), 3)
        quoted = [e for e in ev2 if e["quoted"]]
        unquoted = [e for e in ev2 if not e["quoted"]]
        self.assertTrue(quoted)
        self.assertTrue(unquoted)
        self.assertTrue(any("Dot relay" in (e["source"] or "") for e in unquoted))

    def test_html_contains_required_facts_and_escapes(self):
        page = render_html(self.catalog)
        self.assertIn("<!DOCTYPE html>", page)
        self.assertIn("DnBOX release status", page)
        self.assertIn("nightly-600b3b45-dev", page)
        self.assertIn("Withdrawn", page)
        self.assertIn("Elite never implies Pro", page)
        self.assertIn(ELITE_025, page)
        self.assertIn(ELITE_941, page)
        self.assertIn(SRC_025, page)
        self.assertIn("not a hardware qualification", page.lower())
        self.assertIn("not-performed", page)
        self.assertIn("owner in Orchestrator chat", page)
        # Escaping: no raw tag injection from record text.
        self.assertNotIn("<script>", page.lower().replace(" ", ""))
        self.assertTrue(page.lstrip().startswith("<!DOCTYPE html>"))

    def test_regeneration_is_deterministic_and_matches_committed(self):
        second = build_catalog(self.records)
        self.assertEqual(self.catalog["content_sha256"], second["content_sha256"])
        self.assertEqual(
            json.dumps(self.catalog, sort_keys=True),
            json.dumps(second, sort_keys=True),
        )
        self.assertTrue(COMMITTED_JSON.is_file(), "committed catalog.json is missing")
        self.assertTrue(COMMITTED_HTML.is_file(), "committed index.html is missing")
        committed = json.loads(COMMITTED_JSON.read_text())
        self.assertEqual(committed["content_sha256"], self.catalog["content_sha256"])
        self.assertEqual(COMMITTED_HTML.read_text(), render_html(self.catalog))

    def test_cli_writes_and_dry_run_does_not(self):
        with tempfile.TemporaryDirectory() as td:
            out_j = Path(td) / "catalog.json"
            out_h = Path(td) / "index.html"
            rc = catalog_main(
                ["--records", str(RECORDS), "--out-json", str(out_j), "--out-html", str(out_h)]
            )
            self.assertEqual(rc, 0)
            self.assertTrue(out_j.is_file())
            self.assertTrue(out_h.is_file())
            # dry-run
            before = list(Path(td).iterdir())
            proc = subprocess.run(
                [sys.executable, str(SCRIPT), "--records", str(RECORDS), "--dry-run"],
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(proc.returncode, 0, proc.stderr)
            data = json.loads(proc.stdout)
            self.assertEqual(data["content_sha256"], self.catalog["content_sha256"])
            self.assertEqual(list(Path(td).iterdir()), before)

    def test_cli_usage_exit_2(self):
        proc = subprocess.run(
            [sys.executable, str(SCRIPT), "--records", "/tmp/no-such-rs-dir"],
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 2)
        self.assertIn("USAGE", proc.stderr)


if __name__ == "__main__":
    unittest.main()
