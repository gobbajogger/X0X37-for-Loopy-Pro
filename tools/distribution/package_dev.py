#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Derive an Elite DEV install kit from exact validated artifacts, without a build.

The sole user-facing firmware always embeds its matching Bluetooth controller.
Original raw inputs, record and logs are retained internally, never relabeled.
No device, package manager, SDK, toolchain or network operation is performed.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import shutil
import stat
import subprocess
import sys
import zipfile

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
TOOL_REPO = HERE.parent.parent
import source_inventory


def git(repo, *args):
    return subprocess.check_output(['git', '-C', str(repo), *args], text=True).strip()


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def copy_file(src, dst):
    if not src.is_file() or src.is_symlink():
        raise ValueError('input must be a regular file: ' + src.name)
    shutil.copy2(src, dst)


def make_zip(kit, destination):
    if destination.exists():
        raise ValueError('ZIP destination already exists')
    with zipfile.ZipFile(destination, 'x', zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(kit.rglob('*')):
            relative = path.relative_to(kit.parent).as_posix()
            if path.is_symlink():
                info = zipfile.ZipInfo(relative)
                info.create_system = 3
                info.external_attr = (stat.S_IFLNK | 0o777) << 16
                archive.writestr(info, os.readlink(path))
            elif path.is_file() or path.is_dir():
                # Git still needs empty structural directories, such as refs/.
                archive.write(path, relative)


def export_host_tools(source, output):
    """Export committed tools only; never overlay the runtime checkout."""
    source = source.resolve()
    if git(source, 'status', '--porcelain'):
        raise ValueError('host tools repository must be clean')
    commit = git(source, 'rev-parse', 'HEAD')
    inventory = source_inventory.scan(source, commit)
    files = {}
    for row in git(source, 'ls-tree', '-r', commit, 'tools').splitlines():
        info, name = row.split('\t', 1)
        mode, kind, blob = info.split()
        if kind != 'blob' or mode not in ('100644', '100755'):
            raise ValueError('unsupported host tools source entry: ' + name)
        target = output / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(subprocess.check_output(['git', '-C', str(source), 'cat-file', 'blob', blob]))
        target.chmod(int(mode[-3:], 8))
        files[name] = {'sha256': digest(target), 'git_blob': blob, 'mode': mode}
    for required in ('dev_validate.py', 'fm1_install.py', 'require_gate_pass.sh', 'add_ctl.py', 'validation_inputs.py'):
        if 'tools/' + required not in files:
            raise ValueError('required host tool missing: ' + required)
    return {'commit': commit, 'git_tree': git(source, 'rev-parse', commit + '^{tree}'),
            'tools_git_tree': git(source, 'rev-parse', commit + ':tools'),
            'files': files, 'source_history_inventory': inventory,
            'scope': 'Exact committed tools-only snapshot; not runtime firmware source.'}


def build(source, result, output, zip_path=None, host_tools=None, historical_baseline=False):
    source, result, output = source.resolve(), result.resolve(), output.absolute()
    if zip_path:
        zip_path = zip_path.absolute()
        if zip_path.resolve().is_relative_to(output.resolve()):
            raise ValueError('ZIP must be outside the kit directory')
    if output.exists() or (zip_path and zip_path.exists()):
        raise ValueError('use a new output directory and ZIP path')
    record_path = result / 'record.json'
    record = json.loads(record_path.read_text())
    if (record.get('schema') != 2 or record.get('result') != 'DEV CHECKED' or
            record.get('config') != 'dev' or record.get('model') != 'elite' or
            record.get('controller') is not True):
        raise ValueError('an exact Elite DEV CHECKED record with controller is required')
    commit = record['commit']
    if not re.fullmatch(r'[0-9a-f]{40}', commit):
        raise ValueError('record must name a full runtime commit')
    if historical_baseline and commit != '53d5654c9bbaec10b64c55fb60d0e7d857ba8156':
        raise ValueError('historical baseline mode requires the exact owner-selected 53d source')
    if historical_baseline and host_tools is None:
        raise ValueError('historical baseline requires separately identified maintained host tools')
    git(source, 'cat-file', '-e', commit + '^{commit}')
    inventory = source_inventory.scan(source, commit)
    # Fetch only this commit's reachable history. Never copy untracked inputs,
    # credentials, source repository config, unrelated refs or worktree files.
    output.mkdir(parents=True)
    repo = output / 'x0x37'
    subprocess.run(['git', 'init', '--template=', '-q', str(repo)], check=True)
    subprocess.run(['git', '-C', str(repo), '-c', 'core.hooksPath=/dev/null', 'fetch',
                    '--quiet', '--no-tags', '--update-shallow', str(source), commit], check=True)
    subprocess.run(['git', '-C', str(repo), '-c', 'core.hooksPath=/dev/null',
                    'checkout', '--quiet', '--detach', 'FETCH_HEAD'], check=True)
    (repo / '.git/FETCH_HEAD').unlink(missing_ok=True)
    # Reflogs contain local transfer paths and are unnecessary for source history.
    shutil.rmtree(repo / '.git/logs', ignore_errors=True)
    source_inventory.validate_links(repo)
    inputs = output / 'evidence/validated-build'
    inputs.mkdir(parents=True)
    names = ['record.json', 'x0x-smk37-elite.fwsc', 'ctl.img']
    names += [name + '.log' for name in record['logs']]
    for name in names:
        if not re.fullmatch(r'(record\.json|x0x-smk37-elite\.fwsc|ctl\.img|(?:build|controller|check-[0-9]+)\.log)', name):
            raise ValueError('unexpected validation log name')
        copy_file(result / name, inputs / name)
    host_identity = export_host_tools(host_tools, output / 'host-tools') if host_tools else None
    install_tools = output / 'host-tools/tools' if host_identity else repo / 'tools'
    sys.path.insert(0, str(install_tools))
    import dev_validate
    import add_ctl
    dev_validate.verify(repo, commit, inputs / 'record.json', inputs / 'x0x-smk37-elite.fwsc', inputs / 'ctl.img')
    raw = (inputs / 'x0x-smk37-elite.fwsc').read_bytes()
    ctl = (inputs / 'ctl.img').read_bytes()
    if add_ctl.logical_of(raw)[0] != 'SMK-37 Elite_900':
        raise ValueError('raw package is not Elite')
    firmware = output / 'firmware'
    firmware.mkdir()
    (firmware / 'x0x-smk37-elite-with-bt.fwsc').write_bytes(add_ctl.add(raw, ctl))
    copy_file(HERE / 'verify_combined.py', output / 'verify_combined.py')
    template = (HERE / 'Install.command.in').read_text()
    if template.count('@RUNTIME_COMMIT@') != 1:
        raise ValueError('launcher runtime identity placeholder mismatch')
    for placeholder in ('@HOST_TOOLS_DIR@', '@POST_INSTALL_WATCH@'):
        if template.count(placeholder) != 1:
            raise ValueError('launcher placeholder mismatch: ' + placeholder)
    template = template.replace('@RUNTIME_COMMIT@', commit)
    template = template.replace('@HOST_TOOLS_DIR@', 'host-tools/tools' if host_identity else 'x0x37/tools')
    template = template.replace('@POST_INSTALL_WATCH@', 'manual' if historical_baseline else 'automatic')
    (output / 'Install.command').write_text(template)
    (output / 'Install.command').chmod(0o755)
    for name in ['LICENSE', 'LICENSING.md']:
        copy_file(TOOL_REPO / name, output / name)
    shutil.copytree(repo / 'LICENSES', output / 'LICENSES')
    combined = (firmware / 'x0x-smk37-elite-with-bt.fwsc').read_bytes()
    _, logical = add_ctl.logical_of(combined)
    app_size = add_ctl.CODE_MAX - record['budgets']['code_free']
    if not 0 < app_size <= add_ctl.CODE_MAX:
        raise ValueError('invalid recorded application code size')
    app = add_ctl._open(logical)['area'][0x120:0x120 + app_size]
    content_identities = {'app_bin_bytes': app_size,
                          'app_bin_sha256': hashlib.sha256(app).hexdigest(),
                          'combined_logical_sha256': hashlib.sha256(logical).hexdigest()}
    identities = {f'evidence/validated-build/{name}': digest(inputs / name)
                  for name in ['x0x-smk37-elite.fwsc', 'ctl.img', 'record.json']}
    identities['firmware/x0x-smk37-elite-with-bt.fwsc'] = digest(firmware / 'x0x-smk37-elite-with-bt.fwsc')
    manifest = {'schema': 'x0x37-derived-elite-dev-v1', 'runtime_commit': commit,
                'runtime_git_tree': git(repo, 'rev-parse', 'HEAD^{tree}'),
                'packaging_tools_commit': git(TOOL_REPO, 'rev-parse', 'HEAD'),
                'packaging_tools_clean': not bool(git(TOOL_REPO, 'status', '--porcelain', '--',
                                                      'tools/distribution', 'LICENSING.md')),
                'packaging_tools_sha256': {name: digest(HERE / name) for name in
                    ['package_dev.py', 'source_inventory.py', 'verify_combined.py', 'Install.command.in']},
                'model': 'elite', 'config': 'dev', 'runtime_rebuild': False,
                'host_tools': host_identity,
                'presentation': 'historical-baseline-rebuild' if historical_baseline else 'development-candidate',
                'post_install_watch': 'manual-only' if historical_baseline else 'automatic-45s',
                'identities': identities, 'content_identities': content_identities,
                'source_history_inventory': inventory,
                'source_history_shallow': git(repo, 'rev-parse', '--is-shallow-repository') == 'true',
                'limitations': ['No actual macOS/device installation test is implied.',
                    'The original DEV record and its check scope are unchanged.',
                    'Present queue veto is retained; absent queue is NOT CHECKED.',
                    'Dependency setup may require network; verify-only does not.']}
    (output / 'MANIFEST.json').write_text(json.dumps(manifest, indent=2) + '\n')
    scope = record['plan']['scope']
    leak = any('detect_leaks=0' in word for check in record['plan'].get('checks', []) for word in check['argv'])
    (output / 'BUILD_REPORT.txt').write_text(
        f'Runtime: {commit}\nPackaging tools are separate from runtime source. Packaging did not rebuild firmware.\n'
        f'Original validation scope: {scope}\n'
        + ('LEAK CHECK NOT RUN: leak detection was explicitly disabled in the original plan.\n' if leak else '')
        + 'Read evidence/validated-build/record.json and logs for exact checked inputs and limitations.\n'
        + 'No new device result or SMC symptom fix is claimed.\n')
    for name in ['README_EN.txt', 'README_RU.txt']:
        copy_file(HERE / (name.replace('README_', 'README_HISTORICAL_') if historical_baseline else name), output / name)
    (output / 'README.txt').write_text('Read README_EN.txt or README_RU.txt. Start with Install.command --verify-only.\n')
    subprocess.run([sys.executable, '-B', str(output / 'verify_combined.py')], check=True)
    # Reject escaping/broken links before hashing the materialized source tree.
    source_inventory.validate_links(output)
    sums = ''.join(f'{digest(path)}  {path.relative_to(output).as_posix()}\n'
                   for path in sorted(output.rglob('*')) if path.is_file() and path.name != 'SHA256SUMS')
    (output / 'SHA256SUMS').write_text(sums)
    if zip_path:
        make_zip(output, zip_path)
    return manifest


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-repo', type=Path, required=True)
    parser.add_argument('--validated-result', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--zip', type=Path)
    parser.add_argument('--host-tools-repo', type=Path)
    parser.add_argument('--historical-baseline', action='store_true')
    args = parser.parse_args()
    try:
        result = build(args.source_repo, args.validated_result, args.output, args.zip, args.host_tools_repo, args.historical_baseline)
        print(json.dumps(result, indent=2))
    except (OSError, ValueError, KeyError, TypeError, subprocess.CalledProcessError) as error:
        print('REFUSED: ' + str(error), file=sys.stderr)
        sys.exit(3)
