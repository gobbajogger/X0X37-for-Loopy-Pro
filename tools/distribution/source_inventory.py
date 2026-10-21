#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Inspect all reachable source history before a private development transfer.

Policy follows the reviewed source-history publication exporter: no proprietary
SDK/toolchain payloads, firmware outputs, refs-oscuro, submodules or compiled
libraries/executables. This is not a general secret-redaction guarantee.
"""
import ast
import hashlib
import json
from pathlib import Path
import re
import subprocess

SAFE_BINARY_FIXTURES = {'tests/fixtures/v0102_store.bin', 'tests/fixtures/v1_store.bin'}
BAD_PATH = re.compile(
    r'(^|/)refs/(jieli|oscuro)(/|$)|refs-oscuro|'
    r'(^|/)ble/ctl/(lib|out)(/|$)|'
    r'(^|/)(?:sdk|ac79[-_]sdk|fw[-_]ac79[^/]*sdk|(?:jieli[-_])?toolchains?)(/|$)|'
    r'\.(?:a|o|elf|so|dll|dylib|exe|img|fwsc|boot)$', re.I)


def scan(repo, commit):
    repo = Path(repo)
    def git(*args):
        return subprocess.check_output(['git', '-C', str(repo), *args])
    commits = git('rev-list', commit).decode().splitlines()
    entries, policies, blobs = {}, set(), {}
    for rev in commits:
        for row in git('ls-tree', '-r', '-z', '--full-tree', rev).split(b'\0'):
            if not row:
                continue
            info, path = row.split(b'\t', 1)
            mode, kind, sha = info.decode('ascii').split()
            path = path.decode('utf-8')
            if kind != 'blob' or mode not in ('100644', '100755', '120000'):
                raise ValueError('unsupported source entry: ' + path)
            if BAD_PATH.search(path) or (path.lower().endswith('.bin') and path not in SAFE_BINARY_FIXTURES):
                raise ValueError('excluded reachable source path: ' + path)
            entries.setdefault(sha, set()).add(path)
            if path in ('ble/ctl/fetch_sdk.sh', 'tools/build.py'):
                policies.add((path, sha))
    pins = set()
    def blob(sha):
        if sha not in blobs:
            blobs[sha] = git('cat-file', 'blob', sha)
        return blobs[sha]
    for path, sha in policies:
        text = blob(sha).decode('utf-8')
        if path.endswith('fetch_sdk.sh'):
            pins.update(digest for digest, name in re.findall(r'^([0-9a-f]{64})\s+(\S+)$', text, re.M)
                        if not name.endswith('LICENSE'))
        else:
            for node in ast.parse(text).body:
                if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'SDK_SHA256' for t in node.targets):
                    values = ast.literal_eval(node.value)
                    if not isinstance(values, dict) or not all(isinstance(v, str) and re.fullmatch(r'[0-9a-f]{64}', v) for v in values.values()):
                        raise ValueError('unrecognized SDK pin policy')
                    pins.update(values.values())
    for sha, paths in entries.items():
        raw = blob(sha)
        if hashlib.sha256(raw).hexdigest() in pins:
            raise ValueError('pinned SDK content in reachable source history')
        if (raw.startswith((b'!<arch>\n', b'\x7fELF')) or
                (raw.startswith(b'MZ') and len(raw) > 64 and not paths <= SAFE_BINARY_FIXTURES)):
            raise ValueError('compiled content in reachable source history')
    identity = json.dumps({'commits': commits, 'blobs': sorted(entries)}, separators=(',', ':')).encode()
    return {'reachable_commits': len(commits), 'unique_blobs': len(entries),
            'sdk_pins_checked': len(pins), 'identity_sha256': hashlib.sha256(identity).hexdigest(),
            'scope': 'All locally available reachable history; shallow boundaries remain explicit.'}


def validate_links(root):
    root = Path(root).resolve()
    for path in root.rglob('*'):
        if path.is_symlink() and (not path.resolve().is_relative_to(root) or not path.exists()):
            raise ValueError('unsafe source symlink: ' + str(path.relative_to(root)))
