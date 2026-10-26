#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Private Git and executable fixtures for scope routing and actual test reports."""
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch

TOP = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location('quickcheck_plan', TOP / 'tools/quickcheck_plan.py')
PLAN = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PLAN)


class QuickcheckPlan(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.repo = self.root / 'repo'
        self.repo.mkdir()
        self.env = {k: v for k, v in os.environ.items() if not k.startswith(('X0X_', 'GIT_'))}
        self.env['TMPDIR'] = str(self.root)
        self.runner = TOP / 'tools/quickcheck.sh'
        self.git('init', '-q', '-b', 'integration')
        self.git('config', 'user.name', 'test')
        self.git('config', 'user.email', 'test@example.invalid')
        self.put('.gitignore', 'build/\n')
        self.put('docs/start.md', '# Start\n')
        self.put('firmware/source.c', 'old\n')
        self.put('web/page.js', '// old\n')
        self.put('host/probe.c', '// old\n')
        self.put('tests/host/probe.c', '// old\n')
        self.put('tests/run_tests.sh', '''#!/bin/sh
run() {
 case " $X0X_TESTS " in *" $1 "*) ;; *) return ;; esac
 echo "  ok   $1"
 echo "$1" >> build/ran
}
run fastmath sh tests/host/probe.c
run breaks-web sh web/page.js
run host-build sh host/probe.c
''')
        self.put('build.sh', '#!/bin/sh\necho built >> build/invoked\n')
        (self.repo / 'build.sh').chmod(0o755)
        self.git('add', '.')
        self.git('commit', '-qm', 'Initial fixture.')
        self.base = self.git('rev-parse', 'HEAD')
        self.env['X0X_QC_BASE'] = self.base
        # A mock uname permits orchestration tests on macOS. No target compiler runs.
        self.bin = self.root / 'bin'
        self.bin.mkdir()
        (self.bin / 'uname').write_text('#!/bin/sh\necho Linux\n')
        (self.bin / 'uname').chmod(0o755)
        self.env['PATH'] = str(self.bin) + ':' + self.env['PATH']

    def put(self, path, content):
        p = self.repo / path
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(content)

    def git(self, *args):
        p = subprocess.run(['git', *args], cwd=self.repo, env=self.env, capture_output=True, text=True)
        self.assertEqual(p.returncode, 0, p.stderr)
        return p.stdout.strip()

    def qc(self, success=True):
        p = subprocess.run(['bash', str(self.runner), str(self.repo)],
                           env=self.env, capture_output=True, text=True)
        self.assertEqual(p.returncode, 0 if success else 1, p.stdout + p.stderr)
        return p

    def plan(self):
        return json.loads((self.repo / 'build/qc/plan.json').read_text())

    def test_non_target_routes_run_relevant_tests(self):
        for path, selected in [('web/page.js', 'breaks-web'), ('host/probe.c', 'host-build'),
                               ('tests/host/probe.c', 'fastmath')]:
            with self.subTest(path=path):
                self.put(path, 'changed\n')
                self.qc()
                self.assertFalse(self.plan()['target'])
                self.assertIn(selected, (self.repo / 'build/ran').read_text().splitlines())
                self.assertFalse((self.repo / 'build/invoked').exists())
                self.git('checkout', '--', path)

    def test_docs_skip_target_tests(self):
        self.put('docs/start.md', 'This should be changed; it was written poorly.\n')
        p = self.qc()
        self.assertEqual(self.plan()['tests'], [])
        self.assertFalse((self.repo / 'build/invoked').exists())

    def test_firmware_build_unknown_mixed_empty_and_odd_names(self):
        for paths in [[], ['firmware/source.c'], ['build.sh'], ['unknown.json'],
                      ['docs/start.md', 'firmware/source.c'], ['docs/odd name.md'],
                      ['docs/line\nbreak.md'], ['firmware/readme.md'], ['assets/foo.raw']]:
            with self.subTest(paths=paths):
                for path in paths:
                    self.put(path, (self.repo / path).read_text() + '# change\n' if path == 'build.sh' else 'change\n')
                (self.repo / 'build.sh').chmod(0o755)
                self.qc()
                self.assertTrue(self.plan()['target'])
                self.assertTrue((self.repo / 'build/invoked').exists())
                (self.repo / 'build/invoked').unlink()
                self.git('reset', '--hard', self.base)
                self.git('clean', '-fd')

    def test_deleted_and_renamed_firmware_cannot_hide_in_docs(self):
        self.git('mv', 'firmware/source.c', 'docs/source.md')
        self.qc()
        self.assertTrue(self.plan()['target'])
        self.assertIn('firmware/source.c', self.plan()['changed'])
        self.assertIn('docs/source.md', self.plan()['changed'])

    def test_untracked_staged_unstaged_and_ignored(self):
        self.put('docs/start.md', 'staged\n'); self.git('add', 'docs/start.md')
        self.put('web/page.js', 'unstaged\n')
        self.put('firmware/new.c', 'untracked\n')
        self.put('build/ignored.c', 'ignored\n')
        self.qc()
        self.assertEqual(self.plan()['changed'], ['docs/start.md', 'firmware/new.c', 'web/page.js'])
        self.assertTrue(self.plan()['target'])

    def test_base_and_diff_errors_fail_before_build(self):
        self.env['X0X_QC_BASE'] = 'missing-ref'
        self.assertIn('scope:', self.qc(False).stdout)
        self.env['X0X_QC_BASE'] = self.base
        real_git = shutil.which('git')
        self.put('docs/start.md', 'changed\n')
        for failed_command in ('for-each-ref', 'diff', 'rev-list', 'ls-files'):
            with self.subTest(failed_command=failed_command):
                (self.bin / 'git').write_text('#!/bin/sh\nfor arg do [ "$arg" != ' + failed_command + ' ] || exit 77; done\nexec "' + real_git + '" "$@"\n')
                (self.bin / 'git').chmod(0o755)
                self.assertIn('scope:', self.qc(False).stdout)
        self.assertFalse((self.repo / 'build/invoked').exists())

    def test_mixed_docs_and_web_scope_cannot_hide_missing_tests(self):
        self.put('docs/start.md', 'changed\n')
        self.put('web/page.js', 'changed\n')
        (self.repo / 'tests/run_tests.sh').unlink()
        p = self.qc(False)
        self.assertIn('no runnable tests selected', p.stdout)
        self.assertFalse(self.plan()['target'])
        self.assertTrue(self.plan()['checks'])

    def test_missing_or_failed_test_reports_fail(self):
        for runner in ['exit 0\n', 'echo "  FAIL fastmath"\nexit 1\n']:
            with self.subTest(runner=runner):
                self.put('tests/run_tests.sh', '# X0X_TESTS\n' + runner)
                # Preserve a selectable command, but do not execute/report it.
                self.put('tests/run_tests.sh', (self.repo / 'tests/run_tests.sh').read_text() + 'run fastmath true\n')
                self.put('web/page.js', 'changed\n')
                self.assertIn('tests:', self.qc(False).stdout)

    def test_scenarios_require_upload_only_for_actual_or_unknown_inputs(self):
        runner = self.repo / 'tests/run_tests.sh'
        runner.write_text(runner.read_text() + 'run upload-images true\n')
        for name in ('upload', 'upload44'):
            self.put('tests/scenarios/' + name + '.x0x', (TOP / ('tests/scenarios/' + name + '.x0x')).read_text())
        self.put('tests/scenarios/tour.x0x', 'wait 200\ntap PLAY\n')
        self.put('tests/scenarios/future.x0x', 'unknown_command input\n')
        for name in ('tour', 'upload', 'upload44', 'future'):
            with self.subTest(name=name):
                selected = PLAN.select(self.repo, ['tests/scenarios/' + name + '.x0x'], False)
                self.assertIn('host-build', selected)
                self.assertEqual('upload-images' in selected, name != 'tour')
        original = Path.read_text
        def unreadable(path, *args, **kwargs):
            if path.name == 'tour.x0x':
                raise PermissionError('fixture scenario unreadable')
            return original(path, *args, **kwargs)
        with patch.object(Path, 'read_text', unreadable):
            self.assertIn('upload-images', PLAN.select(self.repo, ['tests/scenarios/tour.x0x'], False))

    def test_test_index_reads_each_source_once(self):
        reads = []
        original = Path.read_text
        def read(path, *args, **kwargs):
            reads.append(path)
            return original(path, *args, **kwargs)
        with patch.object(Path, 'read_text', read):
            selected = PLAN.select(self.repo, ['web/page.js', 'host/probe.c', 'firmware/source.c'], True)
        self.assertIn('fastmath', selected)
        # Parsing and indexing share the same runner bytes.
        self.assertEqual(reads.count(self.repo / 'tests/host/probe.c'), 1)
        self.assertEqual(reads.count(self.repo / 'tests/run_tests.sh'), 1)

    def test_project_web_routes_select_project_suites(self):
        """BF-002: project modules must pull projects-web and projects-page."""
        runner = self.repo / 'tests/run_tests.sh'
        runner.write_text(runner.read_text() + '''
run projects-web node web/test_projects.mjs
run projects-page node web/test_projects_page.mjs
run breaks-page sh web/page.js
run install-slots-web true
''')
        self.put('web/x0x_projects.js', '// projects\n')
        self.put('web/x0x_projects.html', '<html></html>\n')
        self.put('web/x0x_breaks.js', '// shared\n')
        self.put('web/test_projects.mjs', '// project suite\n')
        self.put('web/test_projects_page.mjs', '// project page suite\n')
        project_paths = [
            'web/x0x_projects.js',
            'web/x0x_projects.html',
            'web/x0x_breaks.js',
            'web/test_projects.mjs',
            'web/test_projects_page.mjs',
        ]
        for path in project_paths:
            with self.subTest(path=path):
                selected = PLAN.select(self.repo, [path], False)
                if path.endswith('test_projects.mjs'):
                    self.assertIn('projects-web', selected)
                elif path.endswith('test_projects_page.mjs'):
                    self.assertIn('projects-page', selected)
                else:
                    self.assertIn('projects-web', selected)
                    self.assertIn('projects-page', selected)
        unrelated = PLAN.select(self.repo, ['web/page.js'], False)
        self.assertIn('breaks-web', unrelated)
        self.assertNotIn('projects-web', unrelated)
        self.assertNotIn('projects-page', unrelated)


if __name__ == '__main__':
    unittest.main()
