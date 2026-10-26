#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Plan the minimum local proof for a stage / Project PR.

Local review and stage workers use this instead of a fixed kitchen-sink
X0X_TESTS list. It never weakens package integrity: digests, DEV CHECKED
records, pool_free, and publish hash MATCH stay required when those gates
apply. Prefer skip or narrow over delete.

  python3 tools/local_proof_plan.py --base <ref> --head <ref>
  python3 tools/local_proof_plan.py --base origin/main --head HEAD --stage 4

Exit 0 always when planning succeeds. JSON on stdout.
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

# Product / harness buckets that change merge or publish decisions.
PRODUCT_PREFIXES = (
    'firmware/',
    'host/',
    'web/',
    'tools/',
    'tests/',
    'build.sh',
)
EVIDENCE_PREFIXES = ('evidence/',)
DOC_PREFIXES = ('docs/', '.cursor/skills/', '.agents/skills/', 'skills/')
DOC_ROOT_MD = re.compile(r'^[^/]+\.md$')

# Selectors that local reviews used to re-run on every stage PR even when the
# tip only added evidence/*. They stay available when the matching product
# paths change; they are not a default evidence-only tax.
PUBLISH_SELECTORS = (
    'nightly-publish',
    'installer-targets',
    'installer-digests',
    'install-gate-guard',
    'projects-web',
    'projects-page',
)
STAGE_HANDOFF = 'stage-handoff'
DEV_VALIDATE = 'dev-validate'

# Path → required host regressions for implement / test firmware PRs.
FW_HOST_ROUTES = (
    ('firmware/src/seq/', ('sequencer', 'clk-lock', 'clock-handover')),
    ('firmware/src/app/engine.c', ('sequencer',)),
    ('firmware/src/app/', ('host-build',)),
    ('firmware/src/dsp/drum909', ('drum909', 'drum909-adpcm')),
    ('firmware/src/dsp/drum808', ('drum808',)),
    ('firmware/src/dsp/bass303', ('bass303',)),
    ('firmware/src/ble', ('ble',)),
    ('tests/host/bugfinder_', ()),  # filled from basename below
)


def git(root: Path, *args: str) -> bytes:
    return subprocess.check_output(['git', '-C', str(root), *args])


def changed_paths(root: Path, base: str, head: str) -> list[str]:
    git(root, 'cat-file', '-e', base + '^{commit}')
    git(root, 'cat-file', '-e', head + '^{commit}')
    raw = git(root, 'diff', '--no-ext-diff', '--no-renames', '--name-only', '-z', base, head, '--')
    return sorted(p.decode('utf-8', 'surrogateescape') for p in raw.split(b'\0') if p)


def available_selectors(root: Path) -> set[str]:
    runner = root / 'tests' / 'run_tests.sh'
    if not runner.is_file():
        return set()
    text = runner.read_text(encoding='utf-8', errors='replace').replace('\\\n', ' ')
    names: set[str] = set()
    for line in text.splitlines():
        if line.lstrip().startswith('#'):
            continue
        for m in re.finditer(r'(?:^|[\s;&|])run "?([A-Za-z0-9_.-]+)"?\s+', line):
            names.add(m.group(1))
    return names


def bucket(path: str) -> str:
    if path.startswith(EVIDENCE_PREFIXES) or path.startswith('cloud-handoff/') or path.startswith('local-handoff/'):
        return 'evidence'
    if path.startswith(DOC_PREFIXES) or DOC_ROOT_MD.match(path) or path == 'AGENTS.md':
        return 'docs'
    if path.startswith('firmware/') or path == 'build.sh':
        return 'firmware'
    if path.startswith('web/'):
        return 'web'
    if path.startswith('tools/'):
        return 'tools'
    if path.startswith('tests/') or path.startswith('host/'):
        return 'tests'
    return 'other'


def infer_stage(branch: str | None, paths: list[str], explicit: int | None) -> int | None:
    if explicit is not None:
        return explicit
    if branch:
        m = re.search(r'stage([1-7])', branch)
        if m:
            return int(m.group(1))
    stages = set()
    for p in paths:
        m = re.search(r'evidence/[^/]*-stage([1-7])/', p)
        if m:
            stages.add(int(m.group(1)))
    if len(stages) == 1:
        return stages.pop()
    return None


