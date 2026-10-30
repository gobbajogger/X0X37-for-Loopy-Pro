#!/usr/bin/env python3
"""Rebuild a source-only Git tree from Code Mode GitHub connector JSONL receipts.

No HTTP, Git executable, credentials, or SDK required. Fails closed unless every
blob, mode, byte length and root-tree SHA match the pinned metadata.
"""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys

DEFAULT_TREE = '3e2a673f97a0ae4fc4e3fd5457952fb6e3f7d6e8'
DEFAULT_COUNT = 4223
DEFAULT_BYTES = 23199539
ALLOWED = {'100644', '100755', '120000'}


def object_sha(kind, data):
    return hashlib.sha1(kind.encode('ascii') + b' ' + str(len(data)).encode() + b'\0' + data).hexdigest()


def safe_path(path):
    if not isinstance(path, str) or not path or '\\' in path or '\0' in path:
        raise ValueError('invalid path')
    parts = path.split('/')
    if any(c in ('', '.', '..', '.git') for c in parts) or path.startswith('/'):
        raise ValueError('unsafe path: ' + repr(path))
    return Path(*parts)


def root_tree_sha(entries):
    root = {}
    for e in entries:
        parts = safe_path(e['path']).parts
        folder = root
        for p in parts[:-1]:
            old = folder.setdefault(p, {})
            if not isinstance(old, dict):
                raise ValueError('file/directory collision: ' + e['path'])
            folder = old
        if parts[-1] in folder:
            raise ValueError('duplicate tree name: ' + e['path'])
        folder[parts[-1]] = (e['mode'], e['sha'])

    def digest(node):
        order = sorted(node.items(), key=lambda item: (item[0] + ('/' if isinstance(item[1], dict) else '')).encode())
        payload = bytearray()
        for name, item in order:
            if isinstance(item, dict):
                mode, sha = '40000', digest(item)
            else:
                mode, sha = item
            payload += mode.encode('ascii') + b' ' + name.encode('utf-8') + b'\0' + bytes.fromhex(sha)
        return object_sha('tree', bytes(payload))
    return digest(root)


def restore(input_path, output, expected_tree, expected_count, expected_bytes):
    if output.exists() or output.is_symlink():
        raise ValueError('output already exists: ' + str(output))
    receipts = {}
    cursor = [0, 0]
    batches = 0
    with input_path.open(encoding='utf-8') as stream:
        for line_no, line in enumerate(stream, 1):
            if not line.strip():
                continue
            batch = json.loads(line)
            batches += 1
            if batch.get('tree_sha') != expected_tree or batch.get('truncated') is not False or batch.get('total') != expected_count:
                raise ValueError('tree identity/count/truncation mismatch at line ' + str(line_no))
            if batch.get('cursor') != cursor:
                raise ValueError('missing, duplicate, or reordered batch at line ' + str(line_no) + ', expected ' + str(cursor))
            for r in batch.get('chunks', []):
                i = r['index']
                if not isinstance(i, int) or not 0 <= i < expected_count or r['mode'] not in ALLOWED:
                    raise ValueError('invalid index/mode at line ' + str(line_no))
                safe_path(r['path'])
                if i not in receipts:
                    receipts[i] = {'path': r['path'], 'mode': r['mode'], 'sha': r['sha'], 'size': r['size'], 'total': r['total'], 'b64': ''}
                s = receipts[i]
                if any(s[k] != r[k] for k in ('path', 'mode', 'sha', 'size', 'total')):
                    raise ValueError('metadata drift at index ' + str(i))
                if r['start'] != len(s['b64']):
                    raise ValueError('missing or overlapping chunk at index ' + str(i))
                s['b64'] += r['b64']
                if len(s['b64']) > s['total']:
                    raise ValueError('excess base64 content at index ' + str(i))
            cursor = batch['next']
    if cursor != [expected_count, 0] or len(receipts) != expected_count or set(receipts) != set(range(expected_count)):
        raise ValueError('incomplete transfer: cursor=' + str(cursor) + ', unique blobs=' + str(len(receipts)))
    decoded = []
    total = 0
    for i in range(expected_count):
        r = receipts[i]
        if len(r['b64']) != r['total']:
            raise ValueError('incomplete content at index ' + str(i))
        data = base64.b64decode(r['b64'], validate=True)
        if len(data) != r['size'] or object_sha('blob', data) != r['sha']:
            raise ValueError('content/hash mismatch at index ' + str(i) + ' ' + r['path'])
        total += len(data)
        decoded.append((r, data))
    if total != expected_bytes:
        raise ValueError('total byte count mismatch: ' + str(total))
    actual_tree = root_tree_sha([r for r, data in decoded])
    if actual_tree != expected_tree:
        raise ValueError('root-tree SHA mismatch: ' + actual_tree + ' != ' + expected_tree)
    staging = output.with_name(output.name + '.staging')
    if staging.exists() or staging.is_symlink():
        raise ValueError('staging output already exists: ' + str(staging))
    staging.mkdir(parents=True)
    try:
        for r, data in decoded:
            rel = safe_path(r['path'])
            target = staging / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            if r['mode'] == '120000':
                link = data.decode('utf-8')
                if not link or Path(link).is_absolute() or not (target.parent / link).resolve().is_relative_to(staging.resolve()):
                    raise ValueError('unsafe symlink: ' + r['path'] + ' -> ' + repr(link))
                target.symlink_to(link)
            else:
                target.write_bytes(data)
                target.chmod(0o755 if r['mode'] == '100755' else 0o644)
        os.replace(staging, output)
    except BaseException:
        shutil.rmtree(staging)
        raise
    print(f'PASS: {expected_count} verified blobs, {total} bytes, {batches} batches, tree {actual_tree}')
    print('Exact source reconstructed at:', output)


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--input', type=Path, default=Path('source-transfer.jsonl'))
    p.add_argument('--output', type=Path, default=Path('x0x37-cb5423c4'))
    p.add_argument('--tree', default=DEFAULT_TREE)
    p.add_argument('--count', type=int, default=DEFAULT_COUNT)
    p.add_argument('--bytes', type=int, default=DEFAULT_BYTES)
    a = p.parse_args()
    try:
        restore(a.input, a.output, a.tree, a.count, a.bytes)
    except Exception as exc:
        print(f'FAIL: {exc}', file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
