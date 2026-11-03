"""Opt-in writer-boundary candidate. No production authenticator or store is wired.

Only the actual sole writer may supply the trusted ports. A store must commit the
entire bundle with an atomic expected-revision check; a verifier must independently
authenticate and authorize the exact binding. Neither a digest nor this module
establishes authenticity, authority, liveness or production enforcement.
"""
from copy import deepcopy
from dataclasses import dataclass
import hashlib
import json
from typing import Protocol

if __package__:
    from . import task_priority as priority
else:
    import task_priority as priority


class Rejected(ValueError):
    """No mutation is authorized by this candidate."""


def digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, ensure_ascii=False,
                                    separators=(',', ':'), allow_nan=False).encode()).hexdigest()


@dataclass(frozen=True)
class VerifiedEvidence:
    binding: dict
    principal_id: str
    source_ref: str


class EvidenceVerifier(Protocol):
    def verify(self, evidence_id: str, binding: dict) -> VerifiedEvidence:
        """Fetch trusted proof outside the command; raise unless exact scope is authorized.

        Authenticate the source/principal and required reviewer independence,
        freshness, revocations, completion proof, capacity and action authority.
        A caller-supplied receipt or `human_origin_verified` boolean is not proof.
        """
        ...


class StateStore(Protocol):
    def read(self) -> tuple[str, dict]:
        """Return a complete, authenticated, strongly consistent revision and bundle."""
        ...

    def compare_and_swap(self, expected_revision: str, state: dict) -> str:
        """Atomically compare revision and persist ALL state or reject without writes.

        State includes ledger, checkpoint, claims, accepted completions and replay
        markers. Exceptions may mean an uncertain commit: reconcile, never retry
        automatically. Revision identities must never repeat (no ABA).
        """
        ...


def _text(value):
    return isinstance(value, str) and bool(value.strip())


def _areas(record):
    areas = record.get('priority_assessment', {}).get('work_areas')
    if (not isinstance(areas, list) or not areas or len(set(areas)) != len(areas)
            or any(not _text(a) or a.startswith('/') or a.endswith('/')
                   or any(p in ('', '.', '..') for p in a.split('/'))
                   or '\\' in a or '*' in a for a in areas)):
        raise Rejected('unknown or noncanonical work scope')
    return sorted(areas)


def _overlap(left, right):
    return any(a == b or a.startswith(b + '/') or b.startswith(a + '/')
               for a in left for b in right)


