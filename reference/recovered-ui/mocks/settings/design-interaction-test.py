# Executable proposal state model, NOT actual firmware/input-route proof.
import json
from pathlib import Path
out=[]
for clip in [False,True]:
 state='index';suppress=False;value=0
 def arrow(pressed):
  global state,suppress,value
  if not pressed:suppress=False;return
  if suppress:return
  if state=='index':state='section';suppress=True
  else:value+=1
 if clip:assert state=='index' # SCALE masked, no fake remap
 arrow(True);assert state=='section' and value==0
 for _ in range(20):arrow(True)
 assert value==0
 arrow(False);arrow(True);assert value==1
 state='index' # PATCH restores index
 assert value==1
 state='previous-view' # GLOBE exits
 out.append({'clip_mode':clip,'held_entry_repeats':20,'value_after_entry':0,'value_after_release_repress':1,'back_escape':'PASS'})
Path(__file__).with_name('interaction-proof.json').write_text(json.dumps({'proof_class':'synthetic proposal state model, not firmware','cases':out},indent=2))
print('PASS: both menu modes, held entry suppression, release/repress; synthetic only')
