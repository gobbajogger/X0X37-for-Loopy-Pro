#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Plan one Quickcheck invocation. Unknown scope retains target coverage.

Git names are NUL delimited, with rename detection disabled so both sides remain
visible. The test source index is private to this call; it is never cached.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess
import sys

FAST = set('fastmath sequencer encoder uac trs undo lz motion smk-flash smk-storage smk-apc40-map smk-clip-keys locked-settings'.split())
TEST_ROUTES = [
    ('firmware/src/dsp/drum909', 'drum909 drum909-adpcm drum-pan'),
    ('firmware/src/dsp/drum808', 'drum808 drum-pan'),
    ('firmware/src/dsp/bass303', 'bass303 bass303_lite'),
    ('firmware/src/dsp/breaks', 'breaks'),
    ('firmware/src/dsp/', 'master stereo fastmath'),
    ('firmware/src/seq/', 'sequencer motion loop-bars rec-live'),
    ('firmware/src/storage', 'smk-storage'),
    ('firmware/hal/fm1_flash_map.h', 'smk-flash smk-storage'),
    ('firmware/src/ble', 'ble smk-clip-ble-clear smk-clip-ble-notify diag-size'),
    ('firmware/src/lcd', 'smk-clip-dark'),
    ('tools/fm1_install.py', 'installer installer-slots installer-ctl installer-targets'),
    ('tools/fm1pkg_make.py', 'smk-package'),
    ('tools/upload_breaks.py', 'upload-images upload-tools breaks-web'),
    # FP-001 dual-digest host check (source fingerprint ≠ served install HTML).
    ('tools/check_installer_digests.py', 'installer-digests'),
    ('tests/host/test_check_installer_digests.py', 'installer-digests'),
    # Project companion modules and their shared breaks helper need the project
    # suites; the generic web/ route never names projects-web / projects-page.
    ('web/x0x_projects.js', 'projects-web projects-page'),
    ('web/x0x_projects.html', 'projects-web projects-page'),
    ('web/x0x_breaks.js', 'projects-web projects-page'),
    ('web/test_projects.mjs', 'projects-web'),
    ('web/test_projects_page.mjs', 'projects-page'),
    ('web/', 'breaks-web breaks-page install-slots-web'),
    ('tools/locked_settings.json', 'locked-settings'),
]


def under(path, prefix):
    return path == prefix or path.startswith(prefix + '/')



def route(path):
    # Unusual names are valid Git input, but not proven non-target inputs.
    if not re.fullmatch(r'[A-Za-z0-9_./-]+', path):
        return 'target'
    if path.startswith(('tests/', 'host/', 'web/')):
        return 'checks'
    if (path.startswith('docs/') or
            ('/' not in path and path.endswith('.md'))):
        return 'docs'
    return 'target'


def git(root, *args):
    return subprocess.check_output(['git', '-C', str(root), *args])


def scope(root, base, head, folder):
    # Verify history separately. An empty/error diff is never proof of docs-only.
    git(root, 'cat-file', '-e', base + '^{commit}')
    git(root, 'cat-file', '-e', head + '^{commit}')
    git(root, 'rev-list', base + '..' + head)
    streams = [git(root, 'diff', '--no-ext-diff', '--no-renames', '--name-only', '-z', base, head, '--')]
    if folder:
        streams += [git(root, 'diff', '--no-ext-diff', '--no-renames', '--name-only', '-z', 'HEAD', '--'),
                    git(root, 'ls-files', '--others', '--exclude-standard', '-z')]
    return sorted({p.decode('utf-8', 'surrogateescape') for data in streams for p in data.split(b'\0') if p})


# Known simulator commands with no upload-image dependency. Unknown commands or
# external slot inputs keep upload generation. This list follows host/x0x_host.c.
SCENARIO_LOCAL = set('wait press release tap key tapkey keyvel turn arrow clipmode spin param master midi blelinkend usblinkend wav snapline load peakreset wavstop shot leds expect projimg storedump storeload noise cpu2 storefail storepoke reboot echo'.split())


