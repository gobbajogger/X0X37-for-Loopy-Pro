import bpy,bmesh,math,json,sys
from pathlib import Path
from mathutils import Vector
O=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(O/'DnBOX-v13-scene-proof.blend'))
s=bpy.context.scene
s.cycles.samples=72;s.cycles.use_denoising=False;s.render.use_persistent_data=True
s.render.resolution_x=s.render.resolution_y=480
# Exact original custom outlines, deliberately retaining lowercase n and capital DBOX.
G={
'D': [[(0,0),(.48,0),(.71,.20),(.71,.80),(.48,1),(0,1),(0,.84),(.07,.84),(.07,.16),(0,.16)],[(.27,.21),(.42,.21),(.50,.30),(.50,.70),(.42,.79),(.27,.79)]],
'n': [[(0,0),(.31,0),(.31,.15),(.25,.15),(.25,.48),(.35,.56),(.43,.56),(.49,.49),(.49,.15),(.43,.15),(.43,0),(.74,0),(.74,.15),(.68,.15),(.68,.56),(.54,.73),(.33,.73),(.23,.63),(.23,.73),(.03,.73),(.03,.57),(.08,.57),(.08,.15),(0,.15)]],
'B': [[(0,0),(.50,0),(.70,.18),(.70,.40),(.59,.51),(.68,.61),(.68,.83),(.50,1),(0,1)],[(.22,.18),(.43,.18),(.49,.24),(.49,.37),(.43,.43),(.22,.43)],[(.22,.60),(.41,.60),(.47,.66),(.47,.77),(.41,.83),(.22,.83)]],
'O': [[(.17,0),(.55,0),(.72,.18),(.72,.82),(.55,1),(.17,1),(0,.82),(0,.18)],[(.22,.22),(.22,.78),(.29,.85),(.43,.85),(.50,.78),(.50,.22),(.43,.15),(.29,.15)]],
'X': [[(0,0),(.24,0),(.37,.33),(.52,0),(.76,0),(.50,.51),(.74,1),(.50,1),(.37,.69),(.24,1),(0,1),(.25,.50)]]}
# Make outer contours CCW, holes CW for explicit filled geometry.
def area(p):return sum(p[i][0]*p[(i+1)%len(p)][1]-p[(i+1)%len(p)][0]*p[i][1] for i in range(len(p)))/2
logo=bpy.data.objects.new('brand.DnBOX',None);bpy.context.collection.objects.link(logo)
white=bpy.data.materials.new('brand warm monogram');white.use_nodes=True
bs=white.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=(.83,.87,.78,1);bs.inputs['Roughness'].default_value=.42;bs.inputs['Emission Color'].default_value=(.83,.87,.78,1);bs.inputs['Emission Strength'].default_value=.25
accent=white.copy();accent.name='brand shared B to BOX';ab=accent.node_tree.nodes.get('Principled BSDF')
for f,c in [(1,(.83,.87,.78,1)),(135,(.83,.87,.78,1)),(153,(.54,.86,.09,1)),(192,(.54,.86,.09,1))]:
 for socket in ['Base Color','Emission Color']:ab.inputs[socket].default_value=c;ab.inputs[socket].keyframe_insert('default_value',frame=f)
