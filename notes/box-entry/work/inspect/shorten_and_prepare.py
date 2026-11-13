"""Prepare a work copy of the r2 scene: keep the 36-frame box entrance,
land on the exact 94197fc8 HOME, cut the rest. Does not touch originals."""
import bpy
from pathlib import Path

HOME_PNG = "/workspace/box-entry/work/HOME-94197fc8-480.png"
OUT_BLEND = "/workspace/box-entry/work/DnBOX-v13r2-short.blend"

s = bpy.context.scene

# Hide every mid-film and old-HOME collection. Keep stage, box, and lid wordmark.
hide = {
    "02_SHARED_UI",
    "03_WAVE",
    "04_SLICE",
    "05_FX",
    "06_HOME_DESTINATION",
}
# Keep 07_DnBOX out of the landing: letter meshes enter only after frame 120
# in the original and would otherwise sit off-screen; hide them so they
# cannot cover the current HOME.
hide.add("07_DnBOX")

for name in hide:
    coll = bpy.data.collections.get(name)
    if not coll:
        continue
    coll.hide_render = True
    coll.hide_viewport = True
    for ob in coll.objects:
        ob.hide_render = True
        ob.hide_viewport = True

# Exact current HOME as an unlit emission plane that fills the fixed camera
# at the same depth the landed HOME geometry used (Z = 5.4 - 2.4*80/36).
# Visible width at that depth is 2.4 BU, i.e. the 240-pixel screen at 0.01 BU/px.
Z = 5.4 - 2.4 * 80 / 36
img = bpy.data.images.load(HOME_PNG)
img.colorspace_settings.name = "sRGB"

mat = bpy.data.materials.new("HOME_94197fc8_exact")
mat.use_nodes = True
nt = mat.node_tree
nt.nodes.clear()
out = nt.nodes.new("ShaderNodeOutputMaterial")
em = nt.nodes.new("ShaderNodeEmission")
tex = nt.nodes.new("ShaderNodeTexImage")
tex.image = img
tex.interpolation = "Closest"
tex.extension = "CLIP"
em.inputs["Strength"].default_value = 1.0
nt.links.new(tex.outputs["Color"], em.inputs["Color"])
nt.links.new(em.outputs[0], out.inputs["Surface"])

# 2.4 x 2.4 plane, facing +Z toward the camera at (0,0,5.4).
bpy.ops.mesh.primitive_plane_add(size=2.4, location=(0.0, 0.0, Z))
plane = bpy.context.object
plane.name = "HOME_94197fc8_exact"
plane.data.materials.append(mat)
# Default plane faces +Z already.
plane.rotation_euler = (0, 0, 0)

# Put it in its own collection, in the scene.
newc = bpy.data.collections.new("08_CURRENT_HOME")
s.collection.children.link(newc)
for c in list(plane.users_collection):
    c.objects.unlink(plane)
newc.objects.link(plane)

# Entrance only. Confirmed from the saved scene: open 1-36 at 70 fps.
s.frame_start = 1
s.frame_end = 36
s.frame_set(36)
s.render.resolution_x = 480
s.render.resolution_y = 480
s.render.resolution_percentage = 100
s.render.fps = 70
s.render.fps_base = 1.0
s.render.engine = "CYCLES"
s.cycles.samples = 48
s.cycles.use_denoising = False
s.cycles.device = "CPU"
s.render.use_motion_blur = True
s.render.motion_blur_shutter = 0.38
s.render.use_persistent_data = True
s.render.use_sequencer = False
s.render.image_settings.file_format = "PNG"
s.render.image_settings.color_mode = "RGB"
s.render.image_settings.color_depth = "8"
s.view_settings.view_transform = "Standard"
s.view_settings.look = "None"
s.view_settings.exposure = 0
s.view_settings.gamma = 1
s.render.dither_intensity = 0
s.render.filter_size = 0.5
if hasattr(s.cycles, "pixel_filter_type"):
    s.cycles.pixel_filter_type = "BOX"
for vl in s.view_layers:
    vl.cycles.use_denoising = False

s["delivery_status"] = "short box-entry only: frames 1-36 at 70 fps, landing on 94197fc8 HOME; no review hold"
s["short_entry_frames"] = 36
s["short_entry_fps"] = 70

bpy.ops.file.pack_all()
bpy.ops.wm.save_as_mainfile(filepath=OUT_BLEND)
print("PREPARED", OUT_BLEND, "frames", s.frame_start, s.frame_end, "fps", s.render.fps, "Z", Z, flush=True)
