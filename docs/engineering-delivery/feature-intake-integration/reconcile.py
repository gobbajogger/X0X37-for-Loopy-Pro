#!/usr/bin/env python3
"""Offline intake reconciliation evidence; never authenticates or accepts state.

Historical replay uses the unchanged adapter's explicitly synthetic fixtures and
the exact historical ledger. A supplied observation/checkpoint pair can be
compared at capacity zero, but authenticity remains the existing writer's job.
No network, persistence, worker dispatch or acceptance operation is performed.
"""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'coordination/requests'))
import feature_intake as intake
import github_observation as adapter

HISTORICAL_COMMIT = '4a35a6cdaa0f112db6b3f577542138afd170c20d'
LEDGER_SHA256 = 'ea7bae9470f4197d1d907975b877dfcb28005ddd5968b8d163248334647afdca'


def dry_run(observation, ledger, checkpoint):
    """Refuse missing continuity. Even complete caller input is not authenticated."""
    if checkpoint is None:
        return {'state': 'BLOCKED', 'reason': 'authenticated accepted checkpoint required; no bootstrap',
                'claims': [], 'current_acceptance': False}
    before = copy.deepcopy((observation, ledger, checkpoint))
    first = intake.evaluate(observation, ledger, checkpoint, capacity=0)
    second = intake.evaluate(observation, ledger, checkpoint, capacity=0)
    if first != second or (observation, ledger, checkpoint) != before or first['claims']:
        raise ValueError('nondeterministic, mutating or nonzero-capacity dry-run')
    return {'state': first['state'], 'current_acceptance': False,
            'authenticity': 'not established by this harness', 'claims': [],
            'deterministic': True, 'inputs_unchanged': True, 'proposal': first}


