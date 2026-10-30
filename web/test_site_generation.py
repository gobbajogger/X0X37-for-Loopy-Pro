#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Catalog identity/retention guards with synthetic packages, inside this lane."""
import hashlib
import sys
import tempfile
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from make_pages import merge_version, write_site

def refuses(action):
    try:
        action()
    except SystemExit:
        return
    raise AssertionError('unsafe catalog/output accepted')

with tempfile.TemporaryDirectory(prefix='site-retention-') as td:
    root=Path(td)
    package=root/'old.fwsc'; package.write_bytes(b'synthetic package, no firmware')
    meta={'product':'SMK-37 Elite_9001300','pkg':'../firmware/old.fwsc','file':'old.fwsc','size':package.stat().st_size,'sha256':hashlib.sha256(package.read_bytes()).hexdigest()}
    first=merge_version({'versions':[]}, '0.0.1', {'elite':meta})
    second=merge_version(first,'0.0.1', {'pro':{**meta,'product':'SMK-37 Pro_9001300'}})
    assert set(second['versions'][0]['models'])=={'elite','pro'}
    # History and its version picker must rank 0.0.10 above 0.0.9,
    # rather than treating numeric releases as ordinary strings.
    numbered=second
    for version in ('0.0.9', '0.0.10', '0.0.2'):
        vmeta={**meta, 'file':f'v{version}.fwsc', 'pkg':f'../firmware/v{version}.fwsc'}
        numbered=merge_version(numbered,version,{'elite':vmeta})
    assert [v['version'] for v in numbered['versions']]==['0.0.10','0.0.9','0.0.2','0.0.1']
    assert numbered['latest']=='0.0.10'  # Adding an older model must not downgrade the default.
    assert set(numbered['versions'][-1]['models'])=={'elite','pro'}
    older=merge_version(numbered,'0.0.9',{'pro':{**meta,'product':'SMK-37 Pro_9001300'}})
    assert older['latest']=='0.0.10'
    assert set(older['versions'][1]['models'])=={'elite','pro'}
    # A stale pointer from an external catalog must not be retained as the default.
    repaired=merge_version({'versions':[], 'latest':'0.0.99'}, '0.0.1', {'elite':meta})
    assert repaired['latest']=='0.0.1'
    refuses(lambda:merge_version(first,'0.0.1',{'elite':{**meta,'sha256':'0'*64}}))
    output=root/'site'; retained=output/'previous';(retained/'firmware').mkdir(parents=True)
    (retained/'firmware/old.fwsc').write_bytes(package.read_bytes())
    refuses(lambda:write_site(output,first,{},retained))
    assert (retained/'firmware/old.fwsc').read_bytes()==package.read_bytes()
    refuses(lambda:write_site(root,first,{'old.fwsc':package},None))
    assert package.is_file()
    (retained/'firmware/old.fwsc').write_bytes(b'damaged')
    refuses(lambda:write_site(root/'fresh',first,{},retained))
    assert not (root/'fresh').exists()
print('PASS immutable version/model identities, numeric history/default order, additive models, overlap and missing/changed retained package guards')
