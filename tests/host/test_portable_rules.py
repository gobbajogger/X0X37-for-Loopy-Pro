#!/usr/bin/env python3
"""Check portable validation behavior with private repositories and no hardware."""
import importlib.util
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

TOP = Path(__file__).resolve().parents[2]


def run(args, cwd, env=None):
    return subprocess.run(args, cwd=cwd, env=env, text=True, capture_output=True)


class PortableRules(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.repo = self.root / 'repo'
        self.repo.mkdir()
        self.env = {k: v for k, v in os.environ.items() if not k.startswith(('X0X_', 'GIT_'))}
        self.env.update(HOME=str(self.root), TMPDIR=str(self.root))
        self.git('init', '-q', '-b', 'integration')
        self.git('config', 'user.name', 'test')
        self.git('config', 'user.email', 'test@example.invalid')

    def git(self, *args):
        p = run(['git', *args], self.repo, self.env)
        self.assertEqual(p.returncode, 0, p.stderr)
        return p.stdout.strip()

    def commit(self):
        self.git('add', '.')
        return self.git('commit', '-qm', 'Add test source.')

    def test_local_repo_default_and_explicit_references(self):
        shutil.copytree(TOP / 'tools', self.repo / 'tools')
        (self.repo / 'build.sh').write_text('#!/bin/sh\nprintf "%s\\n" "$NINEW9_DIR" "$EIGHTW8_DIR" "$SCHWUNG303" "$TB3PO_REF" > build/refs.txt\nexit 1\n')
        (self.repo / 'build.sh').chmod(0o755)
        self.commit()
        self.env.update(X0X_QC_BASE='HEAD', X0X_QC_WORK=str(self.root / 'qc'),
                        NINEW9_DIR='/custom/9', EIGHTW8_DIR='/custom/8',
                        SCHWUNG303='/custom/303', TB3PO_REF='/custom/tb.c')
        p = run(['bash', 'tools/quickcheck.sh', 'HEAD'], self.repo, self.env)
        self.assertEqual(p.returncode, 1, p.stdout + p.stderr)
        self.assertIn('Elite build:', p.stdout)
        self.assertEqual((self.root / 'qc/wt/build/refs.txt').read_text().splitlines(),
                         ['/custom/9', '/custom/8', '/custom/303', '/custom/tb.c'])
        self.assertEqual(run(['git', 'remote', 'get-url', '--push', 'origin'],
                             self.root / 'qc/.repo', self.env).stdout.strip(),
                         'DISABLED-no-push-outside-box')

    def test_folder_docs_check_does_not_touch_shared_cache(self):
        (self.repo / 'source').write_text('private test\n')
        self.commit()
        self.env.update(X0X_QC_BASE='HEAD', X0X_QUICK_ONLY='ste',
                        X0X_QC_WORK='/proc/x0x37-must-not-be-created')
        p = run(['bash', str(TOP / 'tools/quickcheck.sh'), str(self.repo)], self.repo, self.env)
        self.assertEqual(p.returncode, 0, p.stdout + p.stderr)
        self.assertNotIn('mkdir:', p.stderr)
        self.assertIn('QUICK PASS', p.stdout)

    def test_installed_hook_uses_configured_local_runner(self):
        (self.repo / 'source').write_text('private local test\n')
        self.commit()
        remote = self.root / 'remote.git'
        p = run(['git', 'init', '-q', '--bare', str(remote)], self.repo, self.env)
        self.assertEqual(p.returncode, 0, p.stderr)
        runner = self.root / 'local-check'
        runner.write_text('#!/bin/sh\ncat > /dev/null\necho "QUICK PASS local check"\n')
        runner.chmod(0o755)
        shutil.copy(TOP / 'tools/pre-push.hook', self.repo / '.git/hooks/pre-push')
        (self.repo / '.git/hooks/pre-push').chmod(0o755)
        self.git('config', 'x0x37.allowedRemote', str(remote))
        self.git('config', 'x0x37.prepushCheck', str(runner))
        self.env['X0X_PREPUSH_ROOTS'] = str(self.root)
        p = run(['git', 'push', str(remote), 'integration'], self.repo, self.env)
        self.assertEqual(p.returncode, 0, p.stderr)
        runner.write_text('#!/bin/sh\ncat > /dev/null\nexit 0\n')
        (self.repo / 'source').write_text('changed\n')
        self.commit()
        p = run(['git', 'push', str(remote), 'integration'], self.repo, self.env)
        self.assertNotEqual(p.returncode, 0)
        self.assertIn('gave no result', p.stderr)

    def test_installed_gate_carries_prepush_guard(self):
        destination = self.root / 'gate'
        destination.mkdir()
        self.env.update(X0X_GATE_DEST=str(destination),
                        X0X_GATE_LOCK=str(self.root / 'gate.lock'), X0X_INSTALL_WAIT='0')
        p = run(['bash', str(TOP / 'tools/install_gate.sh')], self.repo, self.env)
        self.assertEqual(p.returncode, 0, p.stdout + p.stderr)
        self.assertEqual((destination / 'pre-push.hook').read_bytes(),
                         (TOP / 'tools/pre-push.hook').read_bytes())
        self.assertTrue(os.access(destination / 'pre-push.hook', os.X_OK))

    def test_preference_history_does_not_reactivate_old_rules(self):
        spec = importlib.util.spec_from_file_location('prefs', TOP / 'tools/prefs_lint.py')
        prefs = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(prefs)
        text = ('## Team rules\n- 2026-10-19 Use core rule 1 for required safety checks.\n'
                '## Historical preference record — preserved text\n'
                '## Team rules\n- 2026-10-16 Follow agent rule 999.\n')
        self.assertEqual(len(prefs.entries(text, prefs.SECTIONS)), 1)
        self.assertEqual(prefs.lint(text, {'core.md': '1. Use required safety checks.'})[0]['status'], 'OK')
        broken = text.replace('core rule 1', 'core rule 2')
        self.assertEqual(prefs.lint(broken, {'core.md': '1. Use required safety checks.'})[0]['status'], 'NO STEP')


if __name__ == '__main__':
    unittest.main()
