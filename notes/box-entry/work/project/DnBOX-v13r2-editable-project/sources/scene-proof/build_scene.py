import bpy,math,json,sys,os
from pathlib import Path
from mathutils import Vector
O=Path(__file__).resolve().parent
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
s=bpy.context.scene;s.render.engine='CYCLES';s.cycles.device='CPU';s.cycles.samples=12;s.cycles.use_denoising=False
for vl in s.view_layers:vl.cycles.use_denoising=False
s.cycles.max_bounces=4;s.cycles.diffuse_bounces=2;s.cycles.glossy_bounces=2
s.render.resolution_x=480;s.render.resolution_y=480;s.render.resolution_percentage=100;s.render.fps=70;s.frame_start=1;s.frame_end=192
s.render.image_settings.file_format='PNG';s.render.film_transparent=False
s.render.use_motion_blur=True;s.render.motion_blur_shutter=.38
s.world.color=(.025,.025,.025);s.view_settings.view_transform='AgX';s.view_settings.look='AgX - Medium High Contrast'
s.render.threads_mode='FIXED';s.render.threads=8

def mat(name,c,rough=.6,metal=0,emission=0):
 m=bpy.data.materials.new(name);m.diffuse_color=(*c,1);m.use_nodes=True;bs=m.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=(*c,1);bs.inputs['Roughness'].default_value=rough;bs.inputs['Metallic'].default_value=metal
 if emission:bs.inputs['Emission Color'].default_value=(*c,1);bs.inputs['Emission Strength'].default_value=emission
 return m
M={'body':mat('graphite body',(.028,.038,.047),.47,.15),'side':mat('machined side',(.08,.105,.12),.35,.55),'panel':mat('matte working surface',(.012,.019,.025),.62),'white':mat('warm display white',(.75,.82,.77),.5,0,.6),'muted':mat('quiet label',(.22,.31,.32),.6,0,.3),'lime':mat('function lime',(.52,.85,.10),.5,0,.65),'cyan':mat('scope cyan',(.12,.65,.72),.45,0,1.4),'amber':mat('slice amber',(.9,.34,.045),.45,0,.4),'purple':mat('break violet',(.43,.17,.62),.5,0,.3)}
parents={};manifest={}
def parent(name,kind):
 ob=bpy.data.objects.new(name,None);bpy.context.collection.objects.link(ob);parents[name]=ob;manifest[name]={'kind':kind,'children':[]};return ob

def box(name,size,loc,material,par=None,bevel=.015):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.name=name;o.dimensions=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(material)
 if bevel:
  m=o.modifiers.new('physical edge bevel','BEVEL');m.width=bevel;m.segments=3;o.modifiers.new('weighted surface normals','WEIGHTED_NORMAL')
 if par:o.parent=par;manifest[par.name]['children'].append(o.name)
 return o
font_path=O.parent/'project-inputs/fonts/BarlowSemiCondensed-SemiBold.ttf'
font=bpy.data.fonts.load(str(font_path))
def text(name,body,loc,size=.07,col='white',par=None,align='LEFT'):
 c=bpy.data.curves.new(name,'FONT');c.body=body;c.font=font;c.size=size;c.align_x=align;c.extrude=.0007;c.bevel_depth=.0002;o=bpy.data.objects.new(name,c);bpy.context.collection.objects.link(o);o.location=loc;o.data.materials.append(M[col])
 if par:o.parent=par;manifest[par.name]['children'].append(o.name)
 return o

def module(name,w,h,kind):
 p=parent(name,kind);box(name+'.solid',(w,h,.14),(0,0,-.07),M['side'],p,.025);box(name+'.face',(w-.018,h-.018,.025),(0,0,.006),M['panel'],p,.016)
 # These are actual solid edge rails, not extruded screenshot planes.
 box(name+'.edge',(w-.05,.012,.02),(0,h/2-.017,.027),M['muted'],p,.003);return p

def curve(name,points,r=.004,col='cyan',par=None):
 c=bpy.data.curves.new(name,'CURVE');c.dimensions='3D';c.resolution_u=1;c.bevel_depth=r;c.bevel_resolution=2;sp=c.splines.new('POLY');sp.points.add(len(points)-1)
 for p,v in zip(sp.points,points):p.co=(*v,1)
 o=bpy.data.objects.new(name,c);bpy.context.collection.objects.link(o);o.data.materials.append(M[col])
 if par:o.parent=par;manifest[par.name]['children'].append(o.name)
 return o
