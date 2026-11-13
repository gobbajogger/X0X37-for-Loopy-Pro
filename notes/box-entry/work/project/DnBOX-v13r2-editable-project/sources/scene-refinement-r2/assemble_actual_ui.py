import bpy,math,json,hashlib
from pathlib import Path
from mathutils import Vector
O=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(O/'DnBOX-v13r2-opening-rests-WIP.blend'));s=bpy.context.scene
# Keep the existing BOX, exact custom letter meshes, stage, lights and fixed camera.
for cn in ['02_SHARED_UI','03_WAVE','04_SLICE','05_FX','06_HOME_DESTINATION']:
 for ob in list(bpy.data.collections[cn].objects):bpy.data.objects.remove(ob,do_unlink=True)
C={p:bpy.data.collections[n] for p,n in [('WAVE','03_WAVE'),('BREAK','04_SLICE'),('FX','05_FX'),('HOME','06_HOME_DESTINATION')]}
s.world.color=(.0005,.0007,.001);s.view_settings.view_transform='Standard';s.view_settings.look='None';s.view_settings.exposure=0;s.view_settings.gamma=1;s.render.dither_intensity=0
s.cycles.pixel_filter_type='BOX';s.render.filter_size=.5;s.render.image_settings.color_mode='RGB';s.render.image_settings.color_depth='8'
Z=5.4-2.4*80/36;s.camera.data.sensor_fit='HORIZONTAL'
def E(v):v=max(0,min(1,v));return v*v*v*(v*(v*6-15)+10)
def linear(v):v=v/255;return v/12.92 if v<=.04045 else ((v+.055)/1.055)**2.4

def emission(name,rgb):
 m=bpy.data.materials.new(name);m.use_nodes=True;n=m.node_tree.nodes;n.clear();a=n.new('ShaderNodeEmission');a.inputs['Color'].default_value=(*map(linear,rgb),1);b=n.new('ShaderNodeOutputMaterial');m.node_tree.links.new(a.outputs[0],b.inputs[0]);return m
face=bpy.data.materials.new('Actual UI source pixels / unlit display surface');face.use_nodes=True;n=face.node_tree.nodes;n.clear();out=n.new('ShaderNodeOutputMaterial');em=n.new('ShaderNodeEmission');at=n.new('ShaderNodeVertexColor');at.layer_name='source_rgb';face.node_tree.links.new(at.outputs['Color'],em.inputs['Color']);face.node_tree.links.new(em.outputs[0],out.inputs['Surface'])
black=emission('Actual UI zero background',[0,0,0]);bpy.data.objects['constant dark foundation'].data.materials.clear();bpy.data.objects['constant dark foundation'].data.materials.append(black)
side=bpy.data.materials['machined side'];manifest={};objects={}

def cube(name,w,h,p,coll):
 bpy.ops.mesh.primitive_cube_add(size=1);ob=bpy.context.object;ob.name=name;ob.dimensions=(w/100-.002,h/100-.002,.09);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);ob.parent=p;ob.location=(0,0,-.047)
 for c in list(ob.users_collection):c.objects.unlink(ob)
 coll.objects.link(ob);ob.data.materials.append(side);b=ob.modifiers.new('Real solid rounded side edge','BEVEL');b.width=.009;b.segments=3;ob.modifiers.new('Weighted face normals','WEIGHTED_NORMAL');return ob

def module(page,p,j):
 x,y,w,h=p['bounds'];name='actual.'+page+'.'+p['name'];coll=C[page];par=bpy.data.objects.new(name,None);coll.objects.link(par);par.location=((x+w/2)/100-1.2,1.2-(y+h/2)/100,Z);par['source_bounds']=p['bounds'];par['source_sha256']=j.get('sha256',j.get('source_sha256'));par['source_tree']=j['source_tree'];par['semantic_identity']=name
 cube(name+'.solid',w,h,par,coll)
 verts=[];faces=[];colors=[]
 for xx,yy,rw,rgb in p['runs']:
  a=(xx-w/2)/100;b=(h/2-yy)/100;i=len(verts)
  verts.extend([(a,b,0),(a+rw/100,b,0),(a+rw/100,b-.01,0),(a,b-.01,0)]);faces.append((i,i+1,i+2,i+3));colors.extend([(*map(linear,rgb),1)]*4)
 me=bpy.data.meshes.new(name+'.pixel_geometry');me.from_pydata(verts,[],faces);me.update();att=me.color_attributes.new(name='source_rgb',type='FLOAT_COLOR',domain='CORNER')
 for v,c in zip(att.data,colors):v.color=c
 ob=bpy.data.objects.new(name+'.face',me);coll.objects.link(ob);ob.parent=par;ob.data.materials.append(face)
 manifest[name]={'bounds':p['bounds'],'source_sha256':par['source_sha256'],'source_tree':j['source_tree'],'children':[par.name+'.solid',ob.name],'geometry_pixel_runs':len(p['runs']),'final_target':list(par.location),'page':page};objects[name]=par;return par
