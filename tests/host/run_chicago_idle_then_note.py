#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""PCM/state differential against the oscillator checkpoint; no private snapshots.

Builds each original DSP unit once per mode; replaces only modified public .c
units with the baseline version for the reference executable. Optional BASE_REF
pins another pre-optimization commit. Function entry counts are a deterministic
work proxy, NOT instructions, target cycles, or a CPU percentage.
"""
import hashlib
import importlib.util
import json
import os
import shlex
from pathlib import Path
import subprocess
import sys
ROOT = Path(__file__).resolve().parents[2]
os.chdir(ROOT)
sys.dont_write_bytecode = True
out = Path(sys.argv[1] if len(sys.argv)>1 else 'build/chicago-opt-idle-note').resolve()
out.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location('build_config', ROOT/'tools/build.py')
b = importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
units = [*b.ORIGINAL_SYNTH_UNITS,*b.ORIGINAL_SYNTH_BACKENDS,'firmware/src/dsp/x0x_param.c']
cc = os.environ.get('CC','cc')
base = os.environ.get('BASE_REF','9621d6cc7be6c333462771a30bf8e98d0701f9fd')
# Optional public source view for proving each incremental commit separately.
# Missing paths fall back to the workspace; no private inputs are copied.
source_root=Path(os.environ.get('CHICAGO_CANDIDATE_SOURCE',ROOT)).resolve()
candidate={u:str(source_root/u) if (source_root/u).exists() else u for u in units}
# Public implementation only. Never copy or stage private inputs.
changed = set(subprocess.check_output(['git','diff',base,'--name-only'],text=True).splitlines())
changed &= set(units)
flags = ['-std=c99','-Os','-DX0X_HOST','-DX0X_ORIGINAL_SYNTH=1','-DX0X_DEV_WOBBLE=0',
 '-DX0X_WOBBLE=0','-DBOARD_SMK37=1','-DSMK_MODEL=1',*b.ORIGINAL_SYNTH_FLAGS,
 '-Ifirmware/src','-Ifirmware/src/dsp','-Ifirmware/src/dsp/fx','-Ibuild/gen',
 '-Iexperiments/original_synth','-Iexperiments/wobble']
if source_root!=ROOT:flags[1:1]=['-I'+str(source_root/'experiments/original_synth'),'-I'+str(source_root/'firmware/src/dsp')]
subprocess.run([sys.executable,'tools/check_original_inputs.py','--scope','active'],check=True)
# Record the actual input closure and reject edits during the comparison.
deps=subprocess.check_output([cc,*flags,'-MM','tests/host/chicago_idle_then_note_test.c',*candidate.values()],text=True)
paths=set(candidate.values())|{'tests/host/chicago_idle_then_note_test.c',str(Path(__file__).relative_to(ROOT)),'tools/build.py'}
for line in deps.replace('\\\n',' ').splitlines():
 if ':' in line:paths.update(shlex.split(line.split(':',1)[1]))
def closure():return {p:hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in sorted(paths)}
before=closure()
receipt={'base_ref':subprocess.check_output(['git','rev-parse',base],text=True).strip(),
 'reference_units':sorted(changed),'compiler':cc,'flags':flags,
 'candidate_sources':candidate,
 'compiler_version':subprocess.check_output([cc,'--version'],text=True).splitlines()[0],
 'modes':{}}
def compare_files(candidate,reference):
 digest=hashlib.sha256();pos=0
 with candidate.open('rb') as cf,reference.open('rb') as rf:
  while True:
   a=cf.read(1024*1024);r=rf.read(1024*1024)
   if a!=r:
    off=next((i for i,(x,y) in enumerate(zip(a,r)) if x!=y),min(len(a),len(r)))
    raise SystemExit(f'FAIL: {candidate} differs from {reference} at byte {pos+off}')
   if not a:break
   digest.update(a);pos+=len(a)
 return {'status':'PASS','bytes':pos,'sha256':digest.hexdigest()}
for mode in ['normal','sanitized']:
 d=out/mode;d.mkdir(exist_ok=True)
 extra=['-finstrument-functions'] if mode=='normal' else ['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all']
 objects=[];references=[]
 with (d/'compile.log').open('w') as log:
  def compile(src,obj):subprocess.run([cc,*flags,*extra,'-c',str(src),'-o',str(obj)],check=True,stdout=log,stderr=log)
  for u in units:
   obj=d/(u.replace('/','_')+'.o');compile(candidate[u],obj);objects.append(obj)
   if u in changed:
    ref=d/('ref_'+Path(u).name)
    ref.write_bytes(subprocess.check_output(['git','show',base+':'+u]))
    ro=d/('ref_'+obj.name);compile(ref,ro);references.append(ro)
   else:references.append(obj)
  for side,objs in [('candidate',objects),('reference',references)]:
   exe=d/side
   subprocess.run([cc,*flags,*extra,'tests/host/chicago_idle_then_note_test.c',*map(str,objs),'-lm','-o',str(exe)],check=True,stdout=log,stderr=log)
   with (d/(side+'.log')).open('w') as output:
    subprocess.run([str(exe),str(d/(side+'.bin'))],check=True,stdout=output,
     env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
 receipt['modes'][mode]=compare_files(d/'candidate.bin',d/'reference.bin')
 print(mode+': byte-exact PCM and DSP state PASS')
 print('candidate '+(d/'candidate.log').read_text(),end='')
 print('reference '+(d/'reference.log').read_text(),end='')
# Check compiler modes against one another too.
compare_files(out/'normal/candidate.bin',out/'sanitized/candidate.bin')
assert before==closure(),'Source changed during proof'
receipt['source_sha256']=before
(out/'RESULT.json').write_text(json.dumps(receipt,indent=2)+'\n')
# Optional disk cleanup only after all three byte comparisons and source closure
# checks have passed. Receipts retain exact sizes/hashes; failures keep bytes.
if os.environ.get('CLEAN_TRANSCRIPTS')=='1':
 for path in out.glob('*/*.bin'):path.unlink()
