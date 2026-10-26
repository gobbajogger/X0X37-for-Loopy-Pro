#!/usr/bin/env python3
"""Scratch identity/reuse tests. No real builds, gate validation or live evidence."""
import copy
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch, PropertyMock

TOP = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(TOP/'tools'))
import validation_inputs as identity

class Inputs(unittest.TestCase):
    def setUp(self):
        parent = TOP/'build/input-fixtures'
        parent.mkdir(parents=True, exist_ok=True)
        self.tmp = tempfile.TemporaryDirectory(dir=parent)
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.repo = self.root/'repo'; self.repo.mkdir()
        self.git(self.repo, 'init', '-q', '-b', 'integration')
        (self.repo/'code.c').write_text('source')
        self.commit(self.repo)
        self.tc = self.root/'tc'; self.tc.mkdir(); (self.tc/'compiler').write_text('compiler v1')
        self.sdk = self.root/'sdk'; self.sdk.mkdir(); (self.sdk/'library').write_text('sdk v1')
        self.refs = self.root/'refs'; self.ref = self.refs/'reference'; self.ref.mkdir(parents=True)
        self.git(self.ref, 'init', '-q')
        (self.ref/'dsp.c').write_text('reference v1'); self.commit(self.ref)
        self.pins = 'reference='+self.git(self.ref, 'rev-parse', 'HEAD').strip()
        self.runner = self.root/'runner'; self.runner.mkdir()
        for name in ('gate.sh', 'gate_flashmap.py', 'gate_sigwrap.py', 'validation_inputs.py', 'pre-push.hook'):
            shutil.copy2(TOP/'tools'/name, self.runner/name)
        # The gate clears caller build selectors before capturing its inputs.
        # Keep baseline capture equivalent when dev_validate sets MODEL/BOARD.
        self.env = {k:v for k,v in os.environ.items()
                    if k not in ('MODEL', 'BOARD')
                    and not k.startswith(('X0X_', 'JIELI_', 'AC79_', 'GIT_'))}
        self.config = {'models': ['elite', 'pro'], 'configs': {'dev': 1, 'rel': 0}, 'time_scale': 1}
        self.env['X0X_TEST_TIME_SCALE'] = '1'

    def git(self, repo, *args):
        return subprocess.check_output(['git', '-C', str(repo), *args], text=True)

    def commit(self, repo):
        self.git(repo, 'add', '.')
        self.git(repo, '-c', 'user.name=t', '-c', 'user.email=t@t', 'commit', '-qm', 'fixture')

    def capture(self):
        return identity.capture(self.repo, 'HEAD', 'gate', self.config, self.tc, self.sdk,
                                self.refs, self.pins, env=self.env, runner_dir=self.runner)

    def test_identical_and_changes(self):
        baseline = copy.deepcopy(self.capture())
        identity.validate(baseline, self.capture())
        for path in (self.tc/'compiler', self.sdk/'library', self.ref/'dsp.c', self.runner/'gate.sh',
                     self.runner/'gate_flashmap.py', self.runner/'validation_inputs.py', self.runner/'pre-push.hook'):
            with self.subTest(path=path):
                old = path.read_bytes(); path.write_bytes(old+b'\nchanged')
                with self.assertRaisesRegex(ValueError, 'changed'):
                    identity.validate(baseline, self.capture())
                path.write_bytes(old)
        # Pre-migration FM-1-inclusive evidence cannot qualify for the SMK-only gate.
        self.config['models'].append('fm1')
        with self.assertRaisesRegex(ValueError, 'changed'):
            identity.validate(baseline, self.capture())
        self.config['models'].pop()
        self.config['configs']['rel'] = 1
        self.assertNotEqual(baseline['sha256'], self.capture()['sha256'])
        self.config['configs']['rel'] = 0
        self.env['CFLAGS'] = '-DCHANGED'
        self.assertNotEqual(baseline['sha256'], self.capture()['sha256'])
        self.env.pop('CFLAGS')
        (self.repo/'README.md').write_text('docs'); self.commit(self.repo)
        self.assertEqual(baseline['sha256'], self.capture()['sha256'])
        (self.repo/'code.c').write_text('new source'); self.commit(self.repo)
        self.assertNotEqual(baseline['sha256'], self.capture()['sha256'])
        with self.assertRaises(ValueError): identity.validate({})

    def test_installed_runner_name_preserves_identity(self):
        baseline = self.capture()
        installed = self.runner/'x0x37_gate.sh'
        (self.runner/'gate.sh').rename(installed)
        actual = identity.capture(self.repo, 'HEAD', 'gate', self.config, self.tc, self.sdk,
                                  self.refs, self.pins, env=self.env, runner_dir=self.runner,
                                  runner_path=installed)
        self.assertEqual(baseline['sha256'], actual['sha256'])

    def test_split_execution_preserves_all_original_input_bytes(self):
        baseline = self.capture()
        runtime = identity.runtime(self.config, self.env)
        split = identity.capture(self.repo, 'HEAD', 'gate', self.config, self.tc, self.sdk,
            self.refs, self.pins, env=self.env, runner_dir=self.runner, execution=runtime)
        self.assertEqual(baseline, split)
        (self.sdk/'library').write_text('different SDK with identical runtime')
        changed = identity.capture(self.repo, 'HEAD', 'gate', self.config, self.tc, self.sdk,
            self.refs, self.pins, env=self.env, runner_dir=self.runner, execution=runtime)
        with self.assertRaisesRegex(ValueError, 'changed'):
            identity.validate(baseline, changed)
        self.config['worker'] = {'image': 'sha256:first'}
        first = self.capture()
        self.config['worker']['image'] = 'sha256:second'
        self.assertNotEqual(first['sha256'], self.capture()['sha256'])

    def test_real_gate_reuse_routing_with_scratch_inputs(self):
        # Override uname only to reach the portable admission/reuse path on macOS.
        # The missing target compiler stops every non-reuse route before a build.
        fakebin = self.root/'bin'; fakebin.mkdir()
        (fakebin/'uname').write_text('#!/bin/sh\necho Linux\n'); (fakebin/'uname').chmod(0o755)
        self.env.update(PATH=str(fakebin)+os.pathsep+self.env['PATH'], X0X_REPO=str(self.repo),
            X0X_RESULTS=str(self.root/'results'), X0X_GATE_WORK=str(self.root/'work'),
            X0X_TOOLCHAIN=str(self.tc), AC79_SDK=str(self.sdk), X0X_REFS=str(self.refs),
            X0X_REF_PINS=self.pins, X0X_GATE_INNER='1')
        baseline = self.capture()
        tree = __import__('hashlib').sha1(self.git(self.repo, 'ls-tree', '-r', '--full-tree', 'HEAD').encode()).hexdigest()
        results = self.root/'results'; results.mkdir()
        record = results/(tree+'.md')
        def gate(*extra):
            return subprocess.run(['bash', str(self.runner/'gate.sh'), 'HEAD', '--purpose',
                'owner-validation', '--reason', 'Scratch routing regression', *extra], env=self.env,
                capture_output=True, text=True)
        for suffix in ('', '\n- Validation inputs: wrong'):
            record.write_text('Result: PASS\n- Config CRCs: fixture'+suffix+'\n')
            result = gate()
            self.assertNotEqual(result.returncode, 0, result.stdout+result.stderr)
            self.assertNotIn('result used again', result.stdout)
        record.write_text('Result: PASS\n- Config CRCs: fixture\n- Validation inputs: '+baseline['sha256']+'\n')
        logs = self.root/'logs'; logs.mkdir()
        with record.open('a') as stream:
            stream.write(f'- Worktree: {self.root}/source. Logs: {logs}.\n')
        self.assertNotEqual(gate().returncode, 0)  # identity line alone is insufficient
        (logs/'validation-inputs.json').write_text(json.dumps(baseline))
        result = gate()
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
        self.assertIn('result used again', result.stdout)
        for extra in (('--force', 'Verify force routing'), ('--trial',)):
            result = gate(*extra)
            self.assertNotEqual(result.returncode, 0)
            self.assertNotIn('result used again', result.stdout)
        (self.sdk/'library').write_text('changed SDK')
        self.assertNotEqual(gate().returncode, 0)
        self.assertFalse((self.root/'work').exists())

