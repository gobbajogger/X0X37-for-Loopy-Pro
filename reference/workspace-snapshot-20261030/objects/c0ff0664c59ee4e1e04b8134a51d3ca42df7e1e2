#!/usr/bin/env python3
"""Content identity for validation reuse, not permission to build or install.

Capture before and after validation. Hash dependency bytes (including dirty and
untracked reference files), resolved host tools, configuration and external runners.
No mtime cache: replacing bytes at the same path must invalidate evidence.
Install consumers check recorded provenance; they need not have the build SDK.
"""
import argparse
import csv
import hashlib
import importlib.metadata
import importlib.machinery
import json
import os
from pathlib import Path
import platform
import shutil
import stat
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
    return _content_resolved(Path(path).resolve(strict=True), ancestors, exclude)


def _content_resolved(actual, ancestors, exclude):
    if actual in ancestors:
        raise ValueError('dependency symlink cycle: ' + str(actual))
    info = actual.stat()
    if stat.S_ISREG(info.st_mode):
        return {'sha256': file_hash(actual), 'executable': bool(info.st_mode & 0o111)}
    if not stat.S_ISDIR(info.st_mode):
        raise ValueError('not a dependency file/directory: ' + str(actual))
    # Git metadata is mutable bookkeeping, not a build input.
    with os.scandir(actual) as scan:
        entries = sorted(scan, key=lambda entry: entry.name)
    children = {}
    for entry in entries:
        if entry.name in ('.git', '__pycache__', *exclude):
            continue
        child = Path(entry.path)
        if entry.is_symlink():
            child = child.resolve(strict=True)
        children[entry.name] = _content_resolved(child, (*ancestors, actual), exclude)
    return {'sha256': digest(encoded(children))}


def git(repo, *args):
    return subprocess.check_output(['git', '-C', str(repo), *args], text=True).strip()


def source_tree(repo, commit):
    rows = git(repo, 'ls-tree', '-r', '--full-tree', commit).splitlines()
    return digest(''.join(r + '\n' for r in rows if not r.split('\t', 1)[-1].endswith('.md')).encode())


def _manifest_paths(dist):
    """Declared install paths, including ones no longer on disk.

    importlib.metadata.Distribution.files drops missing paths. A package that
    lost a declared file would then fingerprint as a smaller package.
    installed-files.txt is relative to the egg-info directory. A console script
    often lives in the env bin directory, outside site-packages. Keep that as an
    upward-relative path from the distribution root so locate_file can still
    open it. Do not require every declared file to reside below that root.
    """
    text = dist.read_text('RECORD')
    if text:
        return [row[0] for row in csv.reader(text.splitlines()) if row and row[0]]
    text = dist.read_text('installed-files.txt')
    subdir = getattr(dist, '_path', None)
    if text and subdir:
        root = Path(dist.locate_file('')).resolve()
        paths = []
        for name in text.splitlines():
            if name:
                located = (Path(subdir) / name).resolve()
                paths.append(located.relative_to(root, walk_up=True).as_posix())
        return paths
    text = dist.read_text('SOURCES.txt')
    if text:
        return [line for line in text.splitlines() if line]
    return None


def _module_roots(dist):
    """Top-level module names from top_level.txt, or from entry points when it is absent.

    A distro package can ship entry_points.txt and omit both RECORD and
    top_level.txt. An empty or unsafe top_level.txt is not replaced: that file
    was present and does not name modules.
    """
    text = dist.read_text('top_level.txt')
    if text is None:
        roots = []
        for point in dist.entry_points:
            if not point.pattern.match(point.value):
                continue
            module = point.module
            if module:
                roots.append(module.split('.', 1)[0])
        return roots
    return text.split()


