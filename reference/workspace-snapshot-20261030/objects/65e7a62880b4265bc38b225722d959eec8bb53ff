#!/usr/bin/env python3
"""Development evidence integration tests; only scratch repos and fake builds.
No target toolchain, live queue, gate index, or device is used.
"""
import importlib.util
import json
import os
import platform
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[2]
RUNNER = ROOT/'tools/dev_validate.py'
GUARD = ROOT/'tools/require_gate_pass.sh'


class DevelopmentValidation(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(dir=getattr(self, "fixture_parent", None))
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.repo = self.root/'repo'
        self.repo.mkdir()
        self.git('init', '-q')
        # Mock compilation with real .fwsc framing/CRCs and an unflashable fake
        # payload, so version assertions inspect embedded bytes, not filenames.
        fixture_build = '''#!/bin/sh
set -eu
case "$MODEL" in elite|pro) ;; *) exit 1 ;; esac
[ "$X0X_DEV_PADCAL" = 1 ]
version=""
while [ $# -gt 0 ]; do
  case "$1" in
    --release) version="$2"; shift 2 ;;
    *) shift ;;
  esac
done
mkdir -p build
python3 - "$version" "$MODEL" <<'PYMOCK'
from pathlib import Path
import sys
sys.path.insert(0, __TOOL_DIR__)
from fm1pkg_make import ufw
version, model = sys.argv[1:]
model_name = {'pro': 'SMK-37 Pro', 'elite': 'SMK-37 Elite'}[model]
product = model_name + '_900'
filename = 'x0x-smk37' + ('-elite' if model == 'elite' else '')
if version:
    major, minor, patch = (int(s) for s in version.split('.'))
    product = model_name + '_9' + f'{major:02d}{minor:02d}{patch:02d}'
    filename += '-' + version
Path('build', filename + '.fwsc').write_bytes(
    ufw(b'\x00' * 1024, b'FELUCCA-LOADER-1' + b'\x00' * 128, product))
PYMOCK
echo 'app code area 100 B, 40 B free'
echo 'RAM .data+.bss 40 B of 100; pool 100 B of 10000'
'''
        (self.repo/'build.sh').write_text(fixture_build.replace('__TOOL_DIR__', repr(str(ROOT/'tools'))))
        (self.repo/'.gitignore').write_text('build/\nble/ctl/out/\n')
        (self.repo/'test.py').write_text('print("affected test passed")\n')
        self.git('add', '.')
        self.git('-c', 'user.name=t', '-c', 'user.email=t@t', 'commit', '-qm', 'fixture')
        self.commit = self.git('rev-parse', 'HEAD')
        self.plan = self.root/'plan.json'
        self.plan.write_text(json.dumps({'base': self.commit, 'scope': 'fixture behavior',
                                        'checks': [{'reason': 'changed fixture', 'argv': [sys.executable, 'test.py']}]}))
        self.out = self.root/'result'
        self.queue = self.root/'QUEUE.md'
        self.queue.write_text('')
        self.env = os.environ.copy()
        self.tc = self.root/'toolchain'
        self.sdk = self.root/'sdk'
        self.tc.mkdir(); self.sdk.mkdir()
        (self.tc/'compiler').write_text('fixture compiler')
        (self.sdk/'library').write_text('fixture SDK')
        self.env.update(JIELI_TOOLCHAIN=str(self.tc), AC79_SDK=str(self.sdk))
        self.env.update(X0X_REPO=str(self.repo), X0X_QUEUE_FILE=str(self.queue),
                        X0X_RESULTS_DIR=str(self.root/'no-gate-results'))

    def git(self, *args):
        return subprocess.check_output(['git', '-C', str(self.repo), *args], text=True).strip()

    def run_validation(self, controller=False, timeout=1800, version=None, model='elite'):
        launcher = 'import runpy,sys; sys.path.insert(0,str(__import__("pathlib").Path(sys.argv[1]).parent)); from unittest.mock import patch; sys.argv=sys.argv[1:]; a=patch("platform.system",return_value="Linux"); b=patch("platform.machine",return_value="x86_64"); a.start(); b.start(); runpy.run_path(sys.argv[0],run_name="__main__")'
        return subprocess.run([sys.executable, '-c', launcher, str(RUNNER), 'run', '--repo', str(self.repo),
                               '--commit', self.commit, '--model', model, '--plan', str(self.plan),
                               '--output', str(self.out), '--timeout', str(timeout)]
                              + (['--firmware-version', version] if version is not None else [])
                              + ([] if controller else ['--no-controller']),
                              capture_output=True, text=True, env=self.env)

    def guard(self, *args):
        return subprocess.run(['bash', str(GUARD), self.commit, '--fwsc', str(self.out/'x0x-smk37-elite.fwsc'),
                               '--dev-record', str(self.out/'record.json'), *args], env=self.env,
                              capture_output=True, text=True)

    def ready(self):
        r = self.run_validation()
        self.assertEqual(r.returncode, 0, r.stdout+r.stderr)

    def grant(self, version):
        sys.path.insert(0, str(ROOT/'tools'))
        from release_alloc import empty_ledger
        tree = self.git('rev-parse', self.commit + '^{tree}')
        path = self.repo/'releases'/'openbeta'/'ledger.json'
        ledger = empty_ledger()
        if path.is_file():
            ledger = json.loads(path.read_text())
        if not any(item.get('version') == version for item in ledger['allocations']):
            major, minor, patch = (int(part) for part in version.split('.'))
            ledger['allocations'].append({
                'version': version, 'major': major, 'minor': minor, 'patch': patch,
                'display': version + '_openbeta', 'source_commit': self.commit, 'source_tree': tree,
                'immutable': False, 'models': {},
            })
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(json.dumps(ledger, indent=2) + '\n')

    def test_unallocated_firmware_version_is_refused(self):
        result = self.run_validation(version='0.7.2')
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('not allocated', result.stderr)
        self.assertFalse((self.out/'record.json').exists())

    def test_firmware_version_is_stamped_into_dev_package_and_verified(self):
        self.grant('0.7.2')
        result = self.run_validation(version='0.7.2')
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
        record = self.out/'record.json'
        data = json.loads(record.read_text())
        self.assertEqual(data['firmware_version'], '0.7.2')
        self.assertEqual(data['firmware']['version_label'], 'v0.7.2')
        self.assertEqual(data['firmware']['product'], 'SMK-37 Elite_9000702')
        self.assertEqual(data['firmware']['release_display'], '0.7.2_openbeta')
        self.assertNotIn('openbeta', data['firmware']['product'])
        self.assertFalse(data['firmware']['device_tested'])
        self.assertFalse(data['firmware']['installable'])
        self.assertIn('DEVICE NOT TESTED', data['firmware']['display_label'])
        pkg = self.out/'x0x-smk37-elite-0.7.2.fwsc'
        verify_args = [sys.executable, str(RUNNER), 'verify', '--repo', str(self.repo),
                       '--commit', self.commit, '--record', str(record), '--fwsc', str(pkg)]
        ok = subprocess.run(verify_args, capture_output=True, text=True, env=self.env)
        self.assertEqual(ok.returncode, 0, ok.stdout+ok.stderr)
        original = record.read_text()
        data['firmware']['version_label'] = 'v9.9.9'
        record.write_text(json.dumps(data))
        tampered = subprocess.run(verify_args, capture_output=True, text=True, env=self.env)
        self.assertEqual(tampered.returncode, 3, tampered.stdout+tampered.stderr)
        record.write_text(original)

    def test_pro_package_has_independent_versioned_identity(self):
        self.grant('0.7.2')
        r = self.run_validation(version='0.7.2', model='pro')
        self.assertEqual(r.returncode, 0, r.stdout+r.stderr)
        record = json.loads((self.out/'record.json').read_text())
        self.assertEqual(record['firmware']['product'], 'SMK-37 Pro_9000702')
        self.assertEqual(record['firmware']['firmware_version'], '0.7.2')
        self.assertIn('SMK-37 Pro', record['firmware']['display_label'])
        self.assertTrue((self.out/'x0x-smk37-0.7.2.fwsc').exists())

    def test_noncanonical_version_refused_without_a_valid_record(self):
        for invalid in ('01.2.3', '0.7', '99.99.100', '1.2.3-rc1'):
            with self.subTest(version=invalid):
                result = self.run_validation(version=invalid)
                self.assertNotEqual(result.returncode, 0)
                self.assertFalse((self.out/'record.json').exists())
                if self.out.exists():
                    import shutil
                    shutil.rmtree(self.out)

    def test_success_without_gate_and_queue_enforced(self):
        self.ready()
        self.assertEqual(self.guard().returncode, 0)
        self.assertEqual(self.guard('--config', 'rel').returncode, 3)
        self.queue.write_text('- [ ] missing pending change\n')
        self.assertEqual(self.guard().returncode, 3)
        self.assertFalse((self.root/'no-gate-results').exists())

    def test_artifact_log_source_and_controller_mismatch(self):
        self.ready()
        pkg = self.out/'x0x-smk37-elite.fwsc'
        original = pkg.read_bytes()
        pkg.write_bytes(b'other')
        self.assertEqual(self.guard().returncode, 3)
        pkg.write_bytes(original)
        log = self.out/'check-0.log'
        old_log = log.read_bytes()
        log.write_text('changed')
        self.assertEqual(self.guard().returncode, 3)
        log.write_bytes(old_log)
        ctl = self.root/'ctl.img'
        ctl.write_bytes(b'unknown controller')
        self.assertEqual(self.guard('--ctl', str(ctl)).returncode, 3)
        (self.repo/'new.c').write_text('changed source')
        self.git('add', '.')
        self.git('-c', 'user.name=t', '-c', 'user.email=t@t', 'commit', '-qm', 'changed')
        self.commit = self.git('rev-parse', 'HEAD')
        self.assertEqual(self.guard().returncode, 3)

    def test_identity_required_and_transfer_does_not_need_build_dependencies(self):
        self.ready()
        record = self.out/'record.json'
        data = json.loads(record.read_text())
        saved = record.read_text()
        data.pop('validation_inputs')
        record.write_text(json.dumps(data))
        self.assertEqual(self.guard().returncode, 3)
        record.write_text(saved)
        self.env['JIELI_TOOLCHAIN'] = '/missing-on-installer'
        self.assertEqual(self.guard().returncode, 0)

    def test_dependency_change_during_validation_issues_no_record(self):
        plan = json.loads(self.plan.read_text())
        plan['checks'][0]['argv'] = [sys.executable, '-c',
            "from pathlib import Path; Path("+repr(str(self.sdk/'library'))+").write_text('changed SDK')"]
        self.plan.write_text(json.dumps(plan))
        result = self.run_validation()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('validation inputs changed', result.stderr)
        self.assertFalse((self.out/'record.json').exists())

    def test_unenumerable_python_package_issues_no_record(self):
        packages = self.root/'python-packages'
        metadata = packages/'uncovered.egg-info'
        metadata.mkdir(parents=True)
        (metadata/'PKG-INFO').write_text('Name: uncovered\nVersion: 1.0\n')
        self.env.update(PYTHONPATH=str(packages), X0X_VALIDATION_PYTHON_PACKAGES='uncovered')
        plan = json.loads(self.plan.read_text())
        plan['dependencies'] = [str(packages)]
        self.plan.write_text(json.dumps(plan))
        actual = self.run_validation()
        self.assertEqual(actual.returncode, 3, actual.stdout+actual.stderr)
        self.assertFalse((self.out/'record.json').exists())
        self.assertFalse((self.out/'build.log').exists())

    def test_docs_only_commit_reuses_evidence(self):
        self.ready()
        (self.repo/'README.md').write_text('documentation only')
        self.git('add', '.')
        self.git('-c', 'user.name=t', '-c', 'user.email=t@t', 'commit', '-qm', 'docs')
        self.commit = self.git('rev-parse', 'HEAD')
        self.assertEqual(self.guard().returncode, 0)

    def test_controller_selection_and_cache_isolation_request(self):
        ctl = self.repo/'ble/ctl'
        ctl.mkdir(parents=True)
        (ctl/'build_ctl.sh').write_text('printf "controller build\\n"\n[ "$CTL_NO_CACHE" = 1 ] || exit 1\nmkdir -p ble/ctl/out\nprintf controller > ble/ctl/out/ctl.img\n')
        self.git('add', '.')
        (ctl/'build_in_container.sh').write_text('# fixture for B=$(mktemp -d isolation contract\n')
        self.git('add', '.')
        self.git('-c', 'user.name=t', '-c', 'user.email=t@t', 'commit', '-qm', 'controller')
        self.commit = self.git('rev-parse', 'HEAD')
        result = self.run_validation(controller=True)
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
        self.assertEqual(self.guard().returncode, 3)
        image = self.out/'ctl.img'
        self.assertEqual(self.guard('--ctl', str(image)).returncode, 0)
        image.write_bytes(b'changed controller')
        self.assertEqual(self.guard('--ctl', str(image)).returncode, 3)

    def test_failed_check_issues_no_record(self):
        plan = json.loads(self.plan.read_text())
        plan['checks'][0]['argv'] = [sys.executable, '-c', 'raise SystemExit(1)']
        self.plan.write_text(json.dumps(plan))
        self.assertEqual(self.run_validation().returncode, 3)
        self.assertFalse((self.out/'record.json').exists())
        self.assertEqual(len(self.git('worktree', 'list').splitlines()), 1)

    def test_old_controller_scripts_refused_before_execution(self):
        ctl = self.repo/'ble/ctl'
        ctl.mkdir(parents=True)
        (ctl/'build_ctl.sh').write_text('exit 0\n')
        (ctl/'build_in_container.sh').write_text('exit 0\n')
        self.git('add', '.')
        self.git('-c', 'user.name=t', '-c', 'user.email=t@t', 'commit', '-qm', 'old controller')
        self.commit = self.git('rev-parse', 'HEAD')
        self.assertEqual(self.run_validation(controller=True).returncode, 3)
        self.assertFalse((self.out/'check-0.log').exists())
        self.assertFalse((self.out/'record.json').exists())

    def test_timeout_kills_owned_child(self):
        marker = self.root/'child-survived'
        child = 'import time,pathlib; time.sleep(3); pathlib.Path(' + repr(str(marker)) + ').touch()'
        parent = 'import subprocess,sys,time; subprocess.Popen([sys.executable,"-c",' + repr(child) + ']); time.sleep(30)'
        plan = json.loads(self.plan.read_text())
        plan['checks'][0]['argv'] = [sys.executable, '-c', parent]
        self.plan.write_text(json.dumps(plan))
        self.assertEqual(self.run_validation(timeout=1).returncode, 3)
        time.sleep(3)
        self.assertFalse(marker.exists())
        self.assertFalse((self.out/'record.json').exists())
        self.assertEqual(len(self.git('worktree', 'list').splitlines()), 1)

    def test_check_cannot_change_source_commit(self):
        plan = json.loads(self.plan.read_text())
        plan['checks'][0]['argv'] = ['sh', '-c', 'echo injected > injected.c; git add injected.c; git -c user.name=t -c user.email=t@t commit -qm injected']
        self.plan.write_text(json.dumps(plan))
        self.assertEqual(self.run_validation().returncode, 3)
        self.assertFalse((self.out/'record.json').exists())

    def test_missing_package_issues_no_record(self):
        path = self.repo/'build.sh'
        # Tip fixture emits via fm1pkg_make.ufw, not printf → .fwsc.
        path.write_text(path.read_text().replace(
            "Path('build', filename + '.fwsc').write_bytes(\n"
            "    ufw(b'\x00' * 1024, b'FELUCCA-LOADER-1' + b'\x00' * 128, product))\n",
            ""))
        self.git('add', '.')
        self.git('-c', 'user.name=t', '-c', 'user.email=t@t', 'commit', '-qm', 'no package')
        self.commit = self.git('rev-parse', 'HEAD')
        result = self.run_validation()
        self.assertEqual(result.returncode, 3, result.stdout + result.stderr)
        self.assertIn('no development package', result.stderr)
        self.assertFalse((self.out/'record.json').exists())

    def test_missing_plan_and_budget_evidence_refused(self):
        self.plan.write_text(json.dumps({'base': self.commit, 'scope': 'fixture', 'checks': []}))
        self.assertEqual(self.run_validation().returncode, 3)
        self.assertFalse((self.out/'record.json').exists())
        self.plan.write_text(json.dumps({'base': self.commit, 'scope': 'fixture', 'no_tests_reason': 'no host test'}))
        path = self.repo/'build.sh'
        path.write_text(path.read_text().replace('pool 100 B of 10000', 'pool 9999 B of 10000'))
        self.git('add', '.')
        self.git('-c', 'user.name=t', '-c', 'user.email=t@t', 'commit', '-qm', 'bad budget')
        self.commit = self.git('rev-parse', 'HEAD')
        self.assertEqual(self.run_validation().returncode, 3)
        self.assertFalse((self.out/'record.json').exists())

    def test_installer_selects_dev_evidence_without_cached_controller(self):
        self.ready()
        sys.path.insert(0, str(ROOT/'tools'))
        import fm1_install
        from types import SimpleNamespace
        from unittest.mock import patch
        args = SimpleNamespace(commit=self.commit, package=str(self.out/'x0x-smk37-elite.fwsc'),
                               config='dev', ctl=None, no_ctl=True, dev_record=str(self.out/'record.json'))
        with patch.dict(os.environ, self.env):
            self.assertEqual(fm1_install.gate_guard(args), 0)
            args.no_ctl = False
            self.assertEqual(fm1_install.gate_guard(args), 9)
            args.no_ctl = True
            args.config = 'rel'
            self.assertEqual(fm1_install.gate_guard(args), 9)


    def test_worker_cleanup_failure_preserves_original_error_and_scratch(self):
        from types import SimpleNamespace
        from unittest.mock import patch
        import io
        sys.path.insert(0, str(ROOT/'tools'))
        import dev_validate
        args = SimpleNamespace(repo=str(self.repo), commit=self.commit, model='elite', plan=str(self.plan),
            output=str(self.out), no_controller=True, worker_image='fixture', worker_mount=[], timeout=30)
        class BrokenWorker:
            def __init__(inner, *args): pass
            def start(inner): raise ValueError('original startup error')
            def close(inner): raise ValueError('cleanup error')
        with patch.object(dev_validate, 'LinuxWorker', BrokenWorker), patch('sys.stderr', new_callable=io.StringIO) as err:
            with self.assertRaisesRegex(ValueError, 'original startup error'):
                dev_validate.run(args)
            self.assertIn('scratch tree retained', err.getvalue())
            self.assertIn('cleanup error', err.getvalue())
        self.assertEqual(len(self.git('worktree', 'list').splitlines()), 2)
        self.assertTrue(list(self.repo.parent.glob('dev-check-*/source')))
        self.assertFalse((self.out/'record.json').exists())

    def test_worker_stop_precedes_worktree_removal(self):
        from types import SimpleNamespace
        from unittest.mock import patch
        sys.path.insert(0, str(ROOT/'tools'))
        import dev_validate
        args = SimpleNamespace(repo=str(self.repo), commit=self.commit, model='elite', plan=str(self.plan),
            output=str(self.out), no_controller=True, worker_image='fixture', worker_mount=[], timeout=30)
        stopped = []
        owner = self
        class StartupFailure:
            def __init__(inner, *args): pass
            def start(inner): raise ValueError('startup failed')
            def close(inner):
                owner.assertTrue(list(owner.repo.parent.glob('dev-check-*/source')))
                stopped.append(True)
        with patch.object(dev_validate, 'LinuxWorker', StartupFailure):
            with self.assertRaisesRegex(ValueError, 'startup failed'):
                dev_validate.run(args)
        self.assertEqual(stopped, [True])
        self.assertEqual(len(self.git('worktree', 'list').splitlines()), 1)
        self.assertFalse(list(self.repo.parent.glob('dev-check-*')))


@unittest.skipUnless(platform.system() == 'Linux' and platform.machine() in ('x86_64', 'AMD64'),
                     'Linux x86-64 is required for native execution')
class NativeLinuxValidation(DevelopmentValidation):
    def setUp(self):
        self.fixture_parent = ROOT/'build/native-fixtures'
        self.fixture_parent.mkdir(parents=True, exist_ok=True)
        super().setUp()

    def run_validation(self, controller=False, timeout=1800, version=None, model='elite'):
        return subprocess.run([sys.executable, '-B', str(RUNNER), 'run', '--repo', str(self.repo),
            '--commit', self.commit, '--model', model, '--plan', str(self.plan),
            '--output', str(self.out), '--timeout', str(timeout)]
            + (['--firmware-version', version] if version is not None else [])
            + ([] if controller else ['--no-controller']),
            capture_output=True, text=True, env=self.env)


@unittest.skipUnless(os.environ.get('X0X_TEST_WORKER_IMAGE'), 'set X0X_TEST_WORKER_IMAGE for local Docker checks')
class WorkerValidation(DevelopmentValidation):
    def setUp(self):
        self.fixture_parent = ROOT/'build/worker-fixtures'
        self.fixture_parent.mkdir(parents=True, exist_ok=True)
        super().setUp()

    # Reuse all consumer, mutation and failure tests with real worker commands.
    def run_validation(self, controller=False, timeout=1800, version=None, model='elite'):
        plan = json.loads(self.plan.read_text())
        for check in plan.get('checks', []):
            if check['argv'][0] == sys.executable:
                check['argv'][0] = 'python3'
        self.plan.write_text(json.dumps(plan))
        return subprocess.run([sys.executable, '-B', str(RUNNER), 'run', '--repo', str(self.repo),
            '--commit', self.commit, '--model', model, '--plan', str(self.plan),
            '--output', str(self.out), '--timeout', str(timeout),
            '--worker-image', os.environ['X0X_TEST_WORKER_IMAGE']]
            + (['--firmware-version', version] if version is not None else [])
            + ([] if controller else ['--no-controller']),
            capture_output=True, text=True, env=self.env)

    def test_worker_provenance_and_consumer_authentication(self):
        self.ready()
        data = json.loads((self.out/'record.json').read_text())
        worker = data['plan']['worker']
        self.assertTrue(worker['image'].startswith('sha256:'))
        self.assertEqual(worker['platform'][:2], ['Linux', 'x86_64'])
        self.assertEqual(worker['environment']['CTL_NATIVE'], '1')
        self.assertIn('docker', worker['orchestrator_tools'])
        self.assertEqual(data['validation_inputs']['inputs']['python_packages']['pillow']['version'],
            subprocess.check_output(['docker', 'run', '--rm', '--entrypoint', 'python3', worker['image'],
                '-c', 'import PIL; print(PIL.__version__)'], text=True).strip())
        self.assertNotEqual(data['validation_inputs']['inputs']['python_packages']['pillow']['sha256'],
            __import__('hashlib').sha256(b'{}').hexdigest())
        worker['image'] = 'sha256:changed'
        (self.out/'record.json').write_text(json.dumps(data))
        self.assertEqual(self.guard().returncode, 3)

    def package_mutation(self, module):
        # Only the disposable worker's writable layer changes, never the image.
        plan = json.loads(self.plan.read_text())
        plan['checks'][0]['argv'] = ['python3', '-B', '-c',
            'from pathlib import Path; import '+module+' as p; '
            'f=Path(p.__file__); f.write_bytes(f.read_bytes()+b"\\n# mutation\\n"); print(f)']
        self.plan.write_text(json.dumps(plan))
        actual = self.run_validation()
        self.assertEqual(actual.returncode, 3, actual.stdout+actual.stderr)
        self.assertIn('validation inputs changed', actual.stderr)
        self.assertFalse((self.out/'record.json').exists())
        self.assertIn('/PIL/', (self.out/'check-0.log').read_text())
        self.assertEqual(len(self.git('worktree', 'list').splitlines()), 1)

    def test_installed_pillow_source_mutation_issues_no_record(self):
        self.package_mutation('PIL')

    def test_installed_pillow_binary_mutation_issues_no_record(self):
        self.package_mutation('PIL._imaging')

    def test_dependency_change_during_validation_issues_no_record(self):
        # The worker receives these same bytes read-only. A mutation must fail.
        plan = json.loads(self.plan.read_text())
        plan['checks'][0]['argv'] = ['python3', '-c',
            "from pathlib import Path; Path("+repr(str(self.sdk/'library'))+").write_text('changed SDK')"]
        self.plan.write_text(json.dumps(plan))
        actual = self.run_validation()
        self.assertEqual(actual.returncode, 3, actual.stdout+actual.stderr)
        self.assertFalse((self.out/'record.json').exists())
        self.assertEqual((self.sdk/'library').read_text(), 'fixture SDK')

    def test_timeout_kills_owned_child(self):
        # Assert daemon state, because a killed Docker CLI alone leaves exec children alive.
        def containers():
            return set(subprocess.check_output(['docker', 'ps', '-aq', '--filter', 'name=x0x37-dev-'], text=True).split())
        before = containers()
        plan = json.loads(self.plan.read_text())
        plan['checks'][0]['argv'] = ['sh', '-c', 'sleep 40 & wait']
        self.plan.write_text(json.dumps(plan))
        actual = self.run_validation(timeout=2)
        self.assertEqual(actual.returncode, 3, actual.stdout+actual.stderr)
        self.assertFalse((self.out/'record.json').exists())
        self.assertEqual(containers(), before)
        self.assertEqual(len(self.git('worktree', 'list').splitlines()), 1)

    def test_signal_removes_worker_before_scratch(self):
        import signal
        plan = json.loads(self.plan.read_text())
        plan['checks'][0]['argv'] = ['sh', '-c', 'echo ready; sleep 40 & wait']
        self.plan.write_text(json.dumps(plan))
        args = [sys.executable, '-B', str(RUNNER), 'run', '--repo', str(self.repo), '--commit', self.commit,
                '--model', 'elite', '--plan', str(self.plan), '--output', str(self.out), '--no-controller',
                '--worker-image', os.environ['X0X_TEST_WORKER_IMAGE']]
        process = subprocess.Popen(args, env=self.env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        try:
            deadline = time.monotonic()+30
            while time.monotonic() < deadline:
                log = self.out/'check-0.log'
                if log.exists() and 'ready' in log.read_text():
                    break
                if process.poll() is not None:
                    self.fail(str(process.communicate()))
                time.sleep(0.1)
            else:
                self.fail('worker did not start the check')
            # Find only the worker that owns this fixture's scratch mount.
            names = subprocess.check_output(['docker', 'ps', '--format', '{{.Names}}', '--filter', 'name=x0x37-dev-'], text=True).split()
            owned = [name for name in names if str(self.repo) in subprocess.check_output(['docker', 'inspect', name], text=True)]
            self.assertEqual(len(owned), 1)
            process.send_signal(signal.SIGTERM)
            process.communicate(timeout=30)
            self.assertNotEqual(process.returncode, 0)
            self.assertNotEqual(subprocess.run(['docker', 'inspect', owned[0]], capture_output=True).returncode, 0)
            self.assertEqual(len(self.git('worktree', 'list').splitlines()), 1)
            self.assertFalse((self.out/'record.json').exists())
        finally:
            if process.poll() is None:
                process.kill()
                process.communicate()

if __name__ == '__main__':
    unittest.main()
