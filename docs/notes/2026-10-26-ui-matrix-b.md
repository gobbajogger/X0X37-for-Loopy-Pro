# EDIT matrix B

## Owner decision and reason

The owner rejected the four-control 2×2 EDIT layout, requested a 4×4 matrix with a clearly active physical-knob row, then selected mockup B: larger values and a filled active row. This change implements that choice so the player can see sixteen synth parameters without a duplicate bottom strip. Material UI changes should start with mockups; the later whole-UI redesign is not part of this change.

The selected B design images were frozen before implementation:

| Design | SHA-256 |
|---|---|
| Chicago matrix | `2aece799595e36e39cff7fb5d9f84a27710af817236f9029682d8e551dd367d8` |
| Chicago envelope | `e58e05514f846578d458b8eb78c906f3a68e8977b366b7fd0aea2327ff6751a7` |
| Miami matrix | `9153af944858890d7906e7c2d766ba10acab1e99a80815e36852fac5193ae3cb` |
| Miami envelope | `667dc353e7ce3c426ecf55bb0589938e79b30245f8890c84aa22a93a2bafc905` |

The source base is `144f4e042bdfb2cc52ed16a1cf2a26a1148a0101`. Those source and build artifacts remain separate from this follow-on.

## Controls and navigation

Each visible row maps left-to-right to the physical K5–K8 knobs. K2 or the physical < / > buttons move one row and stop at either end; PARA advances one row and wraps at the end. Crossing a four-row boundary displays the next bank. When a step is held, K2 and < / > retain their existing chance edit; PARA still moves the control row, and the footer shows that context. These are the existing navigation gestures, not new shortcut chords. The footer retains their hints. PATCH retains its existing behavior.

| Engine | Row | K5 | K6 | K7 | K8 |
|---|---|---|---|---|---|
| Chicago | Tone / OSC | Cutoff | Resonance | Env Mod | Wave |
| Chicago | Character | Accent | Drive | Gnaw | Bite |
| Chicago | Filter Envelope | Attack | Decay | Sustain | Release |
| Chicago | Amp Envelope | Attack | Decay | Sustain | Release |
| Miami | Carrier | Wave | Level | Sub | Pitch |
| Miami | X-Mod | Wave | Level | Tune | Depth |
| Miami | Wobble / Filter | Shape | Depth | Rate | Cutoff |
| Miami | Amp Envelope | Attack | Decay | Sustain | Release |

Chicago's later bank contains Arp, Send FX, Aux and SOUND. Tune, volume, drive type, accent decay, amp mode, slide and FEnv remain in the SCALE list. Miami's later bank contains Pitch / FX, Output and SOUND. All 32 Chicago and 23 Miami engine parameters remain reachable exactly once across rows and MORE. Unavailable cells remain empty and their knobs do nothing. Drum voice selection and parameter order are unchanged; drums with fewer parameters do not invent extra controls.

SOUND moves after the synth parameter rows. Its engine, preset and cancel controls keep physical K5/K6/K7; K4 still browses sounds while that row is selected. Routing now identifies SOUND by its engine-selector reference, not the historical page-zero index. Rebuilding rows after an engine change preserves the SOUND identity. The header identifies the active engine; SOUND shows requested choices, pending status and the applicable preset name. Unknown retained engine data exposes only the safe engine selector and retains the existing explicit-adoption guard.

### Verified physical legends

SMK-37 Pro and Elite share the printed label set, as the owner confirmed. The source corroborates it in `SMK37.md` (Controls), `firmware/src/app/panel.c` (PANEL_DEFAULT) and `firmware/hal/smk_input.h` (printed names confirmed on a unit on 2026-10-09). The existing < / > routing is in `main_fm1.c` and `smk_arrows.h`.

| Physical | Internal logical role |
|---|---|
| PATCH / PARA / SCALE | HOME / EDIT / SEL |
| CHORD / NOTE REPEAT / GLOBE / BT | ENV / LFO / GLO / SAVE |
| SEQ PLAY / SEQ REC / OCTAVE− / OCTAVE+ | PLAY / REC / OCT− / OCT+ |
| K2 / K3 / K4 | SELECT / ALGORITHM / PRESETS |
| K5 / K6 / K7 / K8 | KNOB1 / KNOB2 / KNOB3 / KNOB4 |

This change fixes on-screen hints, help titles and confirmation captions, not logical roles, controller mappings or host scenario command names. FM-1 retains its own captions. In a remapped/custom panel profile, the existing logical action mapping remains unchanged; default physical captions describe the verified stock SMK profile.

