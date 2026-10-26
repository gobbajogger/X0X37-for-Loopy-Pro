#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Native synthetic .fwsc test vectors: no vendor SDK, no flashable code."""

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'tools'))
from fm1pkg_make import ufw
from firmware_build_labels import canonical_version, inspect_package, version_of_product

SHA = '0123456789abcdef' * 2 + '01234567'


class FirmwareBuildLabelsTest(unittest.TestCase):
    def setUp(self):
        self.work = tempfile.TemporaryDirectory()
        self.addCleanup(self.work.cleanup)
        self.path = Path(self.work.name) / 'test.fwsc'

    def make_package(self, product):
        # Fake application and loader have a valid transport envelope but are
        # not real firmware; only the model/version framing is exercised.
        self.path.write_bytes(ufw(b'\0'*1024, b'FELUCCA-LOADER-1'+b'\0'*128, product))

    def test_version_encoded_in_real_package_identity(self):
        self.make_package('SMK-37 Elite_9000702')
        result = inspect_package(self.path, 'elite', SHA, expected_version='0.7.2')
        self.assertEqual(result['firmware_version'], '0.7.2')
        self.assertEqual(result['release_display'], '0.7.2_openbeta')
        self.assertNotIn('openbeta', result['product'])
        self.assertEqual(result['version_label'], 'v0.7.2')
        self.assertEqual(result['source_commit'], SHA)
        self.assertEqual(result['package_sha256'], __import__('hashlib').sha256(self.path.read_bytes()).hexdigest())
        self.assertTrue(result['build_id'].startswith('g0123456789ab-h'))
        self.assertFalse(result['device_tested'])
        self.assertFalse(result['installable'])
        self.assertEqual(result['status'], 'package-inspected')

    def test_unversioned_legacy_cannot_get_fake_semver(self):
        for product in ('SMK-37 Pro_900', 'SMK-37 Elite_900', 'MKE-P37_900', 'FM-1_906'):
            with self.subTest(product=product):
                model = {'SMK-37 Pro': 'pro', 'SMK-37 Elite': 'elite', 'MKE-P37': 'mke', 'FM-1': 'fm1'}[product.split('_')[0]]
                self.make_package(product)
                result = inspect_package(self.path, model, SHA)
                self.assertIsNone(result['firmware_version'])
                self.assertIn('Unversioned', result['display_label'])
                with self.assertRaisesRegex(ValueError, 'no numeric version'):
                    inspect_package(self.path, model, SHA, '0.7.2')

    def test_wrong_model_version_and_forged_commit_rejected(self):
        self.make_package('SMK-37 Elite_9000702')
        with self.assertRaisesRegex(ValueError, 'model does not match'):
            inspect_package(self.path, 'pro', SHA)
        with self.assertRaisesRegex(ValueError, 'expected firmware'):
            inspect_package(self.path, 'elite', SHA, '0.7.3')
        for bad in ('1234567', 'g' * 40, SHA.upper()):
            with self.subTest(commit=bad), self.assertRaisesRegex(ValueError, '40-character'):
                inspect_package(self.path, 'elite', bad)

    def test_bad_package_crc_loader_and_truncation_rejected(self):
        self.make_package('SMK-37 Pro_9010203')
        raw = bytearray(self.path.read_bytes())
        raw[-1] ^= 1
        self.path.write_bytes(raw)
        with self.assertRaisesRegex(ValueError, 'integrity error'):
            inspect_package(self.path, 'pro', SHA)
        self.path.write_bytes(b'no firmware')
        with self.assertRaises(Exception):
            inspect_package(self.path, 'pro', SHA)
        self.make_package('SMK-37 Pro_9010203')
        self.path.write_bytes(self.path.read_bytes().replace(b'FELUCCA-LOADER-1', b'NO-UPDATE-LOADER'))
        with self.assertRaisesRegex(ValueError, 'update-loader'):
            inspect_package(self.path, 'pro', SHA)

    def test_canonical_semver_and_identity_bounds(self):
        for candidate in ('01.2.3', '1.02.3', '1.2', '1.2.3-beta', '100.0.0', '-1.0.0', '0.7.2_openbeta'):
            with self.subTest(version=candidate), self.assertRaises(ValueError):
                canonical_version(candidate)
        self.assertEqual(version_of_product('SMK-37 Elite_9999999', 'elite'), '99.99.99')
        self.assertEqual(version_of_product('FM-1_9000600', 'fm1'), '0.6.0')
        with self.assertRaises(ValueError):
            version_of_product('SMK-37 Elite_9abc', 'elite')
        for malformed in (None, 900, {"product": "SMK-37 Elite_900"}):
            with self.subTest(product=malformed):
                with self.assertRaisesRegex(ValueError, "text"):
                    version_of_product(malformed, "elite")

    def test_cli_output_and_mismatch_do_not_write_output(self):
        self.make_package('SMK-37 Pro_9000702')
        dest = Path(self.work.name) / 'record.json'
        argv = [sys.executable, str(ROOT/'tools/firmware_build_labels.py'), str(self.path),
                '--model', 'pro', '--source-commit', SHA, '--expected-version', '0.7.2', '--out', str(dest)]
        r = subprocess.run(argv, capture_output=True, text=True)
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(json.loads(dest.read_text())['firmware_version'], '0.7.2')
        dest.unlink()
        bad_argv = argv.copy()
        bad_argv[bad_argv.index('--expected-version') + 1] = '0.7.3'
        bad = subprocess.run(bad_argv,
                             capture_output=True, text=True)
        self.assertEqual(bad.returncode, 3)
        self.assertFalse(dest.exists())


if __name__ == '__main__':
    unittest.main()
