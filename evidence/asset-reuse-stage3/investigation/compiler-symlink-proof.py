#!/usr/bin/env python3
"""Prove persistent compiler resolution handling using disposable source and CC links."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

SOURCE = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(SOURCE / 'tools'))
import asset_cache


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


with tempfile.TemporaryDirectory(prefix='asset-compiler-link-') as tmp:
    base = Path(tmp)
    root = base / 'source'
    for name in ('tools', 'assets', 'firmware/src'):
        shutil.copytree(SOURCE / name, root / name)
    gen = root / 'build/gen'
    gen.mkdir(parents=True)
    compiler_folder = base / 'compiler'
    compiler_folder.mkdir()
    wrapper = compiler_folder / 'fast-cc'
    marker = compiler_folder / 'called'
    wrapper.write_text('#!/bin/sh\n' + f'echo called >> {marker}\n' +
                       'exec /usr/bin/gcc -ffast-math "$@"\n')
    wrapper.chmod(0o755)
    link = compiler_folder / 'cc'
    link.symlink_to('/usr/bin/gcc')
    store = base / 'cache'
    os.environ['CC'] = str(link)
    os.environ.pop('X0X_BREAK_BANK', None)
    os.environ['X0X_ASSET_CACHE'] = '1'
    subprocess.run([sys.executable, '-B', str(root / 'tools/gen_drum_samples.py'),
                    str(gen / 'x0x_drum_samples.h')], check=True)
    cold_hit = asset_cache.generate_asset('breaks', root, gen, store)
    initial = digest(gen / 'x0x_break_bank.h')
    warm_hit = asset_cache.generate_asset('breaks', root, gen, store)
    link.unlink()
    link.symlink_to(wrapper)
    retarget_hit = asset_cache.generate_asset('breaks', root, gen, store)
    retarget_output = digest(gen / 'x0x_break_bank.h')
    wrapper_called_by_hit = marker.exists()
    os.environ['X0X_ASSET_CACHE'] = '0'
    direct_hit = asset_cache.generate_asset('breaks', root, gen, store)
    direct_output = digest(gen / 'x0x_break_bank.h')
    result = {'candidate': 'bb953dfaa086c44e57382e9d82fe9e0a2714f965',
              'cold_hit': cold_hit, 'warm_hit': warm_hit,
              'retarget_hit': retarget_hit, 'wrapper_called_by_hit': wrapper_called_by_hit,
              'disabled_hit': direct_hit, 'wrapper_called_by_disabled': marker.exists(),
              'initial_sha256': initial, 'retarget_sha256': retarget_output,
              'disabled_sha256': direct_output,
              'bytes_differ_after_actual_compiler_change': retarget_output != direct_output}
    print(json.dumps(result, indent=2))
    assert not cold_hit and warm_hit
    assert retarget_hit and not wrapper_called_by_hit
    assert not direct_hit and marker.exists()
    assert retarget_output != direct_output
