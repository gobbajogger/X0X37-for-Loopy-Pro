#!/usr/bin/env python3
"""Offline Git-blob-manifest verification and exact incremental checkout recovery.

Consumes GitHub API tree metadata and explicit base64 blob receipts. Never uses Git,
HTTP, credentials, firmware flashing or executable source. Source is trusted only
when the Git tree hash and every relevant Git blob SHA/size are verified.
"""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import stat
import sys

from restore_github_api_batches import object_sha, root_tree_sha, safe_path

MODES = {'100644', '100755', '120000'}
HASH = re.compile(r'^[a-f0-9]{40}$')


def read_manifest(filename):
    header = {}
    entries = []
    seen = set()
    for number, raw in enumerate(filename.read_text(encoding='utf-8').splitlines(), 1):
        if raw.startswith('# '):
            key, _, value = raw[2:].partition(' ')
            if key in header:
                raise ValueError(f'duplicate manifest header {key}')
            header[key] = value
            continue
        if not raw:
            raise ValueError(f'empty manifest line {number}')
        fields = raw.split('\t')
        if len(fields) != 4:
            raise ValueError(f'invalid manifest row {number}')
        path, mode, sha, size_text = fields
        safe_path(path)
        if path in seen:
            raise ValueError('duplicate manifest path: ' + path)
        if mode not in MODES or not HASH.fullmatch(sha) or not size_text.isdigit():
            raise ValueError('invalid mode/SHA/size: ' + path)
        seen.add(path)
        size = int(size_text)
        entries.append(dict(path=path, mode=mode, sha=sha, size=size))
    for key in ('repository', 'commit', 'tree', 'blobs', 'bytes'):
        if key not in header:
            raise ValueError('missing manifest header: ' + key)
    if not HASH.fullmatch(header['commit']) or not HASH.fullmatch(header['tree']):
        raise ValueError('invalid manifest commit/tree hash')
    total = sum(e['size'] for e in entries)
    if len(entries) != int(header['blobs']) or total != int(header['bytes']):
        raise ValueError('manifest blob count or byte count mismatch')
    computed = root_tree_sha(entries)
    if computed != header['tree']:
        raise ValueError('manifest root-tree hash mismatch: ' + computed)
    return header, entries


def blob_bytes(root, entry):
    relative = safe_path(entry['path'])
    target = root / relative
    mode = entry['mode']
    if mode == '120000':
        if not target.is_symlink():
            raise ValueError('expected symlink: ' + entry['path'])
        link = os.readlink(target)
        if not link or os.path.isabs(link) or not (target.parent / link).resolve().is_relative_to(root.resolve()):
            raise ValueError('unsafe symlink: ' + entry['path'])
        data = os.fsencode(link)
    else:
        if target.is_symlink() or not target.is_file():
            raise ValueError('expected regular file: ' + entry['path'])
        mode_bits = stat.S_IMODE(target.stat().st_mode)
        if bool(mode_bits & 0o111) != (mode == '100755'):
            raise ValueError('executable mode mismatch: ' + entry['path'])
        data = target.read_bytes()
    if len(data) != entry['size'] or object_sha('blob', data) != entry['sha']:
        raise ValueError('blob size/SHA mismatch: ' + entry['path'])
    return data


def verified_cache(root, entries):
    cache = {}
    for e in entries:
        data = blob_bytes(root, e)
        cache.setdefault(e['sha'], data)
    return cache


def missing_blobs(new_entries, cache_shas):
    needed = []
    seen = set(cache_shas)
    for e in new_entries:
        if e['sha'] not in seen:
            needed.append(e)
            seen.add(e['sha'])
    return needed