def classify(stage: int | None, buckets: set[str]) -> str:
    product = buckets - {'evidence', 'docs'}
    if not product and 'evidence' in buckets:
        if stage == 1:
            return 'audit-evidence'
        if stage == 3:
            return 'test-evidence'
        if stage == 4:
            return 'validate-evidence'
        if stage == 5:
            return 'merge-evidence'
        if stage == 7:
            return 'publish-evidence'
        return 'evidence-only'
    if not product and buckets <= {'docs'}:
        return 'docs-only'
    if stage == 2 or ('firmware' in buckets and stage in (None, 2)):
        if 'firmware' in buckets or 'tests' in buckets:
            return 'implement'
    if stage == 3 and product:
        return 'test'
    if stage == 4 and product:
        return 'validate'
    if stage == 5:
        return 'merge'
    if stage == 7:
        return 'publish'
    if buckets <= {'docs', 'tools', 'tests'} and 'firmware' not in buckets and 'web' not in buckets:
        return 'harness'
    if 'firmware' in buckets:
        return 'implement'
    return 'mixed'


def host_selectors_for_paths(paths: list[str], available: set[str]) -> list[str]:
    sel: set[str] = set()
    for path in paths:
        for prefix, names in FW_HOST_ROUTES:
            if path.startswith(prefix) or path == prefix.rstrip('/'):
                sel.update(n for n in names if n in available or not available)
        base = Path(path).name
        if base.startswith('bugfinder_') and base.endswith(('.c', '.h')):
            # bugfinder_clock_handover_test.c → clock-handover when registered
            stem = base[len('bugfinder_'):].rsplit('_test', 1)[0].replace('_', '-')
            if not available or stem in available:
                sel.add(stem)
            if 'clock-handover' in available or stem == 'clock-handover':
                sel.update(n for n in ('sequencer', 'clk-lock', 'clock-handover') if not available or n in available)
        if path == 'tests/run_tests.sh':
            # Registration-only edits still need the newly named selector when present.
            pass
    # Always keep core BF clock set when sequencer.c changed.
    if any(p.endswith('firmware/src/seq/sequencer.c') or p.endswith('/sequencer.c') for p in paths):
        for n in ('sequencer', 'clk-lock', 'clock-handover'):
            if not available or n in available:
                sel.add(n)
    return sorted(sel)


def publish_selectors_needed(paths: list[str], available: set[str]) -> list[str]:
    need: set[str] = set()
    for path in paths:
        if path.startswith('tools/publish_nightly.py') or path.startswith('tests/host/test_publish_nightly'):
            need.add('nightly-publish')
        if path.startswith('tools/make_pages.py') or path.startswith('tests/make_pages_test'):
            need.add('installer-targets')
        if 'installer_digest' in path or path.startswith('tests/host/test_check_installer_digests'):
            need.add('installer-digests')
        if path.startswith('tools/require_gate_pass') or path.startswith('tests/host/test_install_gate_guard'):
            need.add('install-gate-guard')
        if path.startswith('web/test_projects') or path.startswith('web/projects'):
            need.update(('projects-web', 'projects-page'))
        if path.startswith('tools/stage_handoff') or path.startswith('tests/host/test_stage_handoff'):
            need.add(STAGE_HANDOFF)
        if path.startswith('tools/dev_validate') or path.startswith('tests/host/test_dev_validate'):
            need.add(DEV_VALIDATE)
    return sorted(n for n in need if not available or n in available)


def has_dev_checked_records(root: Path, paths: list[str]) -> bool:
    evidence_dirs = set()
    for p in paths:
        if p.startswith('evidence/') and '/record.json' in p:
            evidence_dirs.add(str(Path(p).parent))
        m = re.match(r'(evidence/[^/]+)/', p)
        if m:
            evidence_dirs.add(m.group(1))
    for rel in sorted(evidence_dirs):
        for model in ('elite', 'pro'):
            rec = root / rel / model / 'record.json'
            if not rec.is_file():
                # Tip may only add CHECKS pointing at sibling records already on base.
                continue
            try:
                data = json.loads(rec.read_text(encoding='utf-8'))
            except (OSError, json.JSONDecodeError):
                return False
            if data.get('result') != 'DEV CHECKED':
                return False
        # One complete model pair is enough to mark "records present".
        if (root / rel / 'elite' / 'record.json').is_file() and (root / rel / 'pro' / 'record.json').is_file():
            return True
    # Also accept stage-4 style layout discovered under any evidence path in the diff.
    for p in paths:
        m = re.match(r'(evidence/[^/]+)/', p)
        if not m:
            continue
        base = root / m.group(1)
        elite = base / 'elite' / 'record.json'
        pro = base / 'pro' / 'record.json'
        if elite.is_file() and pro.is_file():
            try:
                e = json.loads(elite.read_text(encoding='utf-8'))
                pr = json.loads(pro.read_text(encoding='utf-8'))
            except (OSError, json.JSONDecodeError):
                continue
            if e.get('result') == 'DEV CHECKED' and pr.get('result') == 'DEV CHECKED':
                return True
    return False


