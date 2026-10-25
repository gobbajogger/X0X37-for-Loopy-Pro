#!/usr/bin/env python3
"""Observable recommendation order, starvation, authority and ownership checks."""
import copy
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location('task_priority', Path(__file__).resolve().parents[1] / 'coordination/requests/task_priority.py')
p = importlib.util.module_from_spec(spec)
spec.loader.exec_module(p)
NOW = '2026-10-25T12:00:00Z'


def record(n, category='ux_repair', effort='small', **changes):
    r = {'request_id': f'LX-REQ-{n:03d}', 'status': 'PLANNED', 'owner': 'existing owner',
         'next_action': 'Run bounded source/test review; retain safety gates.', 'dependencies': [],
         'updated_at': '2026-10-24T12:00:00Z', 'priority_assessment': {
             'category': category, 'severity': 'moderate', 'reach': 'affected workflow',
             'confidence': 'source_verified', 'effort': effort, 'readiness': 'ready',
             'rationale': 'source finding', 'evidence_refs': ['fixture:source'], 'work_areas': [str(n)]}}
    r.update(changes)
    return r


class PriorityTests(unittest.TestCase):
    def plan(self, *records, capacity=2, **kw):
        return p.plan({'records': list(records)}, NOW, capacity, **kw)

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
        dep = record(2, status='COMPLETED')
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
        result = p.plan(ledger, NOW)
        self.assertEqual(ledger, before)
        self.assertEqual(result, p.plan(ledger, NOW))
        self.assertEqual(result['continue_existing'][0]['implementation_owner'], 'existing owner')
        self.assertIsNone(result['lanes']['quick'])

    def test_no_capacity_or_independence_no_new_lane(self):
        self.assertEqual(self.plan(record(1), capacity=0)['lanes'], {'quick': None, 'complex': None})
        self.assertEqual(p.plan({'records': [record(1)]}, NOW)['lanes'], {'quick': None, 'complex': None})
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
        with self.assertRaises(ValueError):
            self.plan(r)
        with self.assertRaises(ValueError):
            self.plan(record(1), capacity=-1)
        with self.assertRaises(ValueError):
            p.plan({'records': [record(1)]}, '2026-10-25')


if __name__ == '__main__':
    unittest.main()
