"""New mapping-layer proof only; unchanged parser proof is reused separately."""
import base64
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import unittest
import zipfile

from mapping import map_bank, restore_bank

ROOT = Path(__file__).resolve().parents[1]
RAW = (ROOT / 'fixtures/synthetic.bank.json').read_bytes()
ARCHIVE_SHA = 'ab702e00b4cc425c9406b9b2b44e68ba0f40e1125d591fcaa422cc6a8c6272b8'


class MappingTests(unittest.TestCase):
    def test_exact_restoration_includes_unknown_state_and_lexical_numbers(self):
        envelope = map_bank(RAW)
        self.assertEqual(restore_bank(envelope), RAW)
        decoded = json.loads(envelope)
        self.assertEqual(base64.b64decode(decoded['raw_base64']), RAW)
        self.assertEqual(decoded['raw_sha256'], hashlib.sha256(RAW).hexdigest())
        self.assertEqual(decoded['format'], 'dnbox-j6-observation-envelope/1')

    def test_numeric_observations_preserve_precision_without_normalization(self):
        records = json.loads(map_bank(RAW))['parameters']
        self.assertEqual([r['number'] for r in records], [
            {'kind': 'decimal', 'value': '1.3200'},
            {'kind': 'decimal', 'value': '-0.008403362'},
            {'kind': 'decimal', 'value': '0.123456789012345678901234567890'},
            {'kind': 'integer', 'value': '9007199254740993'},
            {'kind': 'integer', 'value': '0'},
            {'kind': 'decimal', 'value': '1E-9000'}])
        # Lexical integer -0 is preserved in raw, not claimed by the decoded view.
        self.assertIn(b'"value": -0}', restore_bank(map_bank(RAW)))

    def test_index_uses_array_positions_and_never_invents_targets_or_defaults(self):
        data = json.loads(map_bank(RAW))
        self.assertEqual(data['preset_count'], 2)
        self.assertEqual(len(data['parameters']), 6)
        for index, record in enumerate(data['parameters']):
            self.assertEqual(record['pointer'], f'/presets/0/parameterValues/{index}')
            self.assertEqual(record['semantic_status'], 'unresolved')
            self.assertIsNone(record['dnbox_target'])
        self.assertEqual(data['parameters'][2]['identifier'], 'future/~Param')
        self.assertEqual(map_bank(RAW), map_bank(RAW))

    def test_rejects_corruption_and_fabricated_mapping_or_observations(self):
        data = json.loads(map_bank(RAW))
        for change in ['raw_sha256', 'raw_base64', 'pointer', 'number', 'target', 'count', 'extra']:
            bad = json.loads(json.dumps(data))
            if change == 'raw_sha256': bad['raw_sha256'] = '0' * 64
            if change == 'raw_base64': bad['raw_base64'] = '!'
            if change == 'pointer': bad['parameters'][0]['pointer'] = '/presets/1'
            if change == 'number': bad['parameters'][0]['number']['value'] = '1'
            if change == 'target': bad['parameters'][0]['dnbox_target'] = 'Level'
            if change == 'count': bad['preset_count'] = True
            if change == 'extra': bad['schema_invented'] = True
            with self.subTest(change=change), self.assertRaises(ValueError):
                restore_bank(json.dumps(bad).encode())

    def test_rejects_duplicate_envelope_keys_and_invalid_bank(self):
        with self.assertRaises(ValueError): restore_bank(b'{"format":1,"format":2}')
        with self.assertRaises(ValueError): map_bank(b'{"presets":false}')

    def test_new_mapping_contract_on_all_external_banks(self):
        archive = Path(os.environ.get('J6_ARCHIVE', '../../J6presets(1).zip'))
        raw_archive = archive.read_bytes()  # Missing external fixture is a failure, not a skip.
        self.assertEqual(hashlib.sha256(raw_archive).hexdigest(), ARCHIVE_SHA)
        counts = [0, 0]; union = set(); common = None; occurrences = Counter()
        with zipfile.ZipFile(archive) as z:
            for member in z.namelist():
                if not member.startswith('J6presets/') or not member.endswith('.superj6.bank.json'):
                    continue
                raw = z.read(member)
                encoded = map_bank(raw)
                self.assertEqual(restore_bank(encoded), raw, member)
                mapped = json.loads(encoded)
                original = json.loads(raw)
                expected = sum(len(p['parameterValues']) for p in original['presets'])
                self.assertEqual(len(mapped['parameters']), expected)
                by_pointer = {r['pointer']: r for r in mapped['parameters']}
                self.assertEqual(len(by_pointer), expected)
                for pi, preset in enumerate(original['presets']):
                    ids = {p['identifier'] for p in preset['parameterValues']}
                    occurrences.update(ids)
                    union.update(ids); common = ids if common is None else common & ids
                    for vi, param in enumerate(preset['parameterValues']):
                        record = by_pointer[f'/presets/{pi}/parameterValues/{vi}']
                        self.assertEqual(record['identifier'], param['identifier'])
                        self.assertEqual(record['semantic_status'], 'unresolved')
                        self.assertIsNone(record['dnbox_target'])
                counts[0] += 1; counts[1] += mapped['preset_count']
        self.assertEqual((*counts, len(union), len(common)), (12, 640, 174, 112))
        manifest = json.loads((ROOT / 'mapping-manifest.json').read_text())
        self.assertEqual(manifest['archive_sha256'], ARCHIVE_SHA)
        self.assertEqual(manifest['identifier_count'], len(occurrences))
        self.assertEqual({r['identifier']: r['occurrences'] for r in manifest['parameters']}, dict(occurrences))
        self.assertEqual(len(manifest['parameters']), len(occurrences))
        for record in manifest['parameters']:
            self.assertEqual(record['semantic_status'], 'unresolved')
            self.assertIsNone(record['dnbox_target'])


if __name__ == '__main__': unittest.main()
