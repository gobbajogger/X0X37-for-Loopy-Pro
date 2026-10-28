# SPDX-License-Identifier: GPL-3.0-only
"""Test-only proposed semantics. No wire commands, device backends or write methods.

All observations/capabilities are synthetic assertions made by tests, NOT verified
runtime receipts. This module is never imported by firmware or installer code.
"""
from dataclasses import dataclass


@dataclass(frozen=True)
class Capabilities:
    stable_unit_binding: bool = False
    consistent_raw_snapshot: bool = False
    exact_readback: bool = False
    durable_transaction_status: bool = False
    interruption_safe_commit: bool = False


# Current inspected protocol cannot enter adoption. This is not feature detection.
CURRENT = Capabilities()


@dataclass(frozen=True)
class Observation:
    unit: str
    model: str
    session: str
    challenge: str
    epoch: int
    size: int
    full_digest: str
    head_digest: str
    complete_ranges: bool = True
    consistent: bool = True


@dataclass(frozen=True)
class Pins:
    transaction: str
    package: str
    controller: str
    boot: str
    expected_head: str
    expected_full: str


class Refusal(ValueError):
    pass


class Model:
    """Admission/outcome model only; even hypothetical begin emits no operation."""
    def __init__(self, intent, model, caps=CURRENT):
        self.state = 'START'
        self.model = model
        self.caps = caps
        self.baseline = None
        self.pins = None
        self.challenges = set()
        self.current_session = None
        self.possible_mutation = False
        if model not in ('pro', 'elite'):
            self.refuse('MODEL')
        if intent == 'ordinary':
            self.state = 'EXISTING_ORDINARY_PATH'
        elif intent != 'adopt':
            self.refuse('INTENT')
        elif not (caps.stable_unit_binding and caps.consistent_raw_snapshot):
            self.refuse('UNSUPPORTED_ADOPTION')
        else:
            self.state = 'SNAPSHOT_REQUIRED'

    def refuse(self, reason):
        self.state = 'STOP_NO_AUTORETRY' if self.possible_mutation else 'REFUSED'
        raise Refusal(reason)

    def require_state(self, state):
        if self.state != state:
            self.refuse('STATE')

    def observe(self, item):
        if not item.unit or not item.session or not item.challenge or item.challenge in self.challenges:
            self.refuse('IDENTITY_OR_REPLAY')
        if item.model != self.model:
            self.refuse('MODEL')
        self.challenges.add(item.challenge)

    def snapshot(self, item, backup_retained):
        self.require_state('SNAPSHOT_REQUIRED')
        self.observe(item)
        if item.size != 0x100000 or not item.complete_ranges or not item.consistent:
            self.refuse('INCOMPLETE_SNAPSHOT')
        if not item.full_digest or not item.head_digest or not backup_retained:
            self.refuse('SNAPSHOT_OR_BACKUP')
        self.baseline = item
        self.current_session = item.session
        self.state = 'SNAPSHOT_VERIFIED'

    def prepare(self, pins, *, local_transform_pass, guard_empty, nonboot_equal,
                config_valid, key_model_geometry_match, controller_complete):
        self.require_state('SNAPSHOT_VERIFIED')
        if not all((local_transform_pass, guard_empty, nonboot_equal, config_valid,
                    key_model_geometry_match, controller_complete)):
            self.refuse('LOCAL_PRESERVATION')
        if not all((pins.transaction, pins.package, pins.controller, pins.boot, pins.expected_head, pins.expected_full)):
            self.refuse('PINS')
        if not (self.caps.exact_readback and self.caps.durable_transaction_status
                and self.caps.interruption_safe_commit):
            self.refuse('PROTOCOL_GAP')
        self.pins = pins
        self.state = 'PREPARED_MODEL_ONLY'

    def revalidate(self, item, pins):
        self.require_state('PREPARED_MODEL_ONLY')
        self.observe(item)
        old = self.baseline
        if (item.unit, item.model) != (old.unit, old.model):
            self.refuse('UNIT_CHANGED')
        if (item.session, item.epoch, item.full_digest, item.head_digest, item.size) != (
                old.session, old.epoch, old.full_digest, old.head_digest, old.size):
            self.refuse('STALE_INPUT')
        if not item.complete_ranges or not item.consistent or pins != self.pins:
            self.refuse('STALE_INPUT')
        self.state = 'REVALIDATED'

    def hypothetical_begin(self):
        self.require_state('REVALIDATED')
        self.possible_mutation = True
        self.state = 'VERIFY_REQUIRED'

    def interrupt(self):
        # Cancel/lost ACK/disconnect/timeout are deliberately indistinguishable
        # once a commit could have started. This does not implement cancellation.
        if self.possible_mutation:
            self.state = 'OUTCOME_UNKNOWN'
        elif self.state != 'EXISTING_ORDINARY_PATH':
            self.state = 'CANCELLED_BEFORE_MUTATION'

    def reconnect(self, item, transaction):
        self.require_state('OUTCOME_UNKNOWN')
        self.observe(item)
        if (item.unit, item.model) != (self.baseline.unit, self.baseline.model):
            self.refuse('UNIT_CHANGED')
        if transaction != self.pins.transaction:
            self.refuse('TRANSACTION_CHANGED')
        self.current_session = item.session
        self.state = 'RECONCILE_REQUIRED'

    def readback(self, *, observation, transaction, status, nonboot_equal):
        if self.state not in ('VERIFY_REQUIRED', 'RECONCILE_REQUIRED'):
            self.refuse('STATE')
        self.observe(observation)
        if (observation.unit, observation.model, observation.session) != (
                self.baseline.unit, self.baseline.model, self.current_session):
            self.refuse('UNIT_OR_SESSION_CHANGED')
        if observation.size != 0x100000 or not observation.complete_ranges or not observation.consistent:
            self.refuse('INCOMPLETE_READBACK')
        if transaction != self.pins.transaction:
            self.refuse('TRANSACTION_CHANGED')
        head = observation.head_digest
        full = observation.full_digest
        if status == 'not_started' and head == self.baseline.head_digest and full == self.baseline.full_digest and nonboot_equal:
            # A new capture/revalidation cycle is mandatory. Nothing is replayed.
            self.state = 'NEW_SNAPSHOT_REQUIRED'
        elif status == 'committed' and head == self.pins.expected_head and full == self.pins.expected_full and nonboot_equal:
            self.state = 'ADOPTION_READBACK_VERIFIED'
        else:
            self.refuse('OUTCOME_UNPROVEN')
        # Never END_TO_END_VERIFIED: real boot/guard/app/controller acceptance
        # is outside this model and has no implemented transport.
