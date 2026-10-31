#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exact designated bank parity, compiled-header route, cache and code-tail preservation."""
import hashlib
import os
from pathlib import Path
import shutil
from unittest import mock
import subprocess
import sys
import tempfile

ROOT = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import break_pack as bp

EXPECTED = 'cbfe682a9c69b51dac4ee96db7ab4605c8ecdb2839230efcc5445aa0a8fe199e'
NAMES = ['CRAMEN', 'TASTY', 'HOTPANT']
FRAMES = [70589, 105827, 85725]

def main():
    stock = ROOT / 'assets/factory-breaks'
    raw = (stock / 'factory-break-pack.bin').read_bytes()
    assert hashlib.sha256(raw).hexdigest() == EXPECTED, 'designated stock bank not installed'
    rate, loops = bp.parse_pack(raw)
    assert rate == 44100 and [l['name'] for l in loops] == NAMES
    assert [l['nsamples'] for l in loops] == FRAMES and all(l['bars'] == 1 for l in loops)
    assert bp.encode_pack(bp.loops_from_export(stock)) == raw
    before = {p.name: p.read_bytes() for p in stock.iterdir() if p.is_file()}
    subprocess.run(['sh', 'tools/export_factory_breaks.sh'], cwd=ROOT, check=True)
    assert before == {p.name: p.read_bytes() for p in stock.iterdir() if p.is_file()}, 'export altered stock inputs'
    js = (ROOT / 'web/factory_break_pack.js').read_text()
    assert js == bp.js_module(raw), 'browser and source bank mismatch'
    labels = (ROOT / 'web/x0x_installer.html').read_text()
    assert 'two factory loops' not in labels and '909 GR' not in labels
    with tempfile.TemporaryDirectory() as td:
        td = Path(td)
        bp.write_export_header(stock, td / 'bank.h')
        (td / 'read.c').write_text('#include <stdio.h>\n#include "bank.h"\nint main(void){unsigned i; for(i=0;i<X0X_NBREAKS;i++){printf("%s %u %u\\n", X0X_BREAKS[i].name, X0X_BREAKS[i].n, X0X_BREAKS[i].bars); fwrite(X0X_BREAKS[i].adpcm,1,(X0X_BREAKS[i].n+1)/2,stdout);}return 0;}\n')
        subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror',str(td/'read.c'),'-o',str(td/'read')],check=True)
        got = subprocess.check_output([str(td/'read')])
        expect = b''.join(f"{l['name']} {l['nsamples']} {l['bars']}\n".encode()+l['adpcm'] for l in loops)
        assert got == expect, 'compiled factory header differs from installer bytes'
        code_max = 552672
        app = bytearray(b'\xa5'*366264 + b'\xff'*(code_max-366264))
        placed = bp.place_pack(app,raw,code_max)
        offset = (366264+15)//16*16
        assert placed[:366264] == app[:366264] and len(placed) == code_max
        assert placed[offset:offset+len(raw)] == raw
        assert all(b == 255 for b in placed[offset+len(raw):])
        assert bp.place_pack(placed,raw,code_max) == placed
        try: bp.place_pack(bytearray(b'\xa5'*(code_max-100)+b'\xff'*100),raw,code_max)
        except ValueError: pass
        else: raise AssertionError('over-budget stock bank accepted')
        (td/'pack.bin').write_bytes(raw)
        subprocess.run(['cc','-O2','-Wall','-Wextra','-Werror','-I'+str(ROOT/'firmware/src'),str(ROOT/'tests/brk_pack_test.c'),'-o',str(td/'parser')],check=True)
        out=subprocess.check_output([str(td/'parser'),str(td/'pack.bin')]).decode()
        assert out == '3 CRAMEN 70589 1 35295,TASTY 105827 1 52914,HOTPANT 85725 1 42863\n'
    import gen_cache
    with tempfile.TemporaryDirectory() as tmp:
        fake = Path(tmp)
        shutil.copytree(stock, fake / 'assets/factory-breaks')
        with mock.patch.object(gen_cache, 'SRC', fake), mock.patch.object(gen_cache, '_font_files', return_value=[]), mock.patch.object(gen_cache, '_sources', return_value=[]), mock.patch.dict(os.environ, {'X0X_FACTORY_BREAKS': '1', 'X0X_BREAK_BANK': ''}):
            first = gen_cache.fingerprint()
            asset = fake / 'assets/factory-breaks/CRAMEN.adpcm'
            data = asset.read_bytes()
            asset.write_bytes(bytes([data[0] ^ 1]) + data[1:])
            assert first != gen_cache.fingerprint(), 'factory cache ignores changed sample'
            os.environ['X0X_FACTORY_BREAKS'] = '0'
            first = gen_cache.fingerprint()
            asset.write_bytes(data)
            assert first == gen_cache.fingerprint(), 'empty-bank cache depends on unused samples'
    print('PASS exact three-loop bank, deterministic export, browser parity, compiled header, runtime C parser, tail bounds and replacement')

if __name__ == '__main__': main()
