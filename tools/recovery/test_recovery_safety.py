#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Fixture-only common recovery boundaries; no real-home, network or device I/O."""
import hashlib
import importlib.util
import os
from pathlib import Path
import sys
import tempfile
import unittest
import zipfile

HERE = Path(__file__).resolve().parent


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


artifact = load('artifact_collector', HERE / 'artifact_evidence/collect.py')
sys.path.insert(0, str(HERE / 'install_records'))
install = load('install_collector', HERE / 'install_records/collect_install_records.py')


def invoke(collector, root, output):
    if collector is artifact:
        return collector.collect([('project', root)], output)
    return collector.collect(root, output)


class SafetyTests(unittest.TestCase):
    def test_symlink_ancestor_is_not_followed(self):
        with tempfile.TemporaryDirectory() as tmp:
            base = Path(tmp)
            project = base / 'real/project'
            project.mkdir(parents=True)
            (project / 'smc-install.log').write_text('DO NOT READ')
            (base / 'link').symlink_to(base / 'real', target_is_directory=True)
            for name, collector in [('artifact', artifact), ('install', install)]:
                with self.subTest(collector=name):
                    result = invoke(collector, base / 'link/project', base / (name + '.zip'))
                    self.assertEqual(result['counts']['files'], 0)
                    with zipfile.ZipFile(base / (name + '.zip')) as archive:
                        self.assertFalse(any(p.startswith('evidence/') for p in archive.namelist()))

    def test_existing_output_is_preserved(self):
        with tempfile.TemporaryDirectory() as tmp:
            base = Path(tmp)
            root = base / 'project'
            root.mkdir()
            for name, collector in [('artifact', artifact), ('install', install)]:
                with self.subTest(collector=name):
                    output = base / (name + '.zip')
                    output.write_bytes(b'original output')
                    with self.assertRaises(FileExistsError):
                        invoke(collector, root, output)
                    self.assertEqual(output.read_bytes(), b'original output')

    def test_fifo_not_read_and_source_bytes_preserved(self):
        with tempfile.TemporaryDirectory() as tmp:
            base = Path(tmp)
            root = base / 'project'
            root.mkdir()
            source = root / 'smc-install.log'
            source.write_bytes(b'fixture install text\n')
            expected = hashlib.sha256(source.read_bytes()).hexdigest()
            os.mkfifo(root / 'smc-fifo.log')
            for name, collector in [('artifact', artifact), ('install', install)]:
                with self.subTest(collector=name):
                    output = base / (name + '.zip')
                    result = invoke(collector, root, output)
                    self.assertEqual(result['counts']['files'], 1)
                    self.assertEqual(hashlib.sha256(source.read_bytes()).hexdigest(), expected)
                    with zipfile.ZipFile(output) as archive:
                        self.assertEqual(archive.read('evidence/project/smc-install.log'), source.read_bytes())
                        self.assertFalse(any('fifo' in p for p in archive.namelist()))


if __name__ == '__main__':
    unittest.main()
