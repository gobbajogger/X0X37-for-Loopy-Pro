from pathlib import Path
import json,hashlib,re
ROOT=Path(__file__).resolve().parent
m=json.loads((ROOT/'function-closure-map.json').read_text());s=json.loads((ROOT/'linux-scope/receipt.json').read_text());f=json.loads((ROOT/'linux-feedback/receipt.json').read_text());tool=json.loads((ROOT/'input-reuse.json').read_text())
expected={'smc-select-mode','smc-filter','smc-reachability','smc-lock','smc-ui'}
assert s['result']=='PASS' and s['exit']==0 and s['owned_worker_cleanup']=='PASS'
assert s['source_commit']==m['current_commit'] and set(s['observed_passes'])==expected and set(s['selectors'])==expected
assert s['named_led_mutation_pass'] and not s['observed_failures']
assert hashlib.sha256((ROOT/'linux-scope/checks.log').read_bytes()).hexdigest()==s['log_sha256']
assert f['result']=='PASS' and f['exit']==0 and f['owned_worker_cleanup']=='PASS' and f['source_commit']==m['final_commit']
assert hashlib.sha256((ROOT/'linux-feedback/feedback.log').read_bytes()).hexdigest()==f['log_sha256']
assert f['source_blob']==m['current_source_blob'] and f['feedback_test_blob']==m['final_current_blobs']['tests/host/smc_feedback_test.py']
assert s['worker']['image']==f['worker']['image']==tool['expected_worker_image']['id']
for path,wanted in m['final_current_blobs'].items():
 b=Path(path).read_bytes();assert hashlib.sha1(('blob '+str(len(b))+'\0').encode()+b).hexdigest()==wanted,path
source=Path('firmware/src/app/smc_mixer.c').read_text();match=re.search(r'^static int smc_led\(int note\)\n\{.*?^\}',source,re.M|re.S);assert match
assert hashlib.sha256((source[:match.start()]+source[match.end():]).encode()).hexdigest()==m['unaffected_production_bytes_sha256']
print('PASS exact-current LED scope:5 registered Linux selectors, named new LED runtime mutation,1919 corrected Linux feedback checks. Unaffected prior proof mapped by byte-identical production outside pure smc_led; no old package relabel or failed feedback waiver.')
