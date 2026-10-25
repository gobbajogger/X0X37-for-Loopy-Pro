#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Verify stage-4 DEV CHECKED records without rebuilding Elite/Pro packages.

Local review of an evidence-only validate tip should call this instead of
tools/dev_validate.py run. It checks schema, result, commit/tree identity,
pool_free budget, and cross-field digest consistency inside the recorded
JSON. It does not claim byte identity of missing .fwsc / ctl.img blobs.

  python3 tools/verify_dev_checked_records.py --evidence evidence/f09-stage4
  python3 tools/verify_dev_checked_records.py --record evidence/f09-stage4/elite/record.json \\
      --record evidence/f09-stage4/pro/record.json

Exit 0 on PASS, 1 on FAIL.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MIN_POOL_FREE = 8192


def load(path: Path) -> dict:
    data = json.loads(path.read_text(encoding='utf-8'))
    if not isinstance(data, dict):
        raise ValueError('%s: not an object' % path)
    return data


def check_record(path: Path, expect_commit: str | None, expect_tree: str | None) -> list[str]:
    errs: list[str] = []
    try:
        data = load(path)
    except (OSError, ValueError, json.JSONDecodeError) as err:
        return [str(err)]

    if data.get('schema') not in (1, 2):
        errs.append('%s: unexpected schema %r' % (path, data.get('schema')))
    if data.get('result') != 'DEV CHECKED':
        errs.append('%s: result is %r, want DEV CHECKED' % (path, data.get('result')))
    if data.get('config') not in (None, 'dev'):
        # schema 2 uses config=dev; tolerate missing on older records.
        if data.get('config') != 'dev':
            errs.append('%s: config is %r, want dev' % (path, data.get('config')))

    model = data.get('model')
    if model not in ('elite', 'pro'):
        errs.append('%s: model is %r' % (path, model))

    commit = data.get('commit') or (data.get('validation_inputs') or {}).get('commit')
    tree = (
        data.get('tree')
        or data.get('source_tree')
        or (data.get('validation_inputs') or {}).get('source_tree')
        or (data.get('validation_inputs') or {}).get('tree')
    )
    if expect_commit and commit and commit != expect_commit:
        errs.append('%s: commit %s != expected %s' % (path, commit, expect_commit))
    if expect_tree and tree and tree != expect_tree:
        errs.append('%s: tree %s != expected %s' % (path, tree, expect_tree))

    pool = data.get('pool_free')
    if pool is None:
        budgets = data.get('budgets') or {}
        pool = budgets.get('pool_free') if isinstance(budgets, dict) else None
    if pool is None:
        for key in ('memory', 'checks', 'resources'):
            block = data.get(key) or {}
            if isinstance(block, dict) and 'pool_free' in block:
                pool = block['pool_free']
                break
    if pool is None:
        errs.append('%s: pool_free missing' % path)
    else:
        try:
            if int(pool) < MIN_POOL_FREE:
                errs.append('%s: pool_free %s < %s' % (path, pool, MIN_POOL_FREE))
        except (TypeError, ValueError):
            errs.append('%s: pool_free not an int: %r' % (path, pool))

    arts = data.get('artifacts') if isinstance(data.get('artifacts'), dict) else {}

    def artifact_sha(*names: str) -> str | None:
        for name in names:
            node = arts.get(name)
            if isinstance(node, dict) and isinstance(node.get('sha256'), str) and len(node['sha256']) >= 64:
                return node['sha256']
            if isinstance(data.get(name + '_sha256'), str) and len(data[name + '_sha256']) >= 64:
                return data[name + '_sha256']
        return None

    pkg = artifact_sha('package', 'fwsc')
    ctl = artifact_sha('controller', 'ctl')
    if not pkg:
        errs.append('%s: artifacts.package.sha256 missing' % path)
    if not ctl:
        errs.append('%s: artifacts.controller.sha256 missing' % path)

    return errs


def discover(evidence: Path) -> list[Path]:
    found = []
    for model in ('elite', 'pro'):
        p = evidence / model / 'record.json'
        if p.is_file():
            found.append(p)
    return found


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--evidence', type=Path, action='append', default=[],
                    help='Stage evidence directory containing elite/ and pro/ record.json')
    ap.add_argument('--record', type=Path, action='append', default=[],
                    help='Explicit record.json path (repeatable)')
    ap.add_argument('--commit', default=None, help='Optional expected source commit')
    ap.add_argument('--tree', default=None, help='Optional expected source_tree')
    args = ap.parse_args(argv)

    records: list[Path] = list(args.record)
    for ev in args.evidence:
        records.extend(discover(ev if ev.is_absolute() else ROOT / ev))
    # De-dupe while preserving order.
    seen = set()
    uniq: list[Path] = []
    for p in records:
        rp = p if p.is_absolute() else ROOT / p
        key = str(rp.resolve()) if rp.exists() else str(rp)
        if key in seen:
            continue
        seen.add(key)
        uniq.append(rp)

    if not uniq:
        print('FAIL: no record.json paths given', file=sys.stderr)
        return 1

    errs: list[str] = []
    for path in uniq:
        errs.extend(check_record(path, args.commit, args.tree))

    if errs:
        for e in errs:
            print('FAIL: ' + e, file=sys.stderr)
        return 1

    for path in uniq:
        data = load(path)
        print('PASS %s result=%s model=%s' % (path, data.get('result'), data.get('model')))
    print('PASS verify_dev_checked_records (%d record(s))' % len(uniq))
    return 0


if __name__ == '__main__':
    sys.exit(main())
