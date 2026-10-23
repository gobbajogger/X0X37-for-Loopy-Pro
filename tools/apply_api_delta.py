#!/usr/bin/env python3
"""Offline, fail-closed GitHub-connector blob-delta -> exact Git checkout.

The JSON is transported using permitted connector/host inputs. No network or
GitHub Actions runner is used. Every object SHA and the resulting Git tree and
commit must match independently obtained GitHub Git Data API identities.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from fetch_exact_source_api import SourceError, safe_path, safe_symlink, valid_sha, verified_commit


def run_git(path: Path, *args: str, stdin: bytes | None = None) -> bytes:
    p = subprocess.run(['git', '-C', str(path), *args], input=stdin,
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    if p.returncode:
        raise SourceError('git ' + ' '.join(args[:3]) + ' failed: ' +
                          p.stderr.decode('utf-8', errors='replace')[:500])
    return p.stdout


def sha_blob(raw: bytes) -> str:
    return hashlib.sha1(f'blob {len(raw)}\0'.encode('ascii') + raw).hexdigest()


def lines(raw: bytes) -> list[bytes]:
    # Preserve every actual LF byte. Python str.splitlines() treats non-LF
    # Unicode characters as newlines, which must never corrupt source bytes.
    pieces = raw.split(b"\n")
    return [piece + b"\n" for piece in pieces[:-1]] + ([pieces[-1]] if pieces[-1] else [])


def validate_relative(name: str) -> Path:
    if not isinstance(name, str):
        raise SourceError('Non-string path')
    safe_path(name)
    return Path(name)


def reject_symlink_parents(root: Path, path: Path) -> None:
    for relative in list(path.parents)[:-1]:
        if (root / relative).is_symlink():
            raise SourceError('Path traverses symlink: ' + path.as_posix())


def delta_bytes(entry: dict, before: bytes | None) -> bytes:
    if before is None:
        if entry.get('old') is not None or not isinstance(entry.get('content'), str):
            raise SourceError('New file requires old=null and text content')
        return entry['content'].encode('utf-8')
    if sha_blob(before) != valid_sha(entry.get('old', '')):
        raise SourceError('Original blob SHA does not match delta base')
    if 'content' in entry or not isinstance(entry.get('ops'), list):
        raise SourceError('Changed file requires line edit operations, not replacement')
    original = lines(before)
    previous_end = -1
    for item in entry['ops']:
        if not isinstance(item, list) or len(item) != 3:
            raise SourceError('Malformed line edit')
        index, deleted, text = item
        if type(index) is not int or type(deleted) is not int or not isinstance(text, str):
            raise SourceError('Malformed edit fields')
        if index < 0 or deleted < 0 or index + deleted > len(original) or index < previous_end:
            raise SourceError('Overlapping/out-of-bounds edits')
        previous_end = index + deleted
    updated = original[:]
    for index, deleted, text in reversed(entry['ops']):
        updated[index:index + deleted] = lines(text.encode('utf-8'))
    return b''.join(updated)


def materialize(seed_git: Path, delta_json: Path, destination: Path) -> dict:
    seed_git, destination = seed_git.resolve(), destination.absolute()
    if destination.exists():
        raise SourceError('Destination already exists; refusing overwrite')
    raw = json.loads(delta_json.read_text(encoding='utf-8'))
    if not isinstance(raw, dict):
        raise SourceError('Root manifest must be an object')
    expected_seed = valid_sha(raw.get('seed_tree', ''))
    expected_tree = valid_sha(raw.get('target_tree', ''))
    expected_commit = valid_sha(raw.get('target_commit', ''))
    commit_data = raw.get('github_commit')
    if not isinstance(commit_data, dict):
        raise SourceError('Missing GitHub Git Data API commit payload')
    if not isinstance(raw.get('files'), list) or not isinstance(raw.get('deletions'), list):
        raise SourceError('Missing delta/deletion arrays')
    planned = []
    seen = set()
    for entry in raw['files']:
        if not isinstance(entry, dict):
            raise SourceError('Invalid changed file entry')
        relative = validate_relative(entry.get('path'))
        path = relative.as_posix()
        if path in seen:
            raise SourceError('Duplicate file path')
        seen.add(path)
        new_sha = valid_sha(entry.get('sha', ''))
        mode = entry.get('mode')
        if mode not in ('100644', '100755', '120000'):
            raise SourceError('Unsupported target file mode')
        planned.append((path, relative, mode, new_sha, entry))
    deleted = []
    for path in raw['deletions']:
        relative = validate_relative(path)
        if path in seen:
            raise SourceError('Deleted and changed path collision')
        seen.add(path)
        deleted.append((path, relative))
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.x0x37-delta-', dir=destination.parent) as temp:
        root = Path(temp) / 'checkout'
        subprocess.run(['git', 'clone', '-q', '--no-local', '--', str(seed_git), str(root)],
                       check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        seed = run_git(root, 'rev-parse', 'HEAD^{tree}').decode().strip()
        if seed != expected_seed or run_git(root, 'status', '--porcelain'):
            raise SourceError('Seed Git tree mismatch or unclean checkout')
        for path, relative, mode, blob, entry in planned:
            reject_symlink_parents(root, relative)
            target = root / relative
            old = None
            if entry.get('old') is not None:
                if not target.is_file() or target.is_symlink():
                    raise SourceError('Changed file missing or not regular: ' + path)
                old = target.read_bytes()
            elif target.exists() or target.is_symlink():
                raise SourceError('New file already exists: ' + path)
            value = delta_bytes(entry, old)
            if sha_blob(value) != blob:
                raise SourceError('New Git blob SHA mismatch: ' + path)
            if mode == '120000':
                safe_symlink(path, value)
            target.parent.mkdir(parents=True, exist_ok=True)
            if mode == '120000':
                target.symlink_to(value.decode('utf-8'))
            else:
                target.write_bytes(value)
                target.chmod(0o755 if mode == '100755' else 0o644)
        for path, relative in deleted:
            reject_symlink_parents(root, relative)
            target = root / relative
            if not target.exists() and not target.is_symlink():
                raise SourceError('Requested removal absent from seed: ' + path)
            if target.is_dir() and not target.is_symlink():
                raise SourceError('Refusing directory deletion: ' + path)
            target.unlink()
        run_git(root, 'add', '-A')
        tree = run_git(root, 'write-tree').decode().strip()
        if tree != expected_tree:
            raise SourceError('Full target Git tree mismatch: ' + tree + ' != ' + expected_tree)
        commit_raw = verified_commit(commit_data, expected_commit, expected_tree)
        recorded = run_git(root, 'hash-object', '-t', 'commit', '-w', '--stdin', stdin=commit_raw).decode().strip()
        if recorded != expected_commit:
            raise SourceError('Recorded commit does not match target GitHub SHA')
        (root / '.git' / 'shallow').write_text(expected_commit + '\n', encoding='ascii')
        run_git(root, 'reset', '-q', '--hard', expected_commit)
        if run_git(root, 'rev-parse', 'HEAD').decode().strip() != expected_commit or \
           run_git(root, 'rev-parse', 'HEAD^{tree}').decode().strip() != expected_tree or \
           run_git(root, 'status', '--porcelain'):
            raise SourceError('Final source HEAD/tree/cleanliness mismatch')
        os.rename(root, destination)
    return {'target_commit':expected_commit, 'target_tree':expected_tree,
            'changed':len(planned),'deleted':len(deleted),'clean':True,
            'whole_target_tree_verified':True}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed-git',type=Path,required=True)
    parser.add_argument('--delta-json',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    try:
        print(json.dumps(materialize(args.seed_git,args.delta_json,args.output),sort_keys=True))
        return 0
    except (ValueError,KeyError,OSError,SourceError,subprocess.CalledProcessError) as e:
        print('NO CHECKOUT: '+str(e),file=sys.stderr)
        return 2

if __name__=='__main__':
    raise SystemExit(main())
