import bpy,json,hashlib
from pathlib import Path
O=Path(__file__).resolve().parent;bpy.ops.wm.open_mainfile(filepath=str(O/'DnBOX-v13r2-current-ui-assembly.blend'));s=bpy.context.scene
s.render.resolution_x=s.render.resolution_y=480;s.render.resolution_percentage=100;s.render.fps=70;s.frame_start=1;s.frame_end=192;s.cycles.samples=48;s.cycles.use_denoising=False;s.render.use_persistent_data=True;s.render.use_sequencer=False
s.render.threads_mode='FIXED';s.render.threads=8
for vl in s.view_layers:vl.cycles.use_denoising=False
F=O/'full-render-480';F.mkdir(exist_ok=True)
# Audio packed in the editable project only; encoded movie receives immutable external WAV.
if s.sequence_editor:s.sequence_editor_clear()
ed=s.sequence_editor_create();snd=ed.sequences.new_sound('APPROVED immutable master19dbec64',str(O/'inputs/approved-master.wav'),channel=1,frame_start=1);snd.sound.pack();snd.volume=1.0
s['approved_audio_sha256']=hashlib.sha256((O/'inputs/approved-master.wav').read_bytes()).hexdigest();s['audio_clock']='120960 mono samples@44100;192 frames@70; no changed audio'
bpy.ops.file.pack_all();s.frame_set(1);bpy.ops.wm.save_as_mainfile(filepath=str(O/'DnBOX-v13r2-full-editable.blend'))
for f in range(1,193):
 s.frame_set(f);s.render.filepath=str(F/f'{f:04d}.png');bpy.ops.render.render(write_still=True)
 if f%12==0:print('EIGHTH_RENDERED',f,flush=True)
(O/'full-render-status.json').write_text(json.dumps({'complete':True,'frames':192,'fps':70,'resolution':[480,480],'cycles_samples':48,'audio_sha256':s['approved_audio_sha256'],'source_tree':s['actual_source_tree'],'source_candidate_not_rollout_claim':True},indent=2))