# Persistent central instrument core.
scope=module('ui.scope',1.44,.76,'persistent real-PCM scope');scope.location=(0,.22,0)
text('scope.label','POST / BREAKS',(-.65,.29,.04),.055,'muted',scope)
for y in [-.18,0,.18]:curve('scope.grid'+str(y),[(-.66,y,.035),(.66,y,.035)],.0012,'muted',scope)
wv=curve('scope.actual_pcm',[(-.66+i/179*1.32,0,.055) for i in range(180)],.004,'cyan',scope)
waves=json.loads((O/'scope-common-first-bar.json').read_text())
for fr,vals in enumerate(waves['frames'],1):
 for i,(p,val) in enumerate(zip(wv.data.splines[0].points,vals)):
  p.co=(-.66+i/179*1.32,val*.25,.055,1);p.keyframe_insert(data_path='co',frame=fr)
header=module('ui.header',2.32,.23,'shared page state and tempo');header.location=(0,1.06,0)
label=text('page.label','WAVE',(-1.07,-.035,.04),.1,'white',header);text('tempo.label','175 / B',(.99,-.035,.04),.07,'lime',header,'RIGHT')
track=module('ui.track',2.32,.15,'shared selected source');track.location=(0,.82,0);text('track.name','crunchy amen / BREAKS',(-1.08,-.025,.04),.058,'muted',track)
waveinfo=module('page.wave.info',2.32,.36,'WAVE page source details');text('waveinfo.main','ONE LOOP / SIXTEEN SLICES',(-1.06,.035,.045),.075,'white',waveinfo);text('waveinfo.sub','the sound stays here',(-1.06,-.09,.045),.056,'muted',waveinfo)
slices=module('ui.breaks.slices',2.32,.40,'slice component reused on SLICE and PLAY');text('slice.label','SLICES / PINS',(-1.06,.105,.047),.062,'white',slices)
for i in range(16):
 x=-1.05+i*.14;box('slice.step.%02d'%i,(.105,.115,.052),(x,-.065,.05),M['lime'] if i in [0,4,8,12] else M['amber'] if i in [2,6,11] else M['body'],slices,.009)
 if i in [0,4,8,12]:box('slice.pin.%02d'%i,(.019,.027,.02),(x,.034,.064),M['white'],slices,.003)
stut=module('ui.breaks.stutter',.43,.76,'native stutter; shared final control');text('stut.title','STUT',(-.175,.26,.043),.065,'white',stut);text('stut.rate','4x',(0,.08,.045),.12,'lime',stut,'CENTER')
for i in range(4):box('stut.pip'+str(i),(.048,.055,.038),(-.12+i*.08,-.10,.047),M['lime'] if i==0 else M['body'],stut,.006)
text('stut.native','NATIVE',(0,-.27,.043),.045,'muted',stut,'CENTER')
rev=module('ui.breaks.reverse',.43,.76,'native reverse available but off in B');text('rev.title','REV',(-.15,.26,.043),.07,'white',rev);text('rev.state','OFF',(0,.06,.043),.095,'muted',rev,'CENTER');box('rev.switch',(.20,.08,.05),(0,-.13,.055),M['body'],rev,.012)
crush=module('ui.master.crush',.43,.76,'master Crush persistent FX to PLAY');text('crush.title','CRUSH',(-.18,.26,.043),.057,'white',crush)
bpy.ops.mesh.primitive_cylinder_add(vertices=48,radius=.125,depth=.075,location=(0,0,.075));knob=bpy.context.object;knob.name='crush.physical_dial';knob.parent=crush;knob.data.materials.append(M['side']);manifest[crush.name]['children'].append(knob.name);m=knob.modifiers.new('dial bevel','BEVEL');m.width=.009;m.segments=3
box('crush.marker',(.017,.073,.008),(0,.055,.119),M['lime'],crush,.003);text('crush.master','MASTER',(0,-.25,.043),.046,'muted',crush,'CENTER')
meter=module('ui.master.level',2.32,.40,'level component on FX; physically exits final');text('meter.title','POST / MASTER LEVEL',(-1.06,.10,.045),.06,'white',meter)
box('meter.track',(2.08,.085,.015),(0,-.05,.035),M['body'],meter,.003);box('meter.signal',(1.1,.058,.018),(-.49,-.05,.049),M['cyan'],meter,.002);box('meter.fixed_handle',(.025,.16,.025),(.35,-.05,.06),M['white'],meter,.002)
nav=module('ui.navigation',2.32,.25,'final persistent navigation');text('nav.text','K5        K6        K7        K8',(-1.04,-.025,.043),.075,'white',nav);text('nav.state','PLAY',(1.07,-.025,.044),.06,'lime',nav,'RIGHT')
# Fixed solid backdrop, not a full-screen final screenshot.
box('constant dark foundation',(2.43,2.43,.09),(0,0,-.24),M['body'],None,.035)