def plan(root: Path, base: str, head: str, stage: int | None, branch: str | None) -> dict:
    paths = changed_paths(root, base, head)
    buckets = {bucket(p) for p in paths} if paths else set()
    stage_n = infer_stage(branch, paths, stage)
    kind = classify(stage_n, buckets) if paths else 'empty'
    available = available_selectors(root)
    product_paths = [p for p in paths if bucket(p) not in ('evidence', 'docs')]
    fw_paths = [p for p in paths if bucket(p) in ('firmware', 'tests')]
    tools_paths = [p for p in paths if bucket(p) == 'tools']
    web_paths = [p for p in paths if bucket(p) == 'web']

    x0x: list[str] = []
    commands: list[str] = []
    skip: list[dict] = []
    verify: list[str] = []
    rebuild = False

    def add_sel(*names: str) -> None:
        for n in names:
            if n and n not in x0x and (not available or n in available):
                x0x.append(n)

    # Universal cheap gates for any non-empty PR.
    verify.append('git diff --name-only and --check vs base (scope + whitespace)')
    if any(bucket(p) == 'evidence' for p in paths):
        verify.append('Read tip CHECKS / HANDOFF claims against listed SHAs and digests')

    if kind in ('docs-only', 'empty'):
        skip.append({'what': 'tests/run_tests.sh and package rebuild', 'why': 'docs-only or empty scope; no merge/publish decision'})
    elif kind in (
        'audit-evidence',
        'test-evidence',
        'validate-evidence',
        'merge-evidence',
        'publish-evidence',
        'evidence-only',
    ):
        # Evidence tip: do not re-tax publish/web suites or rebuild packages.
        if any('HANDOFF' in p or 'SUCCESSOR' in p or 'stage' in Path(p).name.lower() for p in paths):
            add_sel(STAGE_HANDOFF)
        if kind == 'merge-evidence':
            add_sel('install-gate-guard')  # stage-6 prompt / gate wording
            verify.append('Spot-check stage-6 prompt package/controller digests vs stage-4 DEV CHECKED records')
        if kind == 'publish-evidence':
            verify.append('Re-fetch live installer catalog and package SHA-256; require MATCH with tip claims')
        if kind == 'validate-evidence' or (stage_n == 4 and kind == 'evidence-only'):
            add_sel(DEV_VALIDATE)
            verify.append('python3 tools/verify_dev_checked_records.py --evidence <stage4-dir>')
            rebuild = False
            skip.append({
                'what': 'Elite+Pro tools/dev_validate.py run (package rebuild)',
                'why': 'Stage-4 tip is evidence-only and DEV CHECKED digests already recorded; verify records instead',
            })
        skip.append({
            'what': 'Default publish/web kitchen sink (' + ' '.join(PUBLISH_SELECTORS) + ')',
            'why': 'Evidence-only tip; those selectors do not change the merge decision unless tools/web changed',
        })
        skip.append({'what': 'node web/test_web.mjs', 'why': 'No web/ product change in three-dot diff'})
        skip.append({'what': 'Full tests/run_tests.sh', 'why': 'Use stage-scoped selectors only'})
        if kind == 'test-evidence':
            verify.append('Re-run host selectors named in tip CHECKS / test-run-summary (not the full suite)')
    elif kind == 'implement' or kind == 'test':
        add_sel(*host_selectors_for_paths(fw_paths or product_paths, available))
        add_sel(*publish_selectors_needed(product_paths, available))
        if any(bucket(p) == 'evidence' for p in paths):
            add_sel(STAGE_HANDOFF)
        if not x0x:
            # Unknown firmware path: keep a minimal safe host set if available.
            for n in ('sequencer', 'clk-lock'):
                if not available or n in available:
                    add_sel(n)
        skip.append({'what': 'Elite+Pro package rebuild', 'why': 'Stage 4 owns DEV CHECKED packages; implement/test keep host BF regressions'})
        skip.append({'what': 'Unrelated publish/web selectors', 'why': 'Only run publish selectors when tools/web paths in the diff require them'})
        if web_paths:
            commands.append('node web/test_web.mjs')
        else:
            skip.append({'what': 'node web/test_web.mjs', 'why': 'No web/ change'})
    elif kind == 'validate':
        # Product change under stage 4: may need a real rebuild.
        rebuild = True
        add_sel(DEV_VALIDATE)
        add_sel(*publish_selectors_needed(product_paths, available))
        verify.append('tools/dev_validate.py run for Elite and Pro on the validated commit; record DEV CHECKED')
        verify.append('pool_free >= 8192; package/controller SHA-256 MATCH records')
    elif kind == 'harness':
        add_sel(*publish_selectors_needed(product_paths, available))
        if any('local_proof' in p or 'merge_project' in p or 'stage_handoff' in p for p in paths):
            add_sel(STAGE_HANDOFF)
        if any('dev_validate' in p or 'local_proof' in p for p in paths):
            add_sel(DEV_VALIDATE)
        # Always exercise this planner when the harness itself changed.
        if any('local_proof' in p for p in paths):
            commands.append('python3 -m unittest tests.host.test_local_proof_plan')
        skip.append({'what': 'Firmware host BF suite and package rebuild', 'why': 'Harness/docs/tools only'})
    else:  # mixed / merge / publish with product
        add_sel(*host_selectors_for_paths(fw_paths, available))
        add_sel(*publish_selectors_needed(product_paths + tools_paths + web_paths, available))
        if stage_n in (5, 7) or kind in ('merge', 'publish'):
            add_sel(STAGE_HANDOFF)
            if stage_n == 5 or kind == 'merge':
                add_sel('install-gate-guard')
            if stage_n == 7 or kind == 'publish':
                verify.append('Live page catalog + package hash MATCH')
        if any(bucket(p) == 'firmware' for p in paths):
            rebuild = stage_n == 4
        if web_paths:
            commands.append('node web/test_web.mjs')

    # Filter unknown selectors when the runner is present.
    if available:
        x0x = [n for n in x0x if n in available]

    if x0x:
        commands.insert(0, "X0X_TEST_JOBS=1 X0X_TESTS='%s' bash tests/run_tests.sh" % ' '.join(x0x))

    if rebuild:
        commands.append(
            'python3 tools/dev_validate.py run --repo <worktree> --commit <validated-sha> '
            '--model elite|pro --plan <plan.json> --output <out-dir>'
        )
    elif kind in ('validate-evidence',) or (stage_n == 4 and kind in ('evidence-only', 'validate-evidence')):
        if DEV_VALIDATE not in x0x and (not available or DEV_VALIDATE in available):
            # ensure listed
            pass

    # Integrity reminder: never skip these when applicable.
    must_keep = [
        'dev_validate record result == DEV CHECKED when claiming package pass',
        'pool_free >= 8192 on affected development packages',
        'publish / install hash MATCH of package and controller bytes',
        'Required BF host regressions when firmware or matching host tests change',
    ]

    return {
        'schema': 1,
        'base': base,
        'head': head,
        'branch': branch,
        'stage': stage_n,
        'kind': kind,
        'changed': paths,
        'buckets': sorted(buckets),
        'x0x_tests': x0x,
        'commands': commands,
        'verify': verify,
        'skip': skip,
        'rebuild_packages': rebuild,
        'must_keep': must_keep,
        'policy': {
            'runners': 'agent cloud VM or Mac only — never GitHub Actions or @codex',
            'prefer': 'stage-scoped selectors; verify digests/records over rebuild when evidence-only',
        },
    }


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--repo', type=Path, default=ROOT)
    ap.add_argument('--base', required=True)
    ap.add_argument('--head', default='HEAD')
    ap.add_argument('--stage', type=int, choices=range(1, 8), default=None)
    ap.add_argument('--branch', default=None, help='Optional branch name for stage inference')
    args = ap.parse_args(argv)
    branch = args.branch
    if branch is None:
        try:
            branch = git(args.repo, 'rev-parse', '--abbrev-ref', 'HEAD').decode().strip()
            if branch == 'HEAD':
                branch = None
        except subprocess.CalledProcessError:
            branch = None
    try:
        out = plan(args.repo, args.base, args.head, args.stage, branch)
    except (OSError, ValueError, subprocess.SubprocessError) as err:
        print('local_proof_plan failed: ' + str(err), file=sys.stderr)
        return 1
    json.dump(out, sys.stdout, indent=2, ensure_ascii=True)
    print()
    return 0


if __name__ == '__main__':
    sys.exit(main())
