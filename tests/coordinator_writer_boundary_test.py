#!/usr/bin/env python3
"""Contract tests with synthetic evidence and a disposable SQLite store only."""
import copy
import importlib.util
import json
from pathlib import Path
import sqlite3
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'coordination/requests'))
import coordinator_writer_boundary as gate
import task_priority as priority

spec = importlib.util.spec_from_file_location('priority_fixture', ROOT / 'tests/task_priority_test.py')
fixtures = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fixtures)


class SQLiteFixture:
    """Test backend: one row atomically holds all state, not the canonical ledger."""
    def __init__(self, path, state):
        self.path = path
        self.failure = None
        self.before_cas = None
        with sqlite3.connect(path) as db:
            db.execute('CREATE TABLE state (id INTEGER PRIMARY KEY, revision INTEGER, body TEXT)')
            db.execute('INSERT INTO state VALUES (1, 0, ?)', (json.dumps(state),))

    def read(self):
        with sqlite3.connect(self.path) as db:
            revision, body = db.execute('SELECT revision, body FROM state WHERE id=1').fetchone()
        return str(revision), json.loads(body)

    def compare_and_swap(self, expected_revision, state):
        if self.before_cas:
            action, self.before_cas = self.before_cas, None
            action()
        with sqlite3.connect(self.path) as db:
            db.execute('BEGIN IMMEDIATE')
            revision, = db.execute('SELECT revision FROM state WHERE id=1').fetchone()
            if str(revision) != expected_revision:
                raise gate.Rejected('stale revision at mutation boundary')
            db.execute('UPDATE state SET revision=?, body=? WHERE id=1',
                       (revision + 1, json.dumps(state)))
            if self.failure == 'before_commit':
                raise OSError('interrupted persistence before commit')
        if self.failure == 'after_commit':
            raise OSError('commit response lost; outcome uncertain')
        return str(revision + 1)


class EvidenceFixture:
    """A test allowlist, deliberately not a production authentication source."""
    def __init__(self):
        self.approved = {}

    def verify(self, evidence_id, binding):
        if self.approved.get(evidence_id) != binding:
            raise gate.Rejected('no independently authenticated evidence')
        return gate.VerifiedEvidence(binding, 'fixture:independent-reviewer', 'fixture:trusted-read')


class BoundaryTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.one = fixtures.record(901)
        self.one['required_evidence'] = ['source', 'test']
        self.two = fixtures.record(902, dependencies=['LX-REQ-901'])
        self.other = fixtures.record(903)
        self.state = {'schema': 1, 'ledger': {'records': [self.one, self.two, self.other]},
                      'checkpoint': {'schema': 1}, 'claims': {}, 'accepted_completions': {},
                      'used_evidence': [], 'used_commands': {}}
        self.store = SQLiteFixture(str(Path(self.tmp.name) / 'state.db'), self.state)
        self.verifier = EvidenceFixture()
        self.inputs = {'source': 'a' * 40, 'as_of': fixtures.NOW, 'capacity': 3}
        self.writer = gate.WriterBoundary(self.store, self.verifier, writer_id='fixture:sole-writer')

    def command(self, kind='complete', rid='LX-REQ-901', name='op-1'):
        finished = fixtures.complete(copy.deepcopy(self.one))
        payload = ({'evidence': finished['evidence'], 'completion': finished['completion']}
                   if kind == 'complete' else {'owner': 'fixture:worker'})
        return {'schema': 1, 'command_id': name, 'evidence_id': 'evidence:' + name,
                'kind': kind, 'request_id': rid, 'expected_revision': self.store.read()[0],
                'inputs_hash': gate.digest(self.inputs), 'payload': payload}

    def approve(self, command):
        binding = self.writer.preview(command, self.inputs)
        self.verifier.approved[command['evidence_id']] = binding
        return command

    def apply(self, command):
        return self.writer.apply(command, self.inputs)

    def test_consistent_forgery_passes_legacy_check_but_boundary_rejects(self):
        forged = fixtures.complete(copy.deepcopy(self.one))
        self.assertEqual(priority.completion_errors(forged), [])
        old = priority.plan({'records': [forged, self.two]}, fixtures.NOW, 1, read_status='success')
        self.assertIn('LX-REQ-902', [r['request_id'] for r in old['claim_candidates']])
        before = self.store.read()
        with self.assertRaisesRegex(gate.Rejected, 'authenticated'):
            self.apply(self.command())
        self.assertEqual(self.store.read(), before)

    def test_authenticated_completion_and_dependency_claim(self):
        self.apply(self.approve(self.command()))
        self.apply(self.approve(self.command('claim', 'LX-REQ-902', 'op-2')))
        revision, state = self.store.read()
        self.assertEqual(revision, '2')
        self.assertEqual(state['claims']['LX-REQ-902']['owner'], 'fixture:worker')
        self.assertEqual(len(state['used_evidence']), 2)
        self.assertEqual(state['checkpoint']['writer_boundary']['op-1']['request_id'], 'LX-REQ-901')

    def test_missing_dependencies_cannot_release_work(self):
        with self.assertRaisesRegex(gate.Rejected, 'dependenc'):
            self.writer.preview(self.command('claim', 'LX-REQ-902'), self.inputs)

    def test_preexisting_forged_completion_requires_independent_acceptance(self):
        state = copy.deepcopy(self.state)
        state['ledger']['records'][0] = fixtures.complete(copy.deepcopy(self.one))
        self.store.compare_and_swap('0', state)
        with self.assertRaisesRegex(gate.Rejected, 'dependenc'):
            self.writer.preview(self.command('claim', 'LX-REQ-902'), self.inputs)

    def test_altered_receipt_request_scope_input_and_revision_are_bound(self):
        changes = [lambda c: c['payload']['completion']['delivery']['evidence_refs'].append('forged'),
                   lambda c: c.update(request_id='LX-REQ-903'),
                   lambda c: c.update(inputs_hash='0' * 64)]
        for change in changes:
            with self.subTest(change=change):
                command = self.approve(self.command())
                change(command)
                with self.assertRaises(gate.Rejected):
                    self.apply(command)
        command = self.approve(self.command())
        state = self.store.read()[1]
        state['ledger']['records'][0]['priority_assessment']['work_areas'] = ['changed']
        self.store.compare_and_swap('0', state)
        command['expected_revision'] = '1'
        with self.assertRaises(gate.Rejected):
            self.apply(command)

    def test_input_tampering_invalidates_binding_even_with_recomputed_hash(self):
        command = self.approve(self.command())
        self.inputs['source'] = 'b' * 40
        command['inputs_hash'] = gate.digest(self.inputs)
        with self.assertRaises(gate.Rejected):
            self.apply(command)

    def test_replay_rejected_even_with_new_command_or_evidence_id(self):
        command = self.approve(self.command())
        self.apply(command)
        command['expected_revision'] = '1'
        with self.assertRaisesRegex(gate.Rejected, 'replay'):
            self.apply(command)
        command['evidence_id'] = 'new-evidence'
        with self.assertRaisesRegex(gate.Rejected, 'replay'):
            self.apply(command)
        command['evidence_id'] = 'evidence:op-1'
        command['command_id'] = 'op-2'
        with self.assertRaisesRegex(gate.Rejected, 'replay'):
            self.apply(command)

    def test_stale_revision_at_actual_cas_preserves_disjoint_commit(self):
        command = self.approve(self.command())
        other = self.approve(self.command('claim', 'LX-REQ-903', 'other'))
        self.store.before_cas = lambda: self.apply(other)
        with self.assertRaisesRegex(gate.Rejected, 'stale revision'):
            self.apply(command)
        state = self.store.read()[1]
        self.assertIn('LX-REQ-903', state['claims'])
        self.assertNotIn('LX-REQ-901', state['accepted_completions'])
        self.assertNotIn('evidence:op-1', state['used_evidence'])

    def test_overlapping_claim_blocks_only_conflict(self):
        state = copy.deepcopy(self.state)
        state['ledger']['records'][1]['dependencies'] = []
        state['ledger']['records'][1]['priority_assessment']['work_areas'] = ['901/child']
        self.store.compare_and_swap('0', state)
        self.apply(self.approve(self.command('claim', 'LX-REQ-901', 'first')))
        with self.assertRaisesRegex(gate.Rejected, 'overlap'):
            self.writer.preview(self.command('claim', 'LX-REQ-902', 'second'), self.inputs)
        self.apply(self.approve(self.command('claim', 'LX-REQ-903', 'third')))
        self.assertEqual(set(self.store.read()[1]['claims']), {'LX-REQ-901', 'LX-REQ-903'})

    def test_unknown_active_scope_blocks_new_claim(self):
        state = copy.deepcopy(self.state)
        state['ledger']['records'][0]['owner_execution_state'] = 'ACTIVE'
        state['ledger']['records'][0]['priority_assessment']['work_areas'] = []
        self.store.compare_and_swap('0', state)
        with self.assertRaisesRegex(gate.Rejected, 'scope'):
            self.writer.preview(self.command('claim', 'LX-REQ-903'), self.inputs)

    def test_interrupted_persistence_rolls_back_entire_bundle(self):
        command = self.approve(self.command())
        before = self.store.read()
        self.store.failure = 'before_commit'
        with self.assertRaises(OSError):
            self.apply(command)
        self.assertEqual(self.store.read(), before)
        self.assertIsNone(self.writer.lookup(command))

    def test_lost_response_is_reconciled_without_repeating_mutation(self):
        command = self.approve(self.command())
        self.store.failure = 'after_commit'
        with self.assertRaises(OSError):
            self.apply(command)
        self.assertEqual(self.writer.lookup(command)['request_id'], 'LX-REQ-901')
        self.assertEqual(self.store.read()[0], '1')

    def test_missing_ports_and_boolean_verification_fail_closed(self):
        with self.assertRaises(gate.Rejected):
            gate.WriterBoundary(self.store, None, writer_id='fixture:sole-writer')
        self.verifier.verify = lambda *args: True
        with self.assertRaises(gate.Rejected):
            self.apply(self.command())

    def test_stale_authenticated_binding_and_invalid_completion_are_rejected(self):
        command = self.command()
        self.verifier.verify = lambda *args: gate.VerifiedEvidence(
            {'expected_revision': 'other'}, 'fixture:reviewer', 'fixture:source')
        with self.assertRaises(gate.Rejected):
            self.apply(command)
        command['payload']['evidence']['test']['state'] = 'NOT_RUN'
        with self.assertRaisesRegex(gate.Rejected, 'unaccepted test'):
            self.writer.preview(command, self.inputs)

    def test_claim_capacity_and_complete_bundle_are_required(self):
        self.inputs['capacity'] = 0
        with self.assertRaisesRegex(gate.Rejected, 'capacity'):
            self.writer.preview(self.command('claim', 'LX-REQ-903'), self.inputs)
        state = self.store.read()[1]
        del state['used_evidence']
        self.store.compare_and_swap('0', state)
        with self.assertRaisesRegex(gate.Rejected, 'bundle'):
            self.writer.preview(self.command(), self.inputs)

    def test_interrupted_claim_does_not_persist_any_scope_or_replay_marker(self):
        command = self.approve(self.command('claim', 'LX-REQ-903'))
        before = self.store.read()
        self.store.failure = 'before_commit'
        with self.assertRaises(OSError):
            self.apply(command)
        self.assertEqual(self.store.read(), before)

    def test_altered_accepted_completion_no_longer_releases_dependency(self):
        self.apply(self.approve(self.command()))
        revision, state = self.store.read()
        state['ledger']['records'][0]['completion']['delivery']['evidence_refs'].append('altered')
        self.store.compare_and_swap(revision, state)
        with self.assertRaisesRegex(gate.Rejected, 'dependenc'):
            self.writer.preview(self.command('claim', 'LX-REQ-902', 'next'), self.inputs)

    def test_command_cannot_change_criterion_or_checkpoint_directly(self):
        command = self.command()
        command['payload']['completion_criterion'] = 'Weakened criterion'
        with self.assertRaises(gate.Rejected):
            self.writer.preview(command, self.inputs)
        command = self.command()
        command['checkpoint'] = {'schema': 1}
        with self.assertRaises(gate.Rejected):
            self.writer.preview(command, self.inputs)

    def test_lookup_rejects_reused_operation_id_with_different_bytes(self):
        command = self.approve(self.command())
        self.apply(command)
        command['payload']['completion']['delivery']['evidence_refs'].append('altered')
        with self.assertRaisesRegex(gate.Rejected, 'collision'):
            self.writer.lookup(command)


if __name__ == '__main__':
    unittest.main()
