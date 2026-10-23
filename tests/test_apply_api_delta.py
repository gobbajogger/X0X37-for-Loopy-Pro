#!/usr/bin/env python3
"""Offline source-byte transport, Git tree, and fail-closed negative tests."""
import copy
import datetime
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from apply_api_delta import materialize, SourceError, sha_blob, delta_bytes, lines, reject_symlink_parents


def g(root,*args,env=None):
    e=os.environ.copy()
    e.update({'GIT_AUTHOR_NAME':'Test User','GIT_AUTHOR_EMAIL':'test@example.org',
              'GIT_COMMITTER_NAME':'Test User','GIT_COMMITTER_EMAIL':'test@example.org',
              'GIT_AUTHOR_DATE':'2026-10-23T10:00:00+00:00',
              'GIT_COMMITTER_DATE':'2026-10-23T10:00:00+00:00'})
    if env:e.update(env)
    return subprocess.check_output(['git','-C',str(root),*args],env=e).decode().strip()


def gh_commit(root, sha):
    raw = subprocess.check_output(['git','-C',str(root),'cat-file','commit',sha])
    header, body=raw.split(b'\n\n',1)
    fields={}
    for line in header.decode().split('\n'):
        k,v=line.split(' ',1)
        if k in ('author','committer'):
            name_email, stamp, offset=v.rsplit(' ',2)
            name,email=name_email.rsplit(' <',1)
            fields[k]={'name':name,'email':email.rstrip('>'),
                       'date':datetime.datetime.fromtimestamp(int(stamp),datetime.timezone.utc)
                                   .strftime('%Y-%m-%dT%H:%M:%SZ')}
        else:fields.setdefault(k,[]).append(v)
    return {'sha':sha,'tree':{'sha':fields['tree'][0]},
            'parents':[{'sha':x} for x in fields.get('parent',[])],
            'author':fields['author'],'committer':fields['committer'],
            'message':body.decode(), 'verification':{'signature':None,'payload':None}}


class DeltaTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        base=Path(self.tmp.name)
        self.seed=base/'seed'
        self.output=base/'output'
        self.repo=base/'repo'
        self.repo.mkdir()
        g(self.repo,'init','-q')
        g(self.repo,'config','user.name','Test User')
        g(self.repo,'config','user.email','test@example.org')
        (self.repo/'main.c').write_text('first\nold\nlast\n')
        (self.repo/'remove.txt').write_text('deleted\n')
        (self.repo/'keep.txt').write_text('stable\n')
        g(self.repo,'add','-A');g(self.repo,'commit','-q','-m','Seed')
        self.seed_tree=g(self.repo,'rev-parse','HEAD^{tree}')
        g(base,'clone','-q','--no-local',str(self.repo),str(self.seed))
        (self.repo/'main.c').write_text('first\nnew\nlast\n')
        (self.repo/'remove.txt').unlink()
        (self.repo/'new.txt').write_text('added\n')
        g(self.repo,'add','-A');g(self.repo,'commit','-q','-m','Next source')
        sha=g(self.repo,'rev-parse','HEAD')
        self.manifest={
            'seed_tree':self.seed_tree,
            'target_tree':g(self.repo,'rev-parse','HEAD^{tree}'),
            'target_commit':sha,
            'github_commit':gh_commit(self.repo,sha),
            'files':[
              {'path':'main.c','old':sha_blob(b'first\nold\nlast\n'),
               'sha':sha_blob(b'first\nnew\nlast\n'),'mode':'100644',
               'ops':[[1,1,'new\n']]},
              {'path':'new.txt','old':None,'sha':sha_blob(b'added\n'),
               'mode':'100644','content':'added\n'}
             ],'deletions':['remove.txt']}
        self.input=base/'delta.json'

    def run_delta(self,data=None,dest=None):
        self.input.write_text(json.dumps(self.manifest if data is None else data))
        return materialize(self.seed,self.input,dest or self.output)

    def test_exact_sha_checkout(self):
        result=self.run_delta()
        self.assertEqual(result['target_commit'],self.manifest['target_commit'])
        self.assertEqual(g(self.output,'rev-parse','HEAD^{tree}'),self.manifest['target_tree'])
        self.assertEqual(g(self.output,'status','--porcelain'),'')
        self.assertEqual((self.output/'main.c').read_text(),'first\nnew\nlast\n')
        self.assertFalse((self.output/'remove.txt').exists())

    def test_mismatched_target_tree_refused_without_output(self):
        m=copy.deepcopy(self.manifest);m['target_tree']='0'*40
        with self.assertRaises(SourceError):self.run_delta(m)
        self.assertFalse(self.output.exists())

    def test_missing_delete_is_no_go(self):
        m=copy.deepcopy(self.manifest);m['deletions']=[]
        with self.assertRaises(SourceError):self.run_delta(m)
        self.assertFalse(self.output.exists())

    def test_missing_change_is_no_go(self):
        m=copy.deepcopy(self.manifest);m['files'].pop()
        with self.assertRaises(SourceError):self.run_delta(m)
        self.assertFalse(self.output.exists())

    def test_bad_old_sha_is_no_go(self):
        m=copy.deepcopy(self.manifest);m['files'][0]['old']='f'*40
        with self.assertRaises(SourceError):self.run_delta(m)
        self.assertFalse(self.output.exists())

    def test_bad_new_sha_is_no_go(self):
        m=copy.deepcopy(self.manifest);m['files'][0]['sha']='f'*40
        with self.assertRaises(SourceError):self.run_delta(m)
        self.assertFalse(self.output.exists())

    def test_path_traversal_is_no_go(self):
        m=copy.deepcopy(self.manifest);m['files'][0]['path']='../outside.c'
        with self.assertRaises(SourceError):self.run_delta(m)
        self.assertFalse(self.output.exists())

    def test_overlapping_edits_are_no_go(self):
        m=copy.deepcopy(self.manifest);m['files'][0]['ops']=[[1,1,'new\n'],[1,1,'bad\n']]
        with self.assertRaises(SourceError):self.run_delta(m)
        self.assertFalse(self.output.exists())

    def test_existing_destination_refused(self):
        self.output.mkdir()
        with self.assertRaises(SourceError):self.run_delta()

    def test_altered_commit_metadata_refused(self):
        m=copy.deepcopy(self.manifest);m['github_commit']['message']='Something else\n'
        with self.assertRaises(SourceError):self.run_delta(m)
        self.assertFalse(self.output.exists())

    def test_symlink_parent_rejected(self):
        root=Path(self.tmp.name)
        (root/'link').symlink_to(root, target_is_directory=True)
        with self.assertRaises(SourceError):
            reject_symlink_parents(root,Path('link/file.txt'))

    def test_symlink_escape_rejected(self):
        m=copy.deepcopy(self.manifest)
        m['files'].append({'path':'link.txt','old':None,'sha':sha_blob(b'../outside'),
                           'mode':'120000','content':'../outside'})
        with self.assertRaises(SourceError):self.run_delta(m)
        self.assertFalse(self.output.exists())

    def test_non_lf_bytes_never_split(self):
        source=b'old\vtext\nlast\rthing\n'
        self.assertEqual(lines(source),[b'old\vtext\n',b'last\rthing\n'])
        value=delta_bytes({'old':sha_blob(source),'ops':[[0,1,'new\vtext\n']]},source)
        self.assertEqual(value,b'new\vtext\nlast\rthing\n')


if __name__=='__main__':unittest.main()
