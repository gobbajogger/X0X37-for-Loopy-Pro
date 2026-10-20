#!/usr/bin/env python3
"""Independent Linux-native consumers and disposable Pillow-byte mutation tests."""
from pathlib import Path
import hashlib
import json
import sys
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tests.host.test_dev_validate import NativeLinuxValidation
from tests.host.test_validation_inputs import PackageBytes, ContentWalk


class InstalledMutation(NativeLinuxValidation):
    def mutation(self, module):
        imported = __import__(module, fromlist=['__file__'])
        path = Path(imported.__file__)
        original = path.read_bytes()
        before = hashlib.sha256(original).hexdigest()
        plan = json.loads(self.plan.read_text())
        plan['checks'][0]['argv'] = ['python3', '-B', '-c',
            'from pathlib import Path; import '+module+' as package; '
            'p=Path(package.__file__); p.write_bytes(p.read_bytes()+b"\\n# independent mutation\\n"); print(p)']
        self.plan.write_text(json.dumps(plan))
        try:
            result = self.run_validation()
            after = hashlib.sha256(path.read_bytes()).hexdigest()
            self.assertNotEqual(before, after)
            self.assertEqual(result.returncode, 3, result.stdout+result.stderr)
            self.assertIn('validation inputs changed', result.stderr)
            self.assertFalse((self.out/'record.json').exists())
            self.assertEqual(len(self.git('worktree', 'list').splitlines()), 1)
            print(json.dumps({'module': module, 'path': str(path), 'before': before,
                              'after': after, 'record_issued': False}), flush=True)
        finally:
            path.write_bytes(original)

    def test_source_byte_mutation(self):
        self.mutation('PIL')

    def test_extension_byte_mutation(self):
        self.mutation('PIL._imaging')


suite = unittest.TestSuite()
suite.addTests(unittest.defaultTestLoader.loadTestsFromTestCase(PackageBytes))
suite.addTests(unittest.defaultTestLoader.loadTestsFromTestCase(ContentWalk))
for name in ('test_success_without_gate_and_queue_enforced',
             'test_dependency_change_during_validation_issues_no_record',
             'test_unenumerable_python_package_issues_no_record'):
    suite.addTest(NativeLinuxValidation(name))
suite.addTest(InstalledMutation('test_source_byte_mutation'))
suite.addTest(InstalledMutation('test_extension_byte_mutation'))
result = unittest.TextTestRunner(verbosity=2).run(suite)
sys.exit(not result.wasSuccessful())
