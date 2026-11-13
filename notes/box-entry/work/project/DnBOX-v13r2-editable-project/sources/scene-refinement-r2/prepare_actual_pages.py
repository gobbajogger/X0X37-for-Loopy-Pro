from PIL import Image
from pathlib import Path
import numpy as np,json,hashlib
O=Path(__file__).resolve().parent;host=O/'latest-source-host'
regions={
 'WAVE':[('header',0,0,240,21),('scope',0,21,204,72),('gain_reduction',204,21,36,72),('compression',0,93,120,64),('pump_status',120,93,120,64)]+[('control.'+n,i*60,157,60,70) for i,n in enumerate(['DRIVE','COMP','PUMP','FILTER'])]+[('footer',0,227,240,13)],
 'BREAK':[('header',0,0,240,21)]+[('slice.'+str(i+1),x,21,w,68) for i,(x,w) in enumerate([(0,33),(33,29),(62,29),(91,29),(120,29),(149,29),(178,29),(207,33)])]+[('steps',0,89,240,15),('slot_names',0,104,240,24),('negative_space',0,128,240,29)]+[('control.'+n,i*60,157,60,70) for i,n in enumerate(['LOOP_A','LOOP_B','B_CHANCE','PITCH'])]+[('footer',0,227,240,13)],
 'FX':[('header',0,0,240,21),('column_names',0,21,240,12)]+[('send.'+n,0,33+i*11,240,11) for i,n in enumerate(['909','808','BRK','CHI','MIA'])]+[('effect_status',0,88,240,45),('negative_space',0,133,240,24)]+[('control.'+n,i*60,157,60,70) for i,n in enumerate(['DECAY','TONE','EMPTY_3','EMPTY_4'])]+[('footer',0,227,240,13)]}
names={'WAVE':'MIX-WAVE-current-state.png','BREAK':'BREAKS-current-state.png','FX':'FX-current-state.png'}
for page,parts in regions.items():
 source=host/names[page];im=Image.open(source).convert('RGB');coverage=np.zeros((240,240),int);data=[]
 for name,x,y,w,h in parts:
  coverage[y:y+h,x:x+w]+=1;runs=[]
  for yy in range(y,y+h):
   xx=x
   while xx<x+w:
    color=im.getpixel((xx,yy));end=xx+1
    while end<x+w and im.getpixel((end,yy))==color:end+=1
    # Actual dynamic scope is later driven by PCM geometry, not frozen screenshot pixels.
    if not(page=='WAVE' and name=='scope'):runs.append([xx-x,yy-y,end-xx,list(color)])
    xx=end
  if page=='WAVE' and name=='scope':runs=[[0,yy,w,[0,0,0]] for yy in range(h)]
  data.append({'name':name,'bounds':[x,y,w,h],'runs':runs})
 assert np.all(coverage==1)
 (O/f'{page}-component-pixels.json').write_text(json.dumps({'source':str(source.relative_to(O)),'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'source_commit':'b57ef24f8bffdb850e6c57145d96115f72e0e953','source_tree':'adc73cd56fe25dfc416a70216da9537c1a17695d','components':data,'state':'same stopped125BPM P1 fixture; explicit page/part navigation; WAVE scope from actual processed PCM, native128-sample/2 decimation'},separators=(',',':')))
 print(page,len(data),sum(len(p['runs']) for p in data))