class PackageBytes(unittest.TestCase):
    def setUp(self):
        parent = TOP/'build/input-fixtures'
        parent.mkdir(parents=True, exist_ok=True)
        self.tmp = tempfile.TemporaryDirectory(dir=parent)
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.meta = self.root/'fixture.egg-info'
        self.meta.mkdir()
        (self.meta/'PKG-INFO').write_text('Name: fixture\nVersion: 1.0\n')
        (self.meta/'top_level.txt').write_text('fixture\n')
        self.pkg = self.root/'fixture'
        self.pkg.mkdir()
        (self.pkg/'__init__.py').write_text('source = 1\n')
        (self.pkg/'native.so').write_bytes(b'binary fixture')
        self.dist = importlib.metadata.Distribution.at(self.meta)

    def runtime(self):
        def distribution(name):
            if name == 'fixture':
                return self.dist
            raise importlib.metadata.PackageNotFoundError(name)
        with patch.object(identity.importlib.metadata, 'distribution', side_effect=distribution):
            return identity.runtime({}, {'X0X_VALIDATION_PYTHON_PACKAGES': 'fixture'})

    def test_distro_source_binary_and_added_file_mutations(self):
        self.assertIsNone(self.dist.files)
        baseline = self.runtime()
        self.assertEqual(baseline['python_packages']['fixture']['version'], '1.0')
        self.assertNotEqual(baseline['python_packages']['fixture']['sha256'], identity.digest(identity.encoded({})))
        for path in (self.pkg/'__init__.py', self.pkg/'native.so', self.pkg/'new.py'):
            with self.subTest(path=path.name):
                original = path.read_bytes() if path.exists() else None
                path.write_bytes(b'changed bytes')
                self.assertNotEqual(baseline, self.runtime())
                if original is None:
                    path.unlink()
                else:
                    path.write_bytes(original)
                self.assertEqual(baseline, self.runtime())
        cache = self.pkg/'__pycache__'
        cache.mkdir(); (cache/'output.pyc').write_bytes(b'output')
        (self.pkg/'output.pyo').write_bytes(b'output')
        self.assertEqual(baseline, self.runtime())

    def test_empty_manifest_uses_installed_modules(self):
        with patch.object(type(self.dist), 'files', new_callable=PropertyMock, return_value=[]):
            self.assertIn('fixture/native.so', identity.package_files(self.dist))

    def test_egg_info_console_script_outside_site_packages(self):
        # installed-files.txt paths are relative to the egg-info directory.
        # ../../../bin/fixture is the env console script, outside site-packages.
        env = self.root/'env'
        sp = env/'lib'/'site-packages'
        egg = sp/'fixture.egg-info'
        pkg = sp/'fixture'
        script = env/'bin'/'fixture'
        pkg.mkdir(parents=True)
        egg.mkdir(parents=True)
        script.parent.mkdir()
        (pkg/'__init__.py').write_text('source = 1\n')
        script.write_text('#!/bin/sh\necho fixture\n')
        (egg/'PKG-INFO').write_text('Name: fixture\nVersion: 1.0\n')
        (egg/'installed-files.txt').write_text('../fixture/__init__.py\n../../../bin/fixture\n')
        dist = importlib.metadata.Distribution.at(egg)
        files = identity.package_files(dist)
        module = 'fixture/__init__.py'
        upward = '../../bin/fixture'
        self.assertEqual(files[module], identity.file_hash(pkg/'__init__.py'))
        self.assertEqual(files[upward], identity.file_hash(script))
        self.assertEqual(set(files), {module, upward})
        script.write_text('#!/bin/sh\necho changed\n')
        self.assertNotEqual(identity.package_files(dist)[upward], files[upward])
        script.unlink()
        with self.assertRaises(OSError):
            identity.package_files(dist)

    def test_normal_manifest_preserves_all_declared_files(self):
        # RECORD can contain data outside the import package, including binaries.
        (self.root/'extra.data').write_bytes(b'data')
        (self.meta/'RECORD').write_text('fixture/__init__.py,,\nfixture/native.so,,\nextra.data,,\nfixture/output.pyc,,\n')
        expected = {p: identity.file_hash(self.root/p)
                    for p in ('fixture/__init__.py', 'fixture/native.so', 'extra.data')}
        self.assertEqual(identity.package_files(self.dist), expected)
        baseline = self.runtime()
        (self.root/'extra.data').write_bytes(b'changed')
        self.assertNotEqual(baseline, self.runtime())
        (self.pkg/'native.so').unlink()
        with self.assertRaises(OSError):
            self.runtime()

    def test_module_source_and_extension_without_importing(self):
        shutil.rmtree(self.pkg)
        (self.root/'fixture.py').write_text('raise RuntimeError("must not import")\n')
        extension = self.root/('fixture'+importlib.machinery.EXTENSION_SUFFIXES[0])
        extension.write_bytes(b'extension fixture')
        files = identity.package_files(self.dist)
        self.assertEqual(set(files), {'fixture.py', extension.name})

    def test_missing_unsafe_or_unresolvable_metadata_fails_closed(self):
        for declaration in ('', '../', '.', 'fixture missing', 'missing'):
            with self.subTest(declaration=declaration):
                (self.meta/'top_level.txt').write_text(declaration)
                with self.assertRaises(ValueError):
                    self.runtime()
        (self.meta/'top_level.txt').unlink()
        with self.assertRaises(ValueError):
            self.runtime()

    def test_symlink_escape_and_cycle_fail_closed(self):
        for target in (self.root.parent, self.pkg):
            link = self.pkg/'linked'
            link.symlink_to(target, target_is_directory=True)
            with self.assertRaises(ValueError):
                self.runtime()
            link.unlink()

    def test_empty_installed_package_fails_closed(self):
        for p in self.pkg.iterdir():
            p.unlink()
        with self.assertRaises(ValueError):
            self.runtime()
        (self.meta/'RECORD').write_text('fixture/output.pyc,,\n')
        with self.assertRaises(ValueError):
            self.runtime()

    def test_optional_absent_packages_remain_explicit(self):
        with patch.object(identity.importlib.metadata, 'distribution', side_effect=importlib.metadata.PackageNotFoundError):
            actual = identity.runtime({}, {'X0X_VALIDATION_PYTHON_PACKAGES': 'optional'})
        self.assertEqual(actual['python_packages'], {'numpy': None, 'optional': None, 'pillow': None})


