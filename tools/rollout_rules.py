#!/usr/bin/env python3
"""Install workspace rule mirrors and entry points without changing source files.

Dry run is the default. --apply saves each replaced file before writing it.
The default canonical source is workspace/x0x37, independent of the runner checkout.
--check exits 1 on drift and writes nothing; --apply backs up replacements.
"""
import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import subprocess
from datetime import datetime, timezone


def repositories(workspace):
    excluded = {'.git', 'refs', 'validation_refs', 'repo-bak', 'node_modules',
                '.agents', '.codex', '.fix861'}
    for root, dirs, files in os.walk(workspace):
        dirs[:] = sorted(d for d in dirs if d not in excluded)
        path = Path(root)
        if '.git' in files or (path / '.git').is_dir():
            dirs[:] = []
            branch = subprocess.check_output(
                ['git', '-C', str(path), 'branch', '--show-current'], text=True).strip()
            if branch == 'refs-oscuro':
                continue
            # Third-party repositories have no x0x37 build and installer pair.
            if (path / 'build.sh').is_file() and (path / 'tools/fm1_install.py').is_file():
                yield path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--workspace', type=Path, required=True)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--apply', action='store_true')
    mode.add_argument('--check', action='store_true')
    parser.add_argument('--source', type=Path, help='maintained checkout (default: workspace/x0x37)')
    args = parser.parse_args()
    workspace = args.workspace.resolve()
    source = (args.source or workspace / 'x0x37').resolve()
    try:
        canonical = source.relative_to(workspace)
    except ValueError:
        parser.error('the canonical source must be a checkout inside the workspace')
    if not (workspace / 'agent-rules.md').is_file() or not (workspace / 'x0x37').is_dir():
        parser.error('workspace must contain agent-rules.md and the x0x37 checkout')
    docs = source / 'docs'
    required = ['agent-rules.md', 'SKILLS.md', 'rules/core.md']
    for name in required:
        if not (docs / name).is_file():
            parser.error('canonical source is missing docs/' + name)
    stamp = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S%fZ')
    backup = workspace / 'repo-bak' / ('rules-rollout-' + stamp)
    changes = []

    def write(target, content):
        old = target.read_bytes() if target.exists() else None
        data = content.encode() if isinstance(content, str) else content
        if old == data:
            return
        relative = target.relative_to(workspace)
        changes.append({'path': str(relative), 'before_sha256':
                        hashlib.sha256(old).hexdigest() if old is not None else None,
                        'after_sha256': hashlib.sha256(data).hexdigest()})
        if args.apply:
            if old is not None:
                saved = backup / relative
                saved.parent.mkdir(parents=True, exist_ok=True)
                saved.write_bytes(old)
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)

    write(workspace / 'AGENTS.md', f'''# x0x37 workspace

Read [agent-rules.md](agent-rules.md) before work. It mirrors maintained
[{canonical}/docs]({canonical}/docs/agent-rules.md) and routes to current rules
and skill status. Current owner instructions win over old checkout text.
Keep each task in its isolated lane; the assigned integration chat owns firmware.
''')
    for name in ['agent-rules.md', 'AGENTS_AND_ROOMS.md', 'USER_PREFERENCES.md']:
        if (docs / name).is_file():
            write(workspace / name, (docs / name).read_bytes())
    for path in sorted((docs / 'rules').glob('*.md')):
        write(workspace / 'rules' / path.name, path.read_bytes())
    write(workspace / 'SKILLS.md', f'''# Writing skills

Read [current sources and status]({canonical}/docs/SKILLS.md).
''')
    # Preserve links to archived evidence from the mirrored rule entry points.
    for path in sorted((docs / 'archive/rules-2026-10-17').rglob('*')):
        if path.is_file() and not (workspace / path.relative_to(docs)).exists():
            write(workspace / path.relative_to(docs), path.read_bytes())
    notice = ('> Current workflow: read [AGENTS.md](AGENTS.md). The dated records below\n'
              '> preserve past plans and evidence. They do not assign current roles or\n'
              '> require the box. Check the current integration task for the live revision.\n'
              '> Firmware budget limits still apply; docs-only work needs no firmware build.\n\n')
    for name in ['BUDGET.md', 'LIVE_BRANCHES', 'ROADMAP.md', 'task-plan.md', 'x0x37-task-plan.md']:
        target = workspace / name
        if target.is_file() and not target.read_text().startswith(notice):
            write(target, notice + target.read_text())
    checkouts = list(repositories(workspace))
    def managed(target, block):
        old = target.read_text() if target.exists() else ''
        start, end = '<!-- x0x37-current-rules -->', '<!-- /x0x37-current-rules -->'
        pattern = re.escape(start) + r'.*?' + re.escape(end) + r'\n*'
        # Replace only our block. All other checkout instructions remain intact.
        rest = re.sub(pattern, '', old, flags=re.S)
        write(target, start + '\n' + block + '\n' + end + '\n\n' + rest)

    for repo in checkouts:
        entry = os.path.relpath(workspace / 'AGENTS.md', repo)
        managed(repo / 'AGENTS.md', f'''# Current x0x37 rules

In the x0x37 workspace, read [current rules]({entry}) before work. Its maintained
rules and skill status supersede older workflow text here. For a standalone
checkout, use `docs/agent-rules.md` and its skill links; recover missing sources
from the maintained project. Unrelated checkout instructions below still apply.
''')
        if repo != source and (repo / 'docs/SKILLS.md').is_file():
            skill = os.path.relpath(docs / 'SKILLS.md', repo / 'docs')
            managed(repo / 'docs/SKILLS.md', f'''# Current skill discovery

Inside the x0x37 workspace use [maintained skill status]({skill}). The older
status below is retained as history and is superseded there. Outside the
workspace use the local installed skills and verify their source/status.
''')
    # Existing Mac hooks protect local history and proprietary refs. Preserve
    # their policy; repair only the recognized stale box-only repair notice.
    seen_hooks = set()
    for repo in checkouts:
        common = Path(subprocess.check_output(
            ['git', '-C', str(repo), 'rev-parse', '--git-common-dir'], text=True).strip())
        if not common.is_absolute():
            common = (repo / common).resolve()
        hook = common / 'hooks/pre-push'
        if hook in seen_hooks or not hook.is_file() or not hook.is_relative_to(workspace):
            continue
        seen_hooks.add(hook)
        text = hook.read_text()
        if '# Mac copy of the x0x37 pre-push hook.' not in text:
            continue
        text = text.replace(
            '# Rollout and repair notes (on the box): /workspace/x0x37-build-and-test-runner/failclosed-hook-rollout.md',
            '# Repair notes: read the current project BUILDING.md; use complete local history.')
        text = text.replace(
            'FIX="fix the clone (unshallow or repair it; notes on the box: /workspace/x0x37-build-and-test-runner/failclosed-hook-rollout.md)"',
            'FIX="fetch complete local history or repair the clone; read the current project BUILDING.md"')
        write(hook, text)
    if args.apply and changes:
        backup.mkdir(parents=True, exist_ok=True)
        (backup / 'manifest.json').write_text(json.dumps(changes, indent=2) + '\n')
    print(json.dumps({'apply': args.apply, 'files': len(changes),
                      'backup': str(backup) if args.apply and changes else None,
                      'changes': changes}, indent=2))
    return 1 if args.check and changes else 0


if __name__ == '__main__':
    raise SystemExit(main())