for page in ['WAVE','BREAK','FX','HOME']:
 j=json.loads((O/(page+'-component-pixels.json')).read_text())
 for p in j['components']:module(page,p,j)
# Motion matches eighth-note landmarks, but actual snare and kick anchor the long phrase.
for name,p in objects.items():
 page=manifest[name]['page'];x,y,w,h=manifest[name]['bounds'];target=tuple(p.location);upper=y+h/2<120;side=-1 if upper else 1
 if page=='WAVE':a,b=(13,25) if upper else (25,36);c,d=49,60
 elif page=='BREAK':a,b=49,60;c,d=73,84
 elif page=='FX':a,b=73,84;c,d=97,109
 else:
  if y<21:a,b=97,108
  elif y<53:a,b=97,108
  elif y<137:a,b=109,120
  else:a,b=109,169
  c,d=999,1000
 manifest[name]['route']={'in':[a,b],'out':[c,d] if page!='HOME' else None,'direction':'LEFT→RIGHT' if upper else 'RIGHT→LEFT','held':[b,c-1] if page!='HOME' else [b,192]}
 for f in range(1,193):
  u=E((f-a)/(b-a))
  if page=='HOME' and y>=137:
   # Snare holds the incoming lower assembly; the 72→144ms t³ phrase carries it inward.
   if f<122:u=.32*E((f-109)/13)
   elif f<133:u=.32
   elif f<162:u=.32+.56*((f-133)/29)**3
   else:u=.88+.12*E((f-162)/6)
   u=max(0,min(1,u))
  p.location=(side*2.7+(target[0]-side*2.7)*u,target[1],target[2]+.26*(1-u));p.rotation_euler=(math.radians(4)*(1-u),-side*math.radians(24)*(1-u),0)
  if page!='HOME' and f>=c:
   v=E((f-c)/(d-c));p.location=(target[0]+(-side*2.7-target[0])*v,target[1],target[2]+.26*v);p.rotation_euler=(math.radians(4)*v,-side*math.radians(24)*v,0)
  p.keyframe_insert('location',frame=f);p.keyframe_insert('rotation_euler',frame=f)
 for fc in p.animation_data.action.fcurves:
  for k in fc.keyframe_points:k.interpolation='LINEAR'
# Actual post-master128-sample scope: same native coordinates, fixed gain, source PCM.
scope=objects['actual.WAVE.scope'];rgb=(65, 206, 255);scope_material=emission('Native part colour / PCM scope',rgb);line_material=emission('Native scope midpoint',[32,32,41]);data=json.loads((O/'approved-scope-native128.json').read_text())
def curve(name,coords,r,material):
 c=bpy.data.curves.new(name,'CURVE');c.dimensions='3D';c.bevel_depth=r;c.bevel_resolution=1;sp=c.splines.new('POLY');sp.points.add(len(coords)-1)
 for p,co in zip(sp.points,coords):p.co=(*co,1)
 ob=bpy.data.objects.new(name,c);C['WAVE'].objects.link(ob);ob.parent=scope;ob.data.materials.append(material);return ob,sp
