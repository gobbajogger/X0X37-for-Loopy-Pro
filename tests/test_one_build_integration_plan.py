#!/usr/bin/env python3
import copy,importlib.util,json,pathlib,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('validator',ROOT/'tools/validate_one_build_integration_plan.py');v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
DATA=json.loads((ROOT/'docs/one-build-integration-inventory.json').read_text())
class Contract(unittest.TestCase):
 def test_valid(self):self.assertEqual(v.validate(DATA),[])
 def changed(self,fn):
  d=copy.deepcopy(DATA);fn(d);self.assertTrue(v.validate(d))
 def test_missing_branch(self):self.changed(lambda d:d['branches'].pop())
 def test_missing_pr(self):self.changed(lambda d:d['open_prs'].pop())
 def test_missing_requirement(self):self.changed(lambda d:d['missing_706'].pop())
 def test_invalid_target(self):self.changed(lambda d:d.update(target_branch='main'))
 def test_invalid_stage_target(self):self.changed(lambda d:d['stages'][0].update(target='nightly'))
 def test_bad_dependency(self):self.changed(lambda d:d['stages'][0].update(after=['S09']))
 def test_removed_boot_hold(self):self.changed(lambda d:d['bootloader'].update(status='READY'))
 def test_substituted_branch(self):self.changed(lambda d:d['branches'][0].update(branch='invented'))
 def test_missing_facet(self):self.changed(lambda d:d['additional_request_facets'].pop())
 def test_removed_own_page_must_fix(self):self.changed(lambda d:d['owner_followup_20261030']['must_fix'].update(status='COMPLETE'))
 def test_false_usrtrim_acceptance(self):self.changed(lambda d:d['owner_followup_20261030']['mboot'].update(owner_untouched_contract='PASS'))
 def test_wrong_document_route(self):self.changed(lambda d:d['owner_followup_20261030']['delivery_route'].update(branch='main'))
if __name__=='__main__':unittest.main()
