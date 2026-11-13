import bpy,math,json,hashlib
from pathlib import Path
from mathutils import Vector
O=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(O/'DnBOX-v13-editable-box-to-home-WIP.blend'))
s=bpy.context.scene;coll=bpy.data.collections['06_HOME_DESTINATION']
marker=bpy.data.objects.get('HOME_LAYOUT_PENDING_ACTUAL_REFERENCE')
if marker:bpy.data.objects.remove(marker,do_unlink=True)
fonts=O.parent/'project-inputs/fonts';ft=bpy.data.fonts.load(str(fonts/'BarlowSemiCondensed-SemiBold.ttf'));fb=bpy.data.fonts.load(str(fonts/'BarlowSemiCondensed-Bold.ttf'))
M=bpy.data.materials
# Match the owner-requested night palette; face material remains quiet, labels stay crisp.
for name,colr in [('graphite body',(.007,.012,.016)),('matte working surface',(.004,.008,.011)),('machined side',(.025,.04,.047)),('quiet label',(.22,.29,.28))]:
 m=M[name];m.diffuse_color=(*colr,1);m.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=(*colr,1)
def material(name,c,em=.25):
 m=bpy.data.materials.new(name);m.use_nodes=True;b=m.node_tree.nodes.get('Principled BSDF');b.inputs['Base Color'].default_value=(*c,1);b.inputs['Roughness'].default_value=.62;b.inputs['Emission Color'].default_value=(*c,1);b.inputs['Emission Strength'].default_value=em;return m
colors={'white':M['warm display white'],'muted':M['quiet label'],'off':material('HOME inactive cells',(.015,.022,.028),0),'edge':material('HOME restrained edge',(.06,.09,.095),0),'amber':material('HOME warm amber',(.53,.31,.045),.3),'909':material('HOME 909',(.65,.27,.04),.35),'808':material('HOME 808',(.6,.08,.055),.35),'CH-A':material('HOME CH-A',(.29,.55,.14),.35),'CH-B':material('HOME CH-B',(.07,.43,.60),.35),'BRK':material('HOME BRK',(.38,.16,.58),.35)}
manifest={};finals={}
def cube(name,size,loc,mat,parent=None,bevel=.005):
 bpy.ops.mesh.primitive_cube_add(size=1);ob=bpy.context.object;ob.name=name;ob.dimensions=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);ob.location=loc;ob.data.materials.append(mat)
 for c in list(ob.users_collection):c.objects.unlink(ob)
 coll.objects.link(ob)
 if parent:ob.parent=parent
 if bevel:m=ob.modifiers.new('small physical bevel','BEVEL');m.width=bevel;m.segments=2;ob.modifiers.new('face normals','WEIGHTED_NORMAL')
 return ob

def module(name,x,y,w,h):
 p=bpy.data.objects.new(name,None);coll.objects.link(p);p.location=((x+w/2)/100-1.2,1.2-(y+h/2)/100,.015);finals[name]=tuple(p.location);manifest[name]={'pixel_bounds':[x,y,w,h],'components':[]};p['pixel_bounds']=[x,y,w,h]
 cube(name+'.body',(w/100,h/100,.045),(0,0,-.0225),M['machined side'],p,.008)
 cube(name+'.face',((w-.8)/100,(h-.8)/100,.012),(0,0,.003),M['matte working surface'],p,.003)
 cube(name+'.highlight',((w-2)/100,.008,.005),(0,h/200-.009,.012),colors['edge'],p,.001)
 return p

def local(p,x,y,z=.025):
 a,b,w,h=p['pixel_bounds'];return((x-a-w/2)/100,(b+h/2-y)/100,z)
def text(p,name,body,x,y,px=12,color='white',align='LEFT',bold=False):
 d=bpy.data.curves.new(name,'FONT');d.body=body;d.font=fb if bold else ft;d.size=px/100;d.align_x=align;d.extrude=.0005;d.bevel_depth=.0001;o=bpy.data.objects.new(name,d);coll.objects.link(o);o.parent=p;o.location=local(p,x,y,.021);o.data.materials.append(colors[color]);manifest[p.name]['components'].append(name);return o

