import bpy,math,json
from pathlib import Path
O=Path(__file__).resolve().parent;BASE=O.parent/'scene-proof'
bpy.ops.wm.open_mainfile(filepath=str(BASE/'DnBOX-v13-box-pages-HOME-editable.blend'))
s=bpy.context.scene
# Same scene graph, corrected hinge, slower opening, short identifiable rests.
def E(u):u=max(0,min(1,u));return u*u*u*(u*(u*6-15)+10)
def keys(ob,values):
 ob.animation_data_clear()
 for f,loc,rot,scale in values:
  if loc is not None:ob.location=loc;ob.keyframe_insert('location',frame=f)
  if rot is not None:ob.rotation_euler=rot;ob.keyframe_insert('rotation_euler',frame=f)
  if scale is not None:ob.scale=scale;ob.keyframe_insert('scale',frame=f)
 if ob.animation_data:
  for fc in ob.animation_data.action.fcurves:
   for k in fc.keyframe_points:k.interpolation='LINEAR'
# Upper hinge; negative X lifts the free lower edge out of the box then folds it above/behind.
lid=bpy.data.objects['box.lid.hinge']
keys(lid,[(f,(0,1.15+1.85*E((f-32)/16),.75),(math.radians(-108)*E((f-8)/28),0,0),None) for f in range(1,193)])
root=bpy.data.objects['box.assembly'];keys(root,[(f,None,(math.radians(9)*(1-E((f-8)/28)),math.radians(-10)*(1-E((f-8)/28)),0),(.76+.24*E((f-8)/28),)*3) for f in range(1,193)])
for ob in bpy.data.objects:
 if ob.name.startswith('box.wall.'):
  old=ob.location.copy();keys(ob,[(f,(old.x,old.y,.25-.45*E((f-18)/24)),None,(1,1,1-.85*E((f-18)/24))) for f in range(1,193)])
pack=bpy.data.objects['ui.opening.pack'];keys(pack,[(f,None,None,(.72+.28*E((f-18)/18),)*3) for f in range(1,193)])
plans={
 'ui.scope':{'target':(0,.22,0),'enter':(20,36,-1),'exit':(97,115,1)},
 'page.wave.info':{'target':(0,-.59,0),'enter':(25,36,1),'exit':(49,60,-1)},
 'ui.breaks.slices':{'target':(0,-.59,.01),'enter':(49,60,1),'exit':(73,84,-1)},
 'ui.breaks.stutter':{'target':(-.965,.22,.025),'enter':(49,60,-1),'exit':(73,84,1)},
 'ui.breaks.reverse':{'target':(.965,.22,.025),'enter':(51,62,-1),'exit':(75,86,1)},
 'ui.master.crush':{'target':(.965,.22,.025),'enter':(73,84,-1),'exit':(97,111,1)},
 'ui.master.level':{'target':(0,-.59,.018),'enter':(73,84,1),'exit':(97,111,-1)}}
for name,p in plans.items():
 ob=bpy.data.objects[name];target=p['target'];values=[]
 for f in range(1,193):
  a,b,side=p['enter'];u=E((f-a)/(b-a));tx,ty,tz=target
  loc=(side*2.65+(tx-side*2.65)*u,ty,tz+.24*(1-u));rot=(math.radians(4)*(1-u),-side*math.radians(24)*(1-u),0)
  a,b,side=p['exit']
  if f>=a:
   u=E((f-a)/(b-a));loc=(tx+(side*2.65-tx)*u,ty,tz+.24*u);rot=(math.radians(4)*u,side*math.radians(24)*u,0)
  values.append((f,loc,rot,None))
 keys(ob,values);ob['readable_rest_frames']=[p['enter'][1],p['exit'][0]-1]
