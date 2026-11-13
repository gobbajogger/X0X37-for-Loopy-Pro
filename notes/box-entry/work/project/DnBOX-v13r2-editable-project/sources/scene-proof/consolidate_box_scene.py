import bpy,math,json
from pathlib import Path
from mathutils import Vector
O=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(O/'DnBOX-v13-scene-with-wordmark.blend'))
s=bpy.context.scene
CFG={'fps':70,'frames':192,'flight_yaw_degrees':30,'flight_pitch_degrees':6,'flight_depth':.38,'opening_frames':[1,18],'home_layout_bound':False,'audio_master_bound':None}
if (O/'scene-parameters.json').exists():CFG.update(json.loads((O/'scene-parameters.json').read_text()))
(O/'scene-parameters.json').write_text(json.dumps(CFG,indent=2))
# Small named collection system, not a second asset framework.
colls={}
for n in ['00_STAGE_CAMERA_LIGHT','01_BOX','02_SHARED_UI','03_WAVE','04_SLICE','05_FX','06_HOME_DESTINATION','07_DnBOX']:
 c=bpy.data.collections.new(n);s.collection.children.link(c);colls[n]=c
for ob in list(bpy.data.objects):
 if ob.type in ['CAMERA','LIGHT'] or ob.name=='constant dark foundation':key='00_STAGE_CAMERA_LIGHT'
 elif ob.name.startswith('brand.'):key='07_DnBOX'
 else:
  p=ob
  while p.parent:p=p.parent
  if p.name.startswith('page.wave'):key='03_WAVE'
  elif p.name in ['ui.breaks.reverse']:key='04_SLICE'
  elif p.name in ['ui.master.level']:key='05_FX'
  else:key='02_SHARED_UI'
 for c in list(ob.users_collection):c.objects.unlink(ob)
 colls[key].objects.link(ob)
# Persisted page states, no transient Python callback dependency.
old=bpy.data.objects.get('page.label');base=old.data.copy();par=old.parent;loc=old.location.copy();bpy.data.objects.remove(old,do_unlink=True)
for label,start,end in [('WAVE',1,36),('SLICE',37,72),('FX',73,120),('HOME',121,192)]:
 data=base.copy();data.body=label;ob=bpy.data.objects.new('state.page.'+label,data);colls['02_SHARED_UI'].objects.link(ob);ob.parent=par;ob.location=loc
 for f,v in [(1,not(start==1)),(start,False),(end+1,True)]:ob.hide_render=v;ob.hide_viewport=v;ob.keyframe_insert('hide_render',frame=f);ob.keyframe_insert('hide_viewport',frame=f)
bpy.app.handlers.frame_change_pre.clear()
def E(v):v=max(0,min(1,v));return v*v*v*(v*(v*6-15)+10)
def route(f,a,b,target,side,exit=False):
 u=E((f-a)/(b-a));x,y,z=target
 if exit:return (x+(side*3.2-x)*u,y,z+CFG['flight_depth']*u),(math.radians(CFG['flight_pitch_degrees'])*u,side*math.radians(CFG['flight_yaw_degrees'])*u,0)
 return(side*3.2+(x-side*3.2)*u,y,z+CFG['flight_depth']*(1-u)),(math.radians(CFG['flight_pitch_degrees'])*(1-u),-side*math.radians(CFG['flight_yaw_degrees'])*(1-u),0)