def curve(p,name,pts,rad,col):
 c=bpy.data.curves.new(name,'CURVE');c.dimensions='3D';c.bevel_depth=rad;c.bevel_resolution=2;sp=c.splines.new('POLY');sp.points.add(len(pts)-1)
 for k,pt in zip(sp.points,pts):k.co=(*local(p,*pt,.03),1)
 o=bpy.data.objects.new(name,c);coll.objects.link(o);o.parent=p;o.data.materials.append(colors[col]);manifest[p.name]['components'].append(name);return o
# Actual source-backed empty HOME fixture: 16 pattern cells, five lanes, four PERFORM controls.
p=module('home.patterns',2,21,236,31)
for i in range(16):
 x=4+i*14+(4 if i>=8 else 0)
 # Real raised rectangular outline, empty interior means no invented content.
 for q,(xx,yy,w,h) in enumerate([(x,23,12,1),(x,40,12,1),(x,24,1,16),(x+11,24,1,16)]):cube('home.pattern.%02d.edge%d'%(i,q),(w/100,h/100,.01),local(p,xx+w/2,yy+h/2,.02),colors['off'],p,.002)
 if i%4==0:text(p,'home.pattern.number'+str(i),str(17+i),x+1,53,12,'amber')
for j,(lab,y) in enumerate(zip(['909','808','CH-A','CH-B','BRK'],[52,68,90,106,122])):
 p=module('home.lane.'+lab,0,y-1,240,16);text(p,'home.label.'+lab,lab,3,y+11,12,lab)
 for i in range(16):
  x=27+i*13+(i>>2);cube('home.step.%s.%02d'%(lab,i),(.12,.09,.015),local(p,x+6,y+6.5,.024),colors['off'],p,.016/2)
# A separate PERFORMANCE tag and individual actual controls, not one finished-page image.
p=module('home.perform.tag',0,158,240,12);text(p,'home.perform.title','PERFORM',236,169,12,'amber','RIGHT')
for j,(title,value,unit,norm,bi) in enumerate([('TEMPO','175','bpm',155/255,False),('SWING','50','%',0,False),('FILTER','OFF','',.5,True),('PUMP','0.0','dB',0,False)]):
 x=j*60;cx=x+30;p=module('home.control.'+title,x,170,60,56);text(p,'home.control.label.'+title,title,cx,181,12,'muted','CENTER')
 a0=.375*2*math.pi;sw=.75*2*math.pi;r=10.3
 def pts(lo,hi):return [(cx+math.cos(a0+sw*t)*r,196+math.sin(a0+sw*t)*r) for t in [lo+(hi-lo)*k/64 for k in range(65)]]
 curve(p,'home.arc.track.'+title,pts(0,1),.017,'off')
 lo=min(norm,.5) if bi else 0;hi=max(norm,.5) if bi else norm
 if hi>lo:curve(p,'home.arc.active.'+title,pts(lo,hi),.017,'CH-A')
 xx,yy=pts(norm,norm)[0];cube('home.arc.handle.'+title,(.027,.027,.012),local(p,xx,yy,.044),colors['white'],p,.013)
 text(p,'home.control.value.'+title,value,cx-(9 if unit else 0),223,15,'amber' if title!='FILTER' else 'muted','CENTER',True)
 if unit:text(p,'home.control.unit.'+title,unit,cx+(12 if title=='TEMPO' else 6),223,11,'muted')
p=module('home.footer',0,227,240,13);text(p,'home.footer.caption','SMC F1-5: PART LEVEL  F6: REV  F7: DLY  F8: FILT',3,238,10,'amber')
# Actual header state uses shared header body. Track details leave when HOME arrives.
for ob in bpy.data.objects:
 if ob.type=='FONT' and not ob.name.startswith(('brand.','box.mark.','home.')):ob.data.font=ft
header=bpy.data.objects['ui.header']
for name in ['state.page.WAVE','state.page.SLICE','state.page.FX','state.page.HOME']:
 ob=bpy.data.objects[name];ob.data.size=.145;ob.location=(-1.12,-.053,.04);ob.data.font=fb
 if name.endswith('HOME'):ob.data.materials.clear();ob.data.materials.append(colors['amber'])
