#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Inspect actual compiled DEV objects; repeat unchanged link with map diagnostics.
Run inside Linux with JIELI_TOOLCHAIN after tools/build.py for both models.
No altered memory regions, successful-link substitution or package qualification.
"""
import hashlib,json,os,pathlib,re,subprocess
root=pathlib.Path(__file__).resolve().parents[2];os.chdir(root)
out=root/'build/wobble-native';out.mkdir(parents=True,exist_ok=True)
tc=pathlib.Path(os.environ['JIELI_TOOLCHAIN']);result={}
units=['dsp/drum909.c','dsp/drum808.c','dsp/x0x_param.c','dsp/bass303.c','dsp/breaks.c','dsp/fxbus.c','dsp/master.c','seq/sequencer.c','seq/tb3po.c','seq/pattern.c','seq/motion.c','app/engine.c']
for model,directory in [('elite','smk37-elite'),('pro','smk37')]:
 b=root/'build'/directory
 objects=[b/n for n in ['crt0.o','fm1_vec.o','fm1_isr.o','fm1_cpu1.o','x0x.o']]
 if (b/'o2/wobble.o').exists():objects.append(b/'o2/wobble.o')
 objects += [b/'o2'/(u.replace('/','_')[:-2]+'.o') for u in units]
 record={'objects':{}}
 for obj in objects:
  hdr=subprocess.check_output([str(tc/'common/bin/objdump'),'-h',str(obj)],text=True)
  totals={k:0 for k in ['text','rodata','data','bss','pool','ram_text','c1_text']}
  for line in hdr.splitlines():
   t=line.split()
   if len(t)>2 and t[0].isdigit() and t[1].startswith('.'):
    kind=t[1].split('.')[1]
    if kind in totals:totals[kind]+=int(t[2],16)
  record['objects'][str(obj.relative_to(b))]={'sections':totals,'sha256':hashlib.sha256(obj.read_bytes()).hexdigest()}
 ld=b/'app.ld' if (b/'app.ld').exists() else root/'firmware/app.ld'
 args=[str(tc/'pi32v2/bin/ld'),'--gc-sections','-e','_start','-T',str(ld),*[str(o) for o in objects],'-Map',str(out/(model+'.map')),'-o',str(out/(model+'-diagnostic.elf'))]
 process=subprocess.run(args,capture_output=True,text=True)
 (out/(model+'-map-link.log')).write_text(process.stdout+process.stderr)
 record['link_exit']=process.returncode;record['diagnostic']=process.stdout+process.stderr
 record['linker_sha256']=hashlib.sha256(ld.read_bytes()).hexdigest()
 result[model]=record
(out/'object-measurement.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({m:{'link_exit':r['link_exit'],'diagnostic':r['diagnostic']} for m,r in result.items()},indent=2))
