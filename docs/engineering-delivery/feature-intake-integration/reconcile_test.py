#!/usr/bin/env python3
"""New fallback-harness checks; the existing 54 unchanged tests are reused."""
import copy
import importlib.util
import json
from pathlib import Path
import sys
import unittest

HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location('reconcile', HERE / 'reconcile.py')


class ReconciliationTests(unittest.TestCase):
    def test_missing_checkpoint_is_refused_without_bootstrap(self):
        self.assertTrue((HERE / 'reconcile.py').exists(), 'missing reconciliation harness')
        module = importlib.util.module_from_spec(SPEC)
        SPEC.loader.exec_module(module)
        result = module.dry_run({'schema': 1}, {'records': []}, None)
        self.assertEqual(result['state'], 'BLOCKED')
        self.assertEqual(result['claims'], [])
        self.assertNotIn('proposed_checkpoint', result)
        self.assertFalse(result['current_acceptance'])

    def test_historical_replay_reports_behaviour_and_never_current_acceptance(self):
        self.assertTrue((HERE / 'reconcile.py').exists(), 'missing reconciliation harness')
        module = importlib.util.module_from_spec(SPEC)
        SPEC.loader.exec_module(module)
        ledger = json.loads(Path(sys.argv[1]).read_text())
        before = copy.deepcopy(ledger)
        result = module.historical_replay(ledger)
        self.assertEqual(result['state'], 'PASS_HISTORICAL_REPLAY_ONLY')
        self.assertFalse(result['current_acceptance'])
        self.assertEqual(result['checks'], {
            'capacity_zero': True, 'deterministic_identity': True,
            'multiple_existing_requests': True, 'existing_not_reenqueued': True,
            'cancellation_retained': True, 'reopen_requires_new_authority': True,
            'explicit_reauthorization': True, 'tombstone_reserved': True,
            'missing_tracked_issue_blocks': True, 'inputs_unchanged': True})
        self.assertEqual(ledger, before)
        self.assertEqual(result['new_request_id'], 'LX-REQ-039')


if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit('usage: reconcile_test.py HISTORICAL_LEDGER')
    unittest.main(argv=[sys.argv[0]], verbosity=2)
