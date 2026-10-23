#!/usr/bin/env python3
"""Materialize a SHA-pinned GitHub checkout via REST Git Data API, without git-clone.

Fetches and verifies immutable commit/tree/blobs. Optional local Git bundle/cache
lets the script download only changed blobs. GitHub REST must be reachable from
this *native* process; access through a separate ChatGPT connector alone is not
network/file-system access for a native subprocess.
"""
from __future__ import annotations

import argparse
import base64
from datetime import datetime
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import subprocess
import sys
import tempfile
import time
import socket
from urllib.error import HTTPError, URLError
from urllib.parse import quote
from urllib.request import Request, urlopen


class SourceError(Exception):
    pass


def git(args: list[str], root: Path, data: bytes | None = None) -> bytes:
    p = subprocess.run(["git", "-C", str(root), *args], input=data, stdout=subprocess.PIPE,
                       stderr=subprocess.PIPE, check=False)
    if p.returncode:
        raise SourceError(f"git {' '.join(args[:4])} failed: {p.stderr.decode(errors='replace').strip()}")
    return p.stdout


def object_sha(kind: str, value: bytes) -> str:
    return hashlib.sha1(f"{kind} {len(value)}\0".encode() + value).hexdigest()


def valid_sha(sha: str) -> str:
    if len(sha) != 40 or any(c not in '0123456789abcdef' for c in sha):
        raise SourceError(f"Invalid Git object id: {sha!r}")
    return sha


def safe_path(p: str) -> None:
    parts = PurePosixPath(p).parts
    if not p or p.startswith("/") or '\\' in p or '\0' in p or '\n' in p or '\r' in p \
            or any(x in (".", "..", ".git") for x in parts):
        raise SourceError(f"Unsafe source path: {p!r}")


def safe_symlink(p: str, data: bytes) -> None:
    try:
        target = data.decode('utf-8')
    except UnicodeDecodeError as exc:
        raise SourceError(f"Non-UTF-8 symlink: {p}") from exc
    if not target or target.startswith('/') or '\\' in target or '\0' in target:
        raise SourceError(f"Unsafe symlink target: {p} -> {target!r}")
    stack = list(PurePosixPath(p).parts[:-1])
    for part in PurePosixPath(target).parts:
        if part == '..':
            if not stack:
                raise SourceError(f"Symlink escapes checkout: {p} -> {target!r}")
            stack.pop()
        elif part not in ('', '.'):
            stack.append(part)


class GitHubAPI:
    def __init__(self, root: str, token: str | None, timeout: int = 30):
        self.root = root.rstrip('/')
        self.token = token
        self.timeout = timeout

    def get(self, path: str) -> dict:
        url = self.root + path
        hdr = {'Accept': 'application/vnd.github+json', 'User-Agent': 'x0x37-exact-source/1',
               'X-GitHub-Api-Version': '2022-11-28'}
        if self.token:
            hdr['Authorization'] = 'Bearer ' + self.token
        for attempt in range(4):
            try:
                with urlopen(Request(url, headers=hdr), timeout=self.timeout) as res:
                    body = res.read(30 * 1024 * 1024 + 1)
                if len(body) > 30 * 1024 * 1024:
                    raise SourceError('GitHub API response exceeds safe 30 MiB limit')
                val = json.loads(body)
                if not isinstance(val, dict):
                    raise SourceError('Unexpected GitHub API response type')
                return val
            except HTTPError as exc:
                if exc.code not in (429, 500, 502, 503, 504) or attempt == 3:
                    raise SourceError(f'GitHub API HTTP {exc.code} reading {path}') from exc
            except (URLError, TimeoutError, OSError) as exc:
                if isinstance(getattr(exc, "reason", None), socket.gaierror):
                    raise SourceError('Native GitHub API DNS unavailable. GitHub connector responses are not mounted as local source bytes.') from exc
                if attempt == 3:
                    raise SourceError(f'Cannot reach GitHub API at {self.root}: {exc}') from exc
            time.sleep(min(2 ** attempt, 4))
        raise SourceError('GitHub API exhausted retries')


