# Chicago presentation and contextual PATCH

This small follow-on starts at frozen `7436d312448a8920ac113658f522f454b44fc508`.
It does not change that source or its native receipts. The owner requested clearer
Chicago presentation, meaningful glyphs, removal of redundant pitch-bend cells,
and PATCH cancellation. The later owner clarification explicitly retains the thin
value bars, including ARP enums; the earlier proposal to remove those bars is not
implemented.

## Visible changes

- LIVE ARP uses ENABLE, DIRECTION, OCTAVES and RATE. Small power, arrow and clock
  shapes use the existing drawing primitives and ASCII faces. Retained subordinate
  values are dim but editable while OFF. The stored Full range is displayed as
  three octaves, which is the current runtime behavior, not an unlimited range.
- FX identifies OFF, INSERT or shared MASTER scope. EQ shows TILT/MIX; Ring shows
  FREQ/MIX; Comp and Crush show AMOUNT with unused Edit2; chorus, ensemble and
  phaser show MIX/SHAPE; short delay MIX/FEEDBACK; BPM delay MIX/DIVISION; Hall
  MIX/DECAY; gated reverb MIX/SHAPE. SHAPE denotes the existing coupled macro.
  Numbers remain raw pots, not newly invented percentages or frequency units.
  Ignored parameters show UNUSED/-- without a misleading active amount bar.
- Chicago damper uses a pedal cue and OFF/ON at the existing pot threshold 64.
  Existing bars remain. Value/unit spacing is consistent; waveform and ADSR
  previews remain.
- Chicago CHI_PBEND and Miami W_PITCH_BEND have no matrix or More references.
  Their IDs, saved values, descriptors, factory data, DSP, motion and physical
  wheel behavior are unchanged. Empty columns stay empty: Chicago damper remains
  physical K6, Miami glide K5 and crush K7. No knob compaction occurs.
- SOUND keeps ENGINE K5 and PRESET K6. K7 is inert, not a hidden cancellation
  control. The fourth cell shows status with check/edit/hourglass cues. A pending
  name retains its existing arrow and the footer shows PATCH: CANCEL REQUEST.
  Only separators before real controls or the status cell remain.

## PATCH contract

PATCH still dispatches on release/tap, never eagerly on modifier press. Existing
performance-test, modal question, overlay and GLO priorities run first. A plain
tap on the selected bass SOUND row cancels a confirmed pending request and stays
on that row. The next tap without a request uses the ordinary back path.

Help, held steps, clip mode, project-busy input synchronization and old/current
button modifiers remain protected. A newly pressed key on that release is not a
cancel gesture. Already held live notes and wheels do not prevent cancellation.
The short nested engine edit hold covers the authoritative check and serialized
cancel publication, not the whole physical button press. Audio may still render;
a boundary cannot apply the request between that check and cancellation.

A sound already applied before the check is not undone. An unstable publication
snapshot is not reported as cancellation: the tap only stays on SOUND. Dirty,
save, history, transport and stored sound are not changed by cancellation itself.

## Proof scope

`chicago_presentation_test.sh` uses the real renderer and physical input edges,
actual engine publication interruption hooks and the production SMK wheel scanner
with host ADC substitutes. It covers both voices/engines, 360 ARP states, all FX
types, retained bars, damper63/64, blank columns, pending/applied/cancelled cases,
CQ/MIDI/note/wheel waits, hold nesting and modifier/modal/help/busy priorities.
A ready-to-apply pending1 request isolates the short hold: the injected boundary
must not apply it, and removing only that hold must fail. Separate publication
stage checks preserve the existing seqlock interruption contract. Existing matrix, live-preset, wheel, SMC and UI checks cover affected
consumers. Host pixels and native builds do not establish device readability,
physical hardware correctness, owner acceptance or release permission.

Frozen source, command exit receipts, actual previews, independent Sol review
and affected native evidence are handed to the root integrator separately.
No push, merge, firmware publication or hardware operation is part of this lane.
