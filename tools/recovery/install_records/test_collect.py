import unittest,tempfile,zipfile,os
from pathlib import Path
from unittest.mock import patch
import collect_install_records as c
class Tests(unittest.TestCase):
 def run_case(self, setup, **limits):
  with tempfile.TemporaryDirectory() as t:
   root=Path(t)/'project';root.mkdir();setup(root)
   with patch.multiple(c,**limits) if limits else patch.object(c,'MAX_FILES',c.MAX_FILES):
    m=c.collect(root,Path(t)/'out.zip')
   return m
 def test_exact_priority(self):
  def setup(r):
   for p in c.EXACT[:2]:f=r/p;f.parent.mkdir(parents=True,exist_ok=True);f.write_text('native')
   (r/'smc').mkdir();(r/'smc/install.log').write_text('other')
  m=self.run_case(setup,MAX_FILES=1)
  copied=[x for x in m['inventory'] if x['status']=='copied'];self.assertEqual(copied[0]['path'],c.EXACT[0]);self.assertEqual(len(copied),1)
 def test_missing(self):
  m=self.run_case(lambda r:None);self.assertEqual(m['counts']['files'],0);self.assertTrue(any(x['status'].startswith('missing') for x in m['inventory']))
 def test_exclusions_and_selected(self):
  def setup(r):
   for p in ['smc_old/install.log','smc_old/docs/install.log','smc_old/.config/install.log','smc_old/host-logs/install.log','unrelated/install.log','smc_old/auth-install.log','smc_old/ctl.img','integration_old/logs/install.log']:
    f=r/p;f.parent.mkdir(parents=True,exist_ok=True);f.write_text('x')
  m=self.run_case(setup);self.assertEqual({x['path'] for x in m['inventory'] if x['status']=='copied'},{'smc_old/install.log','integration_old/logs/install.log'})
 def test_symlink(self):
  def setup(r):
   (r/'smc_old').symlink_to('/tmp');(r/'integration_old').mkdir();(r/'integration_old/install.log').symlink_to('/etc/passwd')
  m=self.run_case(setup);self.assertEqual(m['counts']['files'],0);self.assertEqual(sum(x['status']=='symlink_not_followed' for x in m['inventory']),2)
 def test_exact_parent_symlink(self):
  def setup(r):(r/'final_install_integration').symlink_to('/tmp')
  m=self.run_case(setup);self.assertEqual(m['counts']['files'],0)
 def test_size_and_budget(self):
  def setup(r):
   (r/'smc').mkdir();(r/'smc/install.log').write_text('large')
  m=self.run_case(setup,MAX_FILE=2);self.assertTrue(any(x['status']=='file_size_limit' for x in m['inventory']))
  m=self.run_case(setup,MAX_BYTES=2);self.assertTrue(any(x['status']=='archive_budget_limit' for x in m['inventory']))
 def test_entry_limit(self):
  def setup(r):
   for i in range(8):(r/('smc'+str(i))).mkdir()
  m=self.run_case(setup,MAX_ENTRIES=3);self.assertFalse(m['scan_complete']);self.assertLessEqual(m['counts']['entries'],3)
 def test_changed(self):
  def setup(r):(r/'smc-install.log').write_text('x')
  with patch.object(c,'read_stable',side_effect=ValueError('changed during read')):
   m=self.run_case(setup)
  self.assertEqual(m['counts']['files'],0);self.assertTrue(any(x['status']=='missing_unreadable_or_changed' for x in m['inventory']))
if __name__=='__main__':unittest.main()
