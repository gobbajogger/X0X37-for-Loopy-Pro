#!/usr/bin/env python3
"""Deterministic offline recommendations for the existing sole request coordinator."""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path

CATEGORIES = ('safety_integrity', 'ux_repair', 'small_improvement', 'feature_research', 'unassessed')
SEVERITIES = ('critical', 'major', 'moderate', 'minor', 'unknown')
CONFIDENCE = ('source_verified', 'reported', 'unknown')
CLOSED = {'COMPLETED', 'CANCELLED', 'SUPERSEDED'}
ACTIVE = {'ACTIVE', 'ACTIVE_STAGE_REPORTED'}
STARVATION_DAYS = 14


def timestamp(value):
    result = datetime.fromisoformat(value.replace('Z', '+00:00'))
    if result.tzinfo is None:
        raise ValueError('timestamp needs timezone')
    return result.astimezone(timezone.utc)


def validate_assessment(a):
    if not isinstance(a, dict):
        return ['priority_assessment must be an object']
    errors = []
    for key, choices in {'category': CATEGORIES, 'severity': SEVERITIES,
                         'confidence': CONFIDENCE, 'effort': ('small', 'complex', 'unknown'),
                         'readiness': ('ready', 'blocked', 'unknown')}.items():
        if a.get(key) not in choices:
            errors.append('invalid priority_assessment.' + key)
    for key in ('reach', 'rationale'):
        if not isinstance(a.get(key), str) or not a[key].strip():
            errors.append('missing priority_assessment.' + key)
    for key in ('evidence_refs', 'work_areas'):
        if not isinstance(a.get(key), list) or not a[key] or any(
                not isinstance(x, str) or not x.strip() for x in a[key]):
            errors.append('missing priority_assessment.' + key)
    if 'committed_at' in a:
        try:
            timestamp(a['committed_at'])
        except (ValueError, TypeError, AttributeError):
            errors.append('invalid priority_assessment.committed_at')
    return errors


def deferred(r):
    return (r.get('priority') == 'DEFERRED' or 'DEFERRED' in r.get('status', '')
            or r.get('implementation_authorized') is False or r.get('intake_eligible') is False)


def first_seen(r):
    # Preserve age across feedback edits; earliest retained observation wins.
    dates = [r.get('created_at'), r.get('updated_at')]
    for h in r.get('history', []):
        dates.extend((h.get('observed_at'), h.get('previous', {}).get('updated_at')))
    parsed = [timestamp(d) for d in dates if d]
    return min(parsed) if parsed else None


