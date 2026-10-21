#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Private fake Git histories exercise export exclusions; no network or device."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]/'tools/distribution'))
import source_inventory as inventory

class HistoryTest(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory(prefix='distribution-history-')
        self.repo=Path(self.tmp.name)/'repo'; self.repo.mkdir()
        self.git('init','-q'); self.git('config','user.name','Fixture'); self.git('config','user.email','fixture@example.invalid')
    def tearDown(self): self.tmp.cleanup()
    def git(self,*args): return subprocess.check_output(['git','-C',str(self.repo),*args],stderr=subprocess.DEVNULL,text=True).strip()
    def commit(self,path,data=b'fixture\n'):
        p=self.repo/path; p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
        self.git('add',path);self.git('commit','-qm','fixture');return self.git('rev-parse','HEAD')
    def test_removed_excluded_path_still_refused(self):
        self.commit('refs-oscuro/old.txt');self.git('rm','-q','refs-oscuro/old.txt');self.commit('README.md')
        with self.assertRaisesRegex(ValueError,'excluded reachable'): inventory.scan(self.repo,'HEAD')
    def test_renamed_library_refused(self):
        self.commit('innocent.txt',b'!<arch>\n' + b'x'*80)
        with self.assertRaisesRegex(ValueError,'compiled content'): inventory.scan(self.repo,'HEAD')
    def test_pinned_sdk_renamed_refused(self):
        import hashlib
        data=b'opaque vendor test payload';pin=hashlib.sha256(data).hexdigest()
        self.commit('tools/build.py',("SDK_SHA256 = {'vendor': '"+pin+"'}\n").encode());self.commit('innocent.txt',data)
        with self.assertRaisesRegex(ValueError,'pinned SDK'): inventory.scan(self.repo,'HEAD')
    def test_plain_source_allowed(self):
        self.commit('README.md');self.commit('tools/test.py',b'print(1)\n')
        result=inventory.scan(self.repo,'HEAD');self.assertEqual(result['reachable_commits'],2)
    def test_escaping_symlink_refused(self):
        (self.repo/'escape').symlink_to(self.repo.parent)
        with self.assertRaisesRegex(ValueError,'unsafe source symlink'): inventory.validate_links(self.repo)
    def test_submodule_refused(self):
        commit=self.commit('README.md');self.git('update-index','--add','--cacheinfo','160000',commit,'nested');self.git('commit','-qm','fixture submodule')
        with self.assertRaisesRegex(ValueError,'unsupported source'): inventory.scan(self.repo,'HEAD')

if __name__=='__main__': unittest.main()
