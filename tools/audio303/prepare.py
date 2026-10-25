# SPDX-License-Identifier: GPL-3.0-only
"""Generate small original diagnostics. Values are hypotheses, not equivalent curves."""
import json, pathlib, subprocess
root=pathlib.Path(__file__).resolve().parents[3]
base=[0,0,.6,0,.5,0,0,0,.3,1,0,1,.5,0,0,0,0,0,0,0,.5,0]
# Production array: cutoff reso env wave fenv attack decay sustain release slide
prod=[64,0,0,0,1,0,64,127,0,21]
trials=[]
def add(name,ch=None,p=None,note=48,vel=100,seq=0,accent=0):
 c=base.copy();d=prod.copy()
 for k,v in (ch or {}).items():c[k]=v
 for k,v in (p or {}).items():d[k]=v
 events=[[0,144,note,vel],[22050,128,note,0]]
 frames=44100
 if seq==1:events += [[44100,144,note,vel],[66150,128,note,0]];frames=88200
 if seq>=2:events=[[0,144,note,vel],[11025,144,note+7,vel],[16538,128,note,0],[33075,128,note+7,0]]
 for rep in range(2):trials.append(dict(id=f'{name}-r{rep}',name=name,chicago=c,production=d,note=note,velocity=vel,sequence=seq,accent=accent,frames=frames,events=events))
for n in [48,60]:
 for cut in [.2,.4,.6,.8,1.]:add(f'cut{cut:.1f}-n{n}',{2:cut},note=n)
for wave in [.5,1.]:add(f'wave{wave}',{0:wave},{3:1})
for res in [.5,1.]:add(f'reso{res}',{3:res},{1:round(127*res)})
for vel in [40,80,127]:add(f'vel{vel}',vel=vel)
add('env_positive',{4:.75,9:0},{2:64,4:0})
add('env_amp',{11:0,9:.7,10:.15},{4:0})
add('attack',{7:.3,11:0,9:.7},{5:38,7:89})
add('repeat',seq=1)
add('overlap_glide0',seq=2)
add('overlap_glide_mid',{1:.5},{9:64},seq=2)
add('overlap_retrigger',seq=3)
add('accent_repeat',seq=1,accent=1)
(root/'evidence/manifest.json').write_text(json.dumps(trials,indent=2))
for t in trials:
 args=[t['frames'],t['note'],t['velocity'],*t['production'],t['sequence'],t['accent'],0,0]
 subprocess.run([str(root/'production303'),str(root/'audio'/f"{t['id']}.dsp.f32"),*map(str,args)],check=True)
# Dense source-only candidate library: held-out note is independently rendered.
for n in [48,60]:
 for wave in [0,1]:
  for cut in list(range(0,128,8))+[127]:
   p=prod.copy();p[0]=cut;p[3]=wave
   args=[44100,n,100,*p,0,0,0,0]
   subprocess.run([str(root/'production303'),str(root/'audio'/f'grid-n{n}-w{wave}-c{cut}.dsp.f32'),*map(str,args)],check=True)
print(f'{len(trials)} fresh reference trials; 68 source grid renders')
