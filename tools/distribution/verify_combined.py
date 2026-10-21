#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read-only verification of a derived Elite DEV distribution. Never opens MIDI."""
from pathlib import Path
import argparse
import hashlib
import io
import json
import re
import subprocess
import sys

RAW = 'evidence/validated-build/x0x-smk37-elite.fwsc'
CTL = 'evidence/validated-build/ctl.img'
RECORD = 'evidence/validated-build/record.json'
COMBINED = 'firmware/x0x-smk37-elite-with-bt.fwsc'
FILES = (RAW, CTL, RECORD, COMBINED)
PRODUCT = 'SMK-37 Elite_900'


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def verify(kit):
    kit = Path(kit).resolve()
    manifest = json.loads((kit / 'MANIFEST.json').read_text())
    if manifest.get('schema') != 'x0x37-derived-elite-dev-v1':
        raise ValueError('unsupported distribution manifest')
    commit = manifest['runtime_commit']
    if not re.fullmatch(r'[0-9a-f]{40}', commit):
        raise ValueError('invalid runtime commit')
    if manifest['model'] != 'elite' or manifest['config'] != 'dev':
        raise ValueError('this distribution supports Elite DEV only')
    repo = kit / 'x0x37'
    def git(*args):
        return subprocess.check_output(['git', '--no-optional-locks', '-C', str(repo), *args], text=True).strip()
    if git('rev-parse', 'HEAD') != commit or git('rev-parse', 'HEAD^{tree}') != manifest['runtime_git_tree']:
        raise ValueError('runtime source identity mismatch')
    subprocess.run(['git', '--no-optional-locks', '-c', 'diff.autoRefreshIndex=false',
                    '-C', str(repo), 'diff', '--exit-code', '--quiet', 'HEAD', '--'], check=True)
    if set(manifest['identities']) != set(FILES):
        raise ValueError('artifact identity set mismatch')
    for name in FILES:
        if digest(kit / name) != manifest['identities'][name]:
            raise ValueError('artifact identity mismatch: ' + name)
    host = manifest.get('host_tools')
    install_tools = repo / 'tools'
    if host is not None:
        if not re.fullmatch(r'[0-9a-f]{40}', host['commit']):
            raise ValueError('invalid host tools commit')
        root = kit / 'host-tools'
        expected = host['files']
        actual = {p.relative_to(root).as_posix() for p in root.rglob('*') if p.is_file()}
        if actual != set(expected):
            raise ValueError('host tools file set mismatch')
        for name, info in expected.items():
            path = root / name
            if not name.startswith('tools/') or path.is_symlink() or not path.resolve().is_relative_to(root.resolve()):
                raise ValueError('unsafe host tools path')
            if digest(path) != info['sha256']:
                raise ValueError('host tools identity mismatch: ' + name)
            raw = path.read_bytes()
            blob = hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest()
            if blob != info['git_blob']:
                raise ValueError('host tools Git blob mismatch: ' + name)
        install_tools = root / 'tools'
    sys.path.insert(0, str(install_tools))
    import add_ctl
    import dev_validate
    import fm1_install
    raw_path, ctl_path = kit / RAW, kit / CTL
    record = json.loads((kit / RECORD).read_text())
    if record['commit'] != commit or record['model'] != 'elite' or not record['controller']:
        raise ValueError('original DEV runtime/model/controller identity mismatch')
    dev_validate.verify(repo, commit, kit / RECORD, raw_path, ctl_path)
    raw, ctl = raw_path.read_bytes(), ctl_path.read_bytes()
    combined = (kit / COMBINED).read_bytes()
    product, logical = add_ctl.logical_of(combined)
    raw_product, raw_logical = add_ctl.logical_of(raw)
    if product != PRODUCT or raw_product != PRODUCT:
        raise ValueError('Elite model identity mismatch')
    if add_ctl.ctl_problem(ctl) or add_ctl.controller_of(logical) != ctl:
        raise ValueError('embedded controller ABI/header/CRC/identity mismatch')
    content = manifest.get('content_identities')
    if content is not None:
        size = add_ctl.CODE_MAX - record['budgets']['code_free']
        if not 0 < size <= add_ctl.CODE_MAX:
            raise ValueError('invalid recorded application code size')
        app = add_ctl._open(logical)['area'][0x120:0x120 + size]
        if content != {'app_bin_bytes': size, 'app_bin_sha256': hashlib.sha256(app).hexdigest(),
                       'combined_logical_sha256': hashlib.sha256(logical).hexdigest()}:
            raise ValueError('decoded package identity mismatch')
    if add_ctl.add(raw, ctl) != combined or add_ctl.add(combined, None) != raw:
        raise ValueError('combined artifact is not the exact reversible native derivation')
    args = argparse.Namespace(ctl=str(ctl_path), no_ctl=False)
    native = fm1_install.with_ctl(args, raw_product, raw_logical, io.StringIO())
    if native != logical or args.ctl_bytes != ctl:
        raise ValueError('native installer bytes differ from the distributed combined firmware')
    args = argparse.Namespace(ctl=None, no_ctl=True)
    if fm1_install.with_ctl(args, product, logical, io.StringIO()) != logical or args.ctl_bytes != ctl:
        raise ValueError('native embedded-controller consumer mismatch')
    print('COMBINED OK: Elite DEV, exact embedded controller and native install bytes; original DEV evidence unchanged.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--kit', type=Path, default=Path(__file__).resolve().parent)
    args = parser.parse_args()
    try:
        verify(args.kit)
    except (OSError, ValueError, KeyError, TypeError, subprocess.CalledProcessError) as error:
        print('REFUSED: ' + str(error), file=sys.stderr)
        sys.exit(3)
