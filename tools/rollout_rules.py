#!/usr/bin/env python3
"""Install project skill discovery and compatibility entry points.

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
    workflow_manifest = source / 'skills/workflows.json'
    if not workflow_manifest.is_file():
        parser.error('canonical source is missing skills/workflows.json')
    workflows = json.loads(workflow_manifest.read_text())
    if not workflows:
        parser.error('canonical workflow skill manifest is empty')
    names = set()
    for item in workflows:
        name = item['name']
        if name in names or not re.fullmatch(r'x0x37-[a-z0-9-]+', name):
            parser.error('invalid project skill name: ' + name)
        names.add(name)
        if item['source'] != f'skills/{name}/SKILL.md' or not (source / item['source']).is_file():
            parser.error('missing or invalid project skill: ' + name)
    required = ['agent-rules.md', 'SKILLS.md', 'rules/core.md']
    for name in required:
        if not (docs / name).is_file():
            parser.error('canonical source is missing docs/' + name)
    checkouts = list(repositories(workspace))
    # Refuse collisions before any write, including on an --apply run.
    for location in [workspace, *checkouts]:
        for parent in [location / '.agents', location / '.agents/skills']:
            if ((parent.exists() or parent.is_symlink()) and not parent.is_dir()
                    or not parent.resolve().is_relative_to(workspace)):
                parser.error('unsafe skill discovery directory: ' + str(parent))
        for item in workflows:
            target = location / '.agents/skills' / item['name']
            if target.exists() and not target.is_symlink():
                parser.error('skill discovery path is not a managed symlink: ' + str(target))
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

    def skill_link(target, destination):
        """Own only named workflow links; never replace an unknown directory."""
        relative = os.path.relpath(destination, target.parent)
        old = os.readlink(target) if target.is_symlink() else None
        if old == relative:
            return
        if target.exists() and not target.is_symlink():
            raise ValueError('skill discovery path is not a managed symlink: ' + str(target))
        location = target.relative_to(workspace)
        changes.append({'path': str(location), 'kind': 'symlink',
                        'before_target': old, 'after_target': relative})
        if args.apply:
            if old is not None:
                saved = backup / location
                saved.parent.mkdir(parents=True, exist_ok=True)
                saved.symlink_to(old)
                target.unlink()
            target.parent.mkdir(parents=True, exist_ok=True)
            target.symlink_to(relative)

    def mirror(target, original):
        text = original.read_text()
        def replace(match):
            label, url = match.groups()
            if '://' not in url and not url.startswith('#'):
                resolved = (original.parent / url).resolve()
                if resolved.is_relative_to(source / 'skills'):
                    relative = resolved.relative_to(source / 'skills')
                    url = os.path.relpath(workspace / '.agents/skills' / relative, target.parent)
            return f'[{label}]({url})'
        write(target, re.sub(r'\[([^\]]+)\]\(([^)]+)\)', replace, text))

    write(workspace / 'AGENTS.md', f'''# x0x37 workspace

Initialize [$x0x37-task](.agents/skills/x0x37-task/SKILL.md) at task start.
Initialize [$x0x37-writing](.agents/skills/x0x37-writing/SKILL.md) before output.
Use [skill triggers](agent-rules.md) when each later step begins.
Read the selected `SKILL.md`. Do not load the old rule files.
Maintained skills: [{canonical}/skills]({canonical}/skills).
Current owner instructions win over old checkout text.
Keep each task in its isolated lane; the assigned integration chat owns firmware.
''')
    for name in ['agent-rules.md', 'AGENTS_AND_ROOMS.md', 'USER_PREFERENCES.md']:
        if (docs / name).is_file():
            mirror(workspace / name, docs / name)
    for path in sorted((docs / 'rules').glob('*.md')):
        mirror(workspace / 'rules' / path.name, path)
    for item in workflows:
        skill_link(workspace / '.agents/skills' / item['name'], source / 'skills' / item['name'])
    write(workspace / 'SKILLS.md', f'''# Skill sources and discovery

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
    def managed(target, block):
        old = target.read_text() if target.exists() else ''
        start, end = '<!-- x0x37-current-rules -->', '<!-- /x0x37-current-rules -->'
        pattern = re.escape(start) + r'.*?' + re.escape(end) + r'\n*'
        # Replace only our block. All other checkout instructions remain intact.
        rest = re.sub(pattern, '', old, flags=re.S)
        write(target, start + '\n' + block + '\n' + end + '\n\n' + rest)

    for repo in checkouts:
        entry = os.path.relpath(workspace / 'AGENTS.md', repo)
        managed(repo / 'AGENTS.md', f'''# Current x0x37 skills

In the x0x37 workspace, use [current skill entry points]({entry}) before work.
Initialize the matching `SKILL.md` when its step begins. These skills supersede
older workflow text here. For a standalone checkout, use `docs/agent-rules.md`
and its skill links; recover missing sources
from the maintained project. Unrelated checkout instructions below still apply.
''')
        for item in workflows:
            skill_link(repo / '.agents/skills' / item['name'], source / 'skills' / item['name'])
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
