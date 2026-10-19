#!/usr/bin/env python3
"""Check installed writing skill integrity and local discovery; no network needed.

This checks files, not the quality of an agent's writing or STE compliance.
"""
import hashlib
import json
from pathlib import Path
import re
import sys


def check(root):
    manifest = json.loads((root / 'skills/sources.json').read_text())
    names = {item['name'] for item in manifest['skills']}
    if names != {'caveman', 'asd-ste100'}:
        raise ValueError('Both required writing skills must be recorded.')
    count = 0
    for item in manifest['skills']:
        if not re.fullmatch(r'[0-9a-f]{40}', item['revision']):
            raise ValueError('Source revision is not pinned: ' + item['name'])
        skill = root / 'skills' / item['name'] / 'SKILL.md'
        body = skill.read_text()
        header = body.split('---', 2)
        if len(header) != 3 or header[0].strip():
            raise ValueError('Missing frontmatter: ' + str(skill))
        if not re.search(r'^name: ' + re.escape(item['name']) + r'\s*$', header[1], re.M):
            raise ValueError('Wrong skill name: ' + str(skill))
        if not re.search(r'^description:', header[1], re.M):
            raise ValueError('Missing description: ' + str(skill))
        for relative, digest in item['files'].items():
            path = root / relative
            if not path.resolve().is_relative_to((root / 'skills' / item['name']).resolve()):
                raise ValueError('Source file leaves skill folder: ' + relative)
            if hashlib.sha256(path.read_bytes()).hexdigest() != digest:
                raise ValueError('Source hash mismatch: ' + relative)
            count += 1
        if str(skill.relative_to(root)) not in item['files']:
            raise ValueError('Skill text has no source hash.')
        for target in re.findall(r'\]\(([^)]+)\)', body):
            if '://' not in target and not target.startswith('#'):
                if not (skill.parent / target.split('#')[0]).is_file():
                    raise ValueError('Missing skill reference: ' + target)
        discovery = root / '.agents/skills' / item['name'] / 'SKILL.md'
        if discovery.resolve() != skill.resolve():
            raise ValueError('Wrong discovery path: ' + str(discovery))
        if f'../skills/{item["name"]}/SKILL.md' not in (root / 'docs/SKILLS.md').read_text():
            raise ValueError('Missing startup skill link: ' + item['name'])
    print(f'PASS: 2 pinned writing skills, {count} source hashes, references, and discovery links.')


if __name__ == '__main__':
    try:
        check(Path(__file__).resolve().parent.parent)
    except (OSError, ValueError, KeyError) as error:
        print('FAIL: ' + str(error), file=sys.stderr)
        sys.exit(1)
