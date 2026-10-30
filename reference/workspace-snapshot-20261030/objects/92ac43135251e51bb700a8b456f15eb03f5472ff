#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The pipeline advances only after the next stage's thread id is stored."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

TOP = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(TOP / 'tools'))
import stage_handoff

TS = '2026-10-24T19:20:00Z'


class StageHandoff(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.path = Path(self.tmp.name) / 'pipeline.json'

    def ledger(self):
        return json.loads(self.path.read_text(encoding='utf-8'))

    def test_stage_order_and_default_ledger(self):
        self.assertEqual(stage_handoff.STAGES,
                         ('Audit', 'Implement', 'Test', 'Validate', 'Merge', 'Publish'))
        self.assertEqual(stage_handoff.DEFAULT_LEDGER, TOP / 'cloud-handoff' / 'pipeline.json')

    def test_advance_waits_for_a_stored_thread_id(self):
        stage_handoff.init(self.path)
        before = self.path.read_bytes()
        with self.assertRaisesRegex(stage_handoff.HandoffError, 'thread id is recorded'):
            stage_handoff.advance(self.path)
        self.assertEqual(self.path.read_bytes(), before)

        stage_handoff.record_thread(self.path, 'Audit', 'thread-audit', TS)
        recorded = self.ledger()
        self.assertIsNone(recorded['current'])
        self.assertEqual(recorded['transitions'], [{
            'stage': 'Audit',
            'thread_id': 'thread-audit',
            'timestamp': TS,
            'status': 'recorded',
            'kind': 'thread',
        }])

        stage_handoff.advance(self.path)
        active = self.ledger()
        self.assertEqual(active['current'], 'Audit')
        self.assertEqual(active['transitions'][0]['status'], 'active')
        self.assertEqual(active['transitions'][0]['thread_id'], 'thread-audit')

    def test_missing_or_subagent_thread_does_not_advance(self):
        stage_handoff.init(self.path)
        state = self.ledger()
        state['transitions'].append({
            'stage': 'Audit',
            'thread_id': None,
            'timestamp': TS,
            'status': 'recorded',
            'kind': 'thread',
        })
        self.path.write_text(json.dumps(state, indent=2) + '\n', encoding='utf-8')
        before = self.path.read_bytes()
        with self.assertRaisesRegex(stage_handoff.HandoffError, 'thread id is recorded'):
            stage_handoff.advance(self.path)
        self.assertEqual(self.path.read_bytes(), before)

        state['transitions'][0]['thread_id'] = 'sub-1'
        state['transitions'][0]['kind'] = 'subagent'
        self.path.write_text(json.dumps(state, indent=2) + '\n', encoding='utf-8')
        before = self.path.read_bytes()
        with self.assertRaisesRegex(stage_handoff.HandoffError, 'thread id is recorded'):
            stage_handoff.advance(self.path)
        self.assertEqual(self.path.read_bytes(), before)

    def test_rejected_record_does_not_change_the_ledger(self):
        stage_handoff.init(self.path)
        before = self.path.read_bytes()
        with self.assertRaisesRegex(stage_handoff.HandoffError, 'out of order'):
            stage_handoff.record_thread(self.path, 'Test', 'thread-test', TS)
        with self.assertRaisesRegex(stage_handoff.HandoffError, 'thread id required'):
            stage_handoff.record_thread(self.path, 'Audit', ' ', TS)
        with self.assertRaisesRegex(stage_handoff.HandoffError, 'timestamp'):
            stage_handoff.record_thread(self.path, 'Audit', 'thread-audit', 'yesterday')
        self.assertEqual(self.path.read_bytes(), before)

    def test_each_stage_is_stored_before_it_becomes_current(self):
        stage_handoff.init(self.path)
        previous = None
        for index, stage in enumerate(stage_handoff.STAGES):
            stamp = f'2026-10-24T19:2{index}:00Z'
            thread = f'thread-{stage}'
            stage_handoff.record_thread(self.path, stage, thread, stamp)
            midpoint = self.ledger()
            self.assertEqual(midpoint['current'], previous)
            self.assertEqual(midpoint['transitions'][-1]['thread_id'], thread)
            self.assertEqual(midpoint['transitions'][-1]['status'], 'recorded')
            stage_handoff.advance(self.path)
            self.assertEqual(self.ledger()['current'], stage)
            self.assertEqual(self.ledger()['transitions'][-1]['status'], 'active')
            previous = stage
        with self.assertRaisesRegex(stage_handoff.HandoffError, 'complete'):
            stage_handoff.advance(self.path)
        with self.assertRaisesRegex(stage_handoff.HandoffError, 'complete'):
            stage_handoff.record_thread(self.path, 'Audit', 'thread-again', TS)
        self.assertEqual(self.ledger()['current'], 'Publish')

    def test_one_thread_id_cannot_walk_every_stage(self):
        stage_handoff.init(self.path)
        shared = 'thread-shared'
        stage_handoff.handoff(self.path, 'Audit', shared, TS)
        self.assertEqual(self.ledger()['current'], 'Audit')
        before = self.path.read_bytes()
        with self.assertRaisesRegex(stage_handoff.HandoffError, 'its own thread'):
            stage_handoff.handoff(self.path, 'Implement', shared, '2026-10-24T19:21:00Z')
        self.assertEqual(self.path.read_bytes(), before)
        for index, stage in enumerate(stage_handoff.STAGES[2:], start=2):
            with self.assertRaises(stage_handoff.HandoffError):
                stage_handoff.handoff(self.path, stage, shared, f'2026-10-24T19:2{index}:00Z')
            self.assertEqual(self.path.read_bytes(), before)
        self.assertEqual(self.ledger()['current'], 'Audit')
        self.assertNotEqual(self.ledger()['current'], 'Publish')

        state = self.ledger()
        state['transitions'].append({
            'stage': 'Implement',
            'thread_id': shared,
            'timestamp': TS,
            'status': 'recorded',
            'kind': 'thread',
        })
        self.path.write_text(json.dumps(state, indent=2) + '\n', encoding='utf-8')
        reused = self.path.read_bytes()
        with self.assertRaisesRegex(stage_handoff.HandoffError, 'its own thread'):
            stage_handoff.advance(self.path)
        self.assertEqual(self.path.read_bytes(), reused)
        self.assertEqual(self.ledger()['current'], 'Audit')

        distinct = Path(self.tmp.name) / 'distinct.json'
        stage_handoff.init(distinct)
        for index, stage in enumerate(stage_handoff.STAGES):
            stage_handoff.handoff(distinct, stage, f'thread-{index}', f'2026-10-24T19:3{index}:00Z')
        done = json.loads(distinct.read_text(encoding='utf-8'))
        self.assertEqual(done['current'], 'Publish')
        self.assertEqual([item['thread_id'] for item in done['transitions']],
                         [f'thread-{index}' for index in range(len(stage_handoff.STAGES))])

    def test_handoff_persists_the_thread_before_advance_saves(self):
        stage_handoff.init(self.path)
        seen = []
        real_save = stage_handoff.save

        def spy(path, state):
            seen.append((state['current'], [item['thread_id'] for item in state['transitions']]))
            real_save(path, state)

        original = stage_handoff.save
        stage_handoff.save = spy
        try:
            stage_handoff.handoff(self.path, 'Audit', 'thread-audit', TS)
        finally:
            stage_handoff.save = original
        self.assertEqual(seen[0], (None, ['thread-audit']))
        self.assertEqual(seen[1][0], 'Audit')
        self.assertEqual(self.ledger()['current'], 'Audit')

    def test_subagents_only_help_the_running_stage(self):
        stage_handoff.init(self.path)
        with self.assertRaisesRegex(stage_handoff.HandoffError, 'already running'):
            stage_handoff.note_helper(self.path, 'Audit', 'sub-1', TS)
        stage_handoff.handoff(self.path, 'Audit', 'thread-audit', TS)
        stage_handoff.note_helper(self.path, 'Audit', 'sub-1', TS)
        state = self.ledger()
        self.assertEqual(state['current'], 'Audit')
        self.assertEqual(state['helpers'], [{
            'stage': 'Audit',
            'subagent_id': 'sub-1',
            'timestamp': TS,
            'status': 'in-stage',
        }])
        self.assertEqual(len(state['transitions']), 1)
        before = self.path.read_bytes()
        with self.assertRaisesRegex(stage_handoff.HandoffError, 'thread id is recorded'):
            stage_handoff.advance(self.path)
        with self.assertRaisesRegex(stage_handoff.HandoffError, 'not a subagent'):
            stage_handoff.record_thread(self.path, 'Implement', 'sub-1', TS)
        with self.assertRaisesRegex(stage_handoff.HandoffError, 'already running'):
            stage_handoff.note_helper(self.path, 'Implement', 'sub-2', TS)
        with self.assertRaisesRegex(stage_handoff.HandoffError, 'not a subagent'):
            stage_handoff.note_helper(self.path, 'Audit', 'thread-audit', TS)
        self.assertEqual(self.path.read_bytes(), before)

    def test_cli_consults_the_ledger(self):
        script = TOP / 'tools' / 'stage_handoff.py'

        def run(*args):
            return subprocess.run([sys.executable, str(script), '--ledger', str(self.path), *args],
                                  text=True, capture_output=True)

        created = run('init')
        self.assertEqual(created.returncode, 0, created.stderr)
        refused = run('advance')
        self.assertEqual(refused.returncode, 1, refused.stdout)
        self.assertIn('thread id is recorded', refused.stderr)
        self.assertIsNone(self.ledger()['current'])
        delivered = run('handoff', '--stage', 'Audit', '--thread-id', 'bc-audit', '--timestamp', TS)
        self.assertEqual(delivered.returncode, 0, delivered.stderr)
        self.assertEqual(self.ledger()['current'], 'Audit')
        self.assertEqual(self.ledger()['transitions'][0]['thread_id'], 'bc-audit')
        skipped = run('handoff', '--stage', 'Validate', '--thread-id', 'bc-validate', '--timestamp', TS)
        self.assertEqual(skipped.returncode, 1, skipped.stdout)
        self.assertIn('out of order', skipped.stderr)
        self.assertEqual(self.ledger()['current'], 'Audit')
        self.assertEqual([item['stage'] for item in self.ledger()['transitions']], ['Audit'])
        shown = subprocess.run([sys.executable, str(script), 'show', '--ledger', str(self.path)],
                               text=True, capture_output=True)
        self.assertEqual(shown.returncode, 0, shown.stderr)
        self.assertIn('bc-audit', shown.stdout)


if __name__ == '__main__':
    unittest.main()
