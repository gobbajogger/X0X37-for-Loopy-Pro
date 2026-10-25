#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""local_proof_plan classifies evidence-only vs implement vs validate tips."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

TOP = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location('local_proof_plan', TOP / 'tools/local_proof_plan.py')
PLAN = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PLAN)

VERIFY_SPEC = importlib.util.spec_from_file_location(
    'verify_dev_checked_records', TOP / 'tools/verify_dev_checked_records.py')
VERIFY = importlib.util.module_from_spec(VERIFY_SPEC)
VERIFY_SPEC.loader.exec_module(VERIFY)


def sample_record(model='elite', commit='abc', tree='def', pool=8324):
    return {
        'schema': 2,
        'result': 'DEV CHECKED',
        'config': 'dev',
        'model': model,
        'commit': commit,
        'tree': tree,
        'controller': True,
        'budgets': {'pool_free': pool},
        'artifacts': {
            'package': {'sha256': 'a' * 64},
            'controller': {'sha256': 'b' * 64},
        },
        'validation_inputs': {'schema': 1, 'sha256': 'c' * 64, 'inputs': {}},
    }


class LocalProofPlan(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.repo = Path(self.tmp.name) / 'repo'
        self.repo.mkdir()
        self.env = {k: v for k, v in os.environ.items() if not k.startswith(('X0X_', 'GIT_'))}
        self.git('init', '-q', '-b', 'main')
        self.git('config', 'user.name', 'test')
        self.git('config', 'user.email', 'test@example.invalid')
        self.put('docs/rules/workflow.md', '# wf\n')
        self.put('firmware/src/seq/sequencer.c', 'old\n')
        self.put('tests/run_tests.sh', '''#!/bin/sh
run() { :; }
run sequencer true
run clk-lock true
run clock-handover true
run stage-handoff true
run nightly-publish true
run installer-targets true
run installer-digests true
run install-gate-guard true
run projects-web true
run projects-page true
run dev-validate true
''')
        self.git('add', '.')
        self.git('commit', '-qm', 'base')
        self.base = self.git('rev-parse', 'HEAD')

    def put(self, path, content):
        p = self.repo / path
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(content)

    def git(self, *args):
        p = subprocess.run(['git', *args], cwd=self.repo, env=self.env, capture_output=True, text=True)
        self.assertEqual(p.returncode, 0, p.stderr)
        return p.stdout.strip()

    def commit_all(self, msg):
        self.git('add', '.')
        self.git('commit', '-qm', msg)
        return self.git('rev-parse', 'HEAD')

    def test_evidence_only_skips_kitchen_sink_and_rebuild(self):
        self.put('evidence/f09-stage5/CHECKS.md', 'ok\n')
        self.put('evidence/f09-stage5/HANDOFF.md', 'id\n')
        head = self.commit_all('stage5 evidence')
        out = PLAN.plan(self.repo, self.base, head, 5, 'cursor/f09-stage5-fb90')
        self.assertEqual(out['kind'], 'merge-evidence')
        self.assertFalse(out['rebuild_packages'])
        self.assertIn('stage-handoff', out['x0x_tests'])
        self.assertIn('install-gate-guard', out['x0x_tests'])
        self.assertNotIn('nightly-publish', out['x0x_tests'])
        self.assertNotIn('projects-web', out['x0x_tests'])
        skip_text = ' '.join(s['what'] for s in out['skip'])
        self.assertIn('kitchen sink', skip_text.lower() or skip_text)
        self.assertTrue(any('test_web' in s['what'] for s in out['skip']))

    def test_implement_keeps_bf_host_selectors(self):
        self.put('firmware/src/seq/sequencer.c', 'fixed\n')
        self.put('tests/host/bugfinder_clock_handover_test.c', 'test\n')
        self.put('evidence/f09-stage2/CHECKS.md', 'ok\n')
        head = self.commit_all('bf005 implement')
        out = PLAN.plan(self.repo, self.base, head, 2, 'cursor/f09-stage2-fb90')
        self.assertEqual(out['kind'], 'implement')
        for name in ('sequencer', 'clk-lock', 'clock-handover'):
            self.assertIn(name, out['x0x_tests'])
        self.assertFalse(out['rebuild_packages'])
        self.assertNotIn('nightly-publish', out['x0x_tests'])

    def test_validate_evidence_verifies_records_not_rebuild(self):
        ev = 'evidence/f09-stage4'
        self.put(ev + '/elite/record.json', json.dumps(sample_record('elite')))
        self.put(ev + '/pro/record.json', json.dumps(sample_record('pro')))
        self.put(ev + '/CHECKS.md', 'DEV CHECKED\n')
        head = self.commit_all('stage4 evidence')
        out = PLAN.plan(self.repo, self.base, head, 4, 'cursor/f09-stage4-fb90')
        self.assertEqual(out['kind'], 'validate-evidence')
        self.assertFalse(out['rebuild_packages'])
        self.assertIn('dev-validate', out['x0x_tests'])
        self.assertTrue(any('verify_dev_checked_records' in v for v in out['verify']))
        self.assertTrue(any('rebuild' in s['what'].lower() for s in out['skip']))

    def test_cli_json(self):
        self.put('evidence/x-stage1/AUDIT.md', 'a\n')
        head = self.commit_all('audit')
        p = subprocess.run(
            ['python3', str(TOP / 'tools/local_proof_plan.py'),
             '--repo', str(self.repo), '--base', self.base, '--head', head,
             '--branch', 'cursor/x-stage1-fb90'],
            capture_output=True, text=True, env=self.env)
        self.assertEqual(p.returncode, 0, p.stderr)
        data = json.loads(p.stdout)
        self.assertEqual(data['kind'], 'audit-evidence')
        self.assertEqual(data['schema'], 1)


class VerifyDevCheckedRecords(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.ev = Path(self.tmp.name) / 'evidence' / 'stage4'
        (self.ev / 'elite').mkdir(parents=True)
        (self.ev / 'pro').mkdir(parents=True)

    def write(self, model, **kwargs):
        data = sample_record(model, **kwargs)
        path = self.ev / model / 'record.json'
        path.write_text(json.dumps(data))
        return path

    def test_pass_pair(self):
        self.write('elite', commit='c1', tree='t1', pool=9000)
        self.write('pro', commit='c1', tree='t1', pool=9000)
        self.assertEqual(VERIFY.main(['--evidence', str(self.ev), '--commit', 'c1', '--tree', 't1']), 0)

    def test_fail_low_pool(self):
        self.write('elite', pool=100)
        self.write('pro', pool=9000)
        self.assertEqual(VERIFY.main(['--evidence', str(self.ev)]), 1)

    def test_fail_not_dev_checked(self):
        path = self.write('elite')
        data = json.loads(path.read_text())
        data['result'] = 'FAIL'
        path.write_text(json.dumps(data))
        self.write('pro')
        self.assertEqual(VERIFY.main(['--evidence', str(self.ev)]), 1)


if __name__ == '__main__':
    unittest.main()