def historical_replay(ledger):
    """Replay synthetic authority only; no returned checkpoint is accepted state."""
    spec = importlib.util.spec_from_file_location('observation_fixtures', ROOT / 'tests/github_observation_test.py')
    fixtures = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(fixtures)
    issue, context = fixtures.fixture()
    checkpoint = {'schema': 1}  # Explicit synthetic fixture, never a real bootstrap.
    inputs_before = copy.deepcopy((ledger, context, checkpoint))
    checks = {}

    def observe(item=issue, ctx=context, timeline=None):
        replay = fixtures.Replay(item, timeline=timeline)
        if item['number'] != 101:
            replay.responses = {k.replace('/issues/101', '/issues/' + str(item['number'])): v
                                for k, v in replay.responses.items()}
        result = adapter.observe(ctx, replay, now=lambda: '2026-11-14T00:01:00Z')
        if result['read_status'] != 'success':
            raise ValueError('synthetic adapter observation failed')
        return result

    observation = observe()
    zero = dry_run(observation, ledger, checkpoint)
    checks['capacity_zero'] = zero['claims'] == [] and zero['inputs_unchanged']
    first = intake.evaluate(observation, ledger, checkpoint, capacity=1)
    repeated = intake.evaluate(observe(), ledger, checkpoint, capacity=1)
    rid = first['claim']['request_ids'][0]
    checks['deterministic_identity'] = first == repeated and rid == 'LX-REQ-039'

    # Exercise the historical one-issue/multiple-request mapping on fixture #2.
    existing_issue = dict(issue, number=2, url=adapter.ORIGIN + intake.endpoint(2))
    existing = intake.evaluate(observe(existing_issue), ledger, checkpoint, capacity=1)
    key = f'{intake.REPOSITORY_ID}:{issue["id"]}'
    expected = ['LX-REQ-001', 'LX-REQ-002', 'LX-REQ-028']
    checks['multiple_existing_requests'] = existing['items'][0]['request_ids'] == expected
    checks['existing_not_reenqueued'] = existing['claims'] == []

    saved = first['proposed_checkpoint']  # Still synthetic; not written anywhere.
    no_new_approval = copy.deepcopy(context)
    no_new_approval['approval_receipts'] = []
    closed = dict(issue, state='closed')
    timeline = [{'event': 'closed', 'id': 11, 'created_at': '2026-11-14T00:00:01Z'}]
    cancelled = intake.evaluate(observe(closed, no_new_approval, timeline), ledger, saved, capacity=1)
    retained = cancelled['proposed_checkpoint']
    boundary = retained['cancellations'][key]
    checks['cancellation_retained'] = (cancelled['items'][0]['state'] == 'CANCELLED'
                                      and bool(boundary['ref']) and retained['approvals'][key]['cancelled'])
    reopened_timeline = timeline + [{'event': 'reopened', 'id': 12, 'created_at': '2026-11-14T00:00:02Z'}]
    reopened = intake.evaluate(observe(issue, no_new_approval, reopened_timeline), ledger, retained, capacity=1)
    checks['reopen_requires_new_authority'] = (not reopened['claims']
        and reopened['proposed_checkpoint']['cancellations'][key] == boundary)
    reauthorized_context = copy.deepcopy(context)
    reauthorized_context['approval_receipts'][0]['reopens_after'] = boundary['ref']
    authorized = intake.evaluate(observe(issue, reauthorized_context, reopened_timeline), ledger, retained, capacity=1)
    checks['explicit_reauthorization'] = (authorized['claim']['request_ids'] == [rid]
        and authorized['proposed_checkpoint']['cancellations'][key] == boundary)

    reserved = copy.deepcopy(ledger)
    reserved.setdefault('tombstones', []).append({'request_id': 'LX-REQ-100', 'superseded_by': ['LX-REQ-038']})
    tombstone_plan = intake.evaluate(observation, reserved, checkpoint, capacity=1)
    checks['tombstone_reserved'] = tombstone_plan['claim']['request_ids'] == ['LX-REQ-101']
    missing = copy.deepcopy(saved)
    missing.setdefault('issue_map', {})[f'{intake.REPOSITORY_ID}:999999'] = ['LX-REQ-038']
    missing_plan = intake.evaluate(observation, ledger, missing, capacity=0)
    checks['missing_tracked_issue_blocks'] = (missing_plan['state'] == 'BLOCKED'
        and 'tracked issue absent' in missing_plan['reason'] and not missing_plan['claims']
        and 'proposed_checkpoint' not in missing_plan)
    checks['inputs_unchanged'] = (ledger, context, checkpoint) == inputs_before
    if not all(checks.values()):
        raise ValueError('historical reconciliation failed: ' + ', '.join(k for k, ok in checks.items() if not ok))
    return {'schema': 1, 'state': 'PASS_HISTORICAL_REPLAY_ONLY', 'current_acceptance': False,
            'historical_commit': HISTORICAL_COMMIT, 'synthetic_authority_and_checkpoint': True,
            'ledger_canonical_digest': intake.digest(ledger), 'new_request_id': rid,
            'checks': checks, 'observation_digest': intake.digest(observation),
            'capacity_zero_proposal_digest': intake.digest(zero['proposal']),
            'cancellation_boundary': boundary, 'existing_request_ids': expected,
            'remaining': 'Existing writer must supply authenticated current ledger/checkpoint and authority/context receipts.'}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ledger', required=True, type=Path)
    parser.add_argument('--historical-replay', action='store_true')
    parser.add_argument('--observation', type=Path)
    parser.add_argument('--checkpoint', type=Path)
    args = parser.parse_args(argv)
    if args.historical_replay and (args.observation or args.checkpoint):
        parser.error('historical replay cannot consume operational continuity')
    if not args.historical_replay and not args.observation:
        parser.error('--observation required for capacity-zero comparison')
    paths = [p for p in (args.ledger, args.observation, args.checkpoint) if p]
    try:
        original = {p: p.read_bytes() for p in paths}
        identities = {str(p): hashlib.sha256(b).hexdigest() for p, b in original.items()}
        ledger = json.loads(original[args.ledger])
        if args.historical_replay:
            if identities[str(args.ledger)] != LEDGER_SHA256:
                raise ValueError('historical ledger bytes do not match pinned input')
            result = historical_replay(ledger)
        else:
            result = dry_run(json.loads(original[args.observation]), ledger,
                             json.loads(original[args.checkpoint]) if args.checkpoint else None)
        if any(p.read_bytes() != b for p, b in original.items()):
            raise ValueError('input file bytes changed during qualification')
        result['input_file_sha256'] = identities
    except (OSError, ValueError, KeyError, TypeError) as error:
        result = {'state': 'BLOCKED', 'claims': [], 'current_acceptance': False,
                  'reason': str(error)}
    print(json.dumps(result, indent=2, sort_keys=True))
    return 2 if result['state'] == 'BLOCKED' else 0


if __name__ == '__main__':
    raise SystemExit(main())
