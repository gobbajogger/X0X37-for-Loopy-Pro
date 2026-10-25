#!/usr/bin/env python3
"""Native offline slot-builder no-clobber, rollback and directory binding."""
import ast
import os
import pathlib
import tempfile
import types
import sys
import contextlib
import io
import unittest
from unittest.mock import patch

SOURCE = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else pathlib.Path(__file__).resolve().parents[2] / 'tools/fm1_sample_upload.py'
tree = ast.parse(SOURCE.read_text())
functions = [n for n in tree.body if isinstance(n, ast.FunctionDef)
             and n.name in ('write_slot_pair', 'main')]
if {n.name for n in functions} != {'write_slot_pair', 'main'}:
    raise RuntimeError('native slot build safety implementation or CLI entrypoint missing')
namespace = {'sys': sys, 'Path': pathlib.Path, 'os': os, '__doc__': 'usage',
             'build': lambda name, specs, trim: (b'H' * 480, b'DATA')}
exec(compile(ast.Module(body=functions, type_ignores=[]), str(SOURCE), 'exec'), namespace)
write_slot_pair = namespace['write_slot_pair']
main = namespace['main']

class OfflineSlotBuilder(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = pathlib.Path(self.tmp.name)
        self.directory = self.root / 'owner'
        self.directory.mkdir()
        self.prefix = self.directory / 'break_kit'
        self.hdr = self.directory / 'break_kit.hdr'
        self.data = self.directory / 'break_kit.bin'

    def test_real_cli_build_dispatch_no_overwrite(self):
        path=str(self.prefix)
        with patch.object(sys, 'argv', ['fm1_sample_upload.py', 'build', 'kit', path, 'kick.wav']):
            with contextlib.redirect_stdout(io.StringIO()):
                main()
        self.assertEqual(self.hdr.read_bytes(), b'H'*480)
        self.assertEqual(self.data.read_bytes(), b'DATA')
        with patch.object(sys, 'argv', ['fm1_sample_upload.py', 'build', 'kit', path, 'kick.wav']):
            with self.assertRaises(SystemExit):
                main()
        self.assertEqual(self.hdr.read_bytes(), b'H'*480)

    def test_normal_pair_and_repeat_no_clobber(self):
        write_slot_pair(self.prefix, b'HEADER', b'ADPCM')
        self.assertEqual(self.hdr.read_bytes(), b'HEADER')
        self.assertEqual(self.data.read_bytes(), b'ADPCM')
        with self.assertRaises(FileExistsError):
            write_slot_pair(self.prefix, b'new', b'new')
        self.assertEqual(self.hdr.read_bytes(), b'HEADER')
        self.assertEqual(self.data.read_bytes(), b'ADPCM')

    def test_preexisting_bin_prevents_header_write(self):
        self.data.write_bytes(b'RECORDING')
        with self.assertRaises(FileExistsError):
            write_slot_pair(self.prefix, b'HEADER', b'OVERWRITE')
        self.assertFalse(self.hdr.exists())
        self.assertEqual(self.data.read_bytes(), b'RECORDING')

    def test_symlink_destination_preserves_owner_file(self):
        recording = self.root / 'owner.wav'
        recording.write_bytes(b'ORIGINAL')
        self.hdr.symlink_to(recording)
        with self.assertRaises(FileExistsError):
            write_slot_pair(self.prefix, b'HEADER', b'BIN')
        self.assertTrue(self.hdr.is_symlink())
        self.assertEqual(recording.read_bytes(), b'ORIGINAL')
        self.assertFalse(self.data.exists())

    def test_failure_after_first_file_rolls_back(self):
        original = os.open
        def fail_second(file, flags, *args, **kwargs):
            if file == self.data.name and kwargs.get('dir_fd') is not None:
                raise OSError('simulated disk failure')
            return original(file, flags, *args, **kwargs)
        with patch('os.open', side_effect=fail_second):
            with self.assertRaises(OSError):
                write_slot_pair(self.prefix, b'HEADER', b'ADPCM')
        self.assertFalse(self.hdr.exists())
        self.assertFalse(self.data.exists())

    def test_short_write_does_not_leave_header(self):
        original = os.fdopen
        class ShortStream:
            def __init__(self, f): self.f=f
            def __enter__(self): self.f.__enter__(); return self
            def __exit__(self, *a): return self.f.__exit__(*a)
            def write(self, data): return self.f.write(data[:1])
            def flush(self): return self.f.flush()
            def fileno(self): return self.f.fileno()
        def short(fd, *a, **k): return ShortStream(original(fd,*a,**k))
        with patch('os.fdopen', side_effect=short):
            with self.assertRaisesRegex(OSError, 'short slot build write'):
                write_slot_pair(self.prefix, b'HEADER', b'ADPCM')
        self.assertFalse(self.hdr.exists())
        self.assertFalse(self.data.exists())

    def test_parent_swap_cannot_write_elsewhere(self):
        foreign = self.root / 'foreign'
        foreign.mkdir()
        original_dir = self.root / 'original'
        original = os.open
        def swap_before_bin(file, flags, *args, **kwargs):
            if file == self.data.name and kwargs.get('dir_fd') is not None:
                if not self.directory.is_symlink():
                    self.directory.rename(original_dir)
                    self.directory.symlink_to(foreign, target_is_directory=True)
            return original(file, flags, *args, **kwargs)
        with patch('os.open', side_effect=swap_before_bin):
            with self.assertRaisesRegex(OSError, 'directory changed'):
                write_slot_pair(self.prefix, b'HEADER', b'ADPCM')
        self.assertEqual(list(foreign.iterdir()), [])
        self.assertEqual(list(original_dir.iterdir()), [])

    def test_second_collision_after_preflight_preserves_foreign(self):
        original = os.open
        def race(file, flags, *a, **kw):
            if file == self.data.name and kw.get('dir_fd') is not None:
                self.data.write_bytes(b'OWNED')
            return original(file,flags,*a,**kw)
        with patch('os.open', side_effect=race):
            with self.assertRaises(FileExistsError):
                write_slot_pair(self.prefix, b'HEADER', b'ADPCM')
        self.assertFalse(self.hdr.exists())
        self.assertEqual(self.data.read_bytes(), b'OWNED')

    def test_fdopen_failure_closes_raw_descriptor(self):
        raw=[]
        def fail(fd,*a,**k):
            raw.append(fd);raise OSError(5,'injected fdopen failure')
        with patch('os.fdopen',side_effect=fail):
            with self.assertRaises(OSError):write_slot_pair(self.prefix,b'HEADER',b'ADPCM')
        for fd in raw:
            with self.assertRaises(OSError):os.fstat(fd)
        self.assertFalse(self.hdr.exists())

    def test_identify_after_fstat_failure(self):
        orig=os.fstat
        def fail_file(fd):
            st=orig(fd)
            if not (st.st_mode & 0o40000):
                raise OSError(5,'fstat intermittent')
            return st
        with patch('os.fstat',side_effect=fail_file):
            write_slot_pair(self.prefix,b'HEADER',b'ADPCM')
        self.assertEqual(self.hdr.read_bytes(), b'HEADER')
        self.assertEqual(self.data.read_bytes(), b'ADPCM')

if __name__=='__main__':
    sys.argv = sys.argv[:1]
    unittest.main(verbosity=2)
