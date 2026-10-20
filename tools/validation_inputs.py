#!/usr/bin/env python3
"""Content identity for validation reuse, not permission to build or install.

Capture before and after validation. Hash dependency bytes (including dirty and
untracked reference files), resolved host tools, configuration and external runners.
No mtime cache: replacing bytes at the same path must invalidate evidence.
Install consumers check recorded provenance; they need not have the build SDK.
"""
import argparse
import hashlib
import importlib.metadata
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys

SCHEMA = 1
PINS = 'schwung-9W9=10cbe5c schwung-8W8=94aa271 schwung-303=7680578 schwung-tb3po=f49d81d'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def encoded(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':')).encode()


def file_hash(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def content(path, ancestors=(), exclude=()):
    path = Path(path)
    actual = path.resolve(strict=True)
    if actual in ancestors:
        raise ValueError('dependency symlink cycle: ' + str(path))
    if path.is_file():
        return {'sha256': file_hash(path), 'executable': bool(path.stat().st_mode & 0o111)}
    if not path.is_dir():
        raise ValueError('not a dependency file/directory: ' + str(path))
    # Git metadata is mutable bookkeeping, not a build input.
    return {'sha256': digest(encoded({p.name: content(p, (*ancestors, actual), exclude)
                                     for p in sorted(path.iterdir()) if p.name not in ('.git', '__pycache__', *exclude)}))}


def git(repo, *args):
    return subprocess.check_output(['git', '-C', str(repo), *args], text=True).strip()


def source_tree(repo, commit):
    rows = git(repo, 'ls-tree', '-r', '--full-tree', commit).splitlines()
    return digest(''.join(r + '\n' for r in rows if not r.split('\t', 1)[-1].endswith('.md')).encode())


def capture(repo, commit, mode, config, toolchain, sdk, refs=None, pins=PINS,
            env=None, runner_dir=None, hook=None, runner_path=None):
    env = os.environ if env is None else env
    runner_dir = Path(runner_dir or Path(__file__).resolve().parent)
    runners = ['validation_inputs.py'] + (['gate.sh', 'gate_flashmap.py', 'gate_sigwrap.py']
                                         if mode == 'gate' else ['dev_validate.py'])
    dependencies = {'toolchain': content(toolchain), 'sdk': content(sdk)}
    references = {}
    if mode == 'gate':
        for pin in pins.split():
            name, want = pin.split('=', 1)
            path = Path(refs) / name
            head = git(path, 'rev-parse', 'HEAD')
            if not head.startswith(want):
                raise ValueError('reference pin mismatch: ' + name)
            references[name] = {'pin': want, 'commit': head, 'content': content(path)}
        dependencies['pre-push'] = content(hook or runner_dir / 'pre-push.hook')
    else:
        for key in ('NINEW9_DIR', 'EIGHTW8_DIR', 'SCHWUNG303', 'TB3PO_REF', 'X0X_REFS'):
            if env.get(key):
                references[key] = content(env[key])
    for path in config.get('dependencies', []):
        dependencies[path] = content(path)
    tools = {}
    check_tools = []
    for check in config.get('plan', {}).get('checks', []):
        check_tools.append(check['argv'][0])
        for arg in check['argv'][1:]:
            if Path(arg).is_absolute() and Path(arg).exists():
                dependencies[arg] = content(arg)
    for name in ('bash', 'sh', 'git', 'make', 'node', 'emcc', 'cc', 'c++', 'ld', 'ar', 'objcopy',
                 env.get('CC', 'cc'), env.get('CXX', 'c++'), sys.executable, *check_tools):
        resolved = shutil.which(name, path=env.get('PATH'))
        tools[name if name != sys.executable else 'python'] = content(resolved) if resolved else None
    emcc = shutil.which('emcc', path=env.get('PATH'))
    if emcc:
        # Emscripten's launcher loads Python helpers and an LLVM backend.
        # Explicit EMSDK covers both; distro installs keep them below these roots.
        roots = [Path(env['EMSDK'])] if env.get('EMSDK') else [Path(emcc).resolve().parent]
        for root in roots:
            # Emscripten generates these caches from the captured compiler/sysroot.
            # They are outputs, like a source checkout's build directory.
            dependencies['emscripten:' + str(root)] = content(root, exclude=('cache', '.cache'))
        for key in ('EM_CONFIG', 'LLVM_ROOT', 'BINARYEN_ROOT'):
            if env.get(key):
                dependencies[key] = content(env[key])
    # Include installed Python dependency contents, not merely version strings.
    packages = {}
    for name in sorted(set(['pillow', 'numpy'] + env.get('X0X_VALIDATION_PYTHON_PACKAGES', '').split())):
        try:
            dist = importlib.metadata.distribution(name)
        except importlib.metadata.PackageNotFoundError:
            packages[name] = None
            continue
        files = {str(p): file_hash(dist.locate_file(p)) for p in (dist.files or [])
                 if not str(p).endswith(('.pyc', '.pyo')) and dist.locate_file(p).is_file()}
        packages[name] = {'version': dist.version, 'sha256': digest(encoded(files))}
    settings = {k: v for k, v in env.items() if k.startswith(('X0X_DEV_', 'FELUCCA_', 'EMCC_')) or k in (
        'CC', 'CXX', 'CFLAGS', 'CXXFLAGS', 'CPPFLAGS', 'LDFLAGS', 'CPATH', 'LIBRARY_PATH',
        'X0X_VERSION', 'EMCC_CFLAGS', 'EM_CONFIG', 'LLVM_ROOT', 'BINARYEN_ROOT',
        'LD_LIBRARY_PATH', 'LD_PRELOAD', 'PYTHONPATH', 'NODE_PATH', 'NODE_OPTIONS',
        'X0X_CDC', 'X0X_DEBUG', 'X0X_BREAK_BANK', 'X0X_RAM_CANARY', 'X0X_SMK_LCD_BAUD',
        'X0X_FONTSET', 'X0X_FONT_DIRS', 'CTL_NATIVE', 'CTL_NO_CACHE', 'X0X_TEST_TIME_SCALE', 'BOARD', 'MODEL')}
    payload = {'schema': SCHEMA, 'mode': mode, 'source': source_tree(repo, commit),
               'configuration': config, 'environment': settings,
               'platform': [platform.system(), platform.machine(), platform.release()],
               'validator': {name: content(runner_path if name == 'gate.sh' and runner_path else runner_dir / name) for name in runners},
               'tools': tools, 'python_packages': packages,
               'dependencies': dependencies, 'references': references}
    return {'schema': SCHEMA, 'sha256': digest(encoded(payload)), 'inputs': payload}


def validate(record, expected=None):
    if (not isinstance(record, dict) or record.get('schema') != SCHEMA or
            not isinstance(record.get('inputs'), dict) or
            record['inputs'].get('schema') != SCHEMA or
            record.get('sha256') != digest(encoded(record['inputs']))):
        raise ValueError('missing or invalid validation-input identity')
    if expected is not None and record['sha256'] != expected['sha256']:
        raise ValueError('validation inputs changed')
    return record['sha256']


def main():
    p = argparse.ArgumentParser(description=__doc__)
    sub = p.add_subparsers(dest='action', required=True)
    cap = sub.add_parser('capture')
    for name in ('repo', 'commit', 'toolchain', 'sdk', 'refs', 'hook'):
        cap.add_argument('--' + name, required=True)
    cap.add_argument('--pins', default=PINS)
    cap.add_argument('--runner')
    verify = sub.add_parser('verify')
    verify.add_argument('file')
    verify.add_argument('--sha256', required=True)
    verify.add_argument('--source', required=True)
    a = p.parse_args()
    try:
        if a.action == 'capture':
            print(json.dumps(capture(a.repo, a.commit, 'gate',
                {'models': ['elite', 'pro', 'fm1'], 'configs': {'dev': 1, 'rel': 0}, 'time_scale': 1},
                a.toolchain, a.sdk, a.refs, a.pins, hook=a.hook, runner_path=a.runner), sort_keys=True))
        else:
            record = json.loads(Path(a.file).read_text())
            if (validate(record) != a.sha256 or record['inputs'].get('source') != a.source or
                    record['inputs'].get('mode') != 'gate'):
                raise ValueError('validation-input provenance mismatch')
    except (OSError, ValueError, subprocess.SubprocessError) as e:
        print('validation inputs: ' + str(e), file=sys.stderr)
        return 2
    return 0


if __name__ == '__main__':
    sys.exit(main())