# Scope component starts at x0,y21,w204,h72; draw_master_wave native midpoint is y54.
def scope_xy(x,y):return ((x-102)/100,(57-y)/100,.0008)
curve('actual.WAVE.scope.midpoint',[scope_xy(4,54),scope_xy(200,54)],.004,line_material)
ob,sp=curve('actual.WAVE.scope.processed_PCM',[scope_xy(4+x,54) for x in range(196)],.004,scope_material)
for f,arr in enumerate(data['frames'],1):
 for x,pt in enumerate(sp.points):
  sy=34-int(arr[x*128//196]*30/32768);sy=max(2,min(69,sy));pt.co=(*scope_xy(4+x,20+sy),1);pt.keyframe_insert('co',frame=f)
for fc in ob.data.animation_data.action.fcurves:
 for k in fc.keyframe_points:k.interpolation='CONSTANT'
ob['audio_sha256']=data['audio_sha256'];ob['native_sampling']='128 points; output decimation2; <=10Hz UI latch; no random jitter or amplitude animation'
# Reaffirm opening against first-snare candidate; root/audio receipt supplies exact onset.
lid=bpy.data.objects['box.lid.hinge'];lid.animation_data_clear()
for f in range(1,193):
 lid.rotation_euler=(math.radians(-108)*E((f-6)/20),0,0);lid.location=(0,1.15+1.85*E((f-26)/22),.75);lid.keyframe_insert('location',frame=f);lid.keyframe_insert('rotation_euler',frame=f)
for fc in lid.animation_data.action.fcurves:
 for k in fc.keyframe_points:k.interpolation='LINEAR'
# Clear the box rims before the first full readable WAVE landing, not after it.
root=bpy.data.objects['box.assembly'];root.animation_data_clear()
for f in range(1,193):
 u=E((f-6)/20);root.scale=(.76+.24*u,)*3;root.rotation_euler=(math.radians(9)*(1-u),math.radians(-10)*(1-u),0);root.keyframe_insert('scale',frame=f);root.keyframe_insert('rotation_euler',frame=f)
for wall in [o for o in bpy.data.objects if o.name.startswith('box.wall.')]:
 wall.animation_data_clear();xy=tuple(wall.location)[:2]
 for f in range(1,193):
  u=E((f-14)/18);wall.location=(xy[0],xy[1],.25-.45*u);wall.scale=(1,1,1-.85*u);wall.keyframe_insert('location',frame=f);wall.keyframe_insert('scale',frame=f)
for ob in [root,*[o for o in bpy.data.objects if o.name.startswith('box.wall.')]]:
 for fc in ob.animation_data.action.fcurves:
  for k in fc.keyframe_points:k.interpolation='LINEAR'

# Final custom logo: enters with the stretched snare, releases the menu before the last kick.
for ob in list(bpy.data.objects):
 if ob.name.startswith('brand.') and ob.type=='MESH' and '.shade' not in ob.name:
  letter=ob.name.split('.')[1];side=-1 if letter in ['D','n','B'] else 1;half=-1 if ob.name.endswith('lower') else 1;a,b=(121,133) if side<0 else (125,137);ob.animation_data_clear()
  for f in range(1,193):
   u=E((f-a)/(b-a));x=side*2.8*(1-u);y=.05+.10*(1-u);z=.72+.08*(1-u)
   if f>=145:q=E((f-145)/23);x=half*3.15*q;y=.05+half*.18*q;z=.72+half*.25*q
   ob.location=(x,y,z);ob.rotation_euler=(0,side*.12*(1-u),0);ob.keyframe_insert('location',frame=f);ob.keyframe_insert('rotation_euler',frame=f)
  for fc in ob.animation_data.action.fcurves:
   for k in fc.keyframe_points:k.interpolation='LINEAR'
shade=bpy.data.materials['independent shade opacity'];mix=next(n for n in shade.node_tree.nodes if n.type=='MIX_SHADER');shade.node_tree.animation_data_clear()
for f,v in [(1,0),(120,0),(133,.45),(144,.45),(168,0),(192,0)]:mix.inputs[0].default_value=v;mix.inputs[0].keyframe_insert('default_value',frame=f)
# Fixed final object identities/transforms. No finished-screen texture, fade, or swap.
matrices=[]
for f in [169,181,192]:
 s.frame_set(f);matrices.append({name:[float(v) for row in p.matrix_world for v in row] for name,p in objects.items() if manifest[name]['page']=='HOME'})
assert matrices[0]==matrices[1]==matrices[2]
assert not [i for i in bpy.data.images if i.source=='FILE']
s['scene_status']='Actual latest grouping UI components; exact owner bank; approved audio; source review/pin supersession pending';s['actual_source_tree']='adc73cd56fe25dfc416a70216da9537c1a17695d';s['approved_audio_sha256']=data['audio_sha256']
s.render.resolution_x=s.render.resolution_y=240;s.cycles.samples=32;s.cycles.use_denoising=False;s.render.use_motion_blur=True;s.render.motion_blur_shutter=.38;s.render.use_sequencer=False
for vl in s.view_layers:vl.cycles.use_denoising=False
s.frame_set(169);bpy.ops.file.pack_all();bpy.ops.wm.save_as_mainfile(filepath=str(O/'DnBOX-v13r2-current-ui-assembly.blend'))
(O/'actual-component-assembly-receipt.json').write_text(json.dumps({'objects':manifest,'source_tree':s['actual_source_tree'],'approved_audio_sha256':data['audio_sha256'],'final_home_matrices_constant':[169,181,192],'no_file_image_textures':True,'whole_page_crossfade':False,'pending':'Source candidate b57 physical fader fix supersession; rendered HOME/FX pixel state already correct'},indent=2))
for f in [1,18,26,42,66,90,122,145,169,192]:s.frame_set(f);s.render.filepath=str(O/f'actual-assembly-{f:03d}.png');bpy.ops.render.render(write_still=True)
