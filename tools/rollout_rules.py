#!/usr/bin/env python3
"""Install workspace rule mirrors and entry points without changing source files.

Dry run is the default. --apply saves each replaced file before writing it.
The canonical source is the checkout that holds this script.
"""
import argparse
import hashlib
import json
import os
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
    parser.add_argument('--apply', action='store_true')
    args = parser.parse_args()
    workspace = args.workspace.resolve()
    source = Path(__file__).resolve().parent.parent
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

    write(workspace / 'AGENTS.md', f'''# x0x37 workspace entry point

This folder holds several repositories. Read [agent-rules.md](agent-rules.md)
and [rules/core.md](rules/core.md) before work. These files mirror the maintained
rules in {canonical}/docs. The mirror supports old worktrees.
The latest owner instruction wins over old checkout rules and historical notes.
Use plain correct English and concise caveman style for all project output,
including helper prompts, docs and handoffs. Read SKILLS.md for exact sources
and known missing skills; do not claim missing skills were read or installed.
Keep exact code, commands, paths, quotes and necessary technical meaning.
Complete authorized work in a supported environment. Report actual checks and
remaining dependencies. Local checks do not prove a release or device PASS.
Protect owner data, model guards, flash and RAM limits. Do not push externally.
Keep each task in its owned files and an isolated lane. The firmware integration
chat owns flash_review_integration; the rules audit must not take over firmware.
Pass this entry point and clear done criteria to each helper.
''')
    for name in ['agent-rules.md', 'AGENTS_AND_ROOMS.md', 'USER_PREFERENCES.md']:
        if (docs / name).is_file():
            write(workspace / name, (docs / name).read_bytes())
    for path in sorted((docs / 'rules').glob('*.md')):
        write(workspace / 'rules' / path.name, path.read_bytes())
    write(workspace / 'SKILLS.md', f'''# Writing skills

Read [the maintained skill sources and status]({canonical}/docs/SKILLS.md).
Use plain correct English and concise caveman style for all project output.
Do not claim an unavailable exact skill has been read or installed.
''')
    # Preserve links to archived evidence from the mirrored rule entry points.
    for path in sorted((docs / 'archive/rules-2026-10-17').rglob('*')):
        if path.is_file():
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
    for repo in checkouts:
        if repo == source:
            continue
        discovery = [docs / 'agent-rules.md', docs / 'SKILLS.md', *sorted((docs / 'rules').glob('*.md'))]
        if all((repo / p.relative_to(source)).is_file()
               and (repo / p.relative_to(source)).read_bytes() == p.read_bytes()
               for p in discovery):
            write(repo / 'AGENTS.md', (source / 'AGENTS.md').read_bytes())
            continue
        entry = os.path.relpath(workspace / 'AGENTS.md', repo)
        write(repo / 'AGENTS.md', f'''# x0x37 workspace rules

Read [{entry}]({entry}) at task start. It is the current workspace entry point.
Its rule and skill links apply to every agent and helper in this checkout.
Old local docs, plans and logs are historical when they conflict with it.
If this checkout moves outside the workspace, use docs/agent-rules.md and
docs/rules/core.md from the current maintained project revision. If absent,
recover those files before work; do not fall back to obsolete box-only rules.
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
    if args.apply:
        backup.mkdir(parents=True, exist_ok=True)
        (backup / 'manifest.json').write_text(json.dumps(changes, indent=2) + '\n')
    print(json.dumps({'apply': args.apply, 'files': len(changes),
                      'backup': str(backup) if args.apply else None,
                      'changes': changes}, indent=2))


if __name__ == '__main__':
    main()
