#!/usr/bin/env python3
"""Real loopback HTTP admin tests. No external GitHub, device or deployment."""

from __future__ import annotations

import copy
import json
import shutil
import sys
import tempfile
import threading
import unittest
import urllib.error
import urllib.request
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import admin_http
from admin_http import AdminHTTP, prepare_preview
from validate_release_status import load_json

SEED = load_json(ROOT / "records/0.0.25.1-79280e76.json")
PAST = "2026-11-13T10:00:00-06:00"


def choice():
    return {
        "catalog_id": SEED["catalog_id"], "model": "elite",
        "package_sha256": SEED["models"]["elite"]["package_sha256"],
        "stage": "restart-and-boot-reported", "observed_at": PAST,
        "note": "A human observed restart and a normal boot.",
    }


class TestAdminLoopback(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        root = Path(self.tmp.name)
        self.records = root / "records"
        self.records.mkdir()
        shutil.copy(ROOT / "records/0.0.25.1-79280e76.json",
                    self.records / "0.0.25.1-79280e76.json")
        self.audit = root / "audit.jsonl"
        self.audit.write_text((ROOT / "audit/audit.jsonl").read_text(encoding="utf-8"), encoding="utf-8")
        self.report_file = root / "reports.jsonl"
        self.patch_records = patch.object(admin_http, "RECORDS", self.records)
        self.patch_records.start()
        self.addCleanup(self.patch_records.stop)
        self.server = AdminHTTP(("127.0.0.1", 0), token=None, owner_login="gobbajogger",
                                report_file=self.report_file)
        self.addCleanup(self.server.server_close)
        thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        thread.start()
        self.addCleanup(self.server.shutdown)
        self.base = f"http://127.0.0.1:{self.server.server_port}"

    def call(self, path, body=None, csrf=True):
        data = json.dumps(body).encode("utf-8") if body is not None else None
        headers = {
            "Origin": self.base, "Content-Type": "application/json",
        }
        if csrf:
            headers["X-DnBOX-CSRF"] = self.server.csrf
        request = urllib.request.Request(self.base + path, data=data, headers=headers)
        try:
            with urllib.request.urlopen(request, timeout=4) as response:
                return response.status, json.loads(response.read())
        except urllib.error.HTTPError as e:
            return e.code, json.loads(e.read())

    def test_read_only_status_renders_build_and_owner_auth(self):
        status, result = self.call("/api/status")
        self.assertEqual(status, 200)
        self.assertTrue(result["owner_verified"])
        self.assertFalse(result["commit_enabled"])
        self.assertEqual(len(result["records"]), 1)
        self.assertIn("elite", result["records"][0]["models"])

    def test_real_preview_is_valid_and_does_not_write(self):
        rec_file = self.records / "0.0.25.1-79280e76.json"
        original = rec_file.read_bytes(), self.audit.read_bytes()
        # Uses local test records and the real pinned original audit.
        result = prepare_preview(
            choice(), owner_login="gobbajogger",
            records_dir=self.records, audit_path=self.audit,
        )
        self.assertEqual(result["errors"], [])
        self.assertTrue(result["accepted"])
        self.assertTrue(result["not_hardware_approved"])
        self.assertEqual((rec_file.read_bytes(), self.audit.read_bytes()), original)
        status, actual = self.call("/api/preview", choice())
        self.assertEqual(status, 200)
        self.assertTrue(actual["accepted"])
        self.assertEqual(actual["stage"], "restart-and-boot-reported")
        self.assertFalse(self.server.allow_commit)
        self.assertEqual((rec_file.read_bytes(), self.audit.read_bytes()), original)

    def test_missing_csrf_and_bad_origin_fail(self):
        status, _ = self.call("/api/preview", choice(), csrf=False)
        self.assertEqual(status, 403)
        req = urllib.request.Request(
            self.base + "/api/preview", data=json.dumps(choice()).encode(),
            headers={"Origin": "https://evil.example.org", "Content-Type": "application/json",
                     "X-DnBOX-CSRF": self.server.csrf},
        )
        with self.assertRaises(urllib.error.HTTPError) as caught:
            urllib.request.urlopen(req, timeout=4)
        self.assertEqual(caught.exception.code, 403)

    def test_commit_denied_in_default_dry_run_even_with_preview_id(self):
        _, result = self.call("/api/preview", choice())
        self.assertTrue(result["accepted"])
        status, err = self.call("/api/commit", {"preview_id": result["preview_id"]})
        self.assertEqual(status, 403)
        self.assertIn("disabled", err["error"])

    def test_opt_in_anonymous_report_and_replay_rejection(self):
        body = {
            "catalog_id": SEED["catalog_id"], "model": "elite",
            "package_sha256": SEED["models"]["elite"]["package_sha256"],
            "stage": "transfer-completed", "consent": True,
            "run_id": "12345678-abcd-4321-aaaa-abcdef987654", "sequence": 1,
            "observed_at": PAST, "mac": "DO-NOT-STORE",
        }
        status, answer = self.call("/api/installer-report", body)
        self.assertEqual(status, 202)
        self.assertFalse(answer["authorizes_hardware_approval"])
        persisted = json.loads(self.report_file.read_text().splitlines()[0])
        self.assertEqual(persisted["confidence"], "anonymous-unverified")
        self.assertNotIn("mac", persisted)
        status, _ = self.call("/api/installer-report", body)
        self.assertEqual(status, 422)
        status, _ = self.call("/api/installer-report", {**body, "consent": False, "sequence": 2})
        self.assertEqual(status, 422)
        self.assertEqual(len(self.report_file.read_text().splitlines()), 1)

    def test_no_owner_login_prevents_admin_actions(self):
        with patch.object(admin_http, "RECORDS", self.records):
            guest = AdminHTTP(("127.0.0.1", 0), token=None, owner_login="not-owner")
            try:
                self.assertFalse(guest.owner_verified)
                self.assertFalse(guest.allow_commit)
            finally:
                guest.server_close()

    def test_public_probe_cannot_bind_other_interfaces(self):
        with self.assertRaises(ValueError):
            AdminHTTP(("0.0.0.0", 0), token=None)


if __name__ == "__main__":
    unittest.main()
