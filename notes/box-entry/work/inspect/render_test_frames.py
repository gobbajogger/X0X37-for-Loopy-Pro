import bpy, time
from pathlib import Path
s = bpy.context.scene
out = Path("/workspace/box-entry/work/short-frames")
for f in (1, 36):
    s.frame_set(f)
    s.render.filepath = str(out / f"test-{f:03d}")
    t0 = time.time()
    bpy.ops.render.render(write_still=True)
    dt = time.time() - t0
    print(f"RENDERED frame {f} in {dt:.2f}s -> {s.render.filepath}", flush=True)
