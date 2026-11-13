# DnBOX — editable Blender preview r2

## Open and edit
Open `DnBOX-v13r2.blend` in Blender 4.3.2 or a compatible newer version. The project is self-contained: approved sound and used font data are packed; originals and source data are also supplied separately. Fixed perspective camera, real solid bodies, bevels, lighting, occlusion and temporal motion blur. No finished UI screenshot is swapped in at the end.

This is the completed **r2 preview snapshot**, not owner acceptance of the motion or a firmware boot implementation. Sound was explicitly approved; geometry remains reviewable. No device performance or universal saved-state seamlessness is claimed.

### Scene objects
- `01_BOX`: real upper hinge and lid. NegativeX opening lifts the front edge then folds away above the box.
- `03_WAVE`, `04_SLICE`, `05_FX`, `06_HOME_DESTINATION`: named actual UI components. Each is an editable solid body plus a source-colour mesh face. The source type/values/order are preserved, not approximated by a replacement font.
- `07_DnBOX`: custom SLAB Dn + CUT BOX. One shared B; true solid letter halves; only letters leave. Independent shade dissolves without moving/fading the HOME objects.
- `actual.WAVE.scope.processed_PCM`: real128-point processed-audio scope, native2× sample decimation and10Hz UI latch, no random wiggle.

Material/depth/trajectory are editable in Blender. Pixel-exact text faces are generated geometry: to change parameter values while preserving source fidelity, change the host fixture and regenerate those components; this is intentionally not an unrelated generic FONT substitute.

## Exact clocks and music
192 frames at 70 fps = 120960 samples at 44100 Hz = 2.742857 s, two 175 BPM bars. Approved WAV SHA256: `19dbec641016005cde1e3b9fa4c89497e7eab8c252840d5791dbca3794febb14`.
- First snare front15923 / attack15925; box is open at displayedframe26.
- Stretched snare76200; accelerating grain section83400.
- Last kick106080; HOME is already landed by displayedframe169.
- Crush113400; interface stays calm. No extra tail or third loop.

## UI source and state
Geometry was first generated from grouping candidate b57ef24; an independent bounded export on local d0c40c3 / tree533564a4085bace9e65245b45affbc45c54a647e proved all four component-source images byte-identical. Remote reviewed-source alias 09449ac0d52df90515bb282f239413f0c23cd4fe has the same tree. This records a source candidate, not a firmware rollout.

Actual pages: MIX/WAVE → BREAK → FX → HOME. Presentation order 909/808/BRK/CHI/MIA. Fixture is stopped 125 BPM / P1 with default USER synth voices and no held notes. The soundtrack is 175 BPM independently; no digits were retouched to pretend that the static fixture is playing it. A runtime transition must bind the actual restored live state and respect saved MASTER/mute; none of that firmware work is implemented here.

The private host preview uses the unchanged previously qualified CRAMEN/TASTY/HOTPANT ADPCM bank bytes through the existing native bank interface. This is not a new native package or sample publication. See `evidence` and the bank input receipt.

## Reproduce
The saved .blend can render directly. For full construction from frozen inputs, run the four scripts under `sources/scene-proof/` in the order build_scene → add_wordmark → consolidate_box_scene → add_home_assembly; then sources/scene-refinement-r2/refine_current_scene → assemble_actual_ui → render_full_preview. Each is invoked with `blender -b --python FILE.py`. The final assembly deletes legacy prototype UI and creates the current source-backed components. Frozen component JSON removes any need to rebuild firmware to reproduce the film.

The optional host export script requires a local exact firmware checkout with its already generated headers; set DNBOX_FIRMWARE_SOURCE. It writes only its own output folder. It is not part of a full firmware QA/build/publication pipeline.

The preview MP4 was independently checked: 192 video frames; both tracks 120960 ticks @44100; decoded AAC lag0, no clipping. It keeps the approved master unchanged, with only AAC encoding. Quantization residuals of geometry/codec are measured in `evidence/video-final-check.json`, not hidden by a UI crossfade.

## Pending next revision
The owner requested faster-start/ease-out accents and finer sixteenth-note subcomponents after reviewing this checkpoint. A global Night/pseudo-3D UI implementation is also being prepared separately. Those changes are NOT represented by this frozen r2 preview. Do not use this archive as a claim of final motion acceptance.
