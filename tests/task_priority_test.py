#!/usr/bin/env python3
"""Observable recommendation order, starvation, authority and ownership checks."""
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('task_priority', Path(__file__).resolve().parents[1] / 'coordination/requests/task_priority.py')
p = importlib.util.module_from_spec(spec)
spec.loader.exec_module(p)
NOW = '2026-10-25T12:00:00Z'


def record(n, category='ux_repair', effort='small', **changes):
    r = {'request_id': f'LX-REQ-{n:03d}', 'status': 'PLANNED', 'owner': 'existing owner',
         'title': 'Scoped source request', 'completion_criterion': 'Deliver reviewed source proof.',
         'related_requests': [],
         'next_action': 'Run bounded source/test review; retain safety gates.', 'dependencies': [],
         'updated_at': '2026-10-24T12:00:00Z', 'priority_assessment': {
             'category': category, 'severity': 'moderate', 'reach': 'affected workflow',
             'confidence': 'source_verified', 'effort': effort, 'readiness': 'ready',
             'rationale': 'source finding', 'evidence_refs': ['fixture:source'], 'work_areas': [str(n)]}}
    r.update(changes)
    return r


def complete(r):
    r['status'] = 'COMPLETED'
    r['required_evidence'] = ['source', 'test']
    r['evidence'] = {level: {'state': 'NOT_RUN'} for level in p.EVIDENCE_LEVELS}
    r['evidence']['source'] = {'state': 'SOURCE_DELIVERED', 'revision': 'a' * 40}
    r['evidence']['test'] = {'state': 'PASS', 'receipt': 'fixture:independent-source-proof'}
    r['completion'] = {'state': 'ACCEPTED', 'criterion': r['completion_criterion'],
        'evidence_hashes': {level: p.evidence_hash(r['evidence'][level]) for level in r['required_evidence']},
        'delivery': {'state': 'VERIFIED', 'evidence_refs': ['fixture:exact-remote-source-readback']}}
    return r