def package_files(dist):
    """Hash a manifest, or declared installed modules when distro metadata omits it.

    Do not import modules: imports can execute code and select a shadowing copy.
    Missing/unsafe metadata must stop capture rather than authenticate no bytes.
    A declared manifest file that is not on disk raises OSError.
    """
    manifest = _manifest_paths(dist)
    if manifest is not None:
        files = {}
        for rel in manifest:
            if rel.endswith(('.pyc', '.pyo')):
                continue
            files[rel] = file_hash(dist.locate_file(rel))
    else:
        names = _module_roots(dist)
        if not names or any(not name.isidentifier() for name in names):
            raise ValueError('cannot enumerate Python package: missing or unsafe top_level.txt')
        root = Path(dist.locate_file('')).resolve(strict=True)
        files = {}

        def collect(path, ancestors=()):
            actual = path.resolve(strict=True)
            if root not in actual.parents or actual in ancestors:
                raise ValueError('unsafe Python package path: ' + str(path))
            if path.is_dir():
                for child in sorted(path.iterdir()):
                    if child.name != '__pycache__':
                        collect(child, (*ancestors, actual))
            elif path.is_file():
                if path.suffix not in ('.pyc', '.pyo'):
                    files[str(path.relative_to(root))] = file_hash(path)
            else:
                raise ValueError('not a Python package file/directory: ' + str(path))

        for name in sorted(set(names)):
            paths = [root / name] + [root / (name + suffix)
                                     for suffix in importlib.machinery.all_suffixes()
                                     if suffix not in ('.pyc', '.pyo')]
            installed = [p for p in paths if p.exists() or p.is_symlink()]
            if not installed:
                raise ValueError('cannot find installed Python module: ' + name)
            for path in installed:
                collect(path)
    if not files:
        raise ValueError('cannot fingerprint an empty installed Python package')
    return files


def runtime(config, env=None):
    """Hash the tools and Python packages in the command execution environment."""
    env = os.environ if env is None else env
    tools = {}
    dependencies = {}
    check_tools = []
    for check in config.get('plan', {}).get('checks', []):
        check_tools.append(check['argv'][0])
    for name in ('bash', 'sh', 'git', 'make', 'node', 'cc', 'c++', 'ld', 'ar', 'objcopy',
                 env.get('CC', 'cc'), env.get('CXX', 'c++'), sys.executable, *check_tools):
        resolved = shutil.which(name, path=env.get('PATH'))
        tools[name if name != sys.executable else 'python'] = content(resolved) if resolved else None
    # Include installed Python dependency contents, not merely version strings.
    packages = {}
    for name in sorted(set(['pillow', 'numpy'] + env.get('X0X_VALIDATION_PYTHON_PACKAGES', '').split())):
        try:
            dist = importlib.metadata.distribution(name)
        except importlib.metadata.PackageNotFoundError:
            packages[name] = None
            continue
        files = package_files(dist)
        packages[name] = {'version': dist.version, 'sha256': digest(encoded(files))}
    settings = {k: v for k, v in env.items() if k.startswith(('X0X_DEV_', 'FELUCCA_')) or k in (
        'CC', 'CXX', 'CFLAGS', 'CXXFLAGS', 'CPPFLAGS', 'LDFLAGS', 'CPATH', 'LIBRARY_PATH',
        'X0X_VERSION', 'X0X_BASS_PRESET_PREVIEW', 'X0X_BASS_TAGGED_SOUND',
        'X0X_BASS_NOTE_SOUND',
        'LD_LIBRARY_PATH', 'LD_PRELOAD', 'PYTHONPATH', 'NODE_PATH', 'NODE_OPTIONS',
        'X0X_CDC', 'X0X_DEBUG', 'X0X_BREAK_BANK', 'X0X_RAM_CANARY', 'X0X_SMK_LCD_BAUD',
        'X0X_FONTSET', 'X0X_FONT_DIRS', 'CTL_NATIVE', 'CTL_NO_CACHE', 'X0X_TEST_TIME_SCALE', 'BOARD', 'MODEL')}
    return {'tools': tools, 'python_packages': packages, 'environment': settings,
            'platform': [platform.system(), platform.machine(), platform.release()],
            'dependencies': dependencies}


def capture(repo, commit, mode, config, toolchain, sdk, refs=None, pins=PINS,
            env=None, runner_dir=None, hook=None, runner_path=None, execution=None):
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
    for check in config.get('plan', {}).get('checks', []):
        for arg in check['argv'][1:]:
            if Path(arg).is_absolute() and Path(arg).exists():
                dependencies[arg] = content(arg)
    execution = runtime(config, env) if execution is None else execution
    dependencies.update(execution['dependencies'])
    payload = {'schema': SCHEMA, 'mode': mode, 'source': source_tree(repo, commit),
               'configuration': config, 'environment': execution['environment'],
               'platform': execution['platform'],
               'validator': {name: content(runner_path if name == 'gate.sh' and runner_path else runner_dir / name) for name in runners},
               'tools': execution['tools'], 'python_packages': execution['python_packages'],
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
                {'models': ['elite', 'pro'], 'configs': {'dev': 1, 'rel': 0}, 'time_scale': 1},
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
