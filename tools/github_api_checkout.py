#!/usr/bin/env python3
"""Fail-closed, SHA-pinned source checkout via GitHub's Git Data REST API.

Uses a local Git object cache when supplied. Never calls GitHub Actions,
changes remote refs, publishes binaries, or flashes hardware. A checkout is
usable for development builds, NOT by itself a qualified firmware candidate.
"""
from __future__ import annotations

import argparse
import base64
import binascii
import calendar
import hashlib
import ipaddress
import json
import os
import posixpath
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import time
from urllib.error import HTTPError, URLError
from urllib.parse import quote, urlsplit
from urllib.request import Request, urlopen

HEX = re.compile(r'^[0-9a-f]{40}$')
REPO = re.compile(r'^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$')
MODES = {'100644', '100755', '120000'}


def git(path: Path, *args: str, data: bytes | None = None) -> str:
    p = subprocess.run(['git', '-C', str(path), *args], input=data,
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    if p.returncode:
        raise RuntimeError(f'git {" ".join(args)}: {p.stderr.decode(errors="replace").strip()}')
    return p.stdout.decode('utf-8').strip()


def object_hash(kind: str, data: bytes) -> str:
    return hashlib.sha1(f'{kind} {len(data)}\0'.encode() + data).hexdigest()


def api_base(url: str) -> str:
    u = urlsplit(url.rstrip('/'))
    allowed_local = u.scheme == 'http' and u.hostname in ('localhost', '127.0.0.1', '::1')
    if u.scheme != 'https' and not allowed_local:
        raise ValueError('GitHub API transport must use HTTPS (except loopback tests)')
    if u.hostname not in ('api.github.com', 'localhost', '127.0.0.1', '::1'):
        raise ValueError('Refusing to transmit GitHub API credentials to an untrusted host')
    if not u.netloc or u.username or u.password or u.query or u.fragment or u.path not in ('', '/'):
        raise ValueError('Invalid GitHub API base URL')
    return url.rstrip('/')


class API:
    def __init__(self, base: str, token: str | None = None):
        self.base = api_base(base)
        self.token = token
        self.read_count = 0

    def get(self, path: str) -> dict:
        if not path.startswith('/repos/') or '..' in path or '//' in path:
            raise ValueError('Unsafe GitHub REST path')
        headers = {'Accept': 'application/vnd.github+json',
                   'X-GitHub-Api-Version': '2022-11-28', 'User-Agent': 'x0x37-git-api-checkout/1'}
        if self.token:
            headers['Authorization'] = 'Bearer ' + self.token
        for attempt in range(3):
            try:
                with urlopen(Request(self.base + path, headers=headers), timeout=35) as res:
                    if urlsplit(res.geturl()).hostname != urlsplit(self.base).hostname:
                        raise ValueError('GitHub API redirected to an unexpected host')
                    raw = res.read(8 * 1024 * 1024 + 1)
                    if len(raw) > 8 * 1024 * 1024:
                        raise ValueError('GitHub API object too large')
                self.read_count += 1
                value = json.loads(raw)
                if not isinstance(value, dict):
                    raise ValueError('Unexpected GitHub API JSON object')
                return value
            except HTTPError as exc:
                if exc.code not in (429, 500, 502, 503, 504) or attempt == 2:
                    raise RuntimeError(f'GitHub API GET {path} failed HTTP {exc.code}') from exc
            except URLError as exc:
                if attempt == 2:
                    raise RuntimeError(f'GitHub API GET {path} failed: {exc.reason}') from exc
            time.sleep(0.25 * (2 ** attempt))
        raise AssertionError('Unreachable retry state')


def _tz_candidates():
    # The REST API reports UTC while raw unsigned Git objects preserve local TZ.
    yield '+0000'
    for minute in range(-14 * 60, 14 * 60 + 1, 15):
        if minute:
            yield ('+' if minute >= 0 else '-') + f'{abs(minute)//60:02d}{abs(minute)%60:02d}'


def exact_commit_bytes(commit: dict, wanted: str) -> bytes:
    verification = commit.get('verification') or {}
    payload = verification.get('payload')
    signature = verification.get('signature')
    if payload is not None:
        if not isinstance(payload, str) or '\n\n' not in payload:
            raise ValueError('Malformed signed/verified commit payload')
        raw = payload
        if signature is not None:
            if not isinstance(signature, str) or not signature.endswith('\n'):
                raise ValueError('Malformed GitHub commit signature')
            raw = raw.replace('\n\n', '\ngpgsig ' + signature.replace('\n', '\n ') + '\n\n', 1)
        result = raw.encode('utf-8')
        if object_hash('commit', result) != wanted:
            raise ValueError('GitHub commit payload/signature does not recreate expected SHA')
        return result
    tree = commit.get('tree', {}).get('sha')
    parents = [p.get('sha') for p in commit.get('parents', [])]
    if not isinstance(tree, str) or not HEX.fullmatch(tree) or any(not isinstance(p, str) or not HEX.fullmatch(p) for p in parents):
        raise ValueError('Commit tree/parents are invalid')
    # For old unsigned Git commits, try Git's standard timezone offsets and
    # verify the exact commit object SHA. Never accept an inferred unmatched SHA.
    persons = []
    for role in ('author', 'committer'):
        item = commit[role]
        name, email, utc = item['name'], item['email'], item['date']
        if any(c in name + email for c in '\r\n<>'):
            raise ValueError('Unsupported commit identity')
        timestamp = calendar.timegm(time.strptime(utc, '%Y-%m-%dT%H:%M:%SZ'))
        persons.append((role, name, email, timestamp))
    prefix = f'tree {tree}\n' + ''.join(f'parent {p}\n' for p in parents)
    if not isinstance(commit.get('message'), str):
        raise ValueError('Commit message missing')
    suffix = '\n\n' + commit['message']
    for a in _tz_candidates():
        for c in _tz_candidates():
            pa, pc = persons
            raw = (prefix + f'author {pa[1]} <{pa[2]}> {pa[3]} {a}\n'
                   + f'committer {pc[1]} <{pc[2]}> {pc[3]} {c}' + suffix).encode()
            if object_hash('commit', raw) == wanted:
                return raw
    raise ValueError('Cannot reconstruct original unsigned commit object exactly')


def validated_entries(tree: dict, expected_sha: str) -> list[dict]:
    if tree.get('truncated') is not False or tree.get('sha') != expected_sha:
        raise ValueError('Incomplete/mismatched GitHub tree; refusing checkout')
    entries = []
    seen = set()
    for item in tree.get('tree', []):
        if not isinstance(item, dict):
            raise ValueError('Invalid tree entry')
        if item.get('type') == 'tree':
            continue
        name, mode, sha = item.get('path'), item.get('mode'), item.get('sha')
        if (not isinstance(name, str) or not name or name.startswith('/')
                or any(x in ('', '.', '..', '.git') for x in name.split('/'))
                or any(c in name for c in '\x00\r\n\\') or name in seen
                or mode not in MODES or item.get('type') != 'blob'
                or not isinstance(sha, str) or not HEX.fullmatch(sha)
                or not isinstance(item.get('size'), int) or item['size'] < 0):
            raise ValueError(f'Unsupported/unsafe GitHub tree entry: {name!r}')
        seen.add(name)
        entries.append({'path': name, 'mode': mode, 'sha': sha, 'size': item['size']})
    if not entries:
        raise ValueError('Empty Git source tree')
    return entries


def fetch_missing(api: API, repo: str, source: Path, entries: list[dict]) -> int:
    missing = {}
    for e in entries:
        if e['sha'] in missing:
            continue
        found = subprocess.run(['git', '-C', str(source), 'cat-file', '-e', e['sha'] + '^{blob}'],
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0
        if not found:
            missing[e['sha']] = e['size']
        elif int(git(source, 'cat-file', '-s', e['sha'])) != e['size']:
            raise ValueError('Cached Git blob size mismatch: ' + e['sha'])
    for sha, size in missing.items():
        blob = api.get(f'/repos/{repo}/git/blobs/{sha}')
        if blob.get('sha') != sha or blob.get('encoding') != 'base64':
            raise ValueError('Incorrect GitHub blob response SHA/encoding: ' + sha)
        try:
            content = base64.b64decode(''.join(blob['content'].split()), validate=True)
        except (ValueError, KeyError, binascii.Error) as exc:
            raise ValueError('Malformed GitHub blob base64: ' + sha) from exc
        if len(content) != size or blob.get('size') != size or object_hash('blob', content) != sha:
            raise ValueError('GitHub blob integrity mismatch: ' + sha)
        if git(source, 'hash-object', '-t', 'blob', '-w', '--stdin', data=content) != sha:
            raise ValueError('Imported Git blob differs from GitHub: ' + sha)
    # Keep Git symlinks strictly inside this exact source root. A checked-out
    # source must not let builds read local SDK/credentials through aliases.
    for entry in entries:
        if entry['mode'] == '120000':
            target = subprocess.check_output(['git', '-C', str(source), 'cat-file', 'blob', entry['sha']])
            if b'\x00' in target or b'\n' in target or b'\\' in target:
                raise ValueError('Unsafe Git symlink target: ' + entry['path'])
            relative = target.decode('utf-8')
            joined = posixpath.normpath(posixpath.join(posixpath.dirname(entry['path']), relative))
            if (relative.startswith('/') or joined == '..' or joined.startswith('../')
                    or joined == '.' or not relative):
                raise ValueError('Git symlink points outside source tree: ' + entry['path'])
    return len(missing)


def reconstruct(api: API, repo: str, commit_sha: str, expected_tree: str | None,
                output: Path, seed: Path | None, verify_ref: str | None) -> dict:
    if not REPO.fullmatch(repo) or not HEX.fullmatch(commit_sha):
        raise ValueError('Invalid owner/repository or immutable commit SHA')
    if expected_tree and not HEX.fullmatch(expected_tree):
        raise ValueError('Expected tree must be full lowercase 40-character SHA')
    if verify_ref and not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_.\-/]*', verify_ref):
        raise ValueError('Invalid verification ref')
    endpoint = '/repos/' + repo
    if verify_ref:
        branch = api.get(endpoint + '/branches/' + quote(verify_ref, safe=''))
        if branch.get('commit', {}).get('sha') != commit_sha:
            raise ValueError('Remote verification branch moved or differs from pinned commit')
    commit = api.get(endpoint + '/git/commits/' + commit_sha)
    if commit.get('sha') != commit_sha:
        raise ValueError('GitHub commit identity mismatch')
    tree_sha = commit.get('tree', {}).get('sha')
    if not isinstance(tree_sha, str) or not HEX.fullmatch(tree_sha) or (expected_tree and tree_sha != expected_tree):
        raise ValueError('Unexpected commit tree SHA')
    raw_commit = exact_commit_bytes(commit, commit_sha)
    entries = validated_entries(api.get(endpoint + '/git/trees/' + tree_sha + '?recursive=1'), tree_sha)
    output = output.expanduser().absolute()
    if output.exists() or output.is_symlink():
        raise ValueError('Output already exists; refusing to overwrite')
    if not output.parent.is_dir():
        raise ValueError('Output parent does not exist')
    parent = output.parent.resolve()
    output = parent / output.name
    if seed is not None:
        seed = seed.resolve(strict=True)
        if output == seed or seed in output.parents or output in seed.parents:
            raise ValueError('Output and source cache must not overlap')
        git(seed, 'rev-parse', '--is-inside-work-tree')
        if git(seed, 'status', '--porcelain', '--untracked-files=normal'):
            raise ValueError('Seed checkout must be clean')
    tmp = Path(tempfile.mkdtemp(prefix='.' + output.name + '.api-', dir=parent))
    try:
        if seed:
            # Reuse existing Git blobs, but never alter the source cache.
            git(tmp.parent, 'clone', '-q', '--no-hardlinks', '--no-checkout', str(seed), str(tmp))
        else:
            git(tmp, 'init', '-q')
        downloaded = fetch_missing(api, repo, tmp, entries)
        git(tmp, 'read-tree', '--empty')
        index = ''.join(f"{e['mode']} {e['sha']}\t{e['path']}\n" for e in entries).encode()
        git(tmp, 'update-index', '--index-info', data=index)
        actual_tree = git(tmp, 'write-tree')
        if actual_tree != tree_sha:
            raise ValueError(f'Exact Git tree mismatch: reconstructed {actual_tree}, wanted {tree_sha}')
        if git(tmp, 'hash-object', '-t', 'commit', '-w', '--stdin', data=raw_commit) != commit_sha:
            raise ValueError('Git commit import identity mismatch')
        # Exact HEAD is a shallow boundary: full ancestral history is neither
        # available nor silently invented by the API checkout.
        shallow_path = Path(git(tmp, 'rev-parse', '--git-path', 'shallow'))
        if not shallow_path.is_absolute():
            shallow_path = tmp / shallow_path
        shallow_path.write_text(commit_sha + '\n')
        git(tmp, 'update-ref', 'refs/heads/github-api-nightly', commit_sha)
        git(tmp, 'checkout', '-q', '-f', 'github-api-nightly')
        if (git(tmp, 'rev-parse', 'HEAD') != commit_sha or
                git(tmp, 'rev-parse', 'HEAD^{tree}') != tree_sha or
                git(tmp, 'status', '--porcelain', '--untracked-files=normal')):
            raise ValueError('Final checkout identity/cleanliness verification failed')
        if len(git(tmp, 'ls-files', '-s').splitlines()) != len(entries):
            raise ValueError('Checkout index file count mismatch')
        tmp.rename(output)
    finally:
        if tmp.exists():
            shutil.rmtree(tmp)
    return {'status': 'EXACT_SOURCE_CHECKOUT', 'repo': repo, 'commit': commit_sha,
            'tree': tree_sha, 'files': len(entries), 'downloaded_missing_blobs': downloaded,
            'github_api_requests': api.read_count, 'output': str(output),
            'device_tested': False, 'firmware_built': False, 'installable': False}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', default='gobbajogger/X0X37-for-Loopy-Pro')
    parser.add_argument('--commit', required=True)
    parser.add_argument('--expected-tree')
    parser.add_argument('--verify-ref', help='ref whose current head must match --commit')
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--seed', type=Path, help='clean historical Git checkout as local blob cache')
    parser.add_argument('--api-base', default='https://api.github.com')
    a = parser.parse_args()
    api = API(a.api_base, os.getenv('GH_TOKEN') or os.getenv('GITHUB_TOKEN'))
    try:
        result = reconstruct(api, a.repo, a.commit, a.expected_tree, a.output, a.seed, a.verify_ref)
    except (ValueError, RuntimeError, KeyError, OSError, TypeError, json.JSONDecodeError) as e:
        print(json.dumps({'status': 'SOURCE_CHECKOUT_BLOCKED', 'reason': str(e),
                          'commit': a.commit, 'installable': False}))
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
