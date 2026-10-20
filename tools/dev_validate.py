#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build one development target and run the checks selected in a JSON plan.

run --commit HASH --model elite|pro --plan FILE --output NEW_DIRECTORY
verify --commit HASH --record FILE --fwsc FILE [--ctl FILE]

Plans contain a base commit, scope explanation, and checks [{reason, argv}].
Use no_tests_reason for a change with no applicable host test. Builds and budgets
are mandatory. Results are DEV CHECKED, never release PASS. Run in Linux x86-64, including a local container; commands must be synchronous
native processes, not detached jobs or nested containers. Uses a private Git
worktree; never writes device state, a gate index, or an existing output folder.
"""
import argparse
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

sys.path.insert(0, str(Path(__file__).resolve().parent))
import validation_inputs


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


def verify(repo, commit, record, fwsc, ctl=None):
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
    if bool(ctl) != bool(data['controller']):
        raise ValueError('controller selection differs from validation record')
    if ctl and digest(ctl) != artifacts['controller']['sha256']:
        raise ValueError('controller differs from validated development artifact')
    return data


def run(a):
    if platform.system() != 'Linux' or platform.machine() not in ('x86_64', 'AMD64'):
        raise ValueError('run validation inside Linux x86-64 (a local container is supported); verification runs on any host')
    repo = Path(a.repo).resolve()
    commit = git(repo, 'rev-parse', '--verify', a.commit + '^{commit}')
    plan = json.loads(Path(a.plan).read_text())
    checks = check_plan(plan)
    base = git(repo, 'rev-parse', '--verify', plan['base'] + '^{commit}')
    subprocess.run(['git', '-C', str(repo), 'merge-base', '--is-ancestor', base, commit], check=True)
    output = Path(a.output).resolve()
    output.mkdir(parents=True, exist_ok=False)
    plan['base'] = base
    env = os.environ.copy()
    for key in list(env):
        if key.startswith('X0X_DEV_') or key in ('X0X_CDC', 'X0X_DEBUG', 'X0X_BREAK_BANK', 'X0X_RAM_CANARY',
                'X0X_SMK_LCD_BAUD', 'X0X_FONTSET', 'X0X_FONT_DIRS',
                'FELUCCA_DATA_FAMILY', 'FELUCCA_DATA_UNTAGGED_OK', 'BOARD', 'MODEL'):
            env.pop(key)
    env.pop('JIELI_DOCKER', None)
    env['SDK_LOCAL'] = env.get('AC79_SDK', str(Path.home()/'fw-AC79_AIoT_SDK'))
    env.update(MODEL=a.model, BOARD='smk37', X0X_DEV_PADCAL='1', CTL_NO_CACHE='1', CTL_NATIVE='1')
    def identity():
        return validation_inputs.capture(repo, commit, 'dev',
            {'model': a.model, 'config': 'dev', 'controller': not a.no_controller,
             'plan': plan, 'dependencies': plan.get('dependencies', [])},
            env.get('JIELI_TOOLCHAIN', str(Path.home()/'.jieli/toolchain')),
            env.get('AC79_SDK', str(Path.home()/'fw-AC79_AIoT_SDK')), env=env)
    inputs = identity()
    logs = {}
    # Use a mounted repository sibling, not /tmp, so container builds can see it.
    with tempfile.TemporaryDirectory(prefix='dev-check-', dir=repo.parent) as tmp:
        work = Path(tmp) / 'source'
        subprocess.run(['git', '-C', str(repo), 'worktree', 'add', '--detach', str(work), commit], check=True)
        try:
            if not a.no_controller:
                if ('CTL_NO_CACHE' not in (work/'ble/ctl/build_ctl.sh').read_text() or
                        'B=$(mktemp -d' not in (work/'ble/ctl/build_in_container.sh').read_text()):
                    raise ValueError('selected commit lacks isolated controller build support')
            def command(name, argv):
                log = output / (name + '.log')
                with log.open('w') as stream:
                    process = subprocess.Popen(argv, cwd=work, env=env, stdout=stream,
                                               stderr=subprocess.STDOUT, start_new_session=True)
                    try:
                        rc = process.wait(timeout=a.timeout)
                    except BaseException:
                        # Kill only this command's owned process group before cleanup.
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
            command('build', ['sh', 'build.sh', '--board', 'smk37'])
            limits = budgets((output / 'build.log').read_text())
            if not a.no_controller:
                command('controller', ['sh', 'ble/ctl/build_ctl.sh'])
            unchanged()
            package = work / ('build/x0x-smk37-elite.fwsc' if a.model == 'elite' else 'build/x0x-smk37.fwsc')
            artifacts = {}
            for kind, path in [('package', package)] + ([] if a.no_controller else [('controller', work/'ble/ctl/out/ctl.img')]):
                dest = output / path.name
                shutil.copy2(path, dest)
                artifacts[kind] = {'file': dest.name, 'sha256': digest(dest)}
            validation_inputs.validate(inputs, identity())
            data = {'schema': 2, 'validation_inputs': inputs, 'result': 'DEV CHECKED', 'config': 'dev', 'model': a.model,
                    'commit': commit, 'tree': tree(repo, commit), 'plan': plan,
                    'changed_files': git(repo, 'diff', '--name-only', base, commit).splitlines(),
                    'controller': not a.no_controller, 'budgets': limits,
                    'logs': logs, 'artifacts': artifacts}
            (output/'record.json').write_text(json.dumps(data, indent=2)+'\n')
            print('DEV CHECKED ' + str(output/'record.json'))
        finally:
            subprocess.run(['git', '-C', str(repo), 'worktree', 'remove', '--force', str(work)], check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='action', required=True)
    for action in ('run', 'verify'):
        p = sub.add_parser(action)
        p.add_argument('--repo', default=os.environ.get('X0X_REPO', '.'))
        p.add_argument('--commit', required=True)
        if action == 'run':
            p.add_argument('--model', choices=('elite', 'pro'), required=True)
            p.add_argument('--plan', required=True)
            p.add_argument('--output', required=True)
            p.add_argument('--no-controller', action='store_true')
            p.add_argument('--timeout', type=int, default=1800)
        else:
            p.add_argument('--record', required=True)
            p.add_argument('--fwsc', required=True)
            p.add_argument('--ctl')
    a = parser.parse_args()
    try:
        if a.action == 'run':
            run(a)
        else:
            verify(a.repo, a.commit, a.record, a.fwsc, a.ctl)
            print('DEV CHECKED ' + a.commit)
    except (OSError, ValueError, KeyError, TypeError, subprocess.SubprocessError) as e:
        print('REFUSED: ' + str(e), file=sys.stderr)
        return 3
    return 0


if __name__ == '__main__':
    sys.exit(main())