tempo=bpy.data.objects['tempo.label'];tempo.data.body='175';tempo.data.size=.14;tempo.location=(.92,-.052,.04);tempo.data.materials.clear();tempo.data.materials.append(colors['amber'])
d=bpy.data.curves.new('home.pattern.current','FONT');d.body='P1';d.font=fb;d.size=.145;ob=bpy.data.objects.new('home.pattern.current',d);coll.objects.link(ob);ob.parent=header;ob.location=(.12,-.053,.04);ob.data.materials.append(colors['white'])
for f,v in [(1,True),(121,False)]:ob.hide_render=v;ob.keyframe_insert('hide_render',frame=f)
for ob in list(bpy.data.objects):
 if ob.name.startswith('ui.navigation') or ob.name.startswith('nav.'):bpy.data.objects.remove(ob,do_unlink=True)
# Continuous monotone lateral routes; top L→R, bottom R→L. Final matrices stop changing.
def E(v):v=max(0,min(1,v));return v*v*v*(v*(v*6-15)+10)
for n,tar in finals.items():
 p=bpy.data.objects[n];a,b,w,h=p['pixel_bounds'];top=(b+h/2)<120;side=-1 if top else 1;ix=list(finals).index(n);start=121+min(ix,10);end=146+min(ix,10)
 manifest[n]['route']={'start':start,'end':end,'direction':'LEFT→landing' if top else 'RIGHT→landing'}
 for f in range(1,193):
  u=E((f-start)/(end-start));p.location=(side*3.2+(tar[0]-side*3.2)*u,tar[1],tar[2]+.35*(1-u));p.rotation_euler=(math.radians(6)*(1-u),-side*math.radians(30)*(1-u),0);p.keyframe_insert('location',frame=f);p.keyframe_insert('rotation_euler',frame=f)
 for fc in p.animation_data.action.fcurves:
  for k in fc.keyframe_points:k.interpolation='LINEAR'
track=bpy.data.objects['ui.track'];target=tuple(track.location)
for f in range(1,193):
 u=E((f-121)/25);track.location=(target[0]+3.3*u,target[1],target[2]+.35*u);track.rotation_euler=(.1*u,.52*u,0);track.keyframe_insert('location',frame=f);track.keyframe_insert('rotation_euler',frame=f)
# Final continuity check is component-wise; no viewport screenshot is imported.
matrices=[]
for f in [157,175,192]:
 s.frame_set(f);matrices.append({n:[float(x) for row in bpy.data.objects[n].matrix_world for x in row] for n in finals})
assert matrices[0]==matrices[1]==matrices[2]
assert not [i for i in bpy.data.images if i.source=='FILE']
receipt={'home_source_image':'inputs/HOME-host-reviewed-3eb42089.png','home_source_sha256':'e660bf10d9f27abf686a2dee542eadf0b92a60536a613ee7f0076e8700e50d66','provenance':'Actual HOST synthetic fixture; reviewed source a71944d tree025b3e64 matches merged3eb42089, supplied/verified by root; not hardware/owner saved project','intentional_demo_change':{'TEMPO':[125,175]},'preserved':['empty pattern bank17–32','five empty lanes909/808/CH-A/CH-B/BRK','P1','SWING50%','FILTER OFF','PUMP0.0dB','SMC footer roles'],'components':manifest,'constant_final_matrices_frames':[157,175,192],'raster_UI_textures':0,'audio_master':None,'second_bar_scope':'unbound','palette':'owner requested night'}
(O/'home-assembly-receipt.json').write_text(json.dumps(receipt,indent=2))
config=json.loads((O/'scene-parameters.json').read_text());config['home_layout_bound']=True;config['home_source_sha256']=receipt['home_source_sha256'];(O/'scene-parameters.json').write_text(json.dumps(config,indent=2))
s['scene_status']='HOME bound to actual host fixture; tempo175 demo; final audio still unselected';s.cycles.samples=64;s.cycles.use_denoising=False;s.render.resolution_x=s.render.resolution_y=480
bpy.ops.file.pack_all();s.frame_set(192);bpy.ops.wm.save_as_mainfile(filepath=str(O/'DnBOX-v13-box-pages-HOME-editable.blend'))
for f in [50,166,192]:s.frame_set(f);s.render.filepath=str(O/('home-assembly-proof-%03d.png'%f));bpy.ops.render.render(write_still=True);print('HOME_PROOF',f,flush=True)