## Envelope semantics

The envelope replaces the matrix while an envelope knob is edited, remains for the existing 900 ms inactivity window, then restores the matrix. Values and shape resolve the same held-step lock or live motion value before falling back to the saved sound. The existing exact touch-reference guard prevents a detent from another row/part/engine from opening an unrelated envelope.

The drawing is a **stage-normalised parameter-shape preview**, not an audio trace or a millisecond axis. HOLD is illustrative. There are no time-axis ticks or invented proportional timing. Technical explanation stays here: the owner explicitly removed the word SCHEMATIC from the device screen, but retained useful navigation hints.

- Chicago amp ENV mode: linear attack; exponential decay toward sustain and exponential release. Each displayed RC segment samples five time constants with `exp(-5u)`, rather than the former quadratic easing. The numeric decay/release values are the engine's time constants, not completion times. At release pot zero, `1/50ms` distinguishes stock normal/accent release.
- Chicago Gate mode: amp ADSR controls route to the filter and use its filter time laws. Attack and decay are linear. The actual Gate code refreshes release step from the current level each control block, so release is geometric: `level *= max(0, 1 - block_samples / (sample_rate × release_seconds))`. The preview uses the normal four-sample active-render recurrence over five nominal release times; the engine’s idle/block-size transition and downstream RC filter are not audio-traced. Standalone FENV instead captures the release slope at note-off and releases linearly. The overlay says FILTER ENV / GATE. Separate Filter ADSR controls indicate when inactive or disabled, rather than claiming they drive the active envelope.
- Miami: the engine's actual quadratic pot laws use attack 2–2000 ms, decay 0.1–2500 ms and release 4.75–3000 ms. Attack and decay slopes are linear. Release has a fixed full-scale slope, so release from sustain lasts `sustain × release_time`. Sustain zero has no sustain-level release tail; sustain one skips the decay fall. Sub-10ms readouts retain a decimal instead of silently rounding the minima to 1/5 ms.
- Wave glyphs are compact parameter-state cues, not sampled audio. Chicago shows the continuous saw/square mix. Miami distinguishes triangle/square and the X-Mod pulse choice.

## Differences from the mockup

- Corrected mockup K1–K4 and logical FM-1 names to physical SMK K5–K8 and printed button legends, following the later owner clarification.
- Removed SCHEMATIC and technical curve captions at the owner's request; kept navigation hints and relevant inactive/held-lock warnings.
- Real values and live preset names replace the illustrative mockup values. Existing theme colours remain for some secondary values and status indicators.
- Added active transport, record and unsaved indicators, safe unknown-engine state, pending/cancel status, and a held-step/motion marker.
- The source uses the existing 20/138/82 pixel strip allocation, existing bitmap fonts and drawing primitives. It adds no persistent UI state, font asset or dependency.

## Regression coverage

`sh tests/host/sound_matrix_test.sh` runs the production renderer with independent numeric expectations and checks all parameter targets, partial rows, readout widths, navigation, preset transitions, unknown-engine safety, held locks/motion, direct DSP release recurrence checks, actual curve-pixel checkpoints and overlay lifetime. It produces real 240×240 screenshots for both engines, gate/inactive states, extremes, all row positions and both drum kits.

`tests/host/test_sound_matrix_display.py` runs ordinary host input scenarios for primary-bank geometry, wave controls, row/bank navigation, wrap, stale-touch rejection, held-step editing and transient envelope restoration. Existing preset, SMC mapping, UI feedback and storage/persistence checks remain applicable.

Host tests and source review do not establish native memory margins, hardware behavior or installability. Exact-source Elite/Pro development builds and independent review remain required before rollout. No merge, publication, flash or production promotion is authorized by this change.

## Owner follow-on: screen-bank dots

The top-right page fraction is replaced by circular page dots: the active screen
bank is filled and inactive banks are outlined. They count groups of four matrix
rows, not musical divisions or individual selected rows. The existing dot drawing
primitive is reused. Chicago and Miami both currently have two banks; Miami's
second bank is partial. Unknown retained engines have one safe SOUND bank and
one filled dot. Row selection, controls, pending/edited state and the separate
save-state corner marker are unchanged. Actual framebuffer tests cover every
bass row, both engines, the partial/single bank, both drum kits, indicator bounds
and all four save states. This supersedes only the fraction shown in the earlier
frozen-B screenshots; broader overview redesign remains proposal-only.
