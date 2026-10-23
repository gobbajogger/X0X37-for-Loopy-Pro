#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
import copy
import unittest
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools") if Path(__file__).resolve().parent.name == "tests" else str(Path(__file__).resolve().parent))
from catalog_public_gate import public_catalog

PKG = {"product":"SMK-37 Elite_900", "pkg":"firmware/elite.fwsc", "sha256":"a"*64,"size":651396}
V = {"version":"dev-checked-1", "source_commit":"b"*40, "built_at":"2026-10-22", "lifecycle":"current",
     "status":"dev-checked", "hardware_tested":False, "models":{"elite": PKG}}
CAT = {"repo":"https://github.com/gobbajogger/X0X37-for-Loopy-Pro", "pages":"https://example.org", "versions":[V], "history":[{"id":"failed-source"}]}
GOOD = {"version":"dev-checked-1", "model":"elite", "source_commit":"b"*40,
        "package_sha256":"a"*64, "product":"SMK-37 Elite_900", "controller_sha256":"c"*64,
        "report_sha256":"d"*64, "report_url":"https://example.org/device-report", "device_id_ref":"private-id-ref",
        **{k:True for k in ("physical_device_pass","exact_package_checked","source_and_model_checked", "controller_checked", "nonhardware_gates_pass","backup_readback_pass", "independent_review_pass","owner_publication_approved","evidence_report_reviewed")}}

class PublicFilterTests(unittest.TestCase):
    def test_untested_never_public(self):
        self.assertEqual(public_catalog(CAT, {})['versions'],[])
        self.assertEqual(public_catalog(CAT, {'dev-checked-1/elite':GOOD})['versions'],[])
        self.assertIsNone(public_catalog(CAT, {})['latest'])
    def test_approved_per_model(self):
        c=copy.deepcopy(CAT);c['versions'][0]['hardware_tested']=True
        self.assertEqual(public_catalog(c, {'dev-checked-1/elite':GOOD})['latest'],'dev-checked-1')
        c['versions'][0]['models']['pro']=dict(PKG,product='SMK-37 Pro_900',pkg='firmware/pro.fwsc',sha256='e'*64)
        self.assertEqual(list(public_catalog(c, {'dev-checked-1/elite':GOOD})['versions'][0]['models']),['elite'])
    def test_missing_or_mismatched_claim_fails_closed(self):
        c=copy.deepcopy(CAT);c['versions'][0]['hardware_tested']=True
        for key in GOOD:
            invalid=dict(GOOD);invalid.pop(key)
            self.assertEqual(public_catalog(c, {'dev-checked-1/elite':invalid})['versions'],[],key)
        for key,value in [('package_sha256','f'*64),('model','pro'),('controller_sha256','bad'),('report_url','http://bad')]:
            invalid=dict(GOOD);invalid[key]=value
            self.assertEqual(public_catalog(c, {'dev-checked-1/elite':invalid})['versions'],[],key)
    def test_broken_and_rollback_guard(self):
        c=copy.deepcopy(CAT);v=c['versions'][0];v['hardware_tested']=True
        v['status']='build-failed';self.assertEqual(public_catalog(c,{'dev-checked-1/elite':GOOD})['versions'],[])
        v['status']='dev-checked';v['lifecycle']='superseded'
        self.assertEqual(public_catalog(c,{'dev-checked-1/elite':GOOD})['versions'],[])
        self.assertEqual(public_catalog(c,{'dev-checked-1/elite':dict(GOOD,approved_rollback=True)})['latest'],'dev-checked-1')
    def test_bad_package_paths_or_models(self):
        c=copy.deepcopy(CAT);v=c['versions'][0];v['hardware_tested']=True
        for key,value in [('pkg','../wrong.fwsc'),('size',0),('sha256','f'*64),('product','SMK-37 Pro_900')]:
            x=copy.deepcopy(c);x['versions'][0]['models']['elite'][key]=value
            self.assertEqual(public_catalog(x,{'dev-checked-1/elite':GOOD})['versions'],[],key)

if __name__=='__main__': unittest.main(verbosity=2)
