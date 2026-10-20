#!/usr/bin/env python3
"""Check workflow skill sources, discovery, local links, and legacy decision IDs.

Run after a project skill or its discovery changes. This checks files and links;
it does not prove future model selection or successful workflow execution.
"""
import json
from pathlib import Path
import re
import sys


def check(root):
    rows = json.loads((root / 'skills/workflows.json').read_text())
    names = set()
    for row in rows:
        name = row['name']
        if name in names or not re.fullmatch(r'x0x37-[a-z0-9-]+', name):
            raise ValueError('Invalid or repeated workflow skill name: ' + name)
        names.add(name)
        path = root / row['source']
        if row['source'] != f'skills/{name}/SKILL.md':
            raise ValueError('Unexpected skill path: ' + row['source'])
        text = path.read_text()
        frontmatter = text.split('---', 2)
        if len(frontmatter) != 3 or frontmatter[0].strip():
            raise ValueError('Missing skill frontmatter: ' + name)
        if f'\nname: {name}\n' not in frontmatter[1]:
            raise ValueError('Wrong skill name: ' + name)
        description = re.search(r'^description: (.+)$', frontmatter[1], re.M)
        if not description or json.loads(description[1]) != row['description']:
            raise ValueError('Skill trigger differs from manifest: ' + name)
        if 'disable-model-invocation' in frontmatter[1]:
            raise ValueError('Automatic skill selection was disabled: ' + name)
        discovery = root / '.agents/skills' / name / 'SKILL.md'
        if discovery.read_bytes() != path.read_bytes():
            raise ValueError('Stale discovery source: ' + name)
        legacy = root / 'docs/rules' / row['legacy']
        marker = f'<!-- x0x37-skill: {row["source"]} -->'
        if marker not in legacy.read_text():
            raise ValueError('Missing legacy decision redirect: ' + row['legacy'])
        if row['source'] not in (root / 'docs/agent-rules.md').read_text():
            raise ValueError('Missing task trigger link: ' + name)
        for resource in path.parent.rglob('*.md'):
            for link in re.findall(r'\]\(([^)]+)\)', resource.read_text()):
                if '://' not in link and not link.startswith('#'):
                    if not (resource.parent / link.split('#')[0]).is_file():
                        raise ValueError(f'Missing reference in {resource}: {link}')
    if not names:
        raise ValueError('No workflow skills were recorded')
    print(f'PASS: {len(names)} workflow skills, triggers, discovery, references, and legacy decision entries.')


if __name__ == '__main__':
    try:
        check(Path(__file__).resolve().parents[1])
    except (OSError, ValueError, KeyError) as error:
        print('FAIL: ' + str(error), file=sys.stderr)
        sys.exit(1)
