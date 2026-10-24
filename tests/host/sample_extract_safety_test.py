#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Real-filesystem regression of sample extraction against symlink swapping."""
import ast
import contextlib
import io
import os
from pathlib import Path
import sys
import tempfile
from unittest.mock import patch

SOURCE = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[2] / "tools/x0x_slots.py"
IMAGES = [b'A' * 512 + b'FIRST', b'B' * 512 + b'SECOND']

class SlotError(Exception):
    pass

def call(main, dest, *, mode=None, images=IMAGES):
    scope = dict(sys=sys, Path=Path, os=os, SlotError=SlotError, HDR_LEN=480, DATA_OFF=512,
                 read_archive=lambda data: ({}, images), __doc__='usage')
    exec(compile(main, str(SOURCE), 'exec'), scope)
    src = dest.parent / 'sample.x0xslots'
    src.write_bytes(b'archive-fixture')
    real_mkdir, real_open, real_fdopen = Path.mkdir, os.open, os.fdopen
    transferred = dest.parent / 'moved'
    outsider = dest.parent / 'elsewhere'
    outsider.mkdir(exist_ok=True)
    def injected_mkdir(self, *args, **kwargs):
        if mode == 'swap-at-mkdir' and self == dest:
            dest.symlink_to(outsider, target_is_directory=True)
            return None
        return real_mkdir(self, *args, **kwargs)
    def injected_open(file, flags, *args, **kwargs):
        if mode == 'interrupt-on-second' and file == 'slot2.bin' and kwargs.get('dir_fd') is not None:
            raise OSError('injected write interruption')
        if mode == 'swap-after-bind' and file == 'slot1.hdr' and kwargs.get('dir_fd') is not None:
            if not dest.is_symlink():
                dest.rename(transferred)
                dest.symlink_to(outsider, target_is_directory=True)
        if mode == 'replace-before-rollback' and file == 'slot1.bin' and kwargs.get('dir_fd') is not None:
            (dest / 'slot1.hdr').rename(dest / 'diverted-header.hdr')
            (dest.parent / 'owner.wav').rename(dest / 'slot1.hdr')
            raise OSError('injected interruption after another writer replaced slot filename')
        fd = real_open(file, flags, *args, **kwargs)
        if mode == 'replace-after-open' and file == 'slot1.hdr' and kwargs.get('dir_fd') is not None:
            (dest / 'slot1.hdr').rename(dest / 'diverted-header.hdr')
            (dest / 'slot1.hdr').symlink_to(dest.parent / 'owner.wav')
        return fd
    real_fstat = os.fstat
    def injected_fstat(fd):
        if mode in ('fstat-fails-after-create', 'fstat-fails-and-name-swapped'):
            if mode == 'fstat-fails-and-name-swapped':
                (dest / 'slot1.hdr').rename(dest / 'diverted-header.hdr')
                (dest.parent / 'owner.wav').rename(dest / 'slot1.hdr')
            raise OSError(5, 'injected fd metadata failure')
        return real_fstat(fd)
    class InterruptedFile:
        def __init__(self, f): self.f=f
        def __enter__(self): self.f.__enter__(); return self
        def write(self, data):
            self.f.write(data[:2]); raise OSError('injected partial write failure')
        def __exit__(self, *args): return self.f.__exit__(*args)
    class ShortWriteFile(InterruptedFile):
        def write(self, data):
            return self.f.write(data[:2])
    def injected_fdopen(fd, *args, **kwargs):
        f=real_fdopen(fd,*args,**kwargs)
        if mode == 'partial-first': return InterruptedFile(f)
        if mode == 'short-first': return ShortWriteFile(f)
        return f
    with patch.object(Path, 'mkdir', injected_mkdir), patch.object(os, 'open', injected_open), \
         patch.object(os, 'fdopen', injected_fdopen), patch.object(os, 'fstat', injected_fstat), contextlib.redirect_stdout(io.StringIO()):
        try:
            scope['main'](['extract', str(src), str(dest)])
            return None
        except BaseException as error:
            return error

