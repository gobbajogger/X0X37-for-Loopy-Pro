#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exercise cache boundaries with real subprocesses and content identities.

Run: python3 -B tests/host/test_asset_cache.py
Linux also checks actual interpreter and shared-library discovery.
"""
from concurrent.futures import ThreadPoolExecutor
import contextlib
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import signal
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
import asset_cache as cache

GENERATOR = '''from pathlib import Path
import os, signal
import math
import sampleio, fixture_runtime
SRC = Path(__file__).resolve().parents[1]
SAMPLES = [('sample', 'input.wav')]
if __name__ == '__main__':
    import sys
    data = (SRC / 'assets/909/input.wav').read_text()
    output = Path(sys.argv[1])
    output.write_text(data + fixture_runtime.TOKEN + os.environ.get('X0X_FONTSET', 'default'))
    if data == 'WAIT':
        import time
        time.sleep(30)
    if data == 'FAIL':
        sys.exit(19)
    if data == 'STOP':
        os.kill(os.getpid(), signal.SIGTERM)
    (output.parent / 'x0x_drum_tables.h').write_text('table:' + data)
    if data == 'CHANGE':
        (SRC / 'assets/909/input.wav').write_text('changed')
'''


@unittest.skipUnless(sys.platform == 'linux', 'runtime discovery requires Linux')
class CacheTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.base = Path(self.tmp.name)
        self.root = self.base / 'source'
        (self.root / 'tools').mkdir(parents=True)
        (self.root / 'assets/909').mkdir(parents=True)
        shutil.copyfile(ROOT / 'tools/asset_cache.py', self.root / 'tools/asset_cache.py')
        (self.root / 'tools/build.py').write_text('# fixture build integration\n')
        (self.root / 'tools/gen_drum_samples.py').write_text(GENERATOR)
        (self.root / 'tools/sampleio.py').write_text('# fixture shared helper\n')
        (self.root / 'tools/fixture_runtime.py').write_text('TOKEN = "runtime1"\n')
        self.input = self.root / 'assets/909/input.wav'
        self.input.write_text('audio')
        self.gen = self.root / 'build/gen'
        self.store = self.base / 'cache'
        self.env = patch.dict(os.environ, {'X0X_FONTSET': 'fixture', 'X0X_ASSET_CACHE': '1'})
        self.env.start()

    def tearDown(self):
        self.env.stop()
        self.tmp.cleanup()

    def generate(self, root=None, store=None):
        root = root or self.root
        return cache.generate_asset('drums', root, root / 'build/gen',
                                    self.store if store is None else store)

    def entries(self):
        return list(self.store.glob('drums/*/manifest.json'))

    def test_verified_hit_replaces_missing_and_stale_build_outputs(self):
        self.assertFalse(self.generate())
        expected = (self.gen / 'x0x_drum_samples.h').read_bytes()
        (self.gen / 'x0x_drum_samples.h').unlink()
        (self.gen / 'x0x_drum_tables.h').write_text('stale')
        self.assertTrue(self.generate())
        self.assertEqual(expected, (self.gen / 'x0x_drum_samples.h').read_bytes())
        self.assertEqual('table:audio', (self.gen / 'x0x_drum_tables.h').read_text())

    def test_same_mtime_content_generator_helper_runtime_and_config_changes(self):
        self.assertFalse(self.generate())
        for file in (self.input, self.root / 'tools/gen_drum_samples.py',
                     self.root / 'tools/sampleio.py', self.root / 'tools/fixture_runtime.py'):
            original = file.stat()
            file.write_text(file.read_text() + ('x' if file == self.input else '\n# content edit\n'))
            os.utime(file, ns=(original.st_atime_ns, original.st_mtime_ns))
            self.assertFalse(self.generate(), str(file))
            self.assertTrue(self.generate())
        os.environ['X0X_FONTSET'] = 'other'
        self.assertFalse(self.generate())

    def test_corrupt_missing_and_malformed_entries_are_misses(self):
        self.generate()
        entry = self.entries()[0].parent
        for damage in ('corrupt', 'missing', 'malformed', 'partial'):
            if damage == 'corrupt':
                (entry / 'x0x_drum_samples.h').write_text('corrupt')
            elif damage == 'missing':
                (entry / 'x0x_drum_tables.h').unlink(missing_ok=True)
            elif damage == 'malformed':
                (entry / 'manifest.json').write_text('null')
            else:
                shutil.rmtree(entry)
                entry.mkdir()
            self.assertFalse(self.generate(), damage)
            self.assertEqual('table:audio', (self.gen / 'x0x_drum_tables.h').read_text())

    def test_discovery_reuse_checks_contents_without_restarting_tools(self):
        self.assertFalse(self.generate())
        with patch.object(cache, 'run_checked', side_effect=AssertionError('unexpected discovery')):
            self.assertTrue(self.generate())
        descriptor = next(self.store.glob('discovery/drums/*.json'))
        descriptor.write_text('{partial')
        with patch.object(cache, 'run_checked', wraps=cache.run_checked) as discover:
            self.assertTrue(self.generate())
            self.assertTrue(discover.called)
        # A new earlier import candidate invalidates the recorded resolution graph.
        (self.root / 'tools/math.py').write_text('raise RuntimeError("shadow module")')
        self.assertFalse(self.generate())

    def test_cpu_and_runtime_environment_changes_invalidate_reuse(self):
        self.assertFalse(self.generate())
        with patch.object(cache, 'cpu_identity', return_value={'cpu': ['changed']}):
            self.assertFalse(self.generate())
            self.assertTrue(self.generate())
        with patch.dict(os.environ, {'GLIBC_TUNABLES': 'glibc.cpu.hwcaps=-AVX2'}):
            self.assertFalse(self.generate())

    def test_entry_from_another_key_is_rejected(self):
        self.generate()
        first = self.entries()[0].parent
        self.input.write_text('other audio')
        self.generate()
        second = next(p.parent for p in self.entries() if p.parent != first)
        shutil.rmtree(first)
        shutil.copytree(second, first)
        self.input.write_text('audio')
        self.assertFalse(self.generate())
        self.assertTrue((self.gen / 'x0x_drum_samples.h').read_text().startswith('audio'))

    def test_failure_and_signal_do_not_publish_or_replace_old_output(self):
        self.generate()
        previous = (self.gen / 'x0x_drum_samples.h').read_bytes()
        for value, status in [('FAIL', 19), ('STOP', -15)]:
            self.input.write_text(value)
            with self.assertRaises(subprocess.CalledProcessError) as caught:
                self.generate()
            self.assertEqual(status, caught.exception.returncode)
            self.assertEqual(previous, (self.gen / 'x0x_drum_samples.h').read_bytes())
            self.assertEqual(1, len(self.entries()))
        self.assertFalse(list((self.root / 'build').glob('.asset-*')))

    def test_interrupted_parent_cannot_publish_partial_output(self):
        self.input.write_text('WAIT')
        code = ('import sys;from pathlib import Path;sys.path.insert(0,sys.argv[1]);'
                'import asset_cache;asset_cache.generate_asset("drums",Path(sys.argv[2]),'
                'Path(sys.argv[2])/"build/gen",Path(sys.argv[3]))')
        proc = subprocess.Popen([sys.executable, '-B', '-c', code,
                                 str(ROOT / 'tools'), str(self.root), str(self.store)],
                                start_new_session=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        try:
            deadline = time.monotonic() + 20
            while time.monotonic() < deadline:
                if list((self.root / 'build').glob('.asset-*/gen/x0x_drum_samples.h')):
                    break
                if proc.poll() is not None:
                    self.fail('parent exited before the interrupted-generation check')
                time.sleep(0.02)
            else:
                self.fail('generator did not reach its partial-output boundary')
            os.killpg(proc.pid, signal.SIGTERM)
            proc.communicate(timeout=5)
            self.assertNotEqual(0, proc.returncode)
            self.assertEqual([], self.entries())
            self.assertFalse((self.gen / 'x0x_drum_samples.h').exists())
            self.input.write_text('audio')
            self.assertFalse(self.generate())
        finally:
            if proc.poll() is None:
                os.killpg(proc.pid, signal.SIGKILL)
                proc.communicate(timeout=5)

    def test_input_change_during_generation_does_not_publish(self):
        self.input.write_text('CHANGE')
        with self.assertRaisesRegex(RuntimeError, 'inputs changed'):
            self.generate()
        self.assertEqual([], self.entries())
        self.assertFalse((self.gen / 'x0x_drum_samples.h').exists())

    def test_unwritable_cache_and_unknown_runtime_run_generator(self):
        blocker = self.base / 'not-a-directory'
        blocker.write_text('cache unavailable')
        self.assertFalse(self.generate(store=blocker))
        with patch.object(cache, 'identity', side_effect=ValueError('unknown runtime')):
            self.assertFalse(self.generate())
        self.assertEqual([], self.entries())

    def test_unknown_runtime_still_checks_source_changes(self):
        self.input.write_text('CHANGE')
        with patch.object(cache, 'identity', side_effect=ValueError('unknown runtime')):
            with self.assertRaisesRegex(RuntimeError, 'inputs changed'):
                self.generate()

    def test_fresh_checkout_and_concurrent_publication(self):
        other = self.base / 'fresh-source'
        shutil.copytree(self.root, other)
        with ThreadPoolExecutor(2) as executor:
            results = list(executor.map(self.generate, (self.root, other)))
        self.assertTrue(all(isinstance(value, bool) for value in results))
        self.assertEqual(1, len(self.entries()))
        shutil.rmtree(other / 'build')
        self.assertTrue(self.generate(root=other))
        self.assertEqual((self.gen / 'x0x_drum_samples.h').read_bytes(),
                         (other / 'build/gen/x0x_drum_samples.h').read_bytes())

    def test_runtime_bytes_are_in_identity(self):
        self.gen.mkdir(parents=True)
        with tempfile.TemporaryDirectory() as tmp:
            identity, _ = cache.identity('drums', self.root, self.gen, Path(tmp), None)
        files = identity['files']
        self.assertTrue(any('fixture_runtime.py' in p for p in files))
        self.assertTrue(any('libm.so' in p for p in files))
        self.assertTrue(any('libc.so' in p for p in files))
        self.assertTrue(any('python3' in p for p in files))


@unittest.skipUnless(sys.platform == 'linux', 'runtime discovery requires Linux')
class SelectedAssetTests(unittest.TestCase):
    setUp = CacheTests.setUp
    tearDown = CacheTests.tearDown
    # Use the real font and custom-bank generators for lookup/config semantics.
    def test_font_selection_order_and_content(self):
        shutil.copyfile(ROOT / 'tools/gen_font.py', self.root / 'tools/gen_font.py')
        external = self.base / 'external'
        external.mkdir()
        for font in (ROOT / 'assets/fonts').glob('Barlow*.ttf'):
            shutil.copyfile(font, external / font.name)
        os.environ['X0X_FONTSET'] = 'barlow'
        os.environ['X0X_FONT_DIRS'] = str(external)
        self.gen.mkdir(parents=True)
        with tempfile.TemporaryDirectory() as tmp:
            before, _ = cache.identity('font', self.root, self.gen, Path(tmp), None)
            selected = external / 'BarlowSemiCondensed-Medium.ttf'
            st = selected.stat()
            selected.write_bytes(selected.read_bytes() + b'changed font content')
            os.utime(selected, ns=(st.st_atime_ns, st.st_mtime_ns))
            after, _ = cache.identity('font', self.root, self.gen, Path(tmp), None)
            self.assertNotEqual(before, after)
            bundled = self.root / 'assets/fonts'
            bundled.mkdir()
            shutil.copyfile(selected, bundled / selected.name)
            priority, _ = cache.identity('font', self.root, self.gen, Path(tmp), None)
            self.assertNotEqual(after, priority)
            self.assertIn('$SRC/assets/fonts/' + selected.name, priority['settings']['selected'])
            os.environ['X0X_FONT_DIRS'] = str(self.base / 'absent') + ':' + str(external)
            reordered, _ = cache.identity('font', self.root, self.gen, Path(tmp), None)
            self.assertNotEqual(priority, reordered)
            self.assertTrue(any('freetype' in name for name in before['files']))
            for font in (ROOT / 'assets/fonts').glob('ter-*.bdf'):
                shutil.copyfile(font, bundled / font.name)
            os.environ['X0X_FONTSET'] = 'terminus'
            fontset, _ = cache.identity('font', self.root, self.gen, Path(tmp), None)
            self.assertNotEqual(reordered, fontset)


    def test_builtin_tracks_headers_compiler_and_libraries(self):
        shutil.copytree(ROOT / 'firmware/src', self.root / 'firmware/src')
        for name in ('gen_builtin_break.c', 'gen_builtin_break.sh', 'gen_drum_samples.py', 'sampleio.py'):
            shutil.copyfile(ROOT / 'tools' / name, self.root / 'tools' / name)
        shutil.copytree(ROOT / 'assets/909', self.root / 'assets/909', dirs_exist_ok=True)
        self.gen.mkdir(parents=True)
        subprocess.run([sys.executable, str(self.root / 'tools/gen_drum_samples.py'),
                        str(self.gen / 'x0x_drum_samples.h')], check=True, stdout=subprocess.PIPE)
        with tempfile.TemporaryDirectory() as tmp:
            before, _ = cache.identity('breaks', self.root, self.gen, Path(tmp), None)
            self.assertTrue(any('/cc1' in name for name in before['files']))
            self.assertTrue(any('libm.so' in name for name in before['files']))
            self.assertIn('$SRC/build/gen/x0x_drum_tables.h', before['files'])
            header = self.root / 'firmware/src/dsp/drum909.h'
            st = header.stat()
            header.write_text(header.read_text() + '\n/* dependency content edit */\n')
            os.utime(header, ns=(st.st_atime_ns, st.st_mtime_ns))
            after, _ = cache.identity('breaks', self.root, self.gen, Path(tmp), None)
            self.assertNotEqual(before, after)
            with patch.dict(os.environ, {'CC': 'gcc'}):
                compiler, _ = cache.identity('breaks', self.root, self.gen, Path(tmp), None)
            self.assertNotEqual(after, compiler)
            with patch.dict(os.environ, {'CC': 'cc -O0'}):
                with self.assertRaisesRegex(ValueError, 'wrapper'):
                    cache.identity('breaks', self.root, self.gen, Path(tmp), None)

    def test_custom_bank_name_manifest_and_audio(self):
        import struct
        import wave
        for name in ('gen_break_bank.py', 'sampleio.py'):
            shutil.copyfile(ROOT / 'tools' / name, self.root / 'tools' / name)
        bank = self.base / 'bankA'
        bank.mkdir()
        (bank / 'bank.txt').write_text('loop 1\n')
        with wave.open(str(bank / 'loop.wav'), 'wb') as wav:
            wav.setparams((1, 2, 44100, 0, 'NONE', 'none'))
            wav.writeframes(struct.pack('<8h', 0, 100, 200, 100, 0, -100, -200, 0))
        os.environ['X0X_BREAK_BANK'] = str(bank)
        self.assertFalse(cache.generate_asset('breaks', self.root, self.gen, self.store))
        self.assertTrue(cache.generate_asset('breaks', self.root, self.gen, self.store))
        original = (self.gen / 'x0x_break_bank.h').read_bytes()
        other = self.base / 'bankB'
        shutil.copytree(bank, other)
        os.environ['X0X_BREAK_BANK'] = str(other)
        self.assertFalse(cache.generate_asset('breaks', self.root, self.gen, self.store))
        self.assertNotEqual(original, (self.gen / 'x0x_break_bank.h').read_bytes())
        audio = other / 'loop.wav'
        st = audio.stat()
        data = bytearray(audio.read_bytes())
        data[-2:] = b'\x01\x01'
        audio.write_bytes(data)
        os.utime(audio, ns=(st.st_atime_ns, st.st_mtime_ns))
        self.assertFalse(cache.generate_asset('breaks', self.root, self.gen, self.store))
        (other / 'bank.txt').write_text('loop 2\n')
        self.assertFalse(cache.generate_asset('breaks', self.root, self.gen, self.store))


class LocationTests(unittest.TestCase):
    def test_git_common_directory_survives_real_worktree(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / 'repo'
            root.mkdir()
            def git(*args):
                subprocess.run(['git', '-C', str(root), *args], check=True,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            git('init')
            git('-c', 'user.name=fixture', '-c', 'user.email=fixture@example.invalid',
                'commit', '--allow-empty', '-m', 'fixture')
            other = Path(tmp) / 'worktree'
            git('worktree', 'add', '--detach', str(other))
            self.assertEqual(cache.cache_directory(root), cache.cache_directory(other))
            archive = Path(tmp) / 'archive'
            archive.mkdir()
            self.assertEqual(archive / 'build/asset-cache/v1', cache.cache_directory(archive))


if __name__ == '__main__':
    unittest.main()