def verified_commit(json_commit: dict, wanted: str, tree: str) -> bytes:
    if json_commit.get('sha') != wanted or json_commit.get('tree', {}).get('sha') != tree:
        raise SourceError('Git commit SHA/tree disagree with requested ref')
    verification = json_commit.get('verification') or {}
    payload = verification.get('payload')
    if isinstance(payload, str):
        raw = payload.encode('utf-8')
        if not raw.startswith(f'tree {tree}\n'.encode()) or b'\n\n' not in raw:
            raise SourceError('Malformed original Git commit payload')
        sig = verification.get('signature')
        if sig:
            head, body = raw.split(b'\n\n', 1)
            lines = sig.rstrip('\n').split('\n')
            raw = head + b'\ngpgsig ' + lines[0].encode() + b''.join(
                b'\n ' + line.encode() for line in lines[1:]) + b'\n\n' + body
        if object_sha('commit', raw) != wanted:
            raise SourceError('Original Git commit hash mismatch (signature/metadata)')
        return raw

    # REST Git Data API exposes no raw commit payload for unsigned commits.
    # Reconstruct and verify it from the Git fields, trying real-world 15-minute
    # timezone offsets: GitHub normalizes these dates to UTC and omits the offset.
    if verification.get('signature'):
        raise SourceError('Signed commit has no verification payload')
    parents = json_commit.get('parents', [])
    header = f'tree {tree}\n' + ''.join('parent ' + valid_sha(x['sha']) + '\n' for x in parents)
    persons = []
    for which in ('author', 'committer'):
        person = json_commit.get(which, {})
        name, email, date = (person.get('name'), person.get('email'), person.get('date'))
        if not all(isinstance(v, str) for v in (name, email, date)) or \
           any(x in name + email for x in ('\n', '\r', '<', '>')):
            raise SourceError(f'Invalid {which} identity for commit reconstruction')
        try:
            timestamp = int(datetime.fromisoformat(date.replace('Z', '+00:00')).timestamp())
        except (ValueError, OverflowError) as exc:
            raise SourceError('Invalid Git commit timestamp') from exc
        persons.append((f'{which} {name} <{email}> {timestamp} ',))
    message = json_commit.get('message')
    if not isinstance(message, str):
        raise SourceError('Missing Git commit message')
    # Common offsets first, then exhaustive valid 15-minute zones.
    offsets = [0, 600, -600, 660, -660] + list(range(-720, 841, 15))
    offsets = list(dict.fromkeys(offsets))
    def tz(minutes: int) -> str:
        return ('+' if minutes >= 0 else '-') + f'{abs(minutes)//60:02d}{abs(minutes)%60:02d}'
    for a in offsets:
        for c in offsets:
            raw = (header + persons[0][0] + tz(a) + '\n' + persons[1][0] + tz(c) +
                   '\n\n' + message).encode('utf-8')
            if object_sha('commit', raw) == wanted:
                return raw
    raise SourceError('GitHub unsigned commit metadata insufficient to reproduce original SHA')


