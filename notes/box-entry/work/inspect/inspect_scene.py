import bpy, json
from pathlib import Path
out = Path("/workspace/box-entry/work/inspect/scene-facts.json")
s = bpy.context.scene
cam = s.camera
facts = {
    "blend": bpy.data.filepath,
    "blender": bpy.app.version_string,
    "fps": s.render.fps,
    "fps_base": s.render.fps_base,
    "frame_start": s.frame_start,
    "frame_end": s.frame_end,
    "frame_current": s.frame_current,
    "resolution": [s.render.resolution_x, s.render.resolution_y, s.render.resolution_percentage],
    "engine": s.render.engine,
    "cycles_samples": getattr(s.cycles, "samples", None),
    "use_denoising": getattr(s.cycles, "use_denoising", None),
    "motion_blur": s.render.use_motion_blur,
    "motion_blur_shutter": s.render.motion_blur_shutter,
    "film_transparent": s.render.film_transparent,
    "image_format": s.render.image_settings.file_format,
    "color_mode": s.render.image_settings.color_mode,
    "color_depth": s.render.image_settings.color_depth,
    "view_transform": s.view_settings.view_transform,
    "look": s.view_settings.look,
    "custom_props": {k: str(s[k]) for k in s.keys()},
}
if cam:
    facts["camera"] = {
        "name": cam.name,
        "type": cam.data.type,
        "location": list(cam.location),
        "rotation_euler": list(cam.rotation_euler),
        "lens": getattr(cam.data, "lens", None),
        "sensor_width": getattr(cam.data, "sensor_width", None),
        "sensor_fit": getattr(cam.data, "sensor_fit", None),
        "has_animation": bool(cam.animation_data and cam.animation_data.action),
    }
    if cam.animation_data and cam.animation_data.action:
        keys = []
        for fc in cam.animation_data.action.fcurves:
            pts = sorted({round(kp.co[0], 4) for kp in fc.keyframe_points})
            keys.append({"data_path": fc.data_path, "array_index": fc.array_index, "frames": pts[:40], "n": len(pts)})
        facts["camera"]["keyframes"] = keys
# images / textures
imgs = []
for im in bpy.data.images:
    imgs.append({
        "name": im.name,
        "source": im.source,
        "filepath": im.filepath,
        "size": list(im.size) if im.size else None,
        "packed": bool(im.packed_file),
    })
facts["images"] = imgs
# materials mentioning HOME / screen / display / texture
mats = []
for m in bpy.data.materials:
    nodes = []
    if m.use_nodes and m.node_tree:
        for n in m.node_tree.nodes:
            if n.type in {"TEX_IMAGE", "EMISSION", "BSDF_PRINCIPLED", "ATTRIBUTE"}:
                rec = {"type": n.type, "name": n.name}
                if n.type == "TEX_IMAGE" and getattr(n, "image", None):
                    rec["image"] = n.image.name
                if n.type == "ATTRIBUTE":
                    rec["layer"] = getattr(n, "layer_name", None)
                nodes.append(rec)
    if "HOME" in m.name or "UI" in m.name or "display" in m.name.lower() or "source" in m.name.lower() or any(n.get("type")=="TEX_IMAGE" for n in nodes):
        mats.append({"name": m.name, "nodes": nodes})
facts["relevant_materials"] = mats
# objects: camera-like, home, lid, box
interesting = []
for ob in bpy.data.objects:
    n = ob.name
    if any(s in n.lower() for s in ("camera", "home", "lid", "box.", "brand", "actual.HOME", "06_")) or ob.type == "CAMERA":
        ad = ob.animation_data
        kframes = []
        if ad and ad.action:
            allf = set()
            for fc in ad.action.fcurves:
                for kp in fc.keyframe_points:
                    allf.add(int(round(kp.co[0])))
            kframes = [min(allf), max(allf), len(allf)] if allf else []
        interesting.append({
            "name": n, "type": ob.type, "location": list(ob.location),
            "hide_render": ob.hide_render,
            "parent": ob.parent.name if ob.parent else None,
            "keyframe_span": kframes,
            "collections": [c.name for c in ob.users_collection],
        })
facts["interesting_objects"] = interesting
facts["collections"] = [c.name for c in bpy.data.collections]
# evaluate a few frames: what HOME / lid look like
def loc(name):
    ob = bpy.data.objects.get(name)
    if not ob:
        return None
    return [round(x, 5) for x in ob.matrix_world.translation]
lid = next((o.name for o in bpy.data.objects if "lid" in o.name.lower()), None)
home_objs = [o.name for o in bpy.data.objects if o.name.startswith("actual.HOME") or o.name.startswith("home.")]
facts["lid_object"] = lid
facts["home_object_count"] = len(home_objs)
facts["home_object_sample"] = home_objs[:30]
poses = []
for f in (1, 6, 8, 18, 26, 36, 37, 96, 97, 120, 169, 192):
    s.frame_set(f)
    rec = {"frame": f, "lid": loc(lid) if lid else None}
    if lid:
        rec["lid_rot"] = [round(x, 5) for x in bpy.data.objects[lid].matrix_world.to_euler()]
    # a representative HOME empty
    for hn in home_objs:
        if hn.endswith(".solid") or ".face" in hn:
            continue
        if "HOME" in hn and hn.startswith("actual."):
            rec.setdefault("homes", {})[hn] = loc(hn)
            if len(rec.get("homes", {})) >= 4:
                break
    poses.append(rec)
facts["poses"] = poses
out.write_text(json.dumps(facts, indent=2, default=str))
print("WROTE", out, "frames", facts["frame_start"], facts["frame_end"], "fps", facts["fps"], "res", facts["resolution"], "cam", facts.get("camera",{}).get("name"), "images", len(imgs))
