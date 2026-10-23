#!/usr/bin/env python3
"""Offline fake-GitHub REST tests for the exact-source materializer."""
from __future__ import annotations
import base64
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import threading
import unittest
from urllib.parse import unquote, urlsplit

MOD = Path(__file__).resolve().parents[1] / 'tools' / 'fetch_exact_source_api.py'
spec = importlib.util.spec_from_file_location('fetch_exact_source_api', MOD)
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


def git(root, *args, input=None, env=None):
    p = subprocess.run(['git', '-C', str(root), *args], input=input, env=env,
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if p.returncode:
        raise AssertionError(f'{args} failed: {p.stderr.decode()}')
    return p.stdout.decode().strip()


class Fixture:
    def __init__(self, path):
        self.root = path / 'repo'
        self.root.mkdir()
        git(self.root, 'init', '-q')
        git(self.root, 'config', 'user.name', 'Test Builder')
        git(self.root, 'config', 'user.email', 'builder@example.org')
        (self.root / 'README.md').write_text('base file\n')
        (self.root / 'obsolete.txt').write_text('remove me\n')
        (self.root / 'tools').mkdir()
        (self.root / 'tools' / 'hi.sh').write_text('#!/bin/sh\necho hi\n')
        os.chmod(self.root / 'tools' / 'hi.sh', 0o755)
        git(self.root, 'add', '-A')
        self.commit('initial')
        self.seed = path / 'seed'
        git(path, 'clone', '-q', str(self.root), str(self.seed))
        (self.root / 'README.md').write_text('new nightly source\n')
        (self.root / 'obsolete.txt').unlink()
        (self.root / 'link').symlink_to('tools/hi.sh')
        git(self.root, 'add', '-A')
        self.commit('nightly update')
        self.corrupt = None
        self.truncated = False
        self.moved = False
        self.invalid_path = False
        self.bad_tree = False
        self.blob_requests = []
        self.ref_calls = 0
        self.signature = None
        self.signed_payload = None

    def commit(self, message):
        env = os.environ.copy()
        env['GIT_AUTHOR_DATE'] = '2026-10-23T21:00:00 +1000'
        env['GIT_COMMITTER_DATE'] = '2026-10-23T21:00:00 +1000'
        git(self.root, 'commit', '-q', '-m', message, env=env)
        self.sha = git(self.root, 'rev-parse', 'HEAD')
        self.tree = git(self.root, 'rev-parse', 'HEAD^{tree}')

    def response(self, path):
        base = '/repos/gobbajogger/X0X37-for-Loopy-Pro/git/'
        assert path.startswith(base), path
        p = path[len(base):]
        if p == 'ref/heads/nightly':
            self.ref_calls += 1
            sha = 'f'*40 if self.moved and self.ref_calls == 2 else self.sha
            return dict(ref='refs/heads/nightly', object={'sha': sha})
        if p == 'commits/' + self.sha:
            raw = subprocess.check_output(['git', '-C', str(self.root), 'cat-file', 'commit', self.sha]).decode()
            author_line = next(x for x in raw.splitlines() if x.startswith('author '))
            committer_line = next(x for x in raw.splitlines() if x.startswith('committer '))
            def person(line):
                name, rest = line.split(' <', 1)
                email, tail = rest.split('> ', 1)
                sec, offset = tail.split(' ', 1)
                stamp = datetime.fromtimestamp(int(sec), tz=timezone.utc).isoformat().replace('+00:00', 'Z')
                return {'name': name.split(' ', 1)[1], 'email': email, 'date': stamp}
            return dict(sha=self.sha, tree={'sha': self.tree},
                        parents=[{'sha': x} for x in git(self.root, 'rev-list', '--parents', '-n', '1', self.sha).split()[1:]],
                        author=person(author_line), committer=person(committer_line),
                        message=raw.split('\n\n', 1)[1],
                        verification={'signature': self.signature, 'payload': self.signed_payload})
        if p.startswith('trees/'):
            entries = []
            for line in git(self.root, 'ls-tree', '-r', 'HEAD').splitlines():
                header, name = line.split('\t', 1)
                mode, kind, sha = header.split()
                entries.append(dict(mode=mode, type=kind, sha=sha, path=name))
            if self.invalid_path:
                entries[0]['path'] = '../escaped'
            return dict(sha='f'*40 if self.bad_tree else self.tree, truncated=self.truncated, tree=entries)
        if p.startswith('blobs/'):
            sha = p.split('/')[-1]
            self.blob_requests.append(sha)
            content = subprocess.check_output(['git', '-C', str(self.root), 'cat-file', 'blob', sha])
            if self.corrupt == sha:
                content += b'bad'
            return dict(sha=sha, content=base64.b64encode(content).decode(), encoding='base64', size=len(content))
        raise AssertionError(f'Unexpected API {p}')


class APIHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        try:
            data = self.server.fixture.response(unquote(urlsplit(self.path).path))
            body = json.dumps(data).encode()
            self.send_response(200)
        except Exception as exc:
            body = json.dumps({'error': str(exc)}).encode()
            self.send_response(404)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, *args):
        pass


class GitHubSourceTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.base = Path(self.tmp.name)
        self.fx = Fixture(self.base)
        self.http = ThreadingHTTPServer(('127.0.0.1', 0), APIHandler)
        self.http.fixture = self.fx
        self.thread = threading.Thread(target=self.http.serve_forever, daemon=True)
        self.thread.start()
        self.addCleanup(self.http.server_close)
        self.addCleanup(self.http.shutdown)
        self.dest = self.base / 'out'

    def run_fetch(self, **kwargs):
        return m.materialize('gobbajogger/X0X37-for-Loopy-Pro', 'nightly', self.fx.sha,
                             self.dest, api_root=f'http://127.0.0.1:{self.http.server_port}', **kwargs)

    def test_unsigned_exact_sha_tree_checkout(self):
        result = self.run_fetch()
        self.assertEqual(result['commit'], self.fx.sha)
        self.assertEqual(result['tree'], self.fx.tree)
        self.assertEqual(git(self.dest, 'rev-parse', 'HEAD'), self.fx.sha)
        self.assertEqual(git(self.dest, 'status', '--porcelain'), '')
        self.assertEqual((self.dest / 'README.md').read_text(), 'new nightly source\n')
        self.assertFalse((self.dest / 'obsolete.txt').exists())
        self.assertTrue((self.dest / 'tools' / 'hi.sh').stat().st_mode & 0o111)
        self.assertEqual(os.readlink(self.dest / 'link'), 'tools/hi.sh')

    def test_seed_fetches_only_changed_objects(self):
        out = self.run_fetch(seed_git=self.fx.seed)
        self.assertEqual(out['fetched_blobs'], 2)
        self.assertEqual(len(self.fx.blob_requests), 2)

    def test_seed_bundle_fetches_only_changed_objects(self):
        git(self.fx.seed, 'branch', 'source-snapshot')
        bundle = self.base / 'source-cache.bundle'
        git(self.fx.seed, 'bundle', 'create', str(bundle), 'source-snapshot')
        out = self.run_fetch(seed_bundle=bundle)
        self.assertEqual(out['fetched_blobs'], 2)
        self.assertEqual(git(self.dest, 'rev-parse', 'HEAD^{tree}'), self.fx.tree)

    def test_bad_signed_payload_rejected(self):
        self.fx.signed_payload = 'tree ' + self.fx.tree + '\n\nchanged body\n'
        self.fx.signature = 'faked sig'
        with self.assertRaisesRegex(m.SourceError, 'hash mismatch'):
            self.run_fetch()
        self.assertFalse(self.dest.exists())

    def test_unsafe_symlink_escape_rejected(self):
        link = self.fx.root / 'link'
        link.unlink()
        link.symlink_to('../../outside')
        git(self.fx.root, 'add', '-A')
        self.fx.commit('bad symlink')
        with self.assertRaisesRegex(m.SourceError, 'Symlink escapes'):
            self.run_fetch()
        self.assertFalse(self.dest.exists())

    def test_existing_destination_refused(self):
        self.dest.mkdir()
        with self.assertRaisesRegex(m.SourceError, 'exists'):
            self.run_fetch()

    def test_remote_head_moved_refused(self):
        self.fx.moved = True
        with self.assertRaisesRegex(m.SourceError, 'advanced'):
            self.run_fetch()
        self.assertFalse(self.dest.exists())

    def test_truncated_tree_refused(self):
        self.fx.truncated = True
        with self.assertRaisesRegex(m.SourceError, 'truncated'):
            self.run_fetch()
        self.assertFalse(self.dest.exists())

    def test_corrupt_blob_refused(self):
        self.fx.corrupt = git(self.fx.root, 'rev-parse', 'HEAD:README.md')
        with self.assertRaisesRegex(m.SourceError, 'hash/size'):
            self.run_fetch()
        self.assertFalse(self.dest.exists())

    def test_untrusted_path_refused(self):
        self.fx.invalid_path = True
        with self.assertRaisesRegex(m.SourceError, 'Unsafe'):
            self.run_fetch()
        self.assertFalse(self.dest.exists())

    def test_tree_sha_refused(self):
        self.fx.bad_tree = True
        with self.assertRaisesRegex(m.SourceError, 'mismatched/truncated'):
            self.run_fetch()

    def test_signed_payload_reconstructed(self):
        old = self.fx.sha
        payload = subprocess.check_output(['git', '-C', str(self.fx.root), 'cat-file', 'commit', old])
        sig = '-----BEGIN PGP SIGNATURE-----\n\nnot-a-real-key\n-----END PGP SIGNATURE-----\n'
        h,b = payload.split(b'\n\n', 1)
        lines = sig.rstrip('\n').split('\n')
        signed = h + b'\ngpgsig ' + lines[0].encode() + b''.join(b'\n ' + x.encode() for x in lines[1:]) + b'\n\n' + b
        commit = subprocess.check_output(['git', '-C', str(self.fx.root), 'hash-object', '-t', 'commit', '-w', '--stdin'],input=signed).decode().strip()
        git(self.fx.root, 'update-ref', 'refs/heads/master', commit)
        self.fx.sha = commit
        self.fx.signature = sig
        self.fx.signed_payload = payload.decode()
        got = self.run_fetch()
        self.assertEqual(got['commit'], commit)
        self.assertEqual(git(self.dest, 'rev-parse', 'HEAD'), commit)


if __name__ == '__main__':
    unittest.main()
