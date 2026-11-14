#!/usr/bin/env python3
"""Validate the three canonical roadmap queues; optional verbatim migration check."""
import argparse
from pathlib import Path
import re
from collections import Counter


def require(condition, message):
    if not condition:
        raise ValueError(message)


def validate(root, baseline=None):
    files = {name: root / 'docs' / 'roadmap' / (name + '.md') for name in ('Complete', 'Working', 'Pending')}
    contents = {name: path.read_text() for name, path in files.items()}
    ids = [key for text in contents.values() for key in re.findall(r'^## (.+)$', text, re.M)]
    duplicates = [key for key, count in Counter(ids).items() if count != 1]
    require(not duplicates, f'Duplicate queue IDs: {duplicates}')
    numeric = {key for key in ids if re.fullmatch(r'#\d+', key)}
    require(numeric == {f'#{n}' for n in range(1, 123)}, 'Original 122 IDs missing or changed')
    for name, text in contents.items():
        for entry in re.split(r'^## ', text, flags=re.M)[1:]:
            for field in ('Current disposition', 'Owner', 'Exact input/evidence', 'Next action'):
                require(f'**{field}:**' in entry, f'{name}: missing {field} in {entry.splitlines()[0]}')
    index = (root / 'docs' / 'ROADMAP.md').read_text()
    for name in files:
        require(f'roadmap/{name}.md' in index, f'Index does not discover {name}')
    require(not re.search(r'^\| #\d+ \|', index, re.M), 'Numeric rows duplicated in index')
    if baseline:
        section = ''
        old = []
        for line in baseline.read_text().splitlines():
            if line.startswith('## '):
                section = line[3:]
            if line.startswith('### '):
                section = ''
            selected = section.startswith(('0.0.26 (', '0.0.25 owner requests', 'Later owner requests', 'Owner request list'))
            supplemental = re.match(r'^\| (SMC-04|SMC-05|SMC-30|NS001) \|', line)
            if (selected or supplemental) and line.startswith('|'):
                first = line.split('|')[1].strip()
                if first not in ('#', 'Row') and not first.startswith('---'):
                    old.append(line)
        all_lines = '\n'.join(contents.values()).splitlines()
        for line in old:
            require(all_lines.count(line) == 1, f'Missing or duplicated original record: {line[:80]}')
        require(len(old) <= len(ids), f'Original record total reduced: {len(old)} vs {len(ids)}')
        original_ids = set()
        for line in old:
            key = line.split('|')[1].strip()
            match = re.match(r'M\d+', key)
            original_ids.add(match.group() if match else key)
        require(original_ids.issubset(set(ids)), 'Original record heading IDs missing or changed')
        for name, text in contents.items():
            for entry in re.split(r'^## ', text, flags=re.M)[1:]:
                if entry.splitlines()[0] not in original_ids:
                    require('**Request/source:**' in entry, f'{name}: new record lacks request/source')
    return {name: len(re.findall(r'^## ', text, re.M)) for name, text in contents.items()}

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--baseline', type=Path)
    args = parser.parse_args()
    print(validate(args.root, args.baseline))