def materialize(repo: str, branch: str, expected_sha: str, output: Path,
                *, api_root: str = 'https://api.github.com', token: str | None = None,
                seed_bundle: Path | None = None, seed_git: Path | None = None) -> dict:
    if not repo or len(repo.split('/')) != 2 or any(not x for x in repo.split('/')):
        raise SourceError('Repository must be owner/repo')
    if '/' in branch and ('..' in branch or '\\' in branch) or not branch:
        raise SourceError('Invalid branch')
    expected_sha = valid_sha(expected_sha)
    output = output.absolute()
    if output.exists():
        raise SourceError(f'Output exists; will not overwrite {output}')
    if seed_bundle and seed_git:
        raise SourceError('Choose at most one --seed-bundle or --seed-git')
    if seed_bundle and not seed_bundle.is_file():
        raise SourceError('Missing source bundle')
    if seed_git and not (seed_git / '.git').is_dir() and not (seed_git / 'objects').is_dir():
        raise SourceError('Missing source Git object store')
    api = GitHubAPI(api_root, token)
    base = '/repos/' + '/'.join(quote(x, safe='') for x in repo.split('/'))
    ref = api.get(base + '/git/ref/heads/' + quote(branch, safe='/'))
    actual_sha = (ref.get('object') or {}).get('sha')
    if actual_sha != expected_sha:
        raise SourceError(f'Remote {branch} moved: expected {expected_sha}, observed {actual_sha}')
    c = api.get(base + '/git/commits/' + expected_sha)
    tree_sha = valid_sha((c.get('tree') or {}).get('sha', ''))
    raw_commit = verified_commit(c, expected_sha, tree_sha)
    tree = api.get(base + '/git/trees/' + tree_sha + '?recursive=1')
    if tree.get('sha') != tree_sha or tree.get('truncated') is not False:
        raise SourceError('GitHub returned mismatched/truncated recursive tree')
    entries = []
    for item in tree.get('tree', []):
        path, kind, mode, sha = (item.get('path'), item.get('type'), item.get('mode'), item.get('sha'))
        if not isinstance(path, str):
            raise SourceError('Tree entry has no path')
        safe_path(path)
        if kind == 'tree':
            continue
        if kind != 'blob' or mode not in ('100644', '100755', '120000'):
            raise SourceError(f'Unsupported source tree entry {path}: {kind} {mode}')
        valid_sha(sha)
        entries.append((path, mode, sha))
    if not entries or len(set(x[0] for x in entries)) != len(entries):
        raise SourceError('Empty or duplicate-file source tree')
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.x0x37-api-', dir=output.parent) as tmp:
        root = Path(tmp) / 'source'
        git(['init', '-q', str(root)], Path(tmp))
        if seed_bundle:
            git(['fetch', '-q', '--no-tags', str(seed_bundle.resolve()), 'refs/heads/source-snapshot'], root)
        if seed_git:
            git(['fetch', '-q', '--no-tags', str(seed_git.resolve()), 'HEAD'], root)
        fetched = 0
        for path, mode, sha in entries:
            has = subprocess.run(['git', '-C', str(root), 'cat-file', '-e', f'{sha}^{{blob}}'],
                                 capture_output=True, check=False).returncode == 0
            if not has:
                val = api.get(base + '/git/blobs/' + sha)
                if val.get('sha') != sha or val.get('encoding') != 'base64':
                    raise SourceError(f'Git blob metadata mismatch: {path}')
                try:
                    data = base64.b64decode(val['content'], validate=False)
                except (ValueError, KeyError) as exc:
                    raise SourceError(f'Git blob decoding failed: {path}') from exc
                if val.get('size') != len(data) or object_sha('blob', data) != sha:
                    raise SourceError(f'Git blob content hash/size mismatch: {path}')
                git(['hash-object', '-t', 'blob', '-w', '--stdin'], root, data)
                fetched += 1
            if mode == '120000':
                safe_symlink(path, git(['cat-file', 'blob', sha], root))
        index = b''.join(f'{mode} {sha}\t{path}\0'.encode('utf-8') for path, mode, sha in entries)
        git(['update-index', '-z', '--index-info'], root, index)
        actual_tree = git(['write-tree'], root).decode().strip()
        if actual_tree != tree_sha:
            raise SourceError(f'Rebuilt Git tree mismatch: {actual_tree} != {tree_sha}')
        written = git(['hash-object', '-t', 'commit', '-w', '--stdin'], root, raw_commit).strip().decode()
        if written != expected_sha:
            raise SourceError('Git commit object write hash differs')
        (root / '.git' / 'shallow').write_text(expected_sha + '\n', encoding='ascii')
        git(['read-tree', '--empty'], root)
        git(['checkout', '-f', '-q', '--detach', expected_sha], root)
        if git(['rev-parse', 'HEAD'], root).decode().strip() != expected_sha or \
           git(['rev-parse', 'HEAD^{tree}'], root).decode().strip() != tree_sha or \
           git(['status', '--porcelain'], root):
            raise SourceError('Final checkout SHA/tree/cleanliness verification failed')
        ref_after = api.get(base + '/git/ref/heads/' + quote(branch, safe='/'))
        if (ref_after.get('object') or {}).get('sha') != expected_sha:
            raise SourceError('Nightly branch advanced during source fetch; retry with a new pin')
        os.rename(root, output)
    return dict(repo=repo, branch=branch, commit=expected_sha, tree=tree_sha,
                files=len(entries), fetched_blobs=fetched, source_checked=True)


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo', default='gobbajogger/X0X37-for-Loopy-Pro')
    p.add_argument('--branch', default='nightly')
    p.add_argument('--commit', required=True, help='Exact expected nightly commit SHA (never accept a floating ref)')
    p.add_argument('--output', type=Path, required=True, help='New, empty, private checkout path')
    p.add_argument('--seed-bundle', type=Path, help='Optional local offline source-cache.bundle')
    p.add_argument('--seed-git', type=Path, help='Optional local Git checkout object cache')
    p.add_argument('--api-root', default='https://api.github.com', help=argparse.SUPPRESS)
    args = p.parse_args()
    try:
        result = materialize(args.repo, args.branch, args.commit, args.output,
                             api_root=args.api_root, token=os.environ.get('GITHUB_TOKEN'),
                             seed_bundle=args.seed_bundle, seed_git=args.seed_git)
    except (SourceError, OSError, subprocess.SubprocessError) as exc:
        print(f'SOURCE NOT AVAILABLE / FAIL CLOSED: {exc}', file=sys.stderr)
        return 2
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == '__main__':
    sys.exit(main())