plans={
'page.wave.info':{'target':(0,-.59,0),'enter':(13,32,1),'exit':(37,55,-1)},
'ui.breaks.slices':{'target':(0,-.59,.01),'enter':(37,60,1),'exit':(78,99,-1)},
'ui.breaks.stutter':{'target':(-.965,.22,.025),'enter':(38,64,-1),'exit':(80,106,1)},
'ui.breaks.reverse':{'target':(.965,.22,.025),'enter':(44,69,-1),'exit':(80,109,1)},
'ui.master.crush':{'target':(.965,.22,.025),'enter':(73,100,-1),'exit':(123,151,1)},
'ui.master.level':{'target':(0,-.59,.018),'enter':(78,108,1),'exit':(123,149,-1)},
'ui.navigation':{'target':(0,-1.075,.02),'enter':(138,159,1)},
'ui.scope':{'target':(0,.22,0),'enter':(10,28,-1),'exit':(121,149,1)}}
for name,p in plans.items():
 ob=bpy.data.objects[name];ob.animation_data_clear();ob['route_direction']='LEFT to RIGHT' if p['enter'][2]<0 else 'RIGHT to LEFT';ob['component_identity']=name
 for f in range(1,193):
  a,b,side=p['enter'];loc,rot=route(f,a,b,p['target'],side)
  if 'exit'in p and f>=p['exit'][0]:a,b,side=p['exit'];loc,rot=route(f,a,b,p['target'],side,True)
  ob.location=loc;ob.rotation_euler=rot;ob.keyframe_insert('location',frame=f);ob.keyframe_insert('rotation_euler',frame=f)
 for fc in ob.animation_data.action.fcurves:
  for k in fc.keyframe_points:k.interpolation='LINEAR'
# Reusable physical BOX, one lid opening. It supplies the stage instead of adding ornaments.
M=bpy.data.materials
body=M['graphite body'];side=M['machined side'];face=M['matte working surface']
def cube(name,dims,loc,mat,par=None):
 bpy.ops.mesh.primitive_cube_add(size=1);o=bpy.context.object;o.name=name;o.dimensions=dims;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.location=loc;o.data.materials.append(mat)
 for c in list(o.users_collection):c.objects.unlink(o)
 colls['01_BOX'].objects.link(o)
 b=o.modifiers.new('actual edge radius','BEVEL');b.width=.025;b.segments=3;o.modifiers.new('surface normals','WEIGHTED_NORMAL')
 if par:o.parent=par
 return o
lid=bpy.data.objects.new('box.lid.hinge',None);colls['01_BOX'].objects.link(lid);lid.location=(0,1.15,.75)
cube('box.lid.solid',(2.30,2.30,.10),(0,-1.15,0),body,lid)
cube('box.lid.front',(2.22,2.22,.02),(0,-1.15,.057),face,lid)
for f,a in [(1,0),(4,0),(22,112),(192,112)]:lid.rotation_euler=(math.radians(a),0,0);lid.keyframe_insert('rotation_euler',frame=f)
for f,y in [(1,1.15),(22,1.15),(48,3.0),(192,3.0)]:lid.location=(0,y,.75);lid.keyframe_insert('location',frame=f)
for nm,loc,sz in [('left',(-1.14,0,.25),(.09,2.3,.94)),('right',(1.14,0,.25),(.09,2.3,.94)),('bottom',(0,-1.14,.25),(2.3,.09,.94)),('top',(0,1.14,.25),(2.3,.09,.94))]:
 wall=cube('box.wall.'+nm,sz,loc,side)
 for f,v in [(1,1),(10,1),(28,.15),(192,.15)]:wall.scale.z=v;wall.location.z=.25 if f<=10 else -.20;wall.keyframe_insert('scale',frame=f);wall.keyframe_insert('location',frame=f)
# Reuse the actual custom glyph mesh design on the lid, not a raster decal.
s.frame_set(157);deps=bpy.context.evaluated_depsgraph_get()
for original in [o for o in bpy.data.objects if o.name.startswith('brand.') and o.type=='MESH' and '.shade'not in o.name]:
 mesh=bpy.data.meshes.new_from_object(original.evaluated_get(deps));ob=bpy.data.objects.new('box.mark.'+original.name,mesh);colls['01_BOX'].objects.link(ob);ob.parent=lid;ob.location=(0,-1.43,.09);ob.scale=(.50,.50,.50)
 # Front glyph is shared artwork, separate physical instance on lid.
 ob['artwork_source']='custom SLAB Dn / CUT BOX; same outline as final mark'
# Final wordmark gets visible solid depth and a restrained oblique side light reading.
logo=bpy.data.objects['brand.DnBOX'];logo.rotation_euler=(math.radians(5),math.radians(-7),0);logo.location=(.088,.063,0);logo['depth_design']='solid extruded letter halves; no screen-space fake shadow'
for ob in [o for o in bpy.data.objects if o.name.startswith('brand.') and o.type=='MESH' and '.shade'not in o.name]:
 for v in ob.data.vertices:v.co.z*=2.2
 if ob.data.shape_keys:
  for k in ob.data.shape_keys.key_blocks:
   for v in k.data:v.co.z*=2.2
 ob.data.materials.append(side)
 for poly in ob.data.polygons:
  if abs(poly.normal.z)<.75:poly.material_index=len(ob.data.materials)-1
