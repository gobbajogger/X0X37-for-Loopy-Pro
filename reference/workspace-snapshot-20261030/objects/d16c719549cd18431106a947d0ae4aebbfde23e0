#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""FP-001: dual-digest evidence must not conflate source FP with served HTML."""
import io
import json
import tempfile
import unittest
from pathlib import Path
from unittest import mock

TOP = Path(__file__).resolve().parents[2]
import sys

sys.path.insert(0, str(TOP / "tools"))
import check_installer_digests as digests  # noqa: E402

SOURCE = "8caf87247afb79dda2107f3074d73da7f2512f5d14e55db32b9285baadc38157"
SERVED = "66b56ba5accd1309664320c91db52c28438080a97b2bdb3fa31f6b830f3ca554"
OTHER = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"


class DualDigest(unittest.TestCase):
    def good(self, **extra):
        data = {
            "schema": 1,
            "installer_source_fingerprint": SOURCE,
            "served_install_html_sha256": SERVED,
        }
        data.update(extra)
        return data

    def test_pass_with_separate_fields(self):
        result = digests.check_evidence(self.good())
        self.assertTrue(result["ok"])
        self.assertEqual(result["installer_source_fingerprint"], SOURCE)
        self.assertEqual(result["served_install_html_sha256"], SERVED)
        self.assertIsNone(result["fetched_install_html_sha256"])

    def test_pass_with_installer_sha256_alias_matching_source(self):
        result = digests.check_evidence(self.good(installer_sha256=SOURCE))
        self.assertTrue(result["ok"])

    def test_fail_when_alias_does_not_match_source(self):
        with self.assertRaisesRegex(
            digests.DigestCheckError, "alias must equal installer_source_fingerprint"
        ):
            digests.check_evidence(self.good(installer_sha256=SERVED))

    def test_fail_when_source_equals_served(self):
        with self.assertRaisesRegex(digests.DigestCheckError, "must not equal"):
            digests.check_evidence(
                self.good(
                    installer_source_fingerprint=SOURCE,
                    served_install_html_sha256=SOURCE,
                )
            )

    def test_fail_missing_served_field(self):
        evidence = {
            "schema": 1,
            "installer_source_fingerprint": SOURCE,
        }
        with self.assertRaisesRegex(digests.DigestCheckError, "served install HTML"):
            digests.check_evidence(evidence)

    def test_fail_installer_sha256_alone(self):
        evidence = {"schema": 1, "installer_sha256": SOURCE}
        with self.assertRaisesRegex(digests.DigestCheckError, "conflates"):
            digests.check_evidence(evidence)

    def test_fail_legacy_installer_fp_without_dual_fields(self):
        evidence = {"schema": 1, "installer_fp": SOURCE}
        with self.assertRaisesRegex(digests.DigestCheckError, "conflated field"):
            digests.check_evidence(evidence)

    def test_fail_explicit_single_pin_claim(self):
        with self.assertRaisesRegex(digests.DigestCheckError, "single Installer FP"):
            digests.check_evidence(self.good(installer_fp_equals_both=True))

    def test_fail_when_html_bytes_mismatch(self):
        html = b"<html>not the recorded digest</html>"
        with self.assertRaisesRegex(
            digests.DigestCheckError, "does not match fetched install HTML"
        ):
            digests.check_evidence(self.good(), html_bytes=html)

    def test_pass_when_html_bytes_match(self):
        html = b"exact-bytes-for-served-digest"
        served = digests.sha256_bytes(html)
        evidence = self.good(served_install_html_sha256=served)
        # source must differ
        evidence["installer_source_fingerprint"] = SOURCE
        self.assertNotEqual(SOURCE, served)
        result = digests.check_evidence(evidence, html_bytes=html)
        self.assertEqual(result["fetched_install_html_sha256"], served)

    def test_fetch_compares_served_digest(self):
        html = b"<html id=install></html>"
        served = digests.sha256_bytes(html)
        evidence = self.good(served_install_html_sha256=served)

        class Resp(io.BytesIO):
            status = 200

            def __enter__(self):
                return self

            def __exit__(self, *args):
                return False

        def fake_open(req, timeout=60):
            self.assertIn("install", req.full_url)
            return Resp(html)

        with mock.patch.object(digests.urllib.request, "urlopen", fake_open):
            result = digests.check_evidence(
                evidence, fetch_url="https://example.test/install/"
            )
        self.assertEqual(result["fetched_install_html_sha256"], served)

    def test_fetch_mismatch_fails(self):
        evidence = self.good()

        class Resp(io.BytesIO):
            status = 200

            def __enter__(self):
                return self

            def __exit__(self, *args):
                return False

        def fake_open(req, timeout=60):
            return Resp(b"wrong-html")

        with mock.patch.object(digests.urllib.request, "urlopen", fake_open):
            with self.assertRaisesRegex(
                digests.DigestCheckError, "does not match fetched install HTML"
            ):
                digests.check_evidence(
                    evidence, fetch_url="https://example.test/install/"
                )

    def test_expect_source_mismatch(self):
        with self.assertRaisesRegex(digests.DigestCheckError, "expect_source"):
            digests.check_evidence(self.good(), expect_source=OTHER)

    def test_cli_pass_and_fail(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "digests.json"
            path.write_text(json.dumps(self.good()) + "\n", encoding="utf-8")
            with mock.patch("sys.stdout", new=io.StringIO()), mock.patch(
                "sys.stderr", new=io.StringIO()
            ):
                self.assertEqual(digests.main([str(path)]), 0)
            bad = Path(tmp) / "bad.json"
            bad.write_text(
                json.dumps({"schema": 1, "installer_sha256": SOURCE}) + "\n",
                encoding="utf-8",
            )
            with mock.patch("sys.stdout", new=io.StringIO()), mock.patch(
                "sys.stderr", new=io.StringIO()
            ):
                self.assertEqual(digests.main([str(bad)]), 1)


if __name__ == "__main__":
    unittest.main()
