#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build one development target and run the checks selected in a JSON plan.

run --commit HASH --model elite|pro --plan FILE --output NEW_DIRECTORY
verify --commit HASH --record FILE --fwsc FILE [--ctl FILE]
Use --firmware-version X.Y.Z on run to stamp a numeric DEV identity.
The --release build.py argument stamps identity, NOT release-build validation.

Plans contain a base commit, scope explanation, and checks [{reason, argv}].
Use no_tests_reason for a change with no applicable host test. Builds and budgets
are mandatory. Results are DEV CHECKED, never release PASS. Run in Linux x86-64, or use
--worker-image on macOS for native input hashing and Linux command execution.
Commands must be synchronous processes, not detached jobs or nested containers. Uses a private Git
worktree; never writes device state, a gate index, or an existing output folder.
"""
import argparse
import copy
import hashlib
import json
import os
import platform
from pathlib import Path
import re
import shutil
import signal
import subprocess
import sys
import tempfile
import uuid

sys.path.insert(0, str(Path(__file__).resolve().parent))
import release_alloc
import validation_inputs
from firmware_build_labels import canonical_version, inspect_package


def git(repo, *args):
    return subprocess.check_output(['git', '-C', str(repo), *args], text=True).strip()


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def tree(repo, commit):
    # Match the gate's content scope: Markdown does not invalidate build evidence.
    rows = git(repo, 'ls-tree', '-r', '--full-tree', commit).splitlines()
    data = ''.join(row + '\n' for row in rows if not row.split('\t', 1)[-1].endswith('.md'))
    return hashlib.sha256(data.encode()).hexdigest()


def budgets(log):
    code = re.search(r'app code area (\d+) B, (\d+) B free', log)
    ram = re.search(r'RAM \.data\+\.bss (\d+) B of (\d+)', log)
    pool = re.search(r'pool (\d+) B of (\d+)', log)
    if not all((code, ram, pool)):
        raise ValueError('build log lacks code/RAM/pool budget evidence')
    if int(ram[2]) < int(ram[1]) or int(pool[2]) - int(pool[1]) < 8192:
        raise ValueError('RAM or pool budget exceeded')
    return {'code_free': int(code[2]), 'ram_free': int(ram[2])-int(ram[1]),
            'pool_free': int(pool[2])-int(pool[1])}


def check_plan(plan):
    if not isinstance(plan, dict) or not plan.get('base') or not plan.get('scope'):
        raise ValueError('plan needs base and scope')
    if not isinstance(plan.get('dependencies', []), list) or not all(isinstance(p, str) and p for p in plan.get('dependencies', [])):
        raise ValueError('plan dependencies must be paths to external check inputs')
    checks = plan.get('checks', [])
    if not isinstance(checks, list) or (not checks and not plan.get('no_tests_reason')):
        raise ValueError('plan needs affected checks or no_tests_reason')
    for check in checks:
        if (not isinstance(check, dict) or not check.get('reason') or
                not isinstance(check.get('argv'), list) or not check['argv'] or
                not all(isinstance(v, str) and v for v in check['argv'])):
            raise ValueError('each check needs reason and a nonempty string argv array')
    return checks


def verify(repo, commit, record, fwsc, ctl=None, ledger=None):
    record = Path(record).resolve()
    data = json.loads(record.read_text())
    if (data.get('schema') != 2 or data.get('result') != 'DEV CHECKED' or
            data.get('config') != 'dev' or data.get('model') not in ('elite', 'pro')):
        raise ValueError('not a development validation record')
    if data.get('tree') != tree(repo, commit):
        raise ValueError('development evidence is for another source tree')
    validation_inputs.validate(data.get('validation_inputs'))
    if data['validation_inputs']['inputs'].get('source') != data['tree']:
        raise ValueError('validation input source mismatch')
    expected_config = {'model': data['model'], 'config': 'dev', 'controller': data['controller'],
                       'plan': data['plan'], 'dependencies': data['plan'].get('dependencies', [])}
    if data.get('firmware_version') is not None:
        expected_config['firmware_version'] = canonical_version(data['firmware_version'])
    if (data['validation_inputs']['inputs'].get('mode') != 'dev' or
            data['validation_inputs']['inputs'].get('configuration') != expected_config):
        raise ValueError('validation input configuration mismatch')
    check_plan(data['plan'])
    logs = data['logs']
    expected = {'build'} | {f'check-{i}' for i in range(len(data['plan'].get('checks', [])))}
    if data['controller']:
        expected.add('controller')
    if set(logs) != expected:
        raise ValueError('missing or unexpected validation logs')
    for name, info in logs.items():
        p = record.parent / (name + '.log')
        if info['returncode'] != 0 or digest(p) != info['sha256']:
            raise ValueError('validation log changed or failed: ' + name)
    if budgets((record.parent / 'build.log').read_text()) != data['budgets']:
        raise ValueError('budget evidence mismatch')
    artifacts = data['artifacts']
    if digest(fwsc) != artifacts['package']['sha256']:
        raise ValueError('package differs from validated development artifact')
    if 'firmware' in data:
        identity = inspect_package(fwsc, data['model'], data['commit'],
                                   data.get('firmware_version'))
        if identity != data['firmware'] or identity['firmware_version'] != data.get('firmware_version'):
            raise ValueError('recorded firmware version/identity/build label differs from package')
    elif data.get('firmware_version') is not None:
        raise ValueError('versioned record has no verified firmware identity')
    if data.get('firmware_version') is not None:
        # Allocation is required. A receipt is required only after it has been bound,
        # so a build can be checked before its package hash is recorded.
        try:
            item = release_alloc.require_allocated(repo, data['firmware_version'], data['commit'], ledger)
        except release_alloc.ReleaseError as exc:
            raise ValueError(str(exc)) from exc
        receipt = item.get('models', {}).get(data['model'])
        if receipt is not None:
            if receipt.get('package_sha256') != digest(fwsc):
                raise ValueError('package hash does not match the allocated receipt')
            if ctl and receipt.get('controller_sha256') != digest(ctl):
                raise ValueError('controller hash does not match the allocated receipt')
    if bool(ctl) != bool(data['controller']):
        raise ValueError('controller selection differs from validation record')
    if ctl and digest(ctl) != artifacts['controller']['sha256']:
        raise ValueError('controller differs from validated development artifact')
    return data


def development_env(env, model):
    env = dict(env)
    for key in list(env):
        if key.startswith('X0X_DEV_') or key in ('X0X_CDC', 'X0X_DEBUG', 'X0X_BREAK_BANK', 'X0X_RAM_CANARY',
                'X0X_SMK_LCD_BAUD', 'X0X_FONTSET', 'X0X_FONT_DIRS',
                'FELUCCA_DATA_FAMILY', 'FELUCCA_DATA_UNTAGGED_OK', 'BOARD', 'MODEL', 'JIELI_DOCKER'):
            env.pop(key)
    env['SDK_LOCAL'] = env.get('AC79_SDK', str(Path.home()/'fw-AC79_AIoT_SDK'))
    env.update(MODEL=model, BOARD='smk37', X0X_DEV_PADCAL='1', CTL_NO_CACHE='1', CTL_NATIVE='1')
    return env


class LinuxWorker:
    """One owned container. All commands finish before its scratch tree is removed."""
    def __init__(self, image, mounts, writable, env, timeout):
        self.docker = shutil.which('docker')
        if not self.docker:
            raise ValueError('Docker CLI is required for --worker-image')
        self.timeout = timeout
        self.name = 'x0x37-dev-' + uuid.uuid4().hex
        self.created = False
        self.image = self.call('image', 'inspect', image)[0]
        if (self.image['Os'], self.image['Architecture']) != ('linux', 'amd64'):
            raise ValueError('worker image must be Linux/amd64')
        self.mounts = {str(Path(p).absolute()): str(Path(p).absolute()) not in writable for p in mounts}
        # Docker resolves source symlinks. Also mount their targets for Git metadata and path lookups.
        self.mounts.update({str(Path(p).resolve(strict=True)): str(Path(p).absolute()) not in writable for p in mounts})
        self.host_env = env

    def start(self):
        args = ['create', '--name', self.name, '--platform', 'linux/amd64',
                '--entrypoint', '/bin/sleep']
        for path, readonly in sorted(self.mounts.items()):
            if ',' in path:
                raise ValueError('worker mount paths cannot contain commas')
            args += ['--mount', 'type=bind,source='+path+',target='+path+(',readonly' if readonly else '')]
        args += [self.image['Id'], 'infinity']
        # The unique name permits cleanup even if create is interrupted after creation.
        self.created = True
        env = self.host_env
        self.call(*args, json_output=False)
        self.call('start', self.name, json_output=False)
        original = self.call('exec', self.name, 'env', '-0', json_output=False)
        self.env = dict(item.split('=', 1) for item in original.split('\0') if item)
        # Keep Linux PATH/HOME and image defaults. Forward only build/check inputs.
        keys = ('JIELI_TOOLCHAIN', 'X0X_TOOLCHAIN', 'AC79_SDK', 'SDK_LOCAL', 'NINEW9_DIR',
                'EIGHTW8_DIR', 'SCHWUNG303', 'TB3PO_REF', 'X0X_REFS', 'CC', 'CXX',
                'CFLAGS', 'CXXFLAGS', 'CPPFLAGS', 'LDFLAGS', 'CPATH', 'LIBRARY_PATH',
                'X0X_VERSION', 'X0X_BASS_PRESET_PREVIEW', 'X0X_BASS_TAGGED_SOUND',
                'X0X_BASS_NOTE_SOUND',
                'LD_LIBRARY_PATH', 'LD_PRELOAD', 'PYTHONPATH', 'NODE_PATH', 'NODE_OPTIONS',
                'X0X_TEST_TIME_SCALE', 'X0X_TESTS', 'X0X_TEST_JOBS', 'X0X_HOST_DEFS', 'X0X_JOBS',
                'X0X_GATE_LOCK', 'X0X_GATE_HOLDS_LOCK', 'X0X_REPO', 'X0X_QUEUE_FILE',
                'X0X_RESULTS_DIR', 'X0X_RESULTS', 'X0X_GATE_WORK',
                'X0X_VALIDATION_PYTHON_PACKAGES', 'PYTHONDONTWRITEBYTECODE')
        self.env.update({k: v for k, v in env.items() if k in keys})
        self.env = development_env(self.env, env['MODEL'])
        self.env['PYTHONDONTWRITEBYTECODE'] = '1'

    def call(self, *args, json_output=True):
        result = subprocess.run([self.docker, *args], capture_output=True,
                                text=True, timeout=max(30, self.timeout) if args[0] == 'rm' else self.timeout)
        if result.returncode:
            raise ValueError(f'Docker {args[0]} failed ({result.returncode}): {result.stderr.strip()}')
        return json.loads(result.stdout) if json_output else result.stdout

    def argv(self, work, argv):
        return [self.docker, 'exec', '-w', str(work), self.name, 'env', '-i',
                *[k+'='+v for k, v in sorted(self.env.items())], *argv]

    def snapshot(self, config):
        actual = self.call('inspect', self.name)[0]
        if actual['Image'] != self.image['Id'] or not actual['State']['Running']:
            raise ValueError('worker image or state changed')
        code = ('import json,sys; sys.path.insert(0,sys.argv[1]); import validation_inputs as v; '
                'print(json.dumps(v.runtime(json.loads(sys.argv[2]))))')
        probe = subprocess.run(self.argv(Path(__file__).resolve().parent, ['python3', '-B', '-c', code,
            str(Path(__file__).resolve().parent), json.dumps(config)]), check=True,
            capture_output=True, text=True, timeout=self.timeout)
        runtime = json.loads(probe.stdout)
        if runtime['platform'][:2] != ['Linux', 'x86_64']:
            raise ValueError('worker kernel must be Linux x86_64')
        provenance = {'image': self.image['Id'], 'platform': runtime['platform'],
            'environment': self.env, 'mounts': self.mounts,
            'orchestrator_platform': [platform.system(), platform.machine(), platform.release()],
            'orchestrator_tools': {name: validation_inputs.content(path) for name, path in
                [('docker', self.docker), ('python', sys.executable), ('git', shutil.which('git'))]}}
        return runtime, provenance

    def close(self):
        if self.created:
            # rm -f waits for the container's processes to stop, including exec children.
            # Never remove another task's container or scratch directory.
            self.call('rm', '--force', self.name, json_output=False)
            self.created = False


def run(a):
    firmware_version = getattr(a, 'firmware_version', None)
    system = platform.system()
    if not a.worker_image and (system != 'Linux' or platform.machine() not in ('x86_64', 'AMD64')):
        raise ValueError('use --worker-image on macOS, or run inside Linux x86-64')
    if firmware_version is not None:
        firmware_version = canonical_version(firmware_version)
    repo = Path(a.repo).resolve()
    commit = git(repo, 'rev-parse', '--verify', a.commit + '^{commit}')
    if firmware_version is not None:
        try:
            release_alloc.require_allocated(repo, firmware_version, commit, getattr(a, 'release_ledger', None))
        except release_alloc.ReleaseError as exc:
            raise ValueError(str(exc)) from exc
    plan = json.loads(Path(a.plan).read_text())
    checks = check_plan(plan)
    if 'worker' in plan:
        raise ValueError('worker provenance is generated by the runner, not supplied in the plan')
    base = git(repo, 'rev-parse', '--verify', plan['base'] + '^{commit}')
    subprocess.run(['git', '-C', str(repo), 'merge-base', '--is-ancestor', base, commit], check=True)
    output = Path(a.output).resolve()
    output.mkdir(parents=True, exist_ok=False)
    plan['base'] = base
    env = development_env(os.environ, a.model)
    env.setdefault('JIELI_TOOLCHAIN', str(Path.home()/'.jieli/toolchain'))
    env.setdefault('AC79_SDK', str(Path.home()/'fw-AC79_AIoT_SDK'))
    # Each path is available at the same absolute location on the host and worker.
    tmp = Path(tempfile.mkdtemp(prefix='dev-check-', dir=repo.parent))
    work = tmp / 'source'
    worker = None
    added = False
    logs = {}
    data = None
    failure = None
    try:
        subprocess.run(['git', '-C', str(repo), 'worktree', 'add', '--detach', str(work), commit], check=True)
        added = True
        if not a.no_controller:
            if ('CTL_NO_CACHE' not in (work/'ble/ctl/build_ctl.sh').read_text() or
                    'B=$(mktemp -d' not in (work/'ble/ctl/build_in_container.sh').read_text()):
                raise ValueError('selected commit lacks isolated controller build support')
        if a.worker_image:
            paths = [repo, tmp, Path(__file__).resolve().parent,
                     Path(git(repo, 'rev-parse', '--path-format=absolute', '--git-common-dir')),
                     env['JIELI_TOOLCHAIN'], env['AC79_SDK'], *a.worker_mount,
                     *plan.get('dependencies', [])]
            paths += [env[k] for k in ('NINEW9_DIR', 'EIGHTW8_DIR', 'SCHWUNG303', 'TB3PO_REF', 'X0X_REFS') if env.get(k)]
            paths += [v for check in checks for v in check['argv'][1:]
                      if Path(v).is_absolute() and Path(v).exists()]
            worker = LinuxWorker(a.worker_image, paths,
                {str(tmp), str(paths[3])}, env, a.timeout)
            worker.start()
            env = worker.env

        def identity():
            normalized = copy.deepcopy(plan)
            config = {'model': a.model, 'config': 'dev', 'controller': not a.no_controller,
                      'plan': normalized, 'dependencies': plan.get('dependencies', [])}
            if firmware_version is not None:
                config['firmware_version'] = firmware_version
            execution = None
            if worker:
                execution, provenance = worker.snapshot(config)
                normalized['worker'] = provenance
            return validation_inputs.capture(repo, commit, 'dev', config,
                env['JIELI_TOOLCHAIN'], env['AC79_SDK'], env=env, execution=execution)

        inputs = identity()
        plan = inputs['inputs']['configuration']['plan']

        def command(name, argv):
            log = output / (name + '.log')
            with log.open('w') as stream:
                process = subprocess.Popen(worker.argv(work, argv) if worker else argv,
                    cwd=work, env=None if worker else env, stdout=stream,
                    stderr=subprocess.STDOUT, start_new_session=True)
                try:
                    rc = process.wait(timeout=a.timeout)
                except BaseException:
                    try:
                        os.killpg(process.pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass
                    process.wait()
                    raise
            logs[name] = {'argv': argv, 'returncode': rc, 'sha256': digest(log)}
            if rc:
                raise ValueError(f'{name} failed; see {log}')

        for i, check in enumerate(checks):
            command(f'check-{i}', check['argv'])

        def unchanged():
            if git(work, 'rev-parse', 'HEAD') != commit or git(work, 'status', '--porcelain'):
                raise ValueError('validation changed sources; no record issued')

        unchanged()
        build_args = ['sh', 'build.sh', '--board', 'smk37']
        if firmware_version is not None:
            build_args += ['--release', firmware_version]
        command('build', build_args)
        limits = budgets((output / 'build.log').read_text())
        if not a.no_controller:
            command('controller', ['sh', 'ble/ctl/build_ctl.sh'])
        unchanged()
        stem = 'x0x-smk37-elite' if a.model == 'elite' else 'x0x-smk37'
        package = work / 'build' / (stem + ('-' + firmware_version if firmware_version else '') + '.fwsc')
        if not package.is_file() or package.stat().st_size == 0:
            raise ValueError('build produced no development package')
        artifacts = {}
        for kind, path in [('package', package)] + ([] if a.no_controller else [('controller', work/'ble/ctl/out/ctl.img')]):
            dest = output / path.name
            shutil.copy2(path, dest)
            artifacts[kind] = {'file': dest.name, 'sha256': digest(dest)}
        firmware = inspect_package(package, a.model, commit, firmware_version)
        validation_inputs.validate(inputs, identity())
        data = {'schema': 2, 'validation_inputs': inputs, 'result': 'DEV CHECKED', 'config': 'dev', 'model': a.model,
                'commit': commit, 'tree': tree(repo, commit), 'plan': plan,
                'changed_files': git(repo, 'diff', '--name-only', base, commit).splitlines(),
                'controller': not a.no_controller, 'budgets': limits,
                'firmware_version': firmware['firmware_version'], 'firmware': firmware,
                'logs': logs, 'artifacts': artifacts}
    except BaseException as error:
        failure = error
        raise
    finally:
        try:
            if worker:
                worker.close()
        except BaseException as cleanup_error:
            # Leave the scratch tree intact when worker termination is unconfirmed.
            if failure is None:
                raise
            print('Worker cleanup failed; scratch tree retained: '+str(tmp)+': '+str(cleanup_error), file=sys.stderr)
        else:
            try:
                if added:
                    subprocess.run(['git', '-C', str(repo), 'worktree', 'remove', '--force', str(work)], check=True)
                shutil.rmtree(tmp)
            except BaseException as cleanup_error:
                if failure is None:
                    raise
                print('Scratch cleanup failed: '+str(cleanup_error), file=sys.stderr)
    (output/'record.json').write_text(json.dumps(data, indent=2)+'\n')
    print('DEV CHECKED ' + str(output/'record.json'))

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='action', required=True)
    for action in ('run', 'verify'):
        p = sub.add_parser(action)
        p.add_argument('--repo', default=os.environ.get('X0X_REPO', '.'))
        p.add_argument('--commit', required=True)
        p.add_argument('--release-ledger',
                       help='open-beta ledger to read; default is releases/openbeta/ledger.json in --repo')
        if action == 'run':
            p.add_argument('--model', choices=('elite', 'pro'), required=True)
            p.add_argument('--firmware-version',
                           help='stamp canonical X.Y.Z into the DEV package via build.sh --release; not a release gate')
            p.add_argument('--plan', required=True)
            p.add_argument('--output', required=True)
            p.add_argument('--no-controller', action='store_true')
            p.add_argument('--timeout', type=int, default=1800)
            p.add_argument('--worker-image', help='local immutable Linux/amd64 image or local image tag')
            p.add_argument('--worker-mount', action='append', default=[], help='additional same-path worker mount')
        else:
            p.add_argument('--record', required=True)
            p.add_argument('--fwsc', required=True)
            p.add_argument('--ctl')
    a = parser.parse_args()
    def interrupted(signum, frame):
        raise KeyboardInterrupt('validation interrupted by signal '+str(signum))
    for signum in (signal.SIGTERM, signal.SIGHUP):
        signal.signal(signum, interrupted)
    try:
        if a.action == 'run':
            run(a)
        else:
            verify(a.repo, a.commit, a.record, a.fwsc, a.ctl, a.release_ledger)
            print('DEV CHECKED ' + a.commit)
    except (OSError, ValueError, KeyError, TypeError, subprocess.SubprocessError) as e:
        print('REFUSED: ' + str(e), file=sys.stderr)
        return 3
    return 0


if __name__ == '__main__':
    sys.exit(main())
