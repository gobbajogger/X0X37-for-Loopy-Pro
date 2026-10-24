#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Offline extraction: refuse existing files/symlinks and clean partial output.

Source-main AST on actual temporary filesystem; never connects to a MIDI device.
"""
import ast
import contextlib
import io
from pathlib import Path
import sys
import tempfile
from unittest.mock import patch

SOURCE = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[2] / 'tools/x0x_slots.py'
IMAGES = [b'A' * 512 + b'FIRST', b'B' * 512 + b'SECOND']

class SlotError(Exception):
    pass


def call(main, dest, *, fail=None, images=IMAGES):
    scope = dict(sys=sys, Path=Path, SlotError=SlotError, HDR_LEN=480, DATA_OFF=512,
                 read_archive=lambda data: ({}, images), __doc__='usage')
    exec(compile(main, str(SOURCE), 'exec'), scope)
    src = dest.parent / 'sample.x0xslots'
    src.write_bytes(b'archive-fixture')
    orig_open = Path.open
    def controlled_open(self, mode='r', *args, **kwargs):
        if self.name == fail and mode in ('xb', 'wb'):
            raise OSError('injected write interruption')
        return orig_open(self, mode, *args, **kwargs)
    with patch.object(Path, 'open', controlled_open), contextlib.redirect_stdout(io.StringIO()):
        try:
            scope['main'](['extract', str(src), str(dest)])
            return None
        except BaseException as error:
            return error


def run():
    tree = ast.parse(SOURCE.read_text(encoding='utf-8'))
    methods = [n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == 'main']
    if len(methods) != 1:
        raise RuntimeError('main AST missing or ambiguous')
    main = ast.fix_missing_locations(ast.Module(body=methods, type_ignores=[]))
    checks = []
    def check(label, good, error=None):
        print(('PASS' if good else 'FAIL') + ' ' + label + (f': {error}' if error else ''))
        checks.append(bool(good))
    with tempfile.TemporaryDirectory(prefix='x0x-extract-') as temp:
        root = Path(temp)
        dest = root / 'new'
        e = call(main, dest)
        check('new folder: slot files preserved exactly', e is None and
              (dest/'slot1.hdr').read_bytes() == b'A'*480 and
              (dest/'slot1.bin').read_bytes() == b'FIRST' and
              (dest/'slot2.bin').read_bytes() == b'SECOND', e)
        prior = {p.name: p.read_bytes() for p in dest.iterdir()}
        e = call(main, dest)
        check('repeat extract: no clobber', isinstance(e, SlotError) and
              prior == {p.name: p.read_bytes() for p in dest.iterdir()}, e)
        dest2 = root / 'collision'; dest2.mkdir()
        (dest2/'slot2.bin').write_bytes(b'OWNER')
        e = call(main, dest2)
        check('later-slot collision: preflight prevents earlier writes',
              isinstance(e, SlotError) and sorted(p.name for p in dest2.iterdir()) == ['slot2.bin'] and
              (dest2/'slot2.bin').read_bytes() == b'OWNER', e)
        dest3 = root / 'link'; dest3.mkdir()
        owner_file = root / 'owner.wav'; owner_file.write_bytes(b'ORIGINAL')
        (dest3/'slot1.bin').symlink_to(owner_file)
        e = call(main, dest3)
        check('symlink file: external owner audio not touched', isinstance(e, SlotError) and
              owner_file.read_bytes() == b'ORIGINAL' and not (dest3/'slot1.hdr').exists(), e)
        dest4 = root / 'dirlink'; elsewhere = root / 'elsewhere'; elsewhere.mkdir()
        dest4.symlink_to(elsewhere, target_is_directory=True)
        e = call(main, dest4)
        check('symlink output directory: reject', isinstance(e, SlotError) and
              not list(elsewhere.iterdir()), e)
        dest5 = root / 'failure'; dest5.mkdir()
        e = call(main, dest5, fail='slot2.bin')
        check('interrupted extraction rolls back new files', isinstance(e, OSError) and
              not list(dest5.iterdir()), e)
        dest6 = root / 'empty'; dest6.mkdir()
        e = call(main, dest6, images=[None, IMAGES[1]])
        check('existing empty directory and empty slot remain supported', e is None and
              sorted(p.name for p in dest6.iterdir()) == ['slot2.bin', 'slot2.hdr'], e)
    print(f'SUMMARY {sum(checks)}/{len(checks)}')
    return 0 if all(checks) else 1

if __name__ == '__main__':
    raise SystemExit(run())
