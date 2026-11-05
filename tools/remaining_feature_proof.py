#!/usr/bin/env python3
"""Host-only source/test receipt for a bounded DnBOX feature patch.

Do not use this as independent QA, firmware validation or device acceptance.
The manifest ties the *actual command exits and logs* to committed source bytes.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys


def command(*argv, cwd):
    return subprocess.check_output(argv, cwd=cwd, stderr=subprocess.PIPE, text=True).strip()


def digest(data):
    return hashlib.sha256(data).hexdigest()


def source_snapshot(root, paths):
    if not paths:
        raise ValueError('No explicit source inputs')
    out = []
    for name in paths:
        if (not isinstance(name, str) or not name or '\\' in name or
                name.startswith('/') or any(p in ('.', '..', '') for p in name.split('/'))):
            raise ValueError(f'Unsafe input path: {name!r}')
        path = root / name
        if path.is_symlink() or not path.is_file():
            raise ValueError(f'Missing or symlink input: {name}')
        expected = command('git', 'rev-parse', f'HEAD:{name}', cwd=root)
        actual = command('git', 'hash-object', '--', name, cwd=root)
        if actual != expected:
            raise ValueError(f'Dirty or stale tracked input: {name}')
        out.append({'path': name, 'git_blob': expected, 'sha256': digest(path.read_bytes())})
    return out


def check_commit(root, previous=None):
    head = command('git', 'rev-parse', 'HEAD', cwd=root)
    tree = command('git', 'rev-parse', 'HEAD^{tree}', cwd=root)
    if previous and (head != previous['head'] or tree != previous['tree']):
        raise ValueError('Source HEAD/tree changed')
    return head, tree


def toolchain_snapshot(checks=()):
    """Pin the compiler, host tools and every check executable (including Node)."""
    names = {'compiler': (shlex.split(os.environ.get('CC', 'cc')) or ['cc'])[0],
             'python3': 'python3', 'shell': 'sh'}
    # Include the actual launcher for each declared check. Without this, a
    # different Node or tool binary could reuse a previous HOST_CHECKED receipt.
    # Missing executables are recorded as unavailable; run_checks records 127.
    tools = {}
    for check in checks:
        exe = check['argv'][0]
        found = shutil.which(exe)
        if not found:
            tools['check:' + exe] = {'path': None, 'sha256': None}
            continue
        binary = Path(found).resolve(strict=True)
        tools['check:' + exe] = {'path': str(binary), 'sha256': digest(binary.read_bytes())}
    for role, name in names.items():
        found = shutil.which(name)
        if not found:
            raise ValueError(f'Missing tool: {name}')
        binary = Path(found).resolve(strict=True)
        tools[role] = {'path': str(binary), 'sha256': digest(binary.read_bytes())}
    env = {k: os.environ.get(k, '') for k in (
        'CC', 'CFLAGS', 'LDFLAGS', 'X0X_HOST_DEFS', 'X0X_BREAK_BANK',
        'DRUM_IDENTITY_OUT', 'PYTHONPATH')}
    return {'tools': tools, 'environment': env}


def validate_plan(plan):
    if set(plan) != {'inputs', 'checks'}:
        raise ValueError('Plan requires exactly inputs and checks')
    if not isinstance(plan['inputs'], list) or len(plan['inputs']) != len(set(plan['inputs'])):
        raise ValueError('Missing/duplicated inputs')
    if not isinstance(plan['checks'], list) or not plan['checks']:
        raise ValueError('No host checks')
    seen = set()
    for check in plan['checks']:
        if set(check) != {'name', 'argv'} or not re.fullmatch('[a-z0-9][a-z0-9-]{0,60}', check['name']):
            raise ValueError(f'Invalid check: {check}')
        if check['name'] in seen or not isinstance(check['argv'], list) or not check['argv']:
            raise ValueError('Duplicated/empty check')
        if not all(isinstance(a, str) and a and '\x00' not in a for a in check['argv']):
            raise ValueError('Invalid argument vector')
        seen.add(check['name'])


def run_checks(root, plan, output):
    # A later unsuccessful attempt must never leave a previous PASS reusable.
    # Invalidate before validating the plan, input tree or host executables.
    output.mkdir(parents=True, exist_ok=True)
    (output / 'receipt.json').unlink(missing_ok=True)
    validate_plan(plan)
    head, tree = check_commit(root)
    toolchain = toolchain_snapshot(plan['checks'])
    inputs = source_snapshot(root, plan['inputs'])
    results = []
    failure = False
    for check in plan['checks']:
        log = output / (check['name'] + '.log')
        try:
            p = subprocess.run(check['argv'], cwd=root, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, timeout=180, check=False)
            data, code = p.stdout, p.returncode
        except subprocess.TimeoutExpired as e:
            data, code = (e.stdout or b'') + b'\nTIMEOUT\n', 124
        except OSError as e:
            # Missing or unlaunchable executable is a failed check, not an
            # exception that can strand a previous HOST_CHECKED receipt.
            data, code = f'FAILED TO LAUNCH: {type(e).__name__}: {e}\n'.encode(), 127
        log.write_bytes(data)
        failure |= code != 0
        results.append({'name': check['name'], 'argv': check['argv'], 'exit_code': code,
                        'log': log.name, 'log_sha256': digest(data)})
    try:
        new_head, new_tree = check_commit(root)
        if new_head != head or new_tree != tree or source_snapshot(root, plan['inputs']) != inputs:
            failure = True
    except (ValueError, subprocess.CalledProcessError):
        failure = True
    receipt = {'schema': 1, 'repo': 'gobbajogger/X0X37-for-Loopy-Pro',
               'head': head, 'tree': tree, 'inputs': inputs, 'toolchain': toolchain,
               'checks': results, 'status': 'FAIL' if failure else 'HOST_CHECKED',
               'limits': 'Scoped host commands only; not independent QA, firmware build, model, device or release PASS'}
    (output / 'receipt.json').write_text(json.dumps(receipt, indent=2, sort_keys=True) + '\n')
    return 1 if failure else 0


def verify(root, output):
    receipt = json.loads((output / 'receipt.json').read_text())
    if set(receipt) != {'schema', 'repo', 'head', 'tree', 'inputs', 'checks', 'status', 'limits', 'toolchain'} or receipt['schema'] != 1:
        raise ValueError('Bad receipt schema')
    if receipt['repo'] != 'gobbajogger/X0X37-for-Loopy-Pro':
        raise ValueError('Unexpected repository')
    check_commit(root, receipt)
    if toolchain_snapshot(receipt['checks']) != receipt['toolchain']:
        raise ValueError('Host compiler/runtime/environment changed')
    if source_snapshot(root, [a['path'] for a in receipt['inputs']]) != receipt['inputs']:
        raise ValueError('Checked inputs changed')
    if receipt['status'] != 'HOST_CHECKED' or not receipt['checks']:
        raise ValueError('Tests did not pass')
    for check in receipt['checks']:
        if check['exit_code'] != 0 or (output / check['log']).is_symlink():
            raise ValueError('Failed check or symlink log')
        if (output / check['log']).name != check['name'] + '.log':
            raise ValueError('Invalid log path')
        if digest((output / check['log']).read_bytes()) != check['log_sha256']:
            raise ValueError('Check log changed')
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=('run', 'verify'))
    parser.add_argument('--plan', type=Path)
    parser.add_argument('--repo', type=Path, default=Path.cwd())
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    try:
        root = args.repo.resolve(strict=True)
        if command('git', 'rev-parse', '--show-toplevel', cwd=root) != str(root):
            raise ValueError('Not an exact Git working-tree root')
        output = args.out.resolve()
        if args.mode == 'run':
            # Invalidate even if the JSON cannot be read or parsed. A run that
            # did not begin is still not authority to reuse a stale PASS.
            output.mkdir(parents=True, exist_ok=True)
            (output / 'receipt.json').unlink(missing_ok=True)
            if not args.plan:
                raise ValueError('--plan required for run')
            code = run_checks(root, json.loads(args.plan.read_text()), output)
        else:
            code = verify(root, output)
        print('HOST CHECKED' if code == 0 else 'FAIL')
        return code
    except (ValueError, OSError, subprocess.CalledProcessError, KeyError,
            json.JSONDecodeError) as error:
        print(f'FAIL feature proof: {error}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