def parse_receipts(filename, tree, expected):
    if not expected:
        if filename is not None and filename.exists() and filename.stat().st_size:
            raise ValueError('unexpected receipts: no new blobs required')
        return {}
    if filename is None:
        raise ValueError('missing receipt JSONL for ' + str(len(expected)) + ' new blobs')
    chunks = {}
    cursor = [0, 0]
    batch_count = 0
    for lineno, line in enumerate(filename.read_text(encoding='utf-8').splitlines(), 1):
        if not line.strip():
            continue
        b = json.loads(line)
        batch_count += 1
        if b.get('tree_sha') != tree or b.get('truncated') is not False or b.get('total') != len(expected):
            raise ValueError(f'receipt tree/count mismatch at line {lineno}')
        if b.get('cursor') != cursor:
            raise ValueError(f'receipt missing/reordered batch at line {lineno}: {cursor}')
        for c in b.get('chunks', []):
            i = c.get('index')
            if not isinstance(i, int) or not 0 <= i < len(expected):
                raise ValueError(f'invalid receipt index at line {lineno}')
            e = expected[i]
            if any(c.get(k) != e[k] for k in ('path','mode','sha','size')):
                raise ValueError(f'receipt identity mismatch at index {i}')
            if not isinstance(c.get('start'), int) or c['start'] < 0 or not isinstance(c.get('total'), int):
                raise ValueError(f'invalid receipt offset at index {i}')
            if not isinstance(c.get('b64'), str) or len(c['b64']) % 4:
                raise ValueError(f'invalid receipt base64 chunk at index {i}')
            record = chunks.setdefault(i, dict(b64='', total=c['total']))
            if record['total'] != c['total'] or c['start'] != len(record['b64']):
                raise ValueError(f'missing/overlapping content at index {i}')
            record['b64'] += c['b64']
            if len(record['b64']) > record['total']:
                raise ValueError(f'excess content at index {i}')
        next_cursor = b.get('next')
        if not isinstance(next_cursor, list) or len(next_cursor) != 2 or any(not isinstance(x,int) for x in next_cursor):
            raise ValueError(f'invalid next cursor at line {lineno}')
        if tuple(next_cursor) <= tuple(cursor) or next_cursor[0] > len(expected):
            raise ValueError(f'non-advancing next cursor at line {lineno}')
        cursor = next_cursor
    if cursor != [len(expected), 0] or len(chunks) != len(expected):
        raise ValueError(f'incomplete receipts: cursor={cursor}, complete={len(chunks)}/{len(expected)}')
    data = {}
    for i,e in enumerate(expected):
        record = chunks[i]
        if len(record['b64']) != record['total']:
            raise ValueError(f'incomplete base64 content at index {i}')
        b = base64.b64decode(record['b64'],validate=True)
        if len(b) != e['size'] or object_sha('blob',b) != e['sha']:
            raise ValueError('receipt byte SHA mismatch at index ' + str(i))
        data[e['sha']] = b
    return data


def restore(out, target_entries, data):
    if out.exists() or out.is_symlink():
        raise ValueError('destination exists: ' + str(out))
    staging = out.with_name(out.name + '.staging')
    if staging.exists() or staging.is_symlink():
        raise ValueError('staging exists: ' + str(staging))
    staging.mkdir(parents=True)
    try:
        for e in target_entries:
            content = data[e['sha']]
            target = staging / safe_path(e['path'])
            target.parent.mkdir(parents=True, exist_ok=True)
            if e['mode'] == '120000':
                link = os.fsdecode(content)
                if not link or os.path.isabs(link) or not (target.parent / link).resolve().is_relative_to(staging.resolve()):
                    raise ValueError('unsafe target symlink: ' + e['path'])
                target.symlink_to(link)
            else:
                target.write_bytes(content)
                target.chmod(0o755 if e['mode']=='100755' else 0o644)
        os.replace(staging, out)
    except BaseException:
        shutil.rmtree(staging)
        raise


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--target-manifest',type=Path,required=True)
    p.add_argument('--base-manifest',type=Path)
    p.add_argument('--base-root',type=Path)
    p.add_argument('--receipts',type=Path)
    p.add_argument('--output',type=Path)
    p.add_argument('--needed-json',type=Path)
    a=p.parse_args()
    if bool(a.base_manifest) != bool(a.base_root):
        p.error('--base-manifest and --base-root must be provided together')
    try:
        h,target = read_manifest(a.target_manifest)
        cache = {}
        if a.base_manifest:
            old_h,old = read_manifest(a.base_manifest)
            if old_h['repository']!=h['repository']:
                raise ValueError('base and target repository mismatch')
            cache = verified_cache(a.base_root, old)
        needed = missing_blobs(target,cache)
        print(f'PASS: manifest {h["tree"]}, {len(target)} paths, {h["bytes"]} bytes')
        print(f'Reuse: {sum(e["sha"] in cache for e in target)}/{len(target)} paths, ' +
              f'{len(needed)} unique new blobs needed')
        if a.needed_json:
            a.needed_json.write_text(json.dumps({
                'repository': h['repository'], 'commit':h['commit'], 'tree_sha':h['tree'],
                'total':len(needed), 'entries':needed
            },indent=2)+'\n',encoding='utf-8')
        if a.output:
            cache.update(parse_receipts(a.receipts,h['tree'],needed))
            restore(a.output,target,cache)
            verified_cache(a.output,target)
            print('PASS: exact reconstructed source at',a.output)
        elif a.receipts:
            raise ValueError('--receipts requires --output')
    except (ValueError, OSError, KeyError, IndexError) as exc:
        print('FAIL:',exc,file=sys.stderr)
        sys.exit(1)


if __name__=='__main__':
    main()
