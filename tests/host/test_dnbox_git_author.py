#!/usr/bin/env python3
"""Check the DnBOX default-author and local-only Git contract with local fixtures.

The Git commands below operate on a temporary *local* bare repository only.
The fixture identity is not used for commits to the project repository.
"""
import argparse
import ast
import contextlib
import io
import os
import sys
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch

SOURCE = Path(os.environ.get('DNBOX_PUBLISH_SOURCE', 'tools/publish_nightly.py'))
METHODS = {'_destination', '_local_only_git_remote', '_remote_tip', '_overlay', '_push', 'publish_valid_nightly', 'main'}


def load_scoped_functions():
    source = SOURCE.read_text(encoding='utf-8')
    tree = ast.parse(source, filename=str(SOURCE))
    methods = [x for x in tree.body if isinstance(x, ast.FunctionDef) and x.name in METHODS]
    if {x.name for x in methods} != METHODS:
        raise AssertionError('one or more publisher Git functions missing')
    scope = {'os': os, 'Path': Path, 're': re, 'shutil': shutil,
             'argparse': argparse, 'sys': sys, '__doc__': '',
             'subprocess': subprocess, 'tempfile': tempfile,
             'FORBIDDEN': {'gh-pages', 'main', 'master', 'nightly'},
             'BRANCH': 'nightly-pages',
             'NightlyPublishError': type('NightlyPublishError', (ValueError,), {})}
    exec(compile(ast.Module(body=methods, type_ignores=[]), str(SOURCE), 'exec'), scope)
    return scope


class DefaultAuthorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.methods = load_scoped_functions()

    def setUp(self):
        tmp = tempfile.TemporaryDirectory(prefix='dnbox-default-author-')
        self.addCleanup(tmp.cleanup)
        self.root = Path(tmp.name)
        self.remote = self.root / 'fixture.git'
        self.site = self.root / 'site'
        self.site.mkdir()
        subprocess.run(['git', 'init', '-q', '--bare', str(self.remote)], check=True)
        cfg = self.root / 'fixture-gitconfig'
        cfg.write_text('[user]\n name = DnBOX Fixture Default\n email = fixture@example.invalid\n')
        env = os.environ.copy()
        self.addCleanup(lambda: (os.environ.clear(), os.environ.update(env)))
        os.environ.update({'HOME': str(self.root), 'GIT_CONFIG_GLOBAL': str(cfg), 'GIT_CONFIG_NOSYSTEM': '1'})
        for key in ('GIT_AUTHOR_NAME', 'GIT_AUTHOR_EMAIL', 'GIT_COMMITTER_NAME', 'GIT_COMMITTER_EMAIL'):
            os.environ.pop(key, None)

    def local_git(self, *args):
        return subprocess.check_output(['git', '--git-dir', str(self.remote), *args], text=True).strip()

    def test_default_author_and_fast_forward(self):
        (self.site / 'index.html').write_text('first')
        first = self.methods['_push'](self.site, str(self.remote), 'chat/fixture', None)
        self.assertEqual(self.local_git('show', '-s', '--format=%an <%ae>|%cn <%ce>', first),
                         'DnBOX Fixture Default <fixture@example.invalid>|DnBOX Fixture Default <fixture@example.invalid>')
        (self.site / 'index.html').write_text('second')
        second = self.methods['_push'](self.site, str(self.remote), 'chat/fixture', first)
        self.assertEqual(self.local_git('rev-parse', f'{second}^'), first)
        self.assertEqual(self.local_git('rev-parse', 'refs/heads/chat/fixture'), second)

    def test_network_git_refused_before_git_execution(self):
        for remote in ('https://github.com/gobbajogger/X0X37-for-Loopy-Pro.git',
                       'git@github.com:gobbajogger/X0X37-for-Loopy-Pro.git',
                       'file:///tmp/fixture.git', 'relative/fixture.git'):
            with self.subTest(remote=remote), patch.object(subprocess, 'run') as invoked:
                with self.assertRaisesRegex(self.methods['NightlyPublishError'], 'Git network transport disabled'):
                    self.methods['_push'](self.site, remote, 'chat/fixture', None)
                invoked.assert_not_called()

    def test_protected_targets_refused(self):
        for target in ('main', 'nightly', 'gh-pages', 'refs/heads/main'):
            with self.subTest(target=target):
                with self.assertRaises(self.methods['NightlyPublishError']):
                    self.methods['_push'](self.site, str(self.remote), target, None)

    def test_stale_remote_head_does_not_change_commit(self):
        (self.site / 'index.html').write_text('first')
        first = self.methods['_push'](self.site, str(self.remote), 'chat/fixture', None)
        with self.assertRaisesRegex(self.methods['NightlyPublishError'], 'remote handoff moved'):
            self.methods['_push'](self.site, str(self.remote), 'chat/fixture', None)
        self.assertEqual(self.local_git('rev-parse', 'refs/heads/chat/fixture'), first)

    def test_default_legacy_branch_refused(self):
        for branch in ('nightly-pages', 'refs/heads/nightly-pages', 'feature/other'):
            with self.subTest(branch=branch):
                with self.assertRaisesRegex(self.methods['NightlyPublishError'], 'wip/ or chat/'):
                    self.methods['_push'](self.site, str(self.remote), branch, None)

    def test_firmware_binaries_and_private_headers_refused_before_git(self):
        for filename in ('package.fwsc', 'ctl.img', 'plugin.vst3',
                         'module.component', 'state_codec_private.h',
                         'fx_tables.h', 'miami_factory_typed.h'):
            with self.subTest(filename=filename):
                payload = self.site / filename
                payload.write_bytes(b'binary\x00blob')
                try:
                    with patch.object(subprocess, 'run') as invoked:
                        with self.assertRaisesRegex(self.methods['NightlyPublishError'], 'binary or private'):
                            self.methods['_push'](self.site, str(self.remote), 'chat/fixture', None)
                        invoked.assert_not_called()
                finally:
                    payload.unlink()

    def test_non_utf8_payload_refused_before_git(self):
        payload = self.site / 'bad.txt'
        payload.write_bytes(b'\xff\xfe')
        with patch.object(subprocess, 'run') as invoked:
            with self.assertRaisesRegex(self.methods['NightlyPublishError'], 'UTF-8 text files only'):
                self.methods['_push'](self.site, str(self.remote), 'chat/fixture', None)
            invoked.assert_not_called()

    def test_all_legacy_push_entry_paths_fail_closed(self):
        for label, remote, branch in (
            ('default', str(self.remote), 'nightly-pages'),
            ('explicit-canonical', 'https://github.com/gobbajogger/X0X37-for-Loopy-Pro.git', 'chat/fixture'),
            ('explicit-other', 'https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly.git', 'chat/fixture'),
            ('local', str(self.remote), 'chat/fixture'),
        ):
            with self.subTest(label=label), patch.object(subprocess, 'run') as invoked:
                output = self.root / ('blocked-' + label)
                with self.assertRaisesRegex(self.methods['NightlyPublishError'], 'DnBOX legacy Git push disabled'):
                    self.methods['publish_valid_nightly'](
                        None, None, None, None, None, output, remote=remote, push=True, branch=branch)
                self.assertFalse(output.exists())
                invoked.assert_not_called()
        os.environ['X0X_NIGHTLY_REMOTE'] = str(self.remote)
        with patch.object(subprocess, 'run') as invoked:
            with self.assertRaisesRegex(self.methods['NightlyPublishError'], 'DnBOX legacy Git push disabled'):
                self.methods['publish_valid_nightly'](
                    None, None, None, None, None, self.root / 'blocked-env', push=True, branch='chat/fixture')
            invoked.assert_not_called()

    def test_cli_push_flags_return_error_without_git(self):
        required = ['--elite-record', str(self.root / 'elite.json'),
                    '--elite-fwsc', str(self.root / 'elite.fwsc'),
                    '--pro-record', str(self.root / 'pro.json'),
                    '--pro-fwsc', str(self.root / 'pro.fwsc'),
                    '--out', str(self.root / 'blocked-cli')]
        for flags in (['--push'], ['--push', '--remote', str(self.remote)],
                      ['--push', '--branch', 'chat/fixture',
                       '--remote', 'https://github.com/gobbajogger/X0X37-for-Loopy-Pro.git']):
            with self.subTest(flags=flags), patch.object(subprocess, 'run') as invoked:
                captured = io.StringIO()
                with contextlib.redirect_stderr(captured):
                    result = self.methods['main'](required + flags)
                self.assertEqual(result, 2)
                self.assertIn('DnBOX legacy Git push disabled', captured.getvalue())
                self.assertFalse((self.root / 'blocked-cli').exists())
                invoked.assert_not_called()

    def test_no_default_identity_does_not_forge_one(self):
        os.environ['GIT_CONFIG_GLOBAL'] = '/dev/null'
        (self.site / 'index.html').write_text('first')
        with self.assertRaises(subprocess.CalledProcessError):
            self.methods['_push'](self.site, str(self.remote), 'chat/fixture', None)
        with self.assertRaises(subprocess.CalledProcessError):
            self.local_git('rev-parse', 'refs/heads/chat/fixture')


if __name__ == '__main__':
    unittest.main(verbosity=2)
