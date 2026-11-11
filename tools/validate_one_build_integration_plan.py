#!/usr/bin/env python3
"""Validate the frozen integration-document contract, not firmware correctness."""
import argparse,json,pathlib,re,sys
EXPECTED_PRS={212,635,636,637,638,650,651,677,679,684,685,687,688,689,690,691,692,693,694,695,696,697,698,699,700,701,702,703,704,705,706,707,708,709,710,711,712,713}
MISSING={41,50,54,73,97,98,101,104,105,111,112,113,119,120,122}
SHA=re.compile(r'^[0-9a-f]{40}$')
def validate(d):
 errors=[]
 def check(x,msg):
  if not x:errors.append(msg)
 check(d.get('target_branch')=='wip/one-build','invalid target')
 for k in ('source_pin','coordination_pin'):check(bool(SHA.fullmatch(d.get(k,''))),f'invalid {k}')
 ps=d.get('open_prs',[]);bs=d.get('branches',[])
 check({p.get('pr') for p in ps}==EXPECTED_PRS and len(ps)==len(EXPECTED_PRS),'missing/duplicate/unexpected PR row')
 check(len(bs)==816 and len({b.get('branch') for b in bs})==816,'missing/duplicate branch row')
 # Independent frozen branch-name digest prevents count-preserving row substitution.
 import hashlib
 check(hashlib.sha256('\n'.join(sorted(b.get('branch','') for b in bs)).encode()).hexdigest()==d.get('branch_names_sha256'),'branch identity closure mismatch')
 check({r.get('id') for r in d.get('missing_706',[])}==MISSING and len(d.get('missing_706',[]))==15,'missing/duplicate MISSING706 row')
 check({r.get('id') for r in d.get('additional_request_facets',[])}=={'SMC-'+str(i).zfill(2) for i in range(1,35)}|{'NS'+str(i).zfill(3) for i in list(range(1,23))+[26]},'additional request facets missing')
 for p in ps:
  for k in ('head','base'):check(bool(SHA.fullmatch(p.get(k,''))),f'PR{p.get("pr")} invalid {k}')
  check(p.get('classification') in {'held','include','exclude','superseded'},'invalid PR disposition')
  check(bool(p.get('owner')) and bool(p.get('acceptance')),'missing owner/gate')
 for b in bs:
  check(bool(SHA.fullmatch(b.get('head_sha',''))),'invalid branch pin')
  check(b.get('classification') in {'held','include','exclude','superseded'},'invalid branch disposition')
  if b.get('pr') is None and b.get('classification')=='include':check(False,'unverified unclaimed branch included')
 seen=set()
 for s in d.get('stages',[]):
  check(s.get('target')=='wip/one-build','invalid stage target')
  check(set(s.get('after',[]))<=seen,'invalid dependency order')
  check(s.get('id') not in seen,'duplicate stage');seen.add(s.get('id'))
 check(seen=={'S'+str(i).zfill(2) for i in range(11)},'missing stage')
 boot=d.get('bootloader',{})
 check(boot.get('status')=='PLAN_ONLY_HOLD' and 'USRTRIM' in boot.get('reason',''),'MBOOT hold lost')
 check(boot.get('hardware')=='NOT_RUN','unsupported hardware claim')
 follow=d.get('owner_followup_20261030',{})
 check(follow.get('must_fix',{}).get('id')=='SMC-OWN-PAGE' and follow.get('must_fix',{}).get('status')=='MUST_FIX_NO_COMPLETE_IMPLEMENTATION_FOUND','SMC own-page MUST_FIX lost')
 check(follow.get('mboot',{}).get('status')=='PLAN_ONLY_HOLD' and follow.get('mboot',{}).get('owner_untouched_contract')=='FAIL_FOR_PINNED_SMK_VARIANT','no-USRTRIM hold lost')
 route=follow.get('delivery_route',{})
 check(route.get('pr')==716 and route.get('branch')=='wip/owner-requested-features','invalid existing documentation delivery route')
 return errors
def main():
 ap=argparse.ArgumentParser();ap.add_argument('inventory',nargs='?',default=str(pathlib.Path(__file__).resolve().parents[1]/'docs/one-build-integration-inventory.json'));a=ap.parse_args()
 try: errors=validate(json.loads(pathlib.Path(a.inventory).read_text()))
 except (ValueError,OSError,TypeError,KeyError) as e:print('FAIL:',e);return 1
 print('\n'.join(errors) if errors else 'PASS: 38 PRs, 816 branches, 15 missing requirements, 57 detailed facets, sole-target DAG and MBOOT hold');return bool(errors)
if __name__=='__main__':sys.exit(main())
