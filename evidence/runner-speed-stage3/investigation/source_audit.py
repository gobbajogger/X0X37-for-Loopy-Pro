#!/usr/bin/env python3
"""Review immutable source equivalence for the runner-speed stage3 candidate.

This performs source assertions only; it does not execute the runner or probes.
"""
from pathlib import Path
import hashlib,json,re,subprocess
ROOT=Path(__file__).resolve().parents[3]
BASE='07fa87dc950908d4d720bde9c06c2478cc212b19'
CAND='fe7f0401e79cd5dd4a76307811d2ee0d12446263'
def show(rev,path): return subprocess.check_output(['git','show',f'{rev}:{path}'],cwd=ROOT)
def text(rev,path): return show(rev,path).decode()
def digest(data): return hashlib.sha256(data).hexdigest()
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()==CAND
r='tests/run_tests.sh'; a=text(BASE,r);b=text(CAND,r)
checks=[]
def same(label,x,y):
    assert x==y,label
    checks.append({'check':label,'result':'pass','sha256':digest(x.encode() if isinstance(x,str) else x)})
def segment(s,start,end): return s[s.index(start):s.index(end,s.index(start))]
for path in ['tests/host/smk_clip_knob_test.c','tests/host/smk_clip_chord_display_test.c','tests/host/smk_clip_knob_fake_usb.h']:
    same('Unchanged '+path,show(BASE,path),show(CAND,path))
same('Generation dependencies and lock helpers',segment(a,'needs_gen() {','run() {'),segment(b,'needs_gen() {','run() {'))
same('Slot, generation-lock and background execution block',segment(a,'    gw=0;','note() {'),segment(b,'    gw=0;','note() {'))
same('Scheduler tail',a[a.rindex('if [ "$JOBS" -le 1 ]; then\n    tests_body'):],b[b.rindex('if [ "$JOBS" -le 1 ]; then\n    tests_body'):])
same('Installer lock execution',segment(a,'lk() {','needs_gen() {'),segment(b,'lk() {','needs_gen() {'))
# Match the literal-registration parser used by tools/quickcheck.sh.
def registrations(s):
    out={}
    for line in s.replace('\\\n',' ').splitlines():
        if line.lstrip().startswith('#'): continue
        for m in re.finditer(r'(?:^|[\s;&|])run "?([A-Za-z0-9_.-]+)"?\s+(.*)',line): out.setdefault(m.group(1),m.group(2))
    return out
old,new=registrations(a),registrations(b)
removed=sorted(set(old)-set(new)); assert removed==['emu','emu-slots'],removed
assert set(new)-set(old)==set()
for name in new: assert old[name]==new[name],name
checks.append({'check':'Literal Quickcheck registrations and command strings','result':'pass','retained_count':len(new),'removed_only':removed})
for path in ['tests/run_tests.sh','tests/host/smk_clip_knob_test.sh','tests/host/smk_clip_chord_display_test.sh','tests/host/test_runner_admission.py']:
    assert (ROOT/path).read_bytes()==show(CAND,path),path
checks.append({'check':'Working source matches exact candidate','result':'pass'})
result={'base':BASE,'candidate':CAND,'checks':checks,'source_sha256':{p:digest(show(CAND,p)) for p in [r,'tests/host/smk_clip_knob_test.sh','tests/host/smk_clip_chord_display_test.sh','tests/host/test_runner_admission.py']}}
print(json.dumps(result,indent=2))