class PriorityTests(unittest.TestCase):
    def test_prepared_same_owner_continuation_is_not_a_new_claim(self):
        existing = record(20, effort='complex', owner_execution_state=
            'RECOVERY_COORDINATION_PREPARED_RECEIVER_EXECUTION_UNVERIFIED')
        ready = record(32, 'feature_research', 'complex', owner='unassigned')
        for capacity in (0, 1, 2):
            with self.subTest(capacity=capacity):
                ledger = {'records': [existing, ready]}
                before = copy.deepcopy(ledger)
                result = p.plan(ledger, NOW, capacity, read_status='success')
                self.assertEqual(ledger, before)
                self.assertEqual([r['request_id'] for r in result['continue_existing']], ['LX-REQ-020'])
                self.assertEqual(result['continue_existing'][0]['next_action'], existing['next_action'])
                self.assertEqual(result['continue_existing'][0]['owner_execution_state'], existing['owner_execution_state'])
                self.assertEqual([r['request_id'] for r in result['claim_candidates']],
                                 ['LX-REQ-032'] if capacity else [])
                self.assertNotIn('LX-REQ-020', [r['request_id'] for r in result['delivery_order']])

    def test_retained_scope_blocks_overlap_not_disjoint_work(self):
        existing = record(20, owner_execution_state=
            'RECOVERY_COORDINATION_PREPARED_RECEIVER_EXECUTION_UNVERIFIED')
        overlap = record(1)
        overlap['priority_assessment']['work_areas'] = ['20']
        for capacity in (0, 1, 2):
            result = self.plan(existing, overlap, record(32), capacity=capacity)
            self.assertEqual([r['request_id'] for r in result['claim_candidates']],
                             ['LX-REQ-032'] if capacity else [])
            self.assertEqual(len(result['continue_existing']), 1)
        self.assertEqual(self.plan(existing, record(32), busy_areas=['32'])['claim_candidates'], [])
        existing['priority_assessment']['work_areas'] = []
        result = self.plan(existing, record(32))
        self.assertEqual(result['claim_candidates'], [])
        self.assertEqual(result['claim_admission']['blocked_by'], ['LX-REQ-020'])
        self.assertEqual(result['continue_existing'][0]['recorded_next_action'], existing['next_action'])
        self.assertEqual(result['recovery_actions'][0]['action_kind'], 'READ_ONLY_OWNERSHIP_RECONCILIATION')

    def test_owner_labels_and_unknown_states_do_not_invent_retained_claims(self):
        for owner, state in [('unassigned', None), ('previous owner', 'RELEASED'),
                             ('historical label', 'UNKNOWN'), ('old owner', 'UNAVAILABLE')]:
            with self.subTest(owner=owner, state=state):
                result = self.plan(record(1, owner=owner, owner_execution_state=state))
                self.assertEqual([r['request_id'] for r in result['claim_candidates']], ['LX-REQ-001'])
                self.assertEqual(result['continue_existing'], [])

    def test_closed_or_held_retained_continuation_reserves_no_scope(self):
        for changes in ({'status': 'CANCELLED'}, {'status': 'SUPERSEDED'},
                        {'status': 'COMPLETED'}, {'status': 'DONE'},
                        {'priority': 'DEFERRED'}, {'status': 'OWNER_DEFERRED'},
                        {'implementation_authorized': False}, {'intake_eligible': False}):
            for areas in (['32'], [], None):
                with self.subTest(changes=changes, areas=areas):
                    held = record(20, owner_execution_state=
                        'RECOVERY_COORDINATION_PREPARED_RECEIVER_EXECUTION_UNVERIFIED', **changes)
                    held['priority_assessment']['work_areas'] = areas
                    if held['status'] in p.COMPLETED:
                        held = complete(held)
                        held.update(changes)
                    result = self.plan(held, record(32), capacity=1)
                    self.assertEqual([r['request_id'] for r in result['claim_candidates']], ['LX-REQ-032'])
                    self.assertEqual(result['continue_existing'], [])
                    self.assertEqual(result['recovery_actions'], [])
                    self.assertEqual(result['claim_admission']['blocked_by'], [])
                    self.assertEqual(result['claim_admission']['state'], 'ELIGIBLE')

    def test_blocked_prepared_action_is_excluded_without_blocking_independent_work(self):
        for status in ('BLOCKED_KNOWN_PERMISSION_DENIAL', 'BLOCKED_DEPENDENCY_UNMET'):
            for areas in (['32'], [], None):
                for capacity in (0, 1, 2):
                    with self.subTest(status=status, areas=areas, capacity=capacity):
                        held = record(20, status=status, next_action='Perform the specifically blocked write.',
                            owner_execution_state='RECOVERY_COORDINATION_PREPARED_RECEIVER_EXECUTION_UNVERIFIED')
                        held['priority_assessment']['work_areas'] = areas
                        independent = record(12, owner=held['owner'],
                            owner_execution_state='RECOVERY_COORDINATION_PREPARED_RECEIVER_EXECUTION_UNVERIFIED')
                        ledger = {'records': [held, independent, record(32)]}
                        before = copy.deepcopy(ledger)
                        result = p.plan(ledger, NOW, capacity, read_status='success')
                        self.assertEqual(ledger, before)
                        self.assertEqual([r['request_id'] for r in result['continue_existing']], ['LX-REQ-012'])
                        self.assertEqual(result['continue_existing'][0]['next_action'], independent['next_action'])
                        self.assertEqual([r['request_id'] for r in result['claim_candidates']],
                                         ['LX-REQ-032'] if capacity else [])
                        self.assertEqual(result['recovery_actions'], [])
                        self.assertEqual(result['claim_admission']['blocked_by'], [])
                        excluded = next(r for r in result['excluded'] if r['request_id'] == 'LX-REQ-020')
                        self.assertIn(status, excluded['reason'])
                        self.assertNotEqual(excluded['next_action'], held['next_action'])
                        self.assertIn('Read-only', excluded['next_action'])

    def test_prepared_unfinished_dependency_preserves_independent_work_at_any_capacity(self):
        for areas in (['32'], [], None):
            for capacity in (0, 1, 2):
                with self.subTest(areas=areas, capacity=capacity):
                    dependency = record(16, status='REVIEW_PENDING')
                    dependency['priority_assessment']['readiness'] = 'blocked'
                    held = record(20, status='ACCEPTED_CONTINUATION_HANDOFFS_READY',
                        dependencies=['LX-REQ-016'], next_action='Perform the dependent write.',
                        owner_execution_state='RECOVERY_COORDINATION_PREPARED_RECEIVER_EXECUTION_UNVERIFIED')
                    held['priority_assessment']['work_areas'] = areas
                    independent = record(12, owner=held['owner'],
                        owner_execution_state='RECOVERY_COORDINATION_PREPARED_RECEIVER_EXECUTION_UNVERIFIED')
                    ledger = {'records': [held, dependency, independent, record(32)]}
                    before = copy.deepcopy(ledger)
                    result = p.plan(ledger, NOW, capacity, read_status='success')
                    self.assertEqual(ledger, before)
                    self.assertEqual([r['request_id'] for r in result['continue_existing']], ['LX-REQ-012'])
                    self.assertEqual(result['continue_existing'][0]['next_action'], independent['next_action'])
                    self.assertEqual([r['request_id'] for r in result['claim_candidates']],
                                     ['LX-REQ-032'] if capacity else [])
                    self.assertEqual(result['recovery_actions'], [])
                    self.assertEqual(result['claim_admission']['blocked_by'], [])
                    excluded = next(r for r in result['excluded'] if r['request_id'] == 'LX-REQ-020')
                    self.assertIn('blocked dependency: LX-REQ-016', excluded['reason'])
                    self.assertIn('Read-only', excluded['next_action'])
                    self.assertNotEqual(excluded['next_action'], held['next_action'])

    def test_prepared_missing_cyclic_and_unaccepted_dependencies_do_not_continue(self):
        negative = complete(record(16))
        negative['evidence']['test']['state'] = 'FAIL'
        stale = complete(record(16))
        stale['evidence']['test']['receipt'] = 'fixture:changed-unaccepted-proof'
        cases = [('missing', [], 'LX-REQ-016'),
                 ('cancelled', [record(16, status='CANCELLED')], 'LX-REQ-016'),
                 ('superseded', [record(16, status='SUPERSEDED')], 'LX-REQ-016'),
                 ('bare_done', [record(16, status='DONE')], 'LX-REQ-016'),
                 ('negative', [negative], 'LX-REQ-016'),
                 ('stale', [stale], 'LX-REQ-016'),
                 ('self', [], 'LX-REQ-020'),
                 ('cycle', [record(16, dependencies=['LX-REQ-020'], owner_execution_state=
                    'RECOVERY_COORDINATION_PREPARED_RECEIVER_EXECUTION_UNVERIFIED')], 'LX-REQ-016')]
        for label, dependencies, dependency_id in cases:
            with self.subTest(case=label):
                held = record(20, dependencies=[dependency_id], owner_execution_state=
                    'RECOVERY_COORDINATION_PREPARED_RECEIVER_EXECUTION_UNVERIFIED')
                independent = record(12, owner=held['owner'], owner_execution_state=
                    'RECOVERY_COORDINATION_PREPARED_RECEIVER_EXECUTION_UNVERIFIED')
                ledger = {'records': [held, *dependencies, independent, record(32)]}
                before = copy.deepcopy(ledger)
                result = p.plan(ledger, NOW, 2, read_status='success')
                self.assertEqual(ledger, before)
                self.assertEqual([r['request_id'] for r in result['continue_existing']], ['LX-REQ-012'])
                self.assertEqual([r['request_id'] for r in result['claim_candidates']], ['LX-REQ-032'])
                excluded = next(r for r in result['excluded'] if r['request_id'] == 'LX-REQ-020')
                self.assertIn('blocked dependency: ' + dependency_id, excluded['reason'])
                self.assertIn('Read-only', excluded['next_action'])

    def test_prepared_accepted_dependency_continues_without_spending_capacity(self):
        for status in ('COMPLETED', 'DONE'):
            for capacity in (0, 1, 2):
                with self.subTest(status=status, capacity=capacity):
                    dependency = complete(record(16))
                    dependency['status'] = status
                    existing = record(20, dependencies=['LX-REQ-016'], owner_execution_state=
                        'RECOVERY_COORDINATION_PREPARED_RECEIVER_EXECUTION_UNVERIFIED')
                    result = self.plan(existing, dependency, record(32), capacity=capacity)
                    self.assertEqual([r['request_id'] for r in result['continue_existing']], ['LX-REQ-020'])
                    self.assertEqual(result['continue_existing'][0]['next_action'], existing['next_action'])
                    self.assertEqual([r['request_id'] for r in result['claim_candidates']],
                                     ['LX-REQ-032'] if capacity else [])

    def test_reported_active_dependency_keeps_legacy_owner_and_scope_reservation(self):
        for changes in ({'owner_execution_state': 'ACTIVE'},
                        {'owner_execution_state': 'ACTIVE_STAGE_REPORTED'}, {'status': 'ACTIVE'},
                        {'status': 'ACTIVE', 'owner_execution_state':
                         'RECOVERY_COORDINATION_PREPARED_RECEIVER_EXECUTION_UNVERIFIED'}):
            with self.subTest(changes=changes):
                existing = record(20, dependencies=['LX-REQ-016'], **changes)
                existing['priority_assessment']['work_areas'] = ['32']
                dependency = record(16)
                dependency['priority_assessment']['readiness'] = 'blocked'
                result = self.plan(existing, dependency, record(32), record(33), capacity=2)
                self.assertEqual([r['request_id'] for r in result['continue_existing']], ['LX-REQ-020'])
                self.assertEqual(result['continue_existing'][0]['next_action'], existing['next_action'])
                self.assertEqual([r['request_id'] for r in result['claim_candidates']], ['LX-REQ-033'])

    def test_retained_continuation_obeys_deferral_completion_and_read_gates(self):
        existing = record(20, owner_execution_state=
            'RECOVERY_COORDINATION_PREPARED_RECEIVER_EXECUTION_UNVERIFIED')
        for status in ('unknown', 'failed', 'stale', 'incomplete', 'unavailable', 'rejected'):
            result = p.plan({'records': [existing, record(32)]}, NOW, 2, read_status=status)
            self.assertEqual(result['state'], 'BLOCKED')
            self.assertEqual(result['claim_candidates'], [])
            self.assertEqual(result['continue_existing'], [])
        for held in (dict(existing, implementation_authorized=False),
                     dict(existing, priority='DEFERRED'), complete(copy.deepcopy(existing))):
            result = self.plan(held, record(32))
            self.assertEqual(result['continue_existing'], [])
            self.assertEqual([r['request_id'] for r in result['claim_candidates']], ['LX-REQ-032'])

    def plan(self, *records, capacity=2, **kw):
        return p.plan({'records': list(records)}, NOW, capacity, read_status='success', **kw)

    def test_high_risk_beats_cosmetic_even_if_cosmetic_is_quick(self):
        risky = record(2, 'safety_integrity', 'complex')
        risky['priority_assessment']['severity'] = 'critical'
        result = self.plan(record(1, 'small_improvement'), risky)
        self.assertEqual(result['delivery_order'][0]['request_id'], 'LX-REQ-002')
        self.assertEqual(result['lanes'], {'quick': 'LX-REQ-001', 'complex': 'LX-REQ-002'})

    def test_safety_quick_keeps_first_slot_before_speculative_complex(self):
        critical = record(1, 'safety_integrity')
        feature = record(2, 'feature_research', 'complex', updated_at='2020-01-01T00:00:00Z')
        self.assertEqual(self.plan(critical, feature, capacity=1)['lanes'],
                         {'quick': 'LX-REQ-001', 'complex': None})
        feature['priority_assessment']['work_areas'] = ['1']
        self.assertEqual(self.plan(critical, feature)['lanes'],
                         {'quick': 'LX-REQ-001', 'complex': None})

    def test_quick_ux_repair_precedes_speculative_feature_parallel_progress(self):
        result = self.plan(record(1, 'feature_research', 'complex'), record(2))
        self.assertEqual([r['request_id'] for r in result['delivery_order']], ['LX-REQ-002', 'LX-REQ-001'])
        self.assertEqual(result['lanes']['complex'], 'LX-REQ-001')
        self.assertEqual(result['lanes']['quick'], 'LX-REQ-002')

    def test_aged_complex_gets_reserved_lane_despite_new_quick_stream(self):
        old = record(9, 'feature_research', 'complex', updated_at='2026-10-01T12:00:00Z')
        result = self.plan(record(1, 'ux_repair', 'complex'), old, record(2), record(3))
        self.assertEqual(result['lanes']['complex'], 'LX-REQ-009')
        self.assertEqual(result['lanes']['quick'], 'LX-REQ-002')

    def test_committed_complex_is_not_indefinitely_deferred(self):
        committed = record(9, 'feature_research', 'complex')
        committed['priority_assessment']['committed_at'] = '2026-10-24T12:00:00Z'
        self.assertEqual(self.plan(record(1, 'ux_repair', 'complex'), committed)['lanes']['complex'], 'LX-REQ-009')

    def test_blocked_dependency_not_selected_but_unrelated_ready_work_proceeds(self):
        blocked = record(1, 'safety_integrity', dependencies=['LX-REQ-002'])
        result = self.plan(blocked, record(2), record(3))
        self.assertNotIn('LX-REQ-001', [r['request_id'] for r in result['delivery_order']])
        self.assertIn('blocked dependency', result['excluded'][0]['reason'])
        self.assertEqual(result['lanes']['quick'], 'LX-REQ-002')
        dep = complete(record(2))
        self.assertEqual(self.plan(blocked, dep)['lanes']['quick'], 'LX-REQ-001')
        dep['status'] = 'CANCELLED'
        self.assertIsNone(self.plan(blocked, dep)['lanes']['quick'])

    def test_owner_deferral_always_wins_over_priority_age_and_commitment(self):
        for field, value in [('priority', 'DEFERRED'), ('status', 'OWNER_DEFERRED_IDEA_NOT_INTAKE_READY'),
                             ('implementation_authorized', False), ('intake_eligible', False)]:
            with self.subTest(field=field):
                r = record(38, 'safety_integrity', updated_at='2020-01-01T00:00:00Z', **{field: value})
                r['priority_assessment']['committed_at'] = '2020-01-01T00:00:00Z'
                result = self.plan(r)
                self.assertEqual(result['delivery_order'], [])
                self.assertIsNone(result['lanes']['quick'])

    def test_claim_owner_and_input_stable_active_area_not_competed(self):
        running = record(1, owner_execution_state='ACTIVE_STAGE_REPORTED')
        other = record(2)
        other['priority_assessment']['work_areas'] = ['1']
        ledger = {'records': [running, other]}
        before = copy.deepcopy(ledger)
        result = p.plan(ledger, NOW, read_status='success')
        self.assertEqual(ledger, before)
        self.assertEqual(result, p.plan(ledger, NOW, read_status='success'))
        self.assertEqual(result['continue_existing'][0]['implementation_owner'], 'existing owner')
        self.assertIsNone(result['lanes']['quick'])

    def test_no_capacity_or_independence_no_new_lane(self):
        self.assertEqual(self.plan(record(1), capacity=0)['lanes'], {'quick': None, 'complex': None})
        self.assertEqual(p.plan({'records': [record(1)]}, NOW, read_status='success')['lanes'], {'quick': None, 'complex': None})
        self.assertIsNone(self.plan(record(1), busy_areas=['1'])['lanes']['quick'])
        shared = record(2, 'feature_research', 'complex')
        shared['priority_assessment']['work_areas'] = ['1']
        self.assertIsNone(self.plan(record(1), shared)['lanes']['quick'])

    def test_unknown_effort_is_not_invented_unknown_readiness_is_excluded(self):
        r = record(1, effort='unknown')
        result = self.plan(r)
        self.assertEqual(result['delivery_order'][0]['effort'], 'unknown')
        self.assertEqual(result['lanes'], {'quick': None, 'complex': None})
        r['priority_assessment']['readiness'] = 'unknown'
        self.assertEqual(self.plan(r)['delivery_order'], [])

    def test_feedback_does_not_reset_age(self):
        r = record(1, history=[{'observed_at': '2026-10-01T12:00:00Z', 'previous': {}}])
        self.assertEqual(self.plan(r)['delivery_order'][0]['age_days'], 24)

    def test_existing_validator_importlib_api_accepts_optional_schema(self):
        v_spec = importlib.util.spec_from_file_location('request_validator',
            Path(__file__).resolve().parents[1] / 'coordination/requests/validate_requests.py')
        validator = importlib.util.module_from_spec(v_spec)
        v_spec.loader.exec_module(validator)
        self.assertEqual(validator.validate_assessment(record(1)['priority_assessment']), [])
        self.assertTrue(validator.validate_assessment({'category': 'fake_score'}))

    def test_legacy_without_assessment_and_invalid_assessments(self):
        r = record(1)
        del r['priority_assessment']
        self.assertEqual(self.plan(r)['delivery_order'], [])
        r['priority_assessment'] = {'category': 'fake_score'}
        result = self.plan(r, record(2))
        self.assertEqual([x['request_id'] for x in result['claim_candidates']], ['LX-REQ-002'])
        self.assertIn('priority_assessment', result['excluded'][0]['next_action'])
        with self.assertRaises(ValueError):
            self.plan(record(1), capacity=-1)
        with self.assertRaises(ValueError):
            p.plan({'records': [record(1)]}, '2026-10-25')

    def test_unread_queue_never_becomes_complete_or_claimable(self):
        for status in ('unknown', 'failed', 'stale', 'incomplete', 'NOT_READ'):
            with self.subTest(status=status):
                result = p.plan({'records': []}, NOW, 4, read_status=status)
                self.assertEqual(result['state'], 'BLOCKED')
                self.assertEqual(result['claim_candidates'], [])
        self.assertEqual(p.plan({'records': []}, NOW)['state'], 'BLOCKED')

    def test_invalid_inactive_assessment_is_scoped_but_writer_uncertainty_is_not_ignored(self):
        bad = record(1)
        bad['priority_assessment']['severity'] = 'unrecognized'
        result = self.plan(bad, record(2))
        self.assertEqual([x['request_id'] for x in result['claim_candidates']], ['LX-REQ-002'])
        self.assertIn('priority_assessment.severity', result['excluded'][0]['next_action'])
        bad['owner_execution_state'] = 'ACTIVE'
        del bad['priority_assessment']['work_areas']
        result = self.plan(bad, record(2))
        self.assertEqual(result['claim_admission']['state'], 'BLOCKED')
        self.assertEqual(result['claim_admission']['blocked_by'], ['LX-REQ-001'])
        self.assertEqual(result['claim_candidates'], [])
        self.assertEqual(result['continue_existing'][0]['request_id'], 'LX-REQ-001')
        self.assertEqual(result['recovery_actions'][0]['action_kind'], 'READ_ONLY_OWNERSHIP_RECONCILIATION')
        with self.assertRaisesRegex(ValueError, 'duplicate request IDs'):
            self.plan(record(1), record(1))
        with self.assertRaisesRegex(ValueError, 'duplicate request IDs'):
            self.plan(bad, copy.deepcopy(bad))

    def test_missing_active_areas_keep_existing_and_read_only_recovery_at_any_capacity(self):
        running = record(5, owner_execution_state='ACTIVE_STAGE_REPORTED', owner='existing SMC owner')
        for areas in (None, [], [''], ['smc', None], 'smc'):
            for capacity in (0, 4):
                with self.subTest(areas=areas, capacity=capacity):
                    unknown = record(20, owner_execution_state='ACTIVE_STAGE_REPORTED')
                    unknown['priority_assessment']['work_areas'] = areas
                    ledger = {'records': [running, unknown, record(1, 'safety_integrity')]}
                    before = copy.deepcopy(ledger)
                    result = p.plan(ledger, NOW, capacity, read_status='success')
                    self.assertEqual(ledger, before)
                    self.assertEqual(result['state'], 'WORK_REQUIRED')
                    self.assertEqual(result['claim_admission']['state'], 'BLOCKED')
                    self.assertEqual(result['claim_candidates'], [])
                    self.assertEqual(result['lanes'], {'quick': None, 'complex': None})
                    continuing = {x['request_id']: x for x in result['continue_existing']}
                    self.assertEqual(set(continuing), {'LX-REQ-005', 'LX-REQ-020'})
                    self.assertEqual(continuing['LX-REQ-005']['next_action'], running['next_action'])
                    self.assertEqual(continuing['LX-REQ-005']['implementation_owner'], running['owner'])
                    self.assertEqual(continuing['LX-REQ-020']['recorded_next_action'], unknown['next_action'])
                    self.assertEqual(continuing['LX-REQ-020']['next_action'], result['recovery_actions'][0]['next_action'])
                    self.assertEqual(result['recovery_actions'][0]['action_kind'], 'READ_ONLY_OWNERSHIP_RECONCILIATION')

    def test_actual_ledger_legacy_active_writer_shape_survives_cli_at_zero_capacity(self):
        # Minimal exact active-state/owner/action fields from the 38-request ledger
        # at f893a17cb179483fedf9c504dae273bc759dff64, retained by PR654's reconciliation.
        smc = record(5, status='SOURCE_IMPLEMENTED_PACKAGE_PENDING',
                     owner='SMC layout and control implementation owner',
                     owner_execution_state='ACTIVE_STAGE_REPORTED',
                     next_action='SMC implementation/build owner to finish size recovery and exact-head package, then independent UI/control review and model qualification; preserve installed-source distinction.')
        smc['priority_assessment']['work_areas'] = ['smc']
        scheduler = record(20, status='TWELVE_PROMPT_UPDATES_SAVED_CADENCE_UNCHANGED',
                           owner='Scheduler maintenance coordinator',
                           owner_execution_state='ACTIVE_STAGE_REPORTED',
                           next_action='Scheduler maintenance coordinator to verify actual next-run intake/dispatch and gather outcome evidence; retain existing cadence.',
                           priority_assessment=None)
        tracker = record(27, status='PRIOR_TRACKER_REMOTE_VERIFIED_DELTA_RECONCILIATION_ACTIVE',
                         owner='Sole request tracker writer', owner_execution_state='ACTIVE_STAGE_REPORTED',
                         next_action='Sole request tracker writer to validate this bounded delta, obtain independent privacy/continuity review, push normally and verify remote SHA.',
                         priority_assessment=None)
        with tempfile.TemporaryDirectory() as tmp:
            ledger = Path(tmp) / 'requests.json'
            ledger.write_text(json.dumps({'records': [smc, scheduler, tracker]}))
            observed = subprocess.run([sys.executable, str(Path(p.__file__)), '--ledger', str(ledger),
                '--as-of', '2026-10-27T22:27:00Z', '--capacity', '0', '--read-status', 'success'],
                capture_output=True, text=True)
            self.assertEqual(observed.returncode, 0, observed.stdout + observed.stderr)
            result = json.loads(observed.stdout)
            self.assertEqual(result['state'], 'WORK_REQUIRED')
            self.assertEqual(result['claim_admission']['blocked_by'], ['LX-REQ-020', 'LX-REQ-027'])
            self.assertEqual(result['claim_candidates'], [])
            self.assertEqual([x['request_id'] for x in result['continue_existing']],
                             ['LX-REQ-005', 'LX-REQ-020', 'LX-REQ-027'])
            self.assertEqual([x['implementation_owner'] for x in result['recovery_actions']],
                             [scheduler['owner'], tracker['owner']])

    def test_unknown_active_scope_recovery_does_not_reopen_terminal_or_deferred_work(self):
        done = complete(record(1, owner_execution_state='ACTIVE_STAGE_REPORTED', priority_assessment=None))
        held = record(38, priority='DEFERRED', owner_execution_state='ACTIVE', priority_assessment=None)
        before = copy.deepcopy([done, held])
        result = self.plan(done, held)
        self.assertEqual([done, held], before)
        self.assertEqual(p.completion_errors(done), [])
        self.assertEqual(result['state'], 'WORK_REQUIRED')
        self.assertEqual(result['claim_admission']['state'], 'BLOCKED')
        self.assertEqual(result['claim_candidates'], [])
        self.assertEqual(result['continue_existing'], [])
        self.assertEqual(result['excluded'][0]['reason'], 'closed request')
        self.assertIn('deferral', result['excluded'][1]['reason'])
        self.assertEqual(len(result['recovery_actions']), 2)
        self.assertTrue(all(x['action_kind'] == 'READ_ONLY_OWNERSHIP_RECONCILIATION'
                            for x in result['recovery_actions']))

    def test_bare_or_contradictory_completion_blocks_dependents_only(self):
        for mutation in ('missing', 'negative', 'stale', 'criterion', 'delivery'):
            with self.subTest(mutation=mutation):
                done = complete(record(1))
                if mutation == 'missing':
                    del done['completion']
                elif mutation == 'negative':
                    done['evidence']['test']['state'] = 'NOT_RUN'
                elif mutation == 'stale':
                    done['evidence']['source']['revision'] = 'b' * 40
                elif mutation == 'criterion':
                    done['completion_criterion'] = 'Deliver an installable firmware package.'
                else:
                    done['completion']['delivery']['state'] = 'NOT_DONE'
                result = self.plan(done, record(2, dependencies=['LX-REQ-001']), record(3), capacity=3)
                self.assertEqual([r['request_id'] for r in result['claim_candidates']], ['LX-REQ-003'])
                self.assertTrue(p.completion_errors(done))

    def test_source_only_completion_leaves_unrequested_gates_not_run(self):
        done = complete(record(1))
        self.assertEqual(p.completion_errors(done), [])
        result = self.plan(done, record(2, dependencies=['LX-REQ-001']))
        self.assertEqual(result['claim_candidates'][0]['request_id'], 'LX-REQ-002')
        self.assertEqual(done['evidence']['hardware']['state'], 'NOT_RUN')
        self.assertEqual(done['evidence']['deployed']['state'], 'NOT_RUN')
        del done['owner']
        del done['next_action']
        self.assertEqual(self.plan(done)['state'], 'QUEUE_COMPLETE')

    def test_blocked_or_deferred_queue_stays_pending(self):
        for r in (record(1, status='BLOCKED_ACTION_REVIEW'), record(38, priority='DEFERRED')):
            result = self.plan(r)
            self.assertEqual(result['state'], 'PENDING')
            self.assertEqual(result['claim_candidates'], [])

    def test_reported_active_writer_is_not_released_by_a_terminal_label(self):
        done = complete(record(1, owner_execution_state='ACTIVE_STAGE_REPORTED'))
        conflict = record(2)
        conflict['priority_assessment']['work_areas'] = ['1']
        result = self.plan(done, conflict, record(3), capacity=3)
        self.assertEqual([r['request_id'] for r in result['claim_candidates']], ['LX-REQ-003'])

    def test_fill_verified_capacity_with_disjoint_candidates(self):
        result = self.plan(*(record(i) for i in range(1, 6)), capacity=4)
        self.assertEqual([r['request_id'] for r in result['claim_candidates']],
                         ['LX-REQ-001', 'LX-REQ-002', 'LX-REQ-003', 'LX-REQ-004'])
        self.assertEqual(result['lanes']['quick'], 'LX-REQ-001')
        overlapping = record(2)
        overlapping['priority_assessment']['work_areas'] = ['1']
        result = self.plan(record(1), overlapping, record(3), capacity=4)
        self.assertEqual([r['request_id'] for r in result['claim_candidates']],
                         ['LX-REQ-001', 'LX-REQ-003'])
        with self.assertRaises(ValueError):
            self.plan(record(1), capacity=True)

    def test_completion_validator_rejects_false_done(self):
        v_spec = importlib.util.spec_from_file_location('completion_validator',
            Path(__file__).resolve().parents[1] / 'coordination/requests/validate_requests.py')
        validator = importlib.util.module_from_spec(v_spec)
        v_spec.loader.exec_module(validator)
        done = complete(record(1))
        ledger = {'records': [done], 'scope': {'repositories': []},
                  'unclassified_preserved_branches': [], 'preserved_branch_classification_counts': {}}
        registry = {'initial_request_ids': ['LX-REQ-001'], 'initial_preserved_branch_heads': []}
        self.assertEqual(validator.validate(ledger, registry), [])
        del done['completion']
        self.assertTrue(any('completion' in e for e in validator.validate(ledger, registry)))

    def test_cli_default_cannot_treat_an_unread_queue_as_done(self):
        script = Path(__file__).resolve().parents[1] / 'coordination/requests/task_priority.py'
        with tempfile.TemporaryDirectory() as tmp:
            ledger = Path(tmp) / 'queue.json'
            ledger.write_text(json.dumps({'records': []}))
            args = [sys.executable, str(script), '--ledger', str(ledger), '--as-of', NOW]
            unknown = subprocess.run(args, capture_output=True, text=True)
            self.assertEqual(unknown.returncode, 2)
            self.assertEqual(json.loads(unknown.stdout)['state'], 'BLOCKED')
            observed = subprocess.run(args + ['--read-status', 'success'], capture_output=True, text=True)
            self.assertEqual(observed.returncode, 0)
            self.assertEqual(json.loads(observed.stdout)['state'], 'QUEUE_COMPLETE')


if __name__ == '__main__':
    unittest.main()
