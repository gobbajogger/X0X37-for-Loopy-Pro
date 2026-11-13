from PIL import Image
from pathlib import Path
import json,hashlib,numpy as np
O=Path(__file__).resolve().parent;source=O/'inputs/HOME-actual-b57ef24.png';im=Image.open(source).convert('RGB');assert im.size==(240,240)
regions=[('header',0,0,240,21)]
regions += [('pattern.bank'+str(i+1),x,21,w,32) for i,(x,w) in enumerate([(0,60),(60,60),(120,56),(176,64)])]
regions += [(name,0,y,240,h) for name,y,h in [('lane.909',53,16),('lane.808',69,16),('lane.BRK',85,16),('lane.CHI',101,16),('lane.MIA',117,20),('negative_space',137,20),('perform_label',157,14)]]
regions += [('control.'+name,i*60,171,60,56) for i,name in enumerate(['TEMPO','SWING','FILTER','PUMP'])]
regions += [('footer',0,227,240,13)]
coverage=np.zeros((240,240),int);parts=[]
for name,x,y,w,h in regions:
 coverage[y:y+h,x:x+w]+=1;runs=[]
 for yy in range(y,y+h):
  xx=x
  while xx<x+w:
   col=im.getpixel((xx,yy));end=xx+1
   while end<x+w and im.getpixel((end,yy))==col:end+=1
   runs.append([xx-x,yy-y,end-xx,list(col)]);xx=end
 parts.append({'name':name,'bounds':[x,y,w,h],'runs':runs})
assert np.all(coverage==1)
(O/'HOME-component-pixels.json').write_text(json.dumps({'source':source.name,'sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'source_commit':'b57ef24f8bffdb850e6c57145d96115f72e0e953','source_tree':'adc73cd56fe25dfc416a70216da9537c1a17695d','fixture_state':'125BPM stopped P1 clean default USER voices; no hand edits','components':parts,'coverage':'Each native pixel belongs to exactly one semantic component'},separators=(',',':')))
print(len(parts),sum(len(p['runs']) for p in parts))