def run():
    tree=ast.parse(SOURCE.read_text(encoding='utf-8'))
    main=ast.fix_missing_locations(ast.Module(body=[n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name=='main'],type_ignores=[]))
    checks=[]
    def check(label,passed,error=None):
        passed=bool(passed)
        print(('PASS' if passed else 'FAIL')+' '+label+(f': {error}' if error else ''))
        checks.append(passed)
    with tempfile.TemporaryDirectory(prefix='x0x-extract-r16-') as t:
        root=Path(t)
        dest=root/'new'; e=call(main,dest)
        check('new folder: exact original slot bytes',e is None and (dest/'slot1.hdr').read_bytes()==b'A'*480 and (dest/'slot1.bin').read_bytes()==b'FIRST' and (dest/'slot2.bin').read_bytes()==b'SECOND',e)
        prior={p.name:p.read_bytes() for p in dest.iterdir()}
        e=call(main,dest)
        check('repeat extraction does not overwrite samples',isinstance(e,SlotError) and prior=={p.name:p.read_bytes() for p in dest.iterdir()},e)
        dest2=root/'collision';dest2.mkdir();(dest2/'slot2.bin').write_bytes(b'OWNER')
        e=call(main,dest2)
        check('later collision rejects before first write',isinstance(e,SlotError) and sorted(p.name for p in dest2.iterdir())==['slot2.bin'] and (dest2/'slot2.bin').read_bytes()==b'OWNER',e)
        dest3=root/'link';dest3.mkdir(); owner=root/'owner.wav';owner.write_bytes(b'ORIGINAL');(dest3/'slot1.bin').symlink_to(owner)
        e=call(main,dest3)
        check('per-file symlink preserves owner audio',isinstance(e,SlotError) and owner.read_bytes()==b'ORIGINAL' and not (dest3/'slot1.hdr').exists(),e)
        dest4=root/'dirlink'; elsewhere=root/'elsewhere';elsewhere.mkdir(exist_ok=True);dest4.symlink_to(elsewhere,target_is_directory=True)
        e=call(main,dest4)
        check('existing output directory symlink rejected',isinstance(e,SlotError) and not list(elsewhere.iterdir()),e)
        dest5=root/'interrupted';dest5.mkdir();e=call(main,dest5,mode='interrupt-on-second')
        check('interrupted later create rolls back new files',isinstance(e,OSError) and not list(dest5.iterdir()),e)
        dest6=root/'empty';dest6.mkdir();e=call(main,dest6,images=[None,IMAGES[1]])
        check('existing empty directory / empty slot supported',e is None and sorted(p.name for p in dest6.iterdir())==['slot2.bin','slot2.hdr'],e)
        dest7=root/'swap-before-open';e=call(main,dest7,mode='swap-at-mkdir')
        check('symlink substituted during mkdir rejected',isinstance(e,SlotError),e)
        check('substituted foreign directory receives no files',not list((root/'elsewhere').iterdir()))
        dest8=root/'swap-after-open';e=call(main,dest8,mode='swap-after-bind')
        check('post-open directory substitution is rejected',isinstance(e,SlotError),e)
        check('post-open replacement cannot redirect writes into foreign directory',not list((root/'elsewhere').iterdir()),e)
        check('renamed original directory has no partial files',not list((root/'moved').iterdir()) if (root/'moved').exists() else False,e)
        dest9=root/'partial';dest9.mkdir();e=call(main,dest9,mode='partial-first')
        check('interrupted partial file write cleans opened output',isinstance(e,OSError) and not list(dest9.iterdir()),e)
        check('unrelated owner WAV remains untouched',owner.read_bytes()==b'ORIGINAL')
        dest10=root/'replaced-name';dest10.mkdir();e=call(main,dest10,mode='replace-after-open')
        check('renamed output and symlink replacement cannot falsely succeed',
              isinstance(e,SlotError) and (dest10/'slot1.hdr').is_symlink()
              and (dest10/'diverted-header.hdr').read_bytes()==b'A'*480
              and owner.read_bytes()==b'ORIGINAL',e)
        dest11=root/'foreign-rollback';dest11.mkdir();e=call(main,dest11,mode='replace-before-rollback')
        check('interrupted cleanup does not delete a recording moved into created filename',
              isinstance(e,OSError) and (dest11/'slot1.hdr').is_file()
              and (dest11/'slot1.hdr').read_bytes()==b'ORIGINAL'
              and (dest11/'diverted-header.hdr').read_bytes()==b'A'*480,e)
        dest12=root/'short-write';dest12.mkdir();e=call(main,dest12,mode='short-first')
        check('silent short write is a failure and incomplete file is removed',
              isinstance(e,SlotError) and not list(dest12.iterdir()),e)
        dest13=root/'failed-fstat';dest13.mkdir();e=call(main,dest13,mode='fstat-fails-after-create')
        check('metadata failure immediately after exclusive create rolls back new output',
              isinstance(e,OSError) and not list(dest13.iterdir()),e)
        dest14=root/'failed-fstat-swapped';dest14.mkdir();owner.write_bytes(b'ORIGINAL');e=call(main,dest14,mode='fstat-fails-and-name-swapped')
        check('metadata failure after foreign substitution never deletes owner audio',
              isinstance(e,OSError) and (dest14/'slot1.hdr').read_bytes()==b'ORIGINAL'
              and (dest14/'diverted-header.hdr').read_bytes()==b'' and owner.exists() is False,e)
    print(f'SUMMARY {sum(checks)}/{len(checks)}')
    return 0 if all(checks) else 1

if __name__=='__main__':raise SystemExit(run())