def scenario_needs_upload(path):
    try:
        text = path.read_text()
    except (OSError, UnicodeError):
        return True
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        if 'build/test_upload' in line or line.split()[0] not in SCENARIO_LOCAL:
            return True
    return False


def select(root, changed, target):
    if changed and all(route(p) == 'docs' for p in changed):
        return []
    runner = root / 'tests/run_tests.sh'
    if not runner.exists():
        return []  # A target build can fail first; successful builds still require reports.
    raw = runner.read_text()
    src = raw.replace('\\\n', ' ')
    cmds = {}
    for line in src.splitlines():
        if line.lstrip().startswith('#'):
            continue
        for m in re.finditer(r'(?:^|[\s;&|])run "?([A-Za-z0-9_.-]+)"?\s+(.*)', line):
            cmds.setdefault(m[1], m[2])
    for name in ('drum909', 'drum808', 'bass303', 'breaks'):
        if (root / ('tests/host/run_' + name + '.sh')).exists():
            cmds[name] = 'sh tests/host/run_' + name + '.sh'
    scen = {'scenario-' + p.stem for p in (root / 'tests/scenarios').glob('*.x0x')}
    # Read each test source once, even when many changed paths share a basename.
    index = []
    def walk_error(error):
        raise error
    import os
    for directory, dirs, files in os.walk(root / 'tests', onerror=walk_error):
        dirs.sort()
        for name in sorted(files):
            f = Path(directory) / name
            rel = f.relative_to(root).as_posix()
            if rel.startswith('tests/scenarios/') or not f.is_file():
                continue
            index.append((rel, raw if f == runner else f.read_text(errors='ignore'),
                          {n for n, command in cmds.items() if rel in command}))
    sel = {'locked-settings', 'smk-flash'}
    for path in changed:
        if route(path) == 'docs':
            continue
        hit = {n for n, command in cmds.items() if path in command}
        base = Path(path).name
        for rel, text, names in index:
            if rel != path and base in text:
                hit |= names
        for prefix, names in TEST_ROUTES:
            if path.startswith(prefix):
                hit.update(names.split())
        if path.startswith('tests/scenarios/') and path.endswith('.x0x'):
            # A deleted scenario cannot be run; all remaining scenarios still can.
            hit |= {'host-build'} | ({'scenario-' + base[:-4]} & scen)
        if path.startswith(('firmware/src/app/', 'firmware/src/seq/', 'firmware/src/dsp/', 'host/')):
            hit |= {'host-build'} | scen
        if path.startswith('firmware/') and not hit:
            hit |= FAST
        if path == 'tests/run_tests.sh':
            hit |= FAST
        sel |= hit
    scenarios = [n for n in sel if n.startswith('scenario-')]
    if scenarios:
        sel.add('host-build')
        if any(scenario_needs_upload(root / 'tests/scenarios' / (n[len('scenario-'):] + '.x0x')) for n in scenarios):
            sel.add('upload-images')
    if len(sel) <= 2:
        sel |= FAST
    return sorted(n for n in sel if n in cmds or n in scen)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('root', type=Path)
    ap.add_argument('base')
    ap.add_argument('head')
    ap.add_argument('--folder', action='store_true')
    ap.add_argument('--scope-only', action='store_true')
    args = ap.parse_args()
    changed = scope(args.root, args.base, args.head, args.folder)
    target = not changed or any(route(p) == 'target' for p in changed)
    plan = {'changed': changed, 'target': target,
            'reason': 'empty scope or target/unknown input' if target else 'proven documentation/web/host/test inputs',
            'checks': target or any(route(p) != 'docs' for p in changed),
            'docs': any(route(p) == 'docs' or p.startswith('docs/') for p in changed),
            'tests': [] if args.scope_only else select(args.root, changed, target)}
    json.dump(plan, sys.stdout, indent=2, ensure_ascii=True)
    print()


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        print('Quickcheck planning failed: ' + str(error), file=sys.stderr)
        sys.exit(1)
