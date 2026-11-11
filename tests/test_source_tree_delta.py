#!/usr/bin/env python3
"""Offline full/delta source-recovery regression, including failure cases."""
import base64
import hashlib
import json
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile
import unittest

TOOLS = Path(__file__).resolve().parents[1] / 'tools'
sys.path.insert(0, str(TOOLS))
from restore_github_api_batches import object_sha, root_tree_sha
from source_tree_delta import read_manifest, missing_blobs, verified_cache


def entry(path,mode,data):
    return {'path':path,'mode':mode,'sha':object_sha('blob',data),'size':len(data)}


def manifest(path, entries):
    content = [
      '# GitHub blob manifest; exact pinned source, not a checkout',
      '# repository gobbajogger/X0X37-for-Loopy-Pro',
      '# commit ' + 'a'*40,
      '# tree ' + root_tree_sha(entries),
      '# blobs ' + str(len(entries)),
      '# bytes ' + str(sum(e['size'] for e in entries)),
      '# fields path<TAB>mode<TAB>sha<TAB>size',
    ] + ['\t'.join(map(str,(e['path'],e['mode'],e['sha'],e['size']))) for e in entries]
    path.write_text('\n'.join(content)+'\n')


class DeltaTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.p=Path(self.tmp.name)
        self.old=self.p/'old'
        (self.old/'a').mkdir(parents=True)
        (self.old/'a'/'same.txt').write_bytes(b'SAME\n')
        (self.old/'a'/'run.sh').write_bytes(b'echo old\n')
        (self.old/'a'/'run.sh').chmod(0o755)
        (self.old/'link').symlink_to('a/same.txt')
        self.old_entries = [entry('a/same.txt','100644',b'SAME\n'),entry('a/run.sh','100755',b'echo old\n'),entry('link','120000',b'a/same.txt')]
        self.new_data = [(entry('a/same.txt','100644',b'SAME\n'),b'SAME\n'),
                         (entry('a/run.sh','100755',b'echo new\n'),b'echo new\n'),
                         (entry('a/new.txt','100644',b'NEW!\n'),b'NEW!\n'),
                         (entry('copy.txt','100644',b'SAME\n'),b'SAME\n'),
                         (entry('link','120000',b'a/same.txt'),b'a/same.txt')]
        self.new_entries=[e for e,_ in self.new_data]
        self.old_manifest=self.p/'old.tsv'
        self.target_manifest=self.p/'new.tsv'
        manifest(self.old_manifest,self.old_entries)
        manifest(self.target_manifest,self.new_entries)
        oldcache=verified_cache(self.old,self.old_entries)
        self.needed=missing_blobs(self.new_entries,oldcache)
        self.assertEqual(len(self.needed),2)
        self.receipts=self.p/'received.jsonl'
        payload=[]
        for idx,e in enumerate(self.needed):
            b=next(raw for item,raw in self.new_data if item['sha']==e['sha'])
            s=base64.b64encode(b).decode()
            payload.append({'index':idx,**e,'start':0,'total':len(s),'b64':s})
        self.make_receipts(payload)

    def make_receipts(self,payload):
        row={'tree_sha':root_tree_sha(self.new_entries),'truncated':False,
             'total':len(self.needed),'cursor':[0,0],
             'next':[len(self.needed),0],'chunks':payload}
        self.receipts.write_text(json.dumps(row)+'\n')

    def run_tool(self,*args):
        return subprocess.run([sys.executable,str(TOOLS/'source_tree_delta.py'),
            '--target-manifest',str(self.target_manifest),
            '--base-manifest',str(self.old_manifest),'--base-root',str(self.old),
            *map(str,args)],text=True,capture_output=True)

    def test_manifest_and_needed_list(self):
        h,entries=read_manifest(self.target_manifest)
        self.assertEqual(h['tree'],root_tree_sha(entries))
        r=self.run_tool('--needed-json',self.p/'needed.json')
        self.assertEqual(r.returncode,0,r.stderr)
        actual=json.loads((self.p/'needed.json').read_text())
        self.assertEqual(actual['total'],2)
        self.assertEqual(actual['entries'],self.needed)

    def test_incremental_round_trip_symlink_and_modes(self):
        out=self.p/'newsource'
        r=self.run_tool('--receipts',self.receipts,'--output',out)
        self.assertEqual(r.returncode,0,r.stderr)
        self.assertIn('PASS: exact reconstructed source',r.stdout)
        self.assertEqual((out/'a'/'run.sh').read_bytes(),b'echo new\n')
        self.assertTrue((out/'a'/'run.sh').stat().st_mode & stat.S_IXUSR)
        self.assertEqual((out/'copy.txt').read_bytes(),b'SAME\n')
        self.assertTrue((out/'link').is_symlink())
        self.assertEqual(os.readlink(out/'link'),'a/same.txt')
        self.assertEqual(len(verified_cache(out,self.new_entries)),4)

    def test_corrupt_receipt_fails_closed(self):
        original=json.loads(self.receipts.read_text())
        original['chunks'][0]['b64']=base64.b64encode(b'XXXXXXXXX').decode()
        self.receipts.write_text(json.dumps(original)+'\n')
        r=self.run_tool('--receipts',self.receipts,'--output',self.p/'out')
        self.assertNotEqual(r.returncode,0)
        self.assertIn('receipt byte SHA mismatch',r.stderr)
        self.assertFalse((self.p/'out').exists())

    def test_modified_base_fails_closed(self):
        (self.old/'a'/'same.txt').write_bytes(b'BAD\n')
        r=self.run_tool('--needed-json',self.p/'needed.json')
        self.assertNotEqual(r.returncode,0)
        self.assertIn('blob size/SHA mismatch',r.stderr)

    def test_wrong_root_tree_fails_closed(self):
        s=self.target_manifest.read_text().replace(root_tree_sha(self.new_entries),'0'*40)
        self.target_manifest.write_text(s)
        r=self.run_tool('--needed-json',self.p/'needed.json')
        self.assertNotEqual(r.returncode,0)
        self.assertIn('manifest root-tree hash mismatch',r.stderr)

    def test_incomplete_receipts_fails_closed(self):
        self.receipts.write_text('')
        r=self.run_tool('--receipts',self.receipts,'--output',self.p/'out')
        self.assertNotEqual(r.returncode,0)
        self.assertIn('incomplete receipts',r.stderr)

    def test_full_transfer_without_cache(self):
        needed=missing_blobs(self.new_entries,set())
        receipts=self.p/'full.jsonl'
        chunks=[]
        for idx,e in enumerate(needed):
            b=next(raw for item,raw in self.new_data if item['sha']==e['sha'])
            s=base64.b64encode(b).decode()
            chunks.append({'index':idx,**e,'start':0,'total':len(s),'b64':s})
        receipts.write_text(json.dumps({'tree_sha':root_tree_sha(self.new_entries),'truncated':False,
            'total':len(needed),'cursor':[0,0],'next':[len(needed),0],'chunks':chunks})+'\n')
        out=self.p/'full'
        r=subprocess.run([sys.executable,str(TOOLS/'source_tree_delta.py'),
          '--target-manifest',str(self.target_manifest),'--receipts',str(receipts),
          '--output',str(out)],capture_output=True,text=True)
        self.assertEqual(r.returncode,0,r.stderr)
        self.assertEqual((out/'a'/'run.sh').read_bytes(),b'echo new\n')

    def test_unsafe_link_rejected(self):
        (self.old/'link').unlink()
        (self.old/'link').symlink_to('../../escape')
        r=self.run_tool('--needed-json',self.p/'needed.json')
        self.assertNotEqual(r.returncode,0)
        self.assertIn('unsafe symlink',r.stderr)


if __name__=='__main__':
    unittest.main(verbosity=2)