def plan(ledger, as_of, capacity=0, busy_areas=()):
    """No writes/claims/delegation; caller authenticates readiness, evidence and liveness."""
    now = timestamp(as_of)
    if capacity < 0:
        raise ValueError('capacity must be nonnegative free worker slots')
    records = ledger['records']
    by_id = {r['request_id']: r for r in records}
    if len(by_id) != len(records):
        raise ValueError('duplicate request IDs')
    result = {'schema': 1, 'as_of': as_of, 'delivery_order': [], 'continue_existing': [],
              'excluded': [], 'lanes': {'quick': None, 'complex': None},
              'limits': 'Recommendations only; preserve claims, verify current writers/areas and all QA/safety/action gates.'}
    rows = []
    occupied = set(busy_areas)
    for r in records:
        a = r.get('priority_assessment')
        if a is not None:
            errors = validate_assessment(a)
            if errors:
                raise ValueError(r['request_id'] + ': ' + '; '.join(errors))
        if r.get('status') not in CLOSED and not deferred(r) and (
                r.get('owner_execution_state') in ACTIVE or r.get('status') == 'ACTIVE'):
            occupied.update((a or {}).get('work_areas', []))
    for r in records:
        rid = r['request_id']
        a = r.get('priority_assessment', {})
        reason = None
        if deferred(r):
            reason = 'explicit owner deferral/authorization restriction; only owner reopening can change this'
        elif r.get('status') in CLOSED:
            reason = 'closed request'
        elif r.get('owner_execution_state') in ACTIVE or r.get('status') == 'ACTIVE':
            result['continue_existing'].append({'request_id': rid, 'implementation_owner': r['owner'],
                'next_action': r['next_action'], 'reason': 'existing commitment; verify liveness, never auto-reassign'})
            continue
        else:
            deps = r.get('dependencies', [])
            if any(d not in by_id or by_id[d].get('status') != 'COMPLETED' for d in deps):
                reason = 'blocked dependency: ' + ', '.join(d for d in deps if d not in by_id or by_id[d].get('status') != 'COMPLETED')
            elif a.get('readiness') != 'ready':
                reason = 'dependency/action readiness ' + a.get('readiness', 'unknown')
            elif a.get('category', 'unassessed') == 'unassessed':
                reason = 'needs source-backed category assessment'
        if reason:
            result['excluded'].append({'request_id': rid, 'reason': reason, 'next_action': r['next_action'],
                                       'implementation_owner': r['owner']})
            continue
        seen = first_seen(r)
        age = max(0, (now - seen).days) if seen else None
        committed = a.get('committed_at')
        if committed and timestamp(committed) > now:
            raise ValueError(rid + ': future commitment')
        row = {'request_id': rid, 'category': a['category'], 'severity': a['severity'],
               'reach': a['reach'], 'confidence': a['confidence'], 'effort': a['effort'],
               'dependency_readiness': 'ready', 'age_days': age, 'committed_at': committed,
               'rationale': a['rationale'], 'evidence_refs': a['evidence_refs'],
               'suggested_next_action': r['next_action'], 'implementation_owner': r['owner'],
               'work_areas': a['work_areas']}
        rows.append(row)
    def key(r):
        return (CATEGORIES.index(r['category']), SEVERITIES.index(r['severity']),
                not bool(r['committed_at']), -(r['age_days'] or 0),
                CONFIDENCE.index(r['confidence']), r['request_id'])
    rows.sort(key=key)
    result['delivery_order'] = rows
    # Ready safety work gets first use of a slot, including overlapping areas.
    urgent = next((r for r in rows if r['category'] == 'safety_integrity'
                   and r['effort'] in ('small', 'complex')
                   and not occupied.intersection(r['work_areas'])), None)
    if capacity and urgent:
        lane = 'quick' if urgent['effort'] == 'small' else 'complex'
        result['lanes'][lane] = urgent['request_id']
        occupied.update(urgent['work_areas'])
        capacity -= 1
    # Reserve a complex slot so a stream of ordinary quick repairs cannot starve it.
    # The separate delivery order always retains safety/UX precedence.
    complex_rows = [r for r in rows if r['effort'] == 'complex' and not occupied.intersection(r['work_areas'])]
    complex_rows.sort(key=lambda r: (r['category'] != 'safety_integrity',
        not bool(r['committed_at']), (r['age_days'] or 0) < STARVATION_DAYS, key(r)))
    if capacity and complex_rows and result['lanes']['complex'] is None:
        chosen = complex_rows[0]
        result['lanes']['complex'] = chosen['request_id']
        occupied.update(chosen['work_areas'])
        capacity -= 1
    if capacity and result['lanes']['quick'] is None:
        quick = next((r for r in rows if r['effort'] == 'small'
                      and not occupied.intersection(r['work_areas'])), None)
        if quick:
            result['lanes']['quick'] = quick['request_id']
    result['lane_note'] = ('A lane is a bounded next-stage recommendation, not a claim or release order. '
        'Missing effort/area/liveness evidence needs assessment; never guess capacity or preempt a writer. '
        'Age/commitment protects complex progress within its reserved independent lane, never waives gates.')
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--ledger', type=Path, default=Path(__file__).with_name('requests.json'))
    p.add_argument('--as-of', required=True, help='explicit timezone-aware observation time')
    p.add_argument('--capacity', type=int, default=0, help='verified free worker slots; default none')
    p.add_argument('--busy-area', action='append', default=[], help='current occupied work area; repeatable')
    args = p.parse_args()
    try:
        result = plan(json.loads(args.ledger.read_text()), args.as_of, args.capacity, args.busy_area)
    except (OSError, ValueError, KeyError, TypeError) as e:
        print(json.dumps({'schema': 1, 'state': 'BLOCKED', 'reason': str(e)}, sort_keys=True))
        return 2
    print(json.dumps(result, sort_keys=True, indent=2, ensure_ascii=False))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