class ContentWalk(unittest.TestCase):
    @staticmethod
    def original(path, ancestors=(), exclude=()):
        # Frozen pre-repair algorithm, to check the optimized traversal's contract.
        path = Path(path)
        actual = path.resolve(strict=True)
        if actual in ancestors:
            raise ValueError('dependency symlink cycle: ' + str(path))
        if path.is_file():
            return {'sha256': identity.file_hash(path), 'executable': bool(path.stat().st_mode & 0o111)}
        if not path.is_dir():
            raise ValueError('not a dependency file/directory: ' + str(path))
        return {'sha256': identity.digest(identity.encoded({p.name: ContentWalk.original(p, (*ancestors, actual), exclude)
                for p in sorted(path.iterdir()) if p.name not in ('.git', '__pycache__', *exclude)}))}

    def test_original_identity_and_rejections(self):
        parent = TOP/'build/input-fixtures'
        parent.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=parent) as tmp:
            root = Path(tmp)/'input'; root.mkdir()
            def equal(path=root, exclude=()):
                self.assertEqual(self.original(path, exclude=exclude), identity.content(path, exclude=exclude))
            equal()
            leaf = root/'leaf'; leaf.write_bytes(b'input'); equal(); equal(leaf)
            initial = identity.content(root)
            leaf.chmod(0o755); equal(); self.assertNotEqual(initial, identity.content(root))
            leaf.write_bytes(b'changed'); equal()
            sub = root/'nested'; sub.mkdir(); (sub/'data').write_bytes(b'bytes'); equal()
            (root/'alias').symlink_to(sub, target_is_directory=True); equal()
            (root/'file-alias').symlink_to(leaf); equal()
            # Exclusions apply at every depth, even to dangling links.
            for directory in (root, sub):
                for name in ('.git', '__pycache__', 'ignored'):
                    (directory/name).symlink_to(root/'absent')
            equal(exclude=('ignored',))
            alias = Path(tmp)/'root-alias'; alias.symlink_to(root, target_is_directory=True)
            equal(alias, ('ignored',))
            (sub/'cycle').symlink_to(root, target_is_directory=True)
            for function in (self.original, identity.content):
                with self.assertRaisesRegex(ValueError, 'symlink cycle'):
                    function(root, exclude=('ignored',))
            (sub/'cycle').unlink()
            (root/'dangling').symlink_to(root/'absent')
            for function in (self.original, identity.content):
                with self.assertRaises(FileNotFoundError):
                    function(root, exclude=('ignored',))
            (root/'dangling').unlink()
            os.mkfifo(root/'unsupported')
            for function in (self.original, identity.content):
                with self.assertRaisesRegex(ValueError, 'not a dependency'):
                    function(root, exclude=('ignored',))


if __name__ == '__main__':
    unittest.main()
