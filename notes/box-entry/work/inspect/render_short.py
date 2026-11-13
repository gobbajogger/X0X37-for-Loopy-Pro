import bpy, time
from pathlib import Path
s = bpy.context.scene
s.frame_start = 1
s.frame_end = 36
s.cycles.samples = 48
s.cycles.use_denoising = False
s.render.use_motion_blur = True
s.render.motion_blur_shutter = 0.38
s.render.use_persistent_data = True
s.render.use_sequencer = False
out = Path("/workspace/box-entry/work/short-frames")
t0 = time.time()
for f in range(1, 37):
    s.frame_set(f)
    s.render.filepath = str(out / f"{f:04d}")
    bpy.ops.render.render(write_still=True)
    print(f"FRAME {f}/36  elapsed={time.time()-t0:.1f}s", flush=True)
print(f"DONE 36 frames in {time.time()-t0:.2f}s", flush=True)