# New opening supersedes the old contour-to-letter device. Final type arrives as solid type.
for ob in [o for o in bpy.data.objects if o.name.startswith('brand.') and o.type=='MESH' and '.shade'not in o.name]:
 letter=ob.name.split('.')[1];half=-1 if ob.name.endswith('lower') else 1
 if ob.data.shape_keys:
  ob.data.shape_keys.animation_data_clear()
  for k in ob.data.shape_keys.key_blocks[1:]:k.value=0
 ob.animation_data_clear()
 a,b=(132,151) if letter in ['D','n','B'] else (142,161)
 side=-1 if letter in ['D','n','B'] else 1
 for f in range(1,193):
  u=E((f-a)/(b-a));x=side*3.4*(1-u);y=.05+.16*(1-u);z=.72+.12*(1-u)
  if f>=174:
   q=E((f-174)/13);x=half*3.15*q;y=.05+half*.18*q;z=.72+half*.25*q
  ob.location=(x,y,z);ob.rotation_euler=(0,side*.18*(1-u),0);ob.keyframe_insert('location',frame=f);ob.keyframe_insert('rotation_euler',frame=f)
 for fc in ob.animation_data.action.fcurves:
  for k in fc.keyframe_points:k.interpolation='LINEAR'
# Box begins visibly bounded in the frame, with legible side depth; UI is physically packed inside it.
boxroot=bpy.data.objects.new('box.assembly',None);colls['01_BOX'].objects.link(boxroot)
for ob in list(colls['01_BOX'].objects):
 if ob!=boxroot and ob.parent is None:ob.parent=boxroot
for f,sc,rx,ry in [(1,.76,9,-10),(5,.76,9,-10),(28,1,0,0),(192,1,0,0)]:
 boxroot.scale=(sc,sc,sc);boxroot.rotation_euler=(math.radians(rx),math.radians(ry),0);boxroot.keyframe_insert('scale',frame=f);boxroot.keyframe_insert('rotation_euler',frame=f)
pack=bpy.data.objects.new('ui.opening.pack',None);colls['02_SHARED_UI'].objects.link(pack)
for cn in ['02_SHARED_UI','03_WAVE','04_SLICE','05_FX']:
 for ob in list(colls[cn].objects):
  if ob!=pack and ob.parent is None:ob.parent=pack
for f,sc in [(1,.72),(8,.72),(28,1),(192,1)]:pack.scale=(sc,sc,sc);pack.keyframe_insert('scale',frame=f)
# HOME is intentionally an explicit unbound assembly, not a fake completed screenshot.
marker=bpy.data.objects.new('HOME_LAYOUT_PENDING_ACTUAL_REFERENCE',None);colls['06_HOME_DESTINATION'].objects.link(marker);marker['status']='Await actual source-backed HOME component layout; do not render as final'
for c in colls.values():c['purpose']='Editable component collection; no full-screen raster bake'
s['scene_status']='Geometry/layout revision in progress; HOME and second-bar PCM unbound';s['parameters_file']='scene-parameters.json';s.cycles.samples=64;s.cycles.use_denoising=False;s.render.resolution_x=s.render.resolution_y=480
s.frame_set(1);bpy.ops.wm.save_as_mainfile(filepath=str(O/'DnBOX-v13-editable-box-to-home-WIP.blend'))
(O/'box-route-manifest.json').write_text(json.dumps({'parameters':CFG,'routes':plans,'collections':list(colls),'opening':'One physical hinged lid, same custom wordmark geometry design','missing':['actual HOME layout binding','selected second-bar processed PCM'],'old_preview_preserved':True},indent=2))
for f in [14]:s.frame_set(f);s.render.filepath=str(O/('box-opening-r2-proof-%03d.png'%f));bpy.ops.render.render(write_still=True);print('BOX_STILL',f,flush=True)
