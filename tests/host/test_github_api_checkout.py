#!/usr/bin/env python3
"""Actual native Git + loopback GitHub REST regression suite; no hosted CI."""
from __future__ import annotations

import base64
from datetime import datetime, timezone
import hashlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import importlib.util
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import threading
import unittest
from unittest.mock import patch

S = Path(__file__).resolve().parents[2] / 'tools' / 'github_api_checkout.py'
spec = importlib.util.spec_from_file_location('checkout', S)
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


def g(repo, *args, data=None):
    p = subprocess.run(['git', '-C', str(repo), *args], input=data,
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if p.returncode:
        raise RuntimeError(p.stderr.decode())
    return p.stdout.decode().strip()


def fixture(root):
    r = root / 'remote'
    r.mkdir()
    g(r, 'init', '-q')
    g(r, 'config', 'user.name', 'Nightly Tester')
    g(r, 'config', 'user.email', 'api-checkout@example.invalid')
    # Hosts with user.signingkey / commit.gpgsign=true would otherwise SSH-sign
    # the fixture commit; unsigned timezone reconstruct needs a clean object.
    g(r, 'config', 'commit.gpgsign', 'false')
    (r / 'src').mkdir()
    (r / 'src/test.c').write_text('int x0x_nightly(void) { return 37; }\n')
    (r / 'src/runner.sh').write_text('#!/bin/sh\nexit 0\n')
    (r / 'src/runner.sh').chmod(0o755)
    (r / 'binary.dat').write_bytes(b'\x00\xff\xfe' + bytes(range(100)))
    (r / 'latest-link').symlink_to('src/runner.sh')
    g(r, 'add', '.')
    with patch.dict(os.environ, {'GIT_AUTHOR_DATE': '2026-10-23T12:30:00+10:00',
                                'GIT_COMMITTER_DATE': '2026-10-23T12:30:00+10:00'}):
        g(r, 'commit', '-qm', 'X0X37 checked API source')
    unsigned = g(r, 'rev-parse', 'HEAD')
    plain = subprocess.check_output(['git', '-C', str(r), 'cat-file', 'commit', unsigned])
    signature = '-----BEGIN PGP SIGNATURE-----\nmock test signature, not cryptographic\n-----END PGP SIGNATURE-----\n'
    headers, _, msg = plain.partition(b'\n\n')
    signed_bytes = headers + b'\ngpgsig ' + signature.replace('\n', '\n ').encode() + b'\n' + b'\n' + msg
    signed_sha = g(r, 'hash-object', '-w', '-t', 'commit', '--stdin', data=signed_bytes)
    tree = g(r, 'rev-parse', 'HEAD^{tree}')
    assert m.object_hash('commit', signed_bytes) == signed_sha
    entries = []
    for line in subprocess.check_output(['git', '-C', str(r), 'ls-tree', '-r', '-z', '-l', 'HEAD']).split(b'\0'):
        if not line:
            continue
        head, name = line.split(b'\t', 1)
        mode, typ, sha, size = head.decode().split()
        entries.append({'path': name.decode(), 'mode': mode, 'type': typ,
                        'sha': sha, 'size': int(size)})
    blobs = {}
    for e in entries:
        blobs[e['sha']] = subprocess.check_output(['git', '-C', str(r), 'cat-file', 'blob', e['sha']])
    author_match = re.search(r'^author (.*?) <(.*?)> (\d+) ([+-]\d{4})$', plain.decode().split('\n\n')[0], flags=re.M)
    committer_match = re.search(r'^committer (.*?) <(.*?)> (\d+) ([+-]\d{4})$', plain.decode().split('\n\n')[0], flags=re.M)
    def person(match):
        return {'name': match[1], 'email': match[2],
                'date': datetime.fromtimestamp(int(match[3]), timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ')}
    obj = {'sha': signed_sha, 'tree': {'sha': tree}, 'parents': [],
           'author': person(author_match), 'committer': person(committer_match),
           'message': msg.decode(),
           'verification': {'payload': plain.decode(), 'signature': signature}}
    return r, signed_sha, tree, entries, blobs, obj, unsigned, plain


class APIHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.server.requests.append(self.path)
        value = self.server.responder(self.path)
        if value is None:
            self.send_error(404)
            return
        raw = json.dumps(value).encode()
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(raw)))
        self.end_headers()
        self.wfile.write(raw)

    def log_message(self, *_):
        pass


class GithubAPICheckoutTests(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        (self.remote, self.sha, self.tree, self.entries,
         self.blobs, self.commit, self.unsigned, self.plain) = fixture(self.root)
        self.request_paths = []
        self.tree_response = {'sha': self.tree, 'tree': self.entries, 'truncated': False}
        self.contents = {f'/repos/test/repo/git/blobs/{sha}':
                         {'sha': sha, 'encoding': 'base64', 'size': len(data),
                          'content': base64.b64encode(data).decode()}
                         for sha, data in self.blobs.items()}
        self.contents[f'/repos/test/repo/git/commits/{self.sha}'] = self.commit
        self.contents[f'/repos/test/repo/git/trees/{self.tree}?recursive=1'] = self.tree_response
        self.contents['/repos/test/repo/branches/nightly'] = {'commit': {'sha': self.sha}}
        server = ThreadingHTTPServer(('127.0.0.1', 0), APIHandler)
        server.requests = self.request_paths
        server.responder = lambda path: self.contents.get(path)
        th = threading.Thread(target=server.serve_forever, daemon=True)
        th.start()
        self.addCleanup(server.server_close)
        self.addCleanup(server.shutdown)
        self.api = m.API(f'http://127.0.0.1:{server.server_port}')

    def run_checkout(self, **k):
        return m.reconstruct(self.api, 'test/repo', self.sha, self.tree,
                             k.pop('output', self.root / 'checkout'),
                             k.pop('seed', None), k.pop('verify_ref', 'nightly'))

    def test_signed_exact_remote_sha_tree_permissions_symlink_and_binary(self):
        result = self.run_checkout()
        dst = self.root / 'checkout'
        self.assertEqual(result['status'], 'EXACT_SOURCE_CHECKOUT')
        self.assertEqual(result['commit'], g(dst, 'rev-parse', 'HEAD'))
        self.assertEqual(result['tree'], g(dst, 'rev-parse', 'HEAD^{tree}'))
        self.assertEqual(g(dst, 'status', '--porcelain'), '')
        self.assertTrue((dst / 'src/runner.sh').stat().st_mode & 0o111)
        self.assertEqual((dst / 'latest-link').readlink().as_posix(), 'src/runner.sh')
        self.assertEqual((dst / 'binary.dat').read_bytes(), self.blobs[next(x['sha'] for x in self.entries if x['path']=='binary.dat')])
        self.assertEqual(result['downloaded_missing_blobs'], len(self.blobs))
        self.assertFalse(result['installable'])
        self.assertEqual((dst / '.git/shallow').read_text(), self.sha + '\n')

    def test_seed_cache_reuses_blobs_without_network_download(self):
        seed = self.root / 'seed'
        g(self.root, 'clone', '-q', str(self.remote), str(seed))
        self.assertEqual(g(seed, 'status', '--porcelain'), '')
        result = self.run_checkout(seed=seed)
        self.assertEqual(result['downloaded_missing_blobs'], 0)
        self.assertEqual(g(self.root / 'checkout', 'rev-parse', 'HEAD'), self.sha)
        self.assertEqual(g(seed, 'status', '--porcelain'), '')
        self.assertFalse(any('/git/blobs/' in x for x in self.request_paths))

    def test_unsigned_exact_commit_timezone_reconstruction(self):
        unsigned_commit = dict(self.commit, sha=self.unsigned, verification={'payload': None, 'signature': None})
        self.contents[f'/repos/test/repo/git/commits/{self.unsigned}'] = unsigned_commit
        self.contents['/repos/test/repo/branches/nightly'] = {'commit': {'sha': self.unsigned}}
        self.sha = self.unsigned
        got = self.run_checkout()
        self.assertEqual(g(self.root / 'checkout', 'rev-parse', 'HEAD'), self.unsigned)
        self.assertEqual(g(self.root / 'checkout', 'rev-parse', 'HEAD^{tree}'), got['tree'])

    def test_malicious_tree_path_rejected_before_output(self):
        self.tree_response['tree'][0] = dict(self.tree_response['tree'][0], path='../outside')
        with self.assertRaisesRegex(ValueError, 'Unsupported/unsafe'):
            self.run_checkout()
        self.assertFalse((self.root / 'checkout').exists())

    def test_truncated_tree_rejected_before_output(self):
        self.tree_response['truncated'] = True
        with self.assertRaisesRegex(ValueError, 'Incomplete/mismatched'):
            self.run_checkout()
        self.assertFalse((self.root / 'checkout').exists())

    def test_corrupt_blob_rejected_no_partial_output(self):
        first = self.entries[0]['sha']
        self.contents[f'/repos/test/repo/git/blobs/{first}']['content'] = base64.b64encode(b'tampered').decode()
        with self.assertRaisesRegex(ValueError, 'integrity mismatch'):
            self.run_checkout()
        self.assertFalse((self.root / 'checkout').exists())
        self.assertFalse(list(self.root.glob('.checkout.api-*')))

    def test_bad_remote_ref_fails_closed(self):
        self.contents['/repos/test/repo/branches/nightly'] = {'commit': {'sha': 'f' * 40}}
        with self.assertRaisesRegex(ValueError, 'Remote verification branch moved'):
            self.run_checkout()
        self.assertFalse((self.root / 'checkout').exists())

    def test_unsigned_payload_tamper_rejected(self):
        self.commit['verification']['payload'] += '\nchanged'
        with self.assertRaisesRegex(ValueError, 'does not recreate expected SHA'):
            self.run_checkout()
        self.assertFalse((self.root / 'checkout').exists())

    def test_refuse_existing_output_and_insecure_transport(self):
        (self.root / 'checkout').mkdir()
        with self.assertRaisesRegex(ValueError, 'already exists'):
            self.run_checkout()
        with self.assertRaisesRegex(ValueError, 'HTTPS'):
            m.API('http://api.github.com')
        with self.assertRaisesRegex(ValueError, 'Invalid GitHub API base'):
            m.API('https://user:pass@api.github.com')
        with self.assertRaisesRegex(ValueError, 'untrusted host'):
            m.API('https://attacker.example.org')

    def test_symlink_escaping_source_refused(self):
        e = next(x for x in self.entries if x['path'] == 'latest-link')
        content = b'../../outside'
        new_sha = m.object_hash('blob', content)
        self.contents[f'/repos/test/repo/git/blobs/{new_sha}'] = {
            'sha': new_sha, 'encoding': 'base64', 'size': len(content),
            'content': base64.b64encode(content).decode()}
        # Deliberately malicious tree entries are refused when blob target is read.
        self.tree_response['tree'] = [dict(x, sha=new_sha, size=len(content))
                                      if x['path'] == e['path'] else x for x in self.entries]
        with self.assertRaisesRegex(ValueError, 'outside source tree'):
            self.run_checkout()
        self.assertFalse((self.root / 'checkout').exists())


if __name__ == '__main__':
    unittest.main(verbosity=2)