# Page captions have the same semantic identities and timing; no camera cut.
for label,start,end in [('WAVE',1,48),('SLICE',49,72),('FX',73,96),('HOME',97,192)]:
 ob=bpy.data.objects['state.page.'+label];ob.animation_data_clear()
 for f,v in [(1,start!=1),(start,False),(end+1,True)]:ob.hide_render=v;ob.hide_viewport=v;ob.keyframe_insert('hide_render',frame=f);ob.keyframe_insert('hide_viewport',frame=f)
# Retiming only for the old HOME until the new exact component source is bound.
for ob in list(bpy.data.objects):
 if ob.name.startswith('home.') and ob.type=='EMPTY' and 'pixel_bounds' in ob:
  s.frame_set(192);tar=tuple(ob.location);a,b,w,h=ob['pixel_bounds'];side=-1 if b+h/2<120 else 1;ix=sum(1 for n in bpy.data.objects if n.name.startswith('home.') and n.type=='EMPTY' and n.name<ob.name)%6;start=97+ix;end=115+ix
  keys(ob,[(f,(side*2.65+(tar[0]-side*2.65)*E((f-start)/(end-start)),tar[1],tar[2]+.24*(1-E((f-start)/(end-start)))),(math.radians(4)*(1-E((f-start)/(end-start))),-side*math.radians(24)*(1-E((f-start)/(end-start))),0),None) for f in range(1,193)])
track=bpy.data.objects['ui.track'];s.frame_set(1);tar=tuple(track.location);keys(track,[(f,(tar[0]+2.8*E((f-97)/18),tar[1],tar[2]+.24*E((f-97)/18)),(0,.42*E((f-97)/18),0),None) for f in range(1,193)])
# Dimensional custom mark lands, holds, and tears as letters only; final UI gets a full beat.
for ob in list(bpy.data.objects):
 if ob.name.startswith('brand.') and ob.type=='MESH' and '.shade' not in ob.name:
  letter=ob.name.split('.')[1];side=-1 if letter in ['D','n','B'] else 1;half=-1 if ob.name.endswith('lower') else 1;a,b=(123,137) if side<0 else (129,143);v=[]
  for f in range(1,193):
   u=E((f-a)/(b-a));x=side*2.8*(1-u);y=.05+.10*(1-u);z=.72+.08*(1-u)
   if f>=154:q=E((f-154)/14);x=half*3.15*q;y=.05+half*.18*q;z=.72+half*.25*q
   v.append((f,(x,y,z),(0,side*.12*(1-u),0),None))
  keys(ob,v)
shade=bpy.data.materials['independent shade opacity'];mix=next(n for n in shade.node_tree.nodes if n.type=='MIX_SHADER');shade.node_tree.animation_data_clear()
for f,v in [(1,0),(120,0),(139,.45),(153,.45),(168,0),(192,0)]:mix.inputs[0].default_value=v;mix.inputs[0].keyframe_insert('default_value',frame=f)
s['revision_status']='Corrected opening and rests; current source HOME binding pending; silent geometry revision only'
s['timeline_summary']='8 beats: open1-36; WAVE37-48; SLICE49-72; FX73-96; HOME97-120; logo121-168; final169-192'
s.cycles.samples=32;s.cycles.use_denoising=False;s.render.resolution_x=s.render.resolution_y=240;s.render.use_motion_blur=True;s.render.motion_blur_shutter=.38
for vl in s.view_layers:vl.cycles.use_denoising=False
s.frame_set(1);bpy.ops.wm.save_as_mainfile(filepath=str(O/'DnBOX-v13r2-opening-rests-WIP.blend'))
(O/'timing-revision.json').write_text(json.dumps({'frames':192,'fps':70,'bpm':175,'camera':'unchanged fixed perspective','routes':plans,'opening':'negativeX upper hinge, free lower edge lifts then folds away above;8frame initial hold,28frame opening','final_hold':[169,192],'source_home_pending':True,'audio_master':None},indent=2))
for f in [12,24,36,66,90]:s.frame_set(f);s.render.filepath=str(O/f'route-proof-{f:03d}.png');bpy.ops.render.render(write_still=True)