# Controlled complete side crossings, then a curved depth landing; no spring/jitter.
def E(x):x=max(0,min(1,x));return x*x*x*(x*(x*6-15)+10)
def enter(fr,a,b,target,side=1,depth=.65):
 u=(fr-a)/(b-a);p=E(u);tx,ty,tz=target
 if u<0:return(side*3.3,ty,tz),(.16,-side*math.radians(80),side*.07)
 if u>=1:return target,(0,0,0)
 if u<.68:
  v=E(u/.68);x=side*(3.3-4.85*v);y=ty+math.sin(v*math.pi)*.17;z=tz+depth+.16*math.sin(v*math.pi);ang=-side*math.radians(80)
 else:
  v=E((u-.68)/.32);x=-side*1.55+(tx+side*1.55)*v;y=ty+.08*math.sin(v*math.pi);z=tz+depth*(1-v);ang=-side*math.radians(80)*(1-v)
 return(x,y,z),(.12*(1-p),ang,side*.08*(1-p))
def depart(fr,a,b,target,side=1):
 u=E((fr-a)/(b-a));tx,ty,tz=target;return(tx+(side*3.4-tx)*u,ty+.12*math.sin(u*math.pi),tz+.65*u),(.1*u,side*1.40*u,side*.08*u)
plans={
'page.wave.info':{'target':(0,-.59,0),'enter':(1,22,-1),'exit':(25,52,1)},
'ui.breaks.slices':{'target':(0,-.59,.01),'enter':(25,61,1),'exit':(76,104,-1),'return':(132,169,-1)},
'ui.breaks.stutter':{'target':(-.965,.22,.025),'enter':(34,73,1),'exit':(79,109,-1),'return':(137,173,1)},
'ui.breaks.reverse':{'target':(.965,.22,.025),'enter':(43,81,-1),'exit':(82,113,1)},
'ui.master.crush':{'target':(.965,.22,.025),'enter':(74,123,-1)},
'ui.master.level':{'target':(0,-.59,.018),'enter':(84,130,1),'exit':(136,166,-1)},
'ui.navigation':{'target':(0,-1.075,.02),'enter':(133,175,-1)}}
for name,plan in plans.items():
 ob=parents[name];target=plan['target'];manifest[name]['route']=plan
 for fr in range(1,193):
  a,b,side=plan['enter'];loc,rot=enter(fr,a,b,target,side)
  if 'exit'in plan and fr>=plan['exit'][0]:a,b,side=plan['exit'];loc,rot=depart(fr,a,b,target,side)
  if 'return'in plan and fr>=plan['return'][0]:a,b,side=plan['return'];loc,rot=enter(fr,a,b,target,side)
  ob.location=loc;ob.rotation_euler=rot;ob.keyframe_insert('location',frame=fr);ob.keyframe_insert('rotation_euler',frame=fr)
for name in ['ui.scope','ui.header','ui.track']:manifest[name]['fixed_transform']=list(parents[name].location)
for ob in bpy.data.objects:
 if ob.animation_data and ob.animation_data.action:
  for fc in ob.animation_data.action.fcurves:
   for kp in fc.keyframe_points:kp.interpolation='LINEAR'
if wv.data.animation_data:
 for fc in wv.data.animation_data.action.fcurves:
  for kp in fc.keyframe_points:kp.interpolation='LINEAR'

def frame_state(scene):
 f=scene.frame_current
 label.data.body='WAVE' if f<25 else 'SLICE' if f<73 else 'FX' if f<134 else 'PLAY'
bpy.app.handlers.frame_change_pre.clear();bpy.app.handlers.frame_change_pre.append(frame_state)
# Real area lighting and a fixed perspective camera.
def light(name,loc,power,color,size):
 bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.name=name;o.data.energy=power;o.data.color=color;o.data.shape='DISK';o.data.size=size;o.rotation_euler=(Vector((0,0,0))-o.location).to_track_quat('-Z','Y').to_euler()
light('soft key',(-3,4,4),420,(.88,.95,1),3.5);light('quiet edge',(3,-1,2),140,(.36,.72,.72),2.0)
bpy.ops.object.camera_add(location=(0,0,5.4));cam=bpy.context.object;cam.name='fixed perspective camera';cam.rotation_euler=(0,0,0);cam.rotation_euler=(Vector((0,0,0))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.lens=80;cam.data.sensor_width=36;s.camera=cam
s.render.filepath=str(O/'proof-048.png');s.frame_set(48)
(O/'scene-object-manifest.json').write_text(json.dumps({'scope':'audio-independent actual3D geometry proof; no final selected second-bar scope; no full-screen UI textures','fps':70,'frames':192,'objects':manifest,'camera_fixed':True,'motion_blur':'Cycles temporal shutter0.38; moving solid geometry, no whole-image blur'},indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(O/'DnBOX-v13-scene-proof.blend'))
args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
for fr in ([int(q) for q in args] or [48,88,180]):
 s.frame_set(fr);s.render.filepath=str(O/('proof-%03d.png'%fr));bpy.ops.render.render(write_still=True);print('PROOF READY',s.render.filepath,flush=True)
