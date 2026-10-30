#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Synthetic proposed protocol semantics; never open or emulate a device backend."""
from dataclasses import replace
import unittest
from contract_model import Capabilities, CURRENT, Model, Observation, Pins, Refusal

FUTURE_TEST_ONLY = Capabilities(True, True, True, True, True)
BASE = Observation('synthetic-unit-A', 'elite', 'session-A', 'nonce-1', 4,
                   0x100000, 'synthetic-full', 'synthetic-head')
PINS = Pins('synthetic-tx', 'synthetic-package', 'synthetic-controller',
            'synthetic-boot', 'synthetic-new-head', 'synthetic-new-full')
PROOF = dict(local_transform_pass=True, guard_empty=True, nonboot_equal=True,
             config_valid=True, key_model_geometry_match=True, controller_complete=True)


def prepared():
    m = Model('adopt', 'elite', FUTURE_TEST_ONLY)
    m.snapshot(BASE, True)
    m.prepare(PINS, **PROOF)
    return m


def begun():
    m = prepared()
    m.revalidate(replace(BASE, challenge='nonce-2'), PINS)
    m.hypothetical_begin()
    return m


class Contract(unittest.TestCase):
    def test_current_protocol_cannot_adopt(self):
        for model in ('pro', 'elite'):
            with self.subTest(model=model), self.assertRaisesRegex(Refusal, 'UNSUPPORTED_ADOPTION'):
                Model('adopt', model, CURRENT)

    def test_existing_ordinary_is_not_relabelled_adoption(self):
        m = Model('ordinary', 'elite', CURRENT)
        self.assertEqual(m.state, 'EXISTING_ORDINARY_PATH')
        self.assertFalse(m.possible_mutation)
        with self.assertRaisesRegex(Refusal, 'STATE'):
            m.hypothetical_begin()

    def test_unknown_intent_and_model_refused(self):
        for intent, model in [('adopt', 'fm1'), ('automatic', 'elite')]:
            with self.subTest(intent=intent), self.assertRaises(Refusal):
                Model(intent, model, FUTURE_TEST_ONLY)

    def test_partial_snapshot_and_inconsistent_capture_refused(self):
        for change in [dict(size=0x100000-1), dict(size=0x100001),
                       dict(complete_ranges=False), dict(consistent=False), dict(full_digest='')]:
            m = Model('adopt', 'elite', FUTURE_TEST_ONLY)
            with self.subTest(change=change), self.assertRaises(Refusal):
                m.snapshot(replace(BASE, **change), True)
            self.assertFalse(m.possible_mutation)

    def test_backup_not_retained_refused(self):
        m = Model('adopt', 'elite', FUTURE_TEST_ONLY)
        with self.assertRaisesRegex(Refusal, 'SNAPSHOT_OR_BACKUP'):
            m.snapshot(BASE, False)

    def test_missing_identity_and_wrong_model_refused(self):
        for change in [dict(unit=''), dict(model='pro'), dict(session=''), dict(challenge='')]:
            m = Model('adopt', 'elite', FUTURE_TEST_ONLY)
            with self.subTest(change=change), self.assertRaises(Refusal):
                m.snapshot(replace(BASE, **change), True)

    def test_preservation_calibration_config_key_and_controller_negatives(self):
        for field in PROOF:
            m = Model('adopt', 'elite', FUTURE_TEST_ONLY)
            m.snapshot(BASE, True)
            with self.subTest(field=field), self.assertRaisesRegex(Refusal, 'LOCAL_PRESERVATION'):
                m.prepare(PINS, **dict(PROOF, **{field: False}))
            self.assertFalse(m.possible_mutation)

    def test_readonly_capabilities_do_not_authorize_boot_commit(self):
        for field in ['exact_readback', 'durable_transaction_status', 'interruption_safe_commit']:
            caps = replace(FUTURE_TEST_ONLY, **{field: False})
            m = Model('adopt', 'elite', caps)
            m.snapshot(BASE, True)
            with self.subTest(field=field), self.assertRaisesRegex(Refusal, 'PROTOCOL_GAP'):
                m.prepare(PINS, **PROOF)

    def test_unit_swap_same_product_and_key_claim_cannot_revalidate(self):
        m = prepared()
        with self.assertRaisesRegex(Refusal, 'UNIT_CHANGED'):
            m.revalidate(replace(BASE, unit='synthetic-unit-B', challenge='nonce-2'), PINS)

    def test_replayed_nonce_refused(self):
        m = prepared()
        with self.assertRaisesRegex(Refusal, 'IDENTITY_OR_REPLAY'):
            m.revalidate(BASE, PINS)

    def test_stale_epoch_hash_head_session_or_coverage_refused(self):
        for change in [dict(epoch=5), dict(full_digest='changed'), dict(head_digest='changed'),
                       dict(session='rebooted'), dict(size=0x80000), dict(consistent=False),
                       dict(complete_ranges=False)]:
            m = prepared()
            with self.subTest(change=change), self.assertRaisesRegex(Refusal, 'STALE_INPUT'):
                m.revalidate(replace(BASE, challenge='nonce-2', **change), PINS)

    def test_every_transaction_pin_change_invalidates_preparation(self):
        for field in PINS.__dataclass_fields__:
            m = prepared()
            with self.subTest(field=field), self.assertRaisesRegex(Refusal, 'STALE_INPUT'):
                m.revalidate(replace(BASE, challenge='nonce-2'), replace(PINS, **{field: 'changed'}))

    def test_precommit_cancel_never_plans_a_write(self):
        m = prepared(); m.interrupt()
        self.assertEqual(m.state, 'CANCELLED_BEFORE_MUTATION')
        self.assertFalse(m.possible_mutation)
        with self.assertRaises(Refusal):
            m.hypothetical_begin()

    def test_postbegin_cancel_disconnect_or_lost_ack_is_unknown(self):
        for cause in ['cancel', 'disconnect', 'timeout', 'lost_ack']:
            with self.subTest(cause=cause):
                m = begun(); m.interrupt()
                self.assertEqual(m.state, 'OUTCOME_UNKNOWN')
                with self.assertRaises(Refusal):
                    m.hypothetical_begin()

    def test_reconnect_requires_same_unit_and_durable_transaction(self):
        for unit, tx in [('synthetic-unit-B', PINS.transaction), (BASE.unit, 'foreign-tx')]:
            m = begun(); m.interrupt()
            with self.subTest(unit=unit, tx=tx), self.assertRaises(Refusal):
                m.reconnect(replace(BASE, unit=unit, challenge='nonce-3', session='new-session'), tx)
            self.assertEqual(m.state, 'STOP_NO_AUTORETRY')

    def test_reconnect_does_not_mean_success_or_resume_begin(self):
        m = begun(); m.interrupt()
        m.reconnect(replace(BASE, challenge='nonce-3', session='new-session'), PINS.transaction)
        self.assertEqual(m.state, 'RECONCILE_REQUIRED')
        with self.assertRaises(Refusal):
            m.hypothetical_begin()

    def test_partial_status_wrong_head_or_nonboot_changes_stop(self):
        for status, head, same in [('partial', PINS.expected_head, True),
                                  ('missing', PINS.expected_head, True),
                                  ('committed', 'wrong-head', True),
                                  ('committed', PINS.expected_head, False)]:
            m = begun()
            with self.subTest(status=status, head=head, same=same), self.assertRaises(Refusal):
                m.readback(observation=replace(BASE, challenge='nonce-3', head_digest=head, full_digest=PINS.expected_full), transaction=PINS.transaction, status=status, nonboot_equal=same)
            self.assertEqual(m.state, 'STOP_NO_AUTORETRY')

    def test_not_started_requires_new_capture_not_old_retry(self):
        m = begun(); m.interrupt()
        m.reconnect(replace(BASE, challenge='nonce-3', session='new-session'), PINS.transaction)
        m.readback(observation=replace(BASE, challenge='nonce-4', session='new-session'), transaction=PINS.transaction, status='not_started', nonboot_equal=True)
        self.assertEqual(m.state, 'NEW_SNAPSHOT_REQUIRED')
        with self.assertRaises(Refusal):
            m.hypothetical_begin()

    def test_readback_rejects_unit_swap_replay_partial_and_wrong_full_digest(self):
        for change in [dict(unit='synthetic-unit-B'), dict(challenge='nonce-2'),
                       dict(session='unbound-session'), dict(size=0x80000),
                       dict(consistent=False), dict(full_digest='wrong-full')]:
            m = begun()
            item = replace(BASE, challenge='nonce-3', head_digest=PINS.expected_head,
                           full_digest=PINS.expected_full)
            with self.subTest(change=change), self.assertRaises(Refusal):
                m.readback(observation=replace(item, **change), transaction=PINS.transaction,
                           status='committed', nonboot_equal=True)

    def test_proven_model_readback_is_not_end_to_end_hardware_success(self):
        m = begun()
        m.readback(observation=replace(BASE, challenge='nonce-3', head_digest=PINS.expected_head, full_digest=PINS.expected_full), transaction=PINS.transaction, status='committed', nonboot_equal=True)
        self.assertEqual(m.state, 'ADOPTION_READBACK_VERIFIED')
        self.assertNotIn('END_TO_END', m.state)


if __name__ == '__main__':
    unittest.main(verbosity=2)
