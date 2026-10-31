# Shared parameter-card integration

The owner asked that remaining rotary controls use the common designed UI, not the old rotary arcs inside new panels. Existing proposal glyph families are now connected through one production `control_glyph` dispatcher used by `draw_knobs`.

Affected ordinary strips: HOME, BREAK, drum and synth SEQ, generator, FX, all MIX banks, both SONG banks and GLOBAL's ordinary strip. The existing Chicago/Miami/drum SOUND matrix, clip-owned strip and held-step value display remain their existing specialized presentations, not overwritten by a generic control. Named switches retain their truthful position pips; unmapped cells stay empty. Continuous controls without a specific schematic use the existing graduated value-scale vocabulary instead of an invented response curve.

The shared dispatcher covers pan/transpose, pattern/length/loop selection, actual rate spacing, direction, tape, delay/repeat, reverb, tone, master compression and drive. Existing metronome, swing, filter and pump glyphs remain. These are parameter schematics, not measured audio. All reference bindings, numeric units, motion values, p-locks and touch handling stay on their original path.

Proof: `sh tests/host/ui_shared_glyph_test.sh` enumerates 93 pages / 337 mapped slots at zero/mid/max, verifies glyph bounds and project nonmutation, exact bipolar centre/endpoints and both-kit/bank control maps. `night_ui_test.sh` covers existing common card layout, blank cells and performance glyph behavior. Real native240 rendered caller sheets are in sibling `ui-shared-glyph-evidence-20261027`.

This continues the requested integration; it is not blanket approval of every earlier proposal or a claim that all main-page design work is finished. In particular SONG density is unchanged. Independent review, applicable native build, and owner actual-preview acceptance are separate pending gates. No rollout, hardware or held boot rendering.
