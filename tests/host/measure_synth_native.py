#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile affected objects only. No link, firmware package, device or publication."""
import hashlib,json,os,pathlib,subprocess
root=pathlib.Path(__file__).resolve().parents[2]
os.chdir(root)
out=root/'build/synth_engine/native';out.mkdir(parents=True,exist_ok=True)
base=os.environ.get('SYNTH_REVIEW_BASE','08d3164a265123cb4c84e5104642fd93257d63d7')
for rev,data in [('baseline',subprocess.check_output(['git','show',base+':firmware/src/app/engine.c'])),('candidate',(root/'firmware/src/app/engine.c').read_bytes())]:
 s=data.decode().replace('#include "../','#include "').replace('#include "engine.h"','#include "app/engine.h"').replace('#include "plat.h"','#include "app/plat.h"')
 (out/f'engine_{rev}.c').write_text(s)
compiler='/opt/jieli/pi32v2/bin/clang';dump='/opt/jieli/common/bin/objdump'
flags=['-target','pi32v2','-O2','-ffunction-sections','-fno-builtin','-Wall','-Wno-unused-function','-mcpu=r3','-mfprev1','-ffp-contract=off','-DBOARD_SMK37=1','-DX0X_DEV_PADCAL=1','-Ifirmware/hal','-Ifirmware/src','-Ifirmware/src/app','-Ibuild/gen']
mount=['docker','run','--rm','--platform','linux/amd64','-v',f'{root}:/work','-v',f'{pathlib.Path(os.environ["JIELI_TOOLCHAIN"]).resolve()}:/opt/jieli:ro','-w','/work','debian:bookworm-slim']
result={'status':'NATIVE_AFFECTED_OBJECT_ONLY_NOT_LINKED','baseline':base,'compiler':subprocess.check_output(mount+[compiler,'--version'],text=True),'flags':flags,'models':{}}
for model in ['pro','elite']:
 result['models'][model]={}
 for rev in ['baseline','candidate']:
  path=f'build/synth_engine/native/{model}-{rev}.o'
  extra=['-DSMK_MODEL="SMK-37 Elite"'] if model=='elite' else []
  subprocess.run(mount+[compiler]+flags+extra+['-c',f'build/synth_engine/native/engine_{rev}.c','-o',path],check=True)
  sections=subprocess.check_output(mount+[dump,'-h',path],text=True)
  symbols=subprocess.check_output(mount+[dump,'-t',path],text=True)
  (out/f'{model}-{rev}.sections').write_text(sections)
  (out/f'{model}-{rev}.symbols').write_text(symbols)
  totals={'text':0,'rodata':0,'bss':0,'data':0,'pool':0}
  for line in sections.splitlines():
   t=line.split()
   if len(t)>=4 and t[0].isdigit() and t[1].startswith('.'):
    kind=t[1].split('.')[1]
    if kind in totals: totals[kind]+=int(t[2],16)
  result['models'][model][rev]={'sections':totals,'object_sha256':hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest(),'common_symbols':[line for line in symbols.splitlines() if '*COM*' in line or 'bass_slot' in line]}
 for kind in totals:
  result['models'][model].setdefault('delta',{})[kind]=result['models'][model]['candidate']['sections'][kind]-result['models'][model]['baseline']['sections'][kind]
(out/'measurement.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
