import hashlib, importlib.util, json, os, tempfile, unittest, zipfile
from pathlib import Path
from unittest.mock import patch
spec=importlib.util.spec_from_file_location('collector', Path(__file__).with_name('collect.py'))
c=importlib.util.module_from_spec(spec);spec.loader.exec_module(c)

class Tests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.base=Path(self.tmp.name).resolve();self.root=self.base/'project';self.root.mkdir()
    def tearDown(self):self.tmp.cleanup()
    def run_collect(self, roots=None):return c.collect(roots or [('project',self.root)],self.base/'out.zip')
    def test_copy_hash_missing(self):
        (self.root/'old.fwsc').write_bytes(b'original')
        m=self.run_collect([('project',self.root),('missing',self.base/'absent')]);r=m['inventory'][0]
        self.assertEqual(r['sha256'],hashlib.sha256(b'original').hexdigest())
        self.assertEqual(m['roots'][1]['status'],'missing_unreadable_or_symlink')
        with zipfile.ZipFile(self.base/'out.zip') as z:self.assertEqual(z.read(r['archive_member']),b'original')
    def test_links_and_prune(self):
        outside=self.base/'outside';outside.mkdir();(outside/'install.log').write_text('NO')
        (self.root/'link').symlink_to(outside,target_is_directory=True)
        (self.root/'x.fwsc').symlink_to(outside/'install.log')
        for n in ['.codex','AC79_SDK','toolchain']:
            p=self.root/n;p.mkdir();(p/'install.log').write_text('NO')
        (self.root/'install-auth.json').write_text('NO')
        m=self.run_collect();self.assertEqual(m['counts']['files'],0)
        self.assertTrue(any(r['status']=='symlink_not_followed' for r in m['inventory']))
    def test_root_symlink(self):
        link=self.base/'link';link.symlink_to(self.root,target_is_directory=True)
        m=self.run_collect([('project',link)]);self.assertEqual(m['roots'][0]['status'],'missing_unreadable_or_symlink')
    def test_size_and_total_caps(self):
        (self.root/'a.fwsc').write_bytes(b'12345');(self.root/'b.fwsc').write_bytes(b'123');(self.root/'c.fwsc').write_bytes(b'123')
        with patch.object(c,'MAX_FILE',4),patch.object(c,'MAX_BYTES',4):m=self.run_collect()
        self.assertEqual([r['status'] for r in m['inventory']],['file_size_limit','copied','archive_budget_limit'])
    def test_entry_cap(self):
        for i in range(4):(self.root/(str(i)+'.log')).write_text('ok')
        with patch.object(c,'MAX_ENTRIES',2):m=self.run_collect()
        self.assertFalse(m['scan_complete']);self.assertEqual(m['counts']['entries'],2)
    def test_changed_before_read(self):
        p=self.root/'a.fwsc';p.write_bytes(b'old');fd=c.safe_open_dir(self.root);s=p.stat();p.write_bytes(b'newer')
        try:
            with self.assertRaises(ValueError):c.read_stable(fd,p.name,s,100)
        finally:os.close(fd)
    def test_changed_during_read(self):
        p=self.root/'a.fwsc';p.write_bytes(b'old');fd=c.safe_open_dir(self.root);s=p.stat()
        original=c.os.read;changed=[False]
        def mutate(f,n):
            result=original(f,n)
            if not changed[0]:
                changed[0]=True;p.write_bytes(b'changed-size')
            return result
        try:
            with patch.object(c.os,'read',mutate):
                with self.assertRaises(ValueError):c.read_stable(fd,p.name,s,100)
        finally:os.close(fd)
    def test_controller_only(self):
        (self.root/'ctl.img').write_bytes(b'ctl');(self.root/'auth.json').write_text('secret')
        m=self.run_collect([('controller-cache',self.root)])
        self.assertEqual(m['counts']['files'],1);self.assertEqual(len(m['inventory']),1)
    def test_private_gate_controller_only(self):
        d=self.root/'old.logs/home/.config';(d/'x0x').mkdir(parents=True)
        (d/'x0x/ctl.img').write_bytes(b'old-ctl')
        (d/'x0x/token.log').write_text('SECRET');(d/'credentials.log').write_text('SECRET')
        m=self.run_collect([('gate-cache',self.root)])
        self.assertEqual(m['counts']['files'],1)
        with zipfile.ZipFile(self.base/'out.zip') as z:
            self.assertEqual(z.read('evidence/gate-cache/old.logs/home/.config/x0x/ctl.img'),b'old-ctl')
            self.assertFalse(any('credentials' in n or 'token' in n for n in z.namelist()))
    def test_private_gate_symlink_and_missing(self):
        d=self.root/'old.logs/home/.config';d.mkdir(parents=True)
        outside=self.base/'outside';outside.mkdir();(outside/'ctl.img').write_bytes(b'NO')
        (d/'x0x').symlink_to(outside,target_is_directory=True)
        (self.root/'other.logs/home/.config').mkdir(parents=True)
        m=self.run_collect([('gate-cache',self.root)])
        self.assertEqual(m['counts']['files'],0)
        self.assertEqual(m['status_counts']['missing_unreadable_or_changed_private_ctl'],2)
    def test_existing_output_not_overwritten(self):
        (self.base/'out.zip').write_bytes(b'preserve')
        with self.assertRaises(FileExistsError):self.run_collect()
        self.assertEqual((self.base/'out.zip').read_bytes(),b'preserve')

if __name__=='__main__':unittest.main()
