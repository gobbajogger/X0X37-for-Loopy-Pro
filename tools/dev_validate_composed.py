#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Rebuild and validate a pinned composed DEV package; never install or publish.

Uses the unchanged schema-2 DEV verifier. Builds, checks and composition are real
synchronous commands. Existing artifacts/receipts are never relabeled. Every
output must match the supplied expected hash or no record is emitted.
"""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import signal
import subprocess
import tarfile
import tempfile

import dev_validate as dv
import validation_inputs as vi

SHA = re.compile(r'[0-9a-f]{64}\Z')


def require_hash(path, expected):
    if not isinstance(expected, str) or not SHA.fullmatch(expected):
        raise ValueError('invalid expected SHA256')
    if dv.digest(path) != expected:
        raise ValueError('artifact pin mismatch: ' + str(path))


def prepare_plan(plan, model):
    plan = copy.deepcopy(plan)
    dv.check_plan(plan)
    c = plan['composition']
    for k in ('app_sha256', 'raw_package_sha256', 'controller_sha256',
              'bundled_package_sha256', 'package_sha256'):
        if not isinstance(c.get(k), str) or not SHA.fullmatch(c[k]):
            raise ValueError('missing composition pin: ' + k)
    if not isinstance(c.get('controller_bytes'), int) or c['controller_bytes'] <= 0:
        raise ValueError('invalid controller size')
    composer = Path(c['composer']).resolve(strict=True)
    bank = composer.parent.parent / 'assets/factory-breaks/factory-break-pack.bin'
    required = [composer.parent, bank]
    pins = c['dependency_pins']
    for p in required:
        if str(p) not in pins or vi.content(p)['sha256'] != pins[str(p)]:
            raise ValueError('composition dependency pin mismatch: ' + str(p))
    for p, expected in pins.items():
        if not SHA.fullmatch(expected) or vi.content(p)['sha256'] != expected:
            raise ValueError('dependency pin mismatch: ' + p)
    # Fingerprint runner + imported helpers too, separately from firmware source.
    plan['dependencies'] = sorted(set(plan.get('dependencies', []) + list(pins) +
                                      [str(Path(__file__).resolve().parent)]))
    stem = 'x0x-smk37-elite' if model == 'elite' else 'x0x-smk37'
    raw = 'build/' + stem + '.fwsc'
    common = ['--model', model, '--controller-sha256', c['controller_sha256'],
              '--controller-size', str(c['controller_bytes'])]
    post = [
        {'reason': 'Bundle exact freshly rebuilt controller; authenticate model and image.',
         'argv': ['python3', 'tools/package_nightly.py', raw, 'ble/ctl/out/ctl.img',
                  *common, '-o', 'composed-bundled.fwsc']},
        {'reason': 'Compose pinned stock bank without changing app/controller/other regions.',
         'argv': ['python3', str(composer), 'composed-bundled.fwsc', *common,
                  '--package-sha256', c['bundled_package_sha256'],
                  '--output', 'composed-final.fwsc', '--receipt', 'composition.json']},
        {'reason': 'Reinspect final composed model, CRC, controller and layout.',
         'argv': ['python3', 'tools/package_nightly.py', 'composed-final.fwsc',
                  '--check', *common]},
    ]
    pre_count = len(plan.get('checks', []))
    plan['checks'] = plan.get('checks', []) + post
    dv.check_plan(plan)
    return plan, pre_count, raw


def source_manifest(repo, commit):
    rows = subprocess.check_output(['git', '-C', str(repo), 'ls-tree', '-rz', commit])
    return [r.split(b'\t', 1) for r in rows.split(b'\0') if r]


def unchanged(work, manifest):
    for meta, name in manifest:
        mode, kind, oid = meta.decode().split()
        p = work / os.fsdecode(name)
        if kind != 'blob' or mode not in ('100644', '100755', '120000'):
            raise ValueError('unsupported source entry: ' + str(p))
        if mode == '120000':
            if not p.is_symlink():
                raise ValueError('source symlink changed: ' + str(p))
            data = os.fsencode(os.readlink(p))
        else:
            if p.is_symlink() or bool(p.stat().st_mode & 0o111) != (mode == '100755'):
                raise ValueError('source mode changed: ' + str(p))
            data = p.read_bytes()
        actual = hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
        if actual != oid:
            raise ValueError('source content changed: ' + str(p))


def execute(argv, work, env, log, timeout):
    with log.open('x') as stream:
        p = subprocess.Popen(argv, cwd=work, env=env, stdout=stream,
                             stderr=subprocess.STDOUT, start_new_session=True)
        try:
            rc = p.wait(timeout=timeout)
        except BaseException:
            os.killpg(p.pid, signal.SIGKILL)
            p.wait()
            raise
    if rc:
        raise ValueError('command failed (%d): %s' % (rc, log))
    return {'argv': argv, 'returncode': rc, 'sha256': dv.digest(log)}


def run(a):
    if (platform.system(), platform.machine()) != ('Linux', 'x86_64'):
        raise ValueError('native Linux x86_64 required')
    repo = Path(a.repo).resolve()
    commit = dv.git(repo, 'rev-parse', '--verify', a.commit + '^{commit}')
    plan, pre_count, raw_name = prepare_plan(json.loads(Path(a.plan).read_text()), a.model)
    plan['base'] = dv.git(repo, 'rev-parse', '--verify', plan['base'] + '^{commit}')
    subprocess.run(['git', '-C', str(repo), 'merge-base', '--is-ancestor', plan['base'], commit], check=True)
    out = Path(a.output).resolve()
    out.mkdir(parents=True, exist_ok=False)
    env = dv.development_env(os.environ, a.model)
    env['PYTHONDONTWRITEBYTECODE'] = '1'
    for key in ('JIELI_TOOLCHAIN', 'AC79_SDK'):
        if not env.get(key):
            raise ValueError('explicit pinned dependency required: ' + key)
    c = plan['composition']
    plan['execution_environment'] = {k: env[k] for k in (
        'MODEL', 'BOARD', 'X0X_DEV_PADCAL', 'CTL_NATIVE', 'CTL_NO_CACHE',
        'X0X_GEN_CACHE', 'X0X_BASS_PRESET_PREVIEW', 'X0X_BASS_TAGGED_SOUND',
        'X0X_BASS_NOTE_SOUND') if k in env}
    config = {'model': a.model, 'config': 'dev', 'controller': True,
              'plan': plan, 'dependencies': plan['dependencies']}
    def identity():
        return vi.capture(repo, commit, 'dev', config, env['JIELI_TOOLCHAIN'],
                          env['AC79_SDK'], env=env)
    inputs = identity()
    manifest = source_manifest(repo, commit)
    logs = {}
    with tempfile.TemporaryDirectory(prefix='composed-dev-') as tmp:
        tmp = Path(tmp)
        work = tmp / 'source'
        work.mkdir()
        archive = tmp / 'source.tar'
        with archive.open('wb') as f:
            subprocess.run(['git', '-C', str(repo), 'archive', '--format=tar', commit], stdout=f, check=True)
        with tarfile.open(archive) as f:
            f.extractall(work, filter='data')
        unchanged(work, manifest)
        def command(name, argv):
            logs[name] = execute(argv, work, env, out / (name + '.log'), a.timeout)
        for i, check in enumerate(plan['checks'][:pre_count]):
            command('check-' + str(i), check['argv'])
        unchanged(work, manifest)
        command('build', ['sh', 'build.sh', '--board', 'smk37'])
        limits = dv.budgets((out / 'build.log').read_text())
        app = work / 'build' / ('smk37-elite' if a.model == 'elite' else 'smk37') / 'x0x.bin'
        require_hash(app, c['app_sha256'])
        require_hash(work / raw_name, c['raw_package_sha256'])
        command('controller', ['sh', 'ble/ctl/build_ctl.sh'])
        ctl = work / 'ble/ctl/out/ctl.img'
        require_hash(ctl, c['controller_sha256'])
        if ctl.stat().st_size != c['controller_bytes']:
            raise ValueError('controller size mismatch')
        for i in range(pre_count, len(plan['checks'])):
            command('check-' + str(i), plan['checks'][i]['argv'])
            if i == pre_count:
                require_hash(work / 'composed-bundled.fwsc', c['bundled_package_sha256'])
        final = work / 'composed-final.fwsc'
        require_hash(final, c['package_sha256'])
        unchanged(work, manifest)
        # Recheck expected dependencies and before/after captured inputs.
        for p, expected in c['dependency_pins'].items():
            if vi.content(p)['sha256'] != expected:
                raise ValueError('composition dependency changed: ' + p)
        vi.validate(inputs, identity())
        firmware = dv.inspect_package(final, a.model, commit)
        if firmware['firmware_version'] is not None:
            raise ValueError('this composed DEV path supports unversioned candidates only')
        artifacts = {}
        for kind, path in [('package', final), ('controller', ctl)]:
            shutil.copyfile(path, out / path.name)
            artifacts[kind] = {'file': path.name, 'sha256': dv.digest(path)}
        shutil.copyfile(work / 'composition.json', out / 'composition.json')
    data = {'schema': 2, 'producer': 'dev_validate_composed.py', 'result': 'DEV CHECKED',
            'config': 'dev', 'model': a.model, 'commit': commit, 'tree': dv.tree(repo, commit),
            'validation_inputs': inputs, 'plan': plan, 'controller': True,
            'changed_files': dv.git(repo, 'diff', '--name-only', plan['base'], commit).splitlines(),
            'budgets': limits, 'logs': logs, 'artifacts': artifacts,
            'firmware_version': None, 'firmware': firmware}
    record = out / 'record.json'
    record.write_text(json.dumps(data, indent=2) + '\n')
    try:
        dv.verify(repo, commit, record, out / artifacts['package']['file'],
                  out / artifacts['controller']['file'])
    except BaseException:
        record.unlink()
        raise
    print('DEV CHECKED ' + str(record))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for arg in ('repo', 'commit', 'plan', 'output'):
        p.add_argument('--' + arg, required=True)
    p.add_argument('--model', required=True, choices=['elite', 'pro'])
    p.add_argument('--timeout', type=int, default=1800)
    a = p.parse_args()
    try:
        run(a)
    except (OSError, ValueError, KeyError, TypeError, subprocess.SubprocessError) as e:
        print('REFUSED: ' + str(e))
        return 3
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