# Actual dark overlay separate from UI. Only this alpha is animated.
shade=bpy.data.materials.new('independent shade opacity');shade.use_nodes=True
nt=shade.node_tree;nt.nodes.clear();out=nt.nodes.new('ShaderNodeOutputMaterial');mix=nt.nodes.new('ShaderNodeMixShader');tr=nt.nodes.new('ShaderNodeBsdfTransparent');black=nt.nodes.new('ShaderNodeEmission');black.inputs['Color'].default_value=(0,0,0,1);nt.links.new(tr.outputs[0],mix.inputs[1]);nt.links.new(black.outputs[0],mix.inputs[2]);nt.links.new(mix.outputs[0],out.inputs[0])
for f,v in [(1,0),(122,0),(149,.52),(173,.52),(187,0),(192,0)]:mix.inputs[0].default_value=v;mix.inputs[0].keyframe_insert('default_value',frame=f)
bpy.ops.mesh.primitive_plane_add(size=2,location=(0,0,.38));sh=bpy.context.object;sh.name='brand.shade.NOT_UI';sh.scale=(1.4,1.4,1);sh.data.materials.append(shade);sh.visible_shadow=False
# Position at final camera-projected size, each actual letter has thickness and bevel.
scale=.51; widths=[.71,.74,.70,.72,.76];gap=.045;total=sum(widths)+gap*4;cursor=-total/2
pieces=[];starts=[(110,131),(114,135),(118,139),(132,153),(136,157)]
for gi,(letter,polys) in enumerate(G.items()):
 cu=bpy.data.curves.new('custom '+letter,'CURVE');cu.dimensions='2D';cu.fill_mode='BOTH';cu.resolution_u=1;cu.extrude=.025;cu.bevel_depth=.004;cu.bevel_resolution=2
 for j,p in enumerate(polys):
  if (area(p)>0)!=(j==0):p=list(reversed(p))
  sp=cu.splines.new('POLY');sp.points.add(len(p)-1);sp.use_cyclic_u=True
  for pp,xy in zip(sp.points,p):pp.co=(xy[0]+cursor,xy[1],0,1)
 ob=bpy.data.objects.new('brand.'+letter,cu);bpy.context.collection.objects.link(ob);bpy.context.view_layer.objects.active=ob;ob.select_set(True)
 for q in bpy.context.selected_objects:
  if q!=ob:q.select_set(False)
 bpy.ops.object.convert(target='MESH');ob=bpy.context.object;ob.data.materials.append(white if gi<2 else accent)
 # Permanent two halves cut through the real letter mesh; same pieces form then exit.
 for half in [-1,1]:
  part=ob.copy();part.data=ob.data.copy();bpy.context.collection.objects.link(part);part.name='brand.%s.%s'%(letter,'lower' if half<0 else 'upper');part.parent=logo
  bm=bmesh.new();bm.from_mesh(part.data)
  bmesh.ops.bisect_plane(bm,geom=list(bm.verts)+list(bm.edges)+list(bm.faces),dist=.00001,plane_co=(cursor+widths[gi]/2,.46,0),plane_no=(.12,1,0),clear_inner=half>0,clear_outer=half<0)
  cut_origin=Vector((cursor+widths[gi]/2,.46,0));cut_normal=Vector((.12,1,0)).normalized()
  boundary=[e for e in bm.edges if e.is_boundary and all(abs((v.co-cut_origin).dot(cut_normal))<.0001 for v in e.verts)]
  if boundary:bmesh.ops.holes_fill(bm,edges=boundary,sides=0)
  bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
  bm.to_mesh(part.data);bm.free();part.data.update()
  # Basis is final letter. The source shape is one piece of the original scope frame.
  part.shape_key_add(name='Readable letter');k=part.shape_key_add(name='Original frame segment')
  for p in k.data:
   x,y,z=p.co
   if gi==0:p.co=(-.735/scale+(.5-y)*.028,(-.10+.70*(x-cursor)/widths[gi])/scale,z)
   elif gi==1:p.co=((- .735+.735*(x-cursor)/widths[gi])/scale,.615/scale+(y-.4)*.035,z)
   elif gi==2:p.co=((.735*(x-cursor)/widths[gi])/scale,.615/scale+(y-.4)*.035,z)
   elif gi==3:p.co=(.735/scale+(y-.5)*.028,(-.10+.70*(x-cursor)/widths[gi])/scale,z)
   else:p.co=((- .735+1.47*(x-cursor)/widths[gi])/scale,-.175/scale+(y-.5)*.035,z)
  a,b=starts[gi]
  for f,v in [(1,1),(a,1),(b,0),(192,0)]:k.value=v;k.keyframe_insert('value',frame=f)
  # Frame starts on scope surface, becomes spatial type before tearing, never alters UI.
  for f,z,y in [(1,.075,0),(a,.075,0),(b,.72,.05),(173,.72,.05),(187,.72+half*.25,.05+half*.18),(192,.72+half*.25,.05+half*.18)]:
   x=0 if f<=173 else half*(3.15 if f>=187 else 0)
   part.location=(x,y,z);part.scale=(scale,scale,scale);part.rotation_euler=(0,0,0 if f<=173 else half*.10);part.keyframe_insert('location',frame=f);part.keyframe_insert('rotation_euler',frame=f)
  pieces.append(part.name)
 bpy.data.objects.remove(ob,do_unlink=True);cursor+=widths[gi]+gap
# Monotone cubic interpolation avoids spring overshoot.
for dat in [*bpy.data.objects,*bpy.data.shape_keys,*bpy.data.node_groups]:
 if getattr(dat,'animation_data',None) and dat.animation_data.action:
  for fc in dat.animation_data.action.fcurves:
   for kp in fc.keyframe_points:kp.interpolation='BEZIER';kp.handle_left_type='AUTO_CLAMPED';kp.handle_right_type='AUTO_CLAMPED'
# Preserve original piece routes' linear sampled curves, not newly smoothed drift.
for ob in bpy.data.objects:
 if ob.name.startswith(('ui.','page.')) and ob.animation_data and ob.animation_data.action:
  for fc in ob.animation_data.action.fcurves:
   for kp in fc.keyframe_points:kp.interpolation='LINEAR'
def state(scene):
 f=scene.frame_current;bpy.data.objects['page.label'].data.body='WAVE' if f<25 else 'SLICE' if f<73 else 'FX' if f<134 else 'PLAY'
bpy.app.handlers.frame_change_pre.clear();bpy.app.handlers.frame_change_pre.append(state)
(O/'wordmark-geometry-receipt.json').write_text(json.dumps({'casing':'DnBOX','construction':'custom slab D/lowercase n; angular capital BOX; one shared B','letter_halves':pieces,'origin':'these same meshes begin compressed along the original scope frame','ending':'only logo mesh halves depart; separate shade opacity dissolves; UI objects untouched','audio':'no second-bar master chosen or bound'},indent=2))
s.frame_set(157);bpy.ops.wm.save_as_mainfile(filepath=str(O/'DnBOX-v13-scene-with-wordmark.blend'))
for f in [137,157]:
 s.frame_set(f);s.render.filepath=str(O/('wordmark-proof-%03d.png'%f));bpy.ops.render.render(write_still=True);print('WORDMARK_PROOF',f,flush=True)