class WriterBoundary:
    """A testable integration seam, not a role assignment or a default executor."""
    def __init__(self, store: StateStore, verifier: EvidenceVerifier, *, writer_id: str):
        if (not _text(writer_id) or not callable(getattr(store, 'read', None))
                or not callable(getattr(store, 'compare_and_swap', None))
                or not callable(getattr(verifier, 'verify', None))):
            raise Rejected('trusted store, independent verifier and assigned writer required')
        self.store, self.verifier, self.writer_id = store, verifier, writer_id

    def _prepare(self, command, inputs):
        command, inputs = deepcopy(command), deepcopy(inputs)
        required = {'schema', 'command_id', 'evidence_id', 'kind', 'request_id',
                    'expected_revision', 'inputs_hash', 'payload'}
        if (set(command) != required or command['schema'] != 1
                or command['kind'] not in ('complete', 'claim')
                or any(not _text(command[k]) for k in required - {'schema', 'payload'})
                or not isinstance(command['payload'], dict)):
            raise Rejected('invalid bounded command')
        if command['inputs_hash'] != digest(inputs):
            raise Rejected('changed input identity')
        revision, before = self.store.read()
        if revision != command['expected_revision']:
            raise Rejected('stale revision')
        required_state = {'schema', 'ledger', 'checkpoint', 'claims',
                          'accepted_completions', 'used_evidence', 'used_commands'}
        if (set(before) != required_state or before['schema'] != 1
                or any(not isinstance(before[k], dict) for k in required_state - {'schema', 'used_evidence'})
                or not isinstance(before['used_evidence'], list)):
            raise Rejected('complete trusted bundle required; no implicit bootstrap')
        if (command['command_id'] in before['used_commands']
                or command['evidence_id'] in before['used_evidence']):
            raise Rejected('replay of command or evidence')
        after = deepcopy(before)
        records = after['ledger']['records']
        by_id = {r['request_id']: r for r in records}
        if len(by_id) != len(records) or command['request_id'] not in by_id:
            raise Rejected('unknown or duplicate request')
        rid = command['request_id']
        record = by_id[rid]
        scope = _areas(record)
        # Consistent JSON alone never releases a dependency. Legacy terminal
        # records need a separately authenticated acceptance in this trusted store.
        completed = {r['request_id'] for r in records if r.get('status') in priority.COMPLETED
                     and not priority.completion_errors(r)
                     and after['accepted_completions'].get(r['request_id']) == digest(r)}
        payload = command['payload']
        previous = deepcopy(record)
        if command['kind'] == 'complete':
            if set(payload) != {'evidence', 'completion'} or priority.deferred(record):
                raise Rejected('completion payload or owner restriction')
            if record.get('status') in ('CANCELLED', 'SUPERSEDED'):
                raise Rejected('closed request cannot be completed')
            record.update(deepcopy(payload), status='COMPLETED', owner_execution_state='COMPLETED')
            errors = priority.completion_errors(record)
            if errors:
                raise Rejected('; '.join(errors))
            after['claims'].pop(rid, None)
        else:
            if set(payload) != {'owner'} or not _text(payload['owner']) or payload['owner'] == 'unassigned':
                raise Rejected('claim needs one actual accountable owner')
            if (record.get('status') in priority.CLOSED or priority.deferred(record)
                    or str(record.get('status', '')).startswith('BLOCKED')
                    or priority.existing_commitment(record, completed)):
                raise Rejected('request closed, blocked, deferred or already owned')
            if any(dep not in completed for dep in record.get('dependencies', [])):
                raise Rejected('dependency lacks authenticated completion')
            assessment = record.get('priority_assessment')
            if (priority.validate_assessment(assessment) or assessment['readiness'] != 'ready'
                    or assessment['category'] == 'unassessed'):
                raise Rejected('source-backed readiness assessment required')
            occupied = {}
            for other in records:
                if priority.existing_commitment(other, completed):
                    occupied[other['request_id']] = _areas(other)
            for other_id, claim in after['claims'].items():
                if other_id not in by_id or claim != {
                        'owner': by_id[other_id].get('owner'), 'work_areas': _areas(by_id[other_id])}:
                    raise Rejected('claim and ledger ownership disagree')
                occupied[other_id] = claim['work_areas']
            if any(_overlap(scope, areas) for areas in occupied.values()):
                raise Rejected('overlapping claim or retained scope')
            capacity = inputs.get('capacity')
            if type(capacity) is not int or capacity <= len(occupied):
                raise Rejected('no verified admission capacity')
            record.update(owner=payload['owner'], owner_execution_state='ACTIVE', status='ACTIVE')
            after['claims'][rid] = {'owner': payload['owner'], 'work_areas': scope}
        observed_at = inputs.get('as_of')
        if not _text(observed_at):
            raise Rejected('writer observation time required')
        priority.timestamp(observed_at)
        record.setdefault('history', []).append({
            'observed_at': observed_at,
            'previous': {key: value for key, value in previous.items() if key != 'history'}})
        record['updated_at'] = observed_at
        if command['kind'] == 'complete':
            after['accepted_completions'][rid] = digest(record)
        receipt = {'request_id': rid, 'command_hash': digest(command),
                   'evidence_id': command['evidence_id'], 'kind': command['kind']}
        after['used_commands'][command['command_id']] = receipt
        after['used_evidence'].append(command['evidence_id'])
        after['checkpoint'].setdefault('writer_boundary', {})[command['command_id']] = deepcopy(receipt)
        binding = {'schema': 1, 'writer_id': self.writer_id, 'request_id': rid,
                   'request_hash': digest(previous), 'scope_hash': digest(scope),
                   'inputs_hash': digest(inputs), 'expected_revision': revision,
                   'command_hash': digest(command), 'before_hash': digest(before), 'after_hash': digest(after)}
        return revision, after, binding

    def preview(self, command, inputs):
        """Return a binding for review; no authentication or mutation occurs."""
        return self._prepare(command, inputs)[2]

    def apply(self, command, inputs):
        revision, after, binding = self._prepare(command, inputs)
        proof = self.verifier.verify(command['evidence_id'], deepcopy(binding))
        if (not isinstance(proof, VerifiedEvidence) or proof.binding != binding
                or not _text(proof.principal_id) or not _text(proof.source_ref)):
            raise Rejected('independently authenticated exact binding required')
        # This port call is the mutation boundary. A preceding read/check is not
        # a lock. The backend must compare and commit atomically, without retry.
        return self.store.compare_and_swap(revision, after)

    def lookup(self, command):
        """Reconcile a lost response via a fresh strongly consistent trusted read."""
        _, state = self.store.read()
        receipt = state['used_commands'].get(command['command_id'])
        if receipt and receipt['command_hash'] != digest(command):
            raise Rejected('command ID collision')
        return deepcopy(receipt)
