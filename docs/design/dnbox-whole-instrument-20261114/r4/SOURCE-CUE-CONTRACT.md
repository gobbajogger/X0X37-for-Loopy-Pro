# Frozen R3 cue diagnosis

This is a read-only source diagnosis for the existing integration owner, not an independent acceptance review. Frozen proposal `96b4f43498a089b326e463097b2e9641458e4589` and current firmware source `3c41ef4bedf9701f549ce2752b73306e2fab1eb8` remain unchanged. Effective model/effort is not exposed. No emulator, firmware, test suite, render generation or device action was run.

The proposal file was read back at exact96b4 through the GitHub connector: blob `0ddc54b6d27bc80d8d379b1bfcc6ba26726a908a`; local26265 bytes match that Git blob; SHA256 `14d061b50e0c03ce9ae0c7802b549e58d3d9a75c5ec53ee182f6318a74bddb1f`. Existing native Chicago/Juno PNGs were viewed, without regeneration. Their hashes are in SOURCE-PINS.json.

The reported defects are supported by the code. In proposal.js51–63, graph() chooses pictures from a label regex rather than engine/parameter identity. In matrix()102 it is called without val, so val defaults to50. WAVE always draws the same sine; GLIDE and BITE fall through to the same center-marker scale. The Chicago ADSR drawing at101 is also a fixed illustrative polyline and fixed text. Restoring overview geometry did not restore these source cue semantics.

In proposal.js96, every Juno/Drone patch control defaults to50%. In102 a separate branch bypasses graph() completely and draws only that text plus a half-length bar. Juno's requested semantic glyphs were therefore not implemented. These blanks are not a current-source glyph regression: current3c41 also has no dedicated Juno waveform or inline ADSR glyph. They are an unfulfilled proposed enhancement. Do not describe invented Juno pictures as existing actual firmware.

The input model has a second related gap: editList()130 and event()155 freeze any label WAVE as an enum. Current Juno WAVE is a continuous q0..127 saw/pulse mix, not an enum. ui_original_states() returns no special Juno selector count. Although juno6_set() writes a three-bucket waveform member, juno6_render() actually reads macro0 continuously; that member is not the render's shape selector. Continuous Juno WAVE must remain editable with its existing gesture, clamp and preview ownership. Other modeled scalar increments are unbounded percentages rather than source q0..127/acceleration behavior; do not call them full firmware parameter execution.

## Exact Chicago meanings to retain

| Control | Current actual source path | Meaning lost by frozen R3 |
|---|---|---|
| WAVE, id0, OSCILLATOR K5 | ui.c2913–2926 → ui_original_wave.h64–95 → ui.c6915–6970/7273–7280 | Raw pre-filter/effects oscillator shape cue, projected from restored original native fields1/9 (shapes),3/11 (gains),22 (width). Same lock/motion preview as numeric value. Fixed sine is not equivalent. Untouched factory native floats matter; macro percentage alone does not recover the whole snapshot. |
| GLIDE, id1, OSCILLATOR K6 | ui.c7888–7905, engine.c106–107 → original_runtime.c320–328 | A pitch ramp whose width follows q/127; valid current/target pitch supplies a traveling dot. This is qualitative, not a calibrated time plot. If no live pitch snapshot is supplied, a proposal must not imply that its cursor is actual playback. |
| BITE, id5, FILTER2 K5 | ui.c7830/7906 | Value-shaped soft clipping: g=.3+6q; tanh(g*sin(2π*1.5t))/tanh(g),26×19 source glyph. This pictures the control; it is not a captured output waveform or a guarantee of DSP/audio parity. |

Numeric readouts, the glyph, selected engine/slot and lock/motion/base scope must describe the same source value. Source matrix_value()7042–7054 chooses held lock, otherwise active motion, otherwise base. Retain labels, exact units and sparse K5–8 columns. Do not substitute an engine-agnostic regex picture or invent a live cursor.

## Juno: authentic semantics available for the next UI delta

Source ui_original_controls.h18–21 fixes four editable rows: OSCILLATOR [0,1,12,15]; FILTER [2,3,4,9]; ENVELOPE [5,6,7,8]; LFO/CHORUS [10,11,13,14]. Keep those K5–8 positions plus the fifth SOUND row. IDs16/17 are runtime bend/damper, excluded from patch editing. There is no Juno arp page.

All16 patch controls are q0..127, max127, no name-enum table. Current LCD displays round(q*100/127)% using (q*100+63)/127 integer division. Existing source does not display their physical units. cue-semantics.json gives every exact control law and source lines; these laws support truthful qualitative cue derivations, not a claim that a proposed glyph already exists.

| Row | Source-backed cue meaning to preserve |
|---|---|
| OSCILLATOR | WAVE is continuous saw/pulse crossfade; SUB is square one octave below with gain up to.5; WIDTH is pulse duty10–90%, subsequently modulated by PWM; LEVEL scales the mixed six-voice output. No three-state waveform selector. |
| FILTER | CUTOFF changes nominal lowpass coefficient base35+12000x², with envelope addition and coefficient clamp; RES is feedback amount. ENV is unipolar additive envelope depth, not Chicago's bipolar ENV. HPF is bypassed at0; positive values set a first-order highpass coefficient. Exact Hz response is not qualified by reading those formulas. |
| ENVELOPE | Linear attack and linear decay to sustain, exponential release toward the retirement threshold. Attack/decay/release laws differ from Chicago's native envelope. No reuse of the Chicago waveform/ADSR interpreter as if Juno were the same engine. |
| LFO/CHORUS | LFO is a triangle controlling pulse width; LFO RATE is.1+8x² Hz, LFO DEPTH is±.2x of duty before clamp. CHO RATE is separate sine-modulated shared-delay rate.1+4.9x Hz; CHORUS is continuous wet amount up to.5 with opposite wet polarity in left/right. No source OFF/I/II chorus enum or tempo-sync divisions. |

Use source q values for the known factory fixture. JUNO POLY (factory0) gives displayed rows [33,22,50,79]%, [55,14,35,0]%, [6,31,72,31]%, [12,6,20,35]%. Frozen R3's all50 values cannot stand in for that preset. A generic inspection fixture may instead use explicit custom q values and identify itself as such; it must not carry an unverified preset descriptor.

The source is original Juno-style/PolyBLEP-informed C, not a demonstrated AudioKit J6 port. juno6.h2–4 explicitly says no Plaits source copied. juno_stock_bank.h2–4 explicitly says native q0..127 factory data, not imported Roland/plug-in patches. It provides8 exact names/vectors; the first4 identities must not be reordered. The older026 continuation still describes4 presets and its old evidence limits; use current code/bank for current counts. Neither the names, six voices nor recognizable controls establish AudioKit J6 sound, factory-bank, hardware or audio parity. No pinned AudioKitJ6 source/bank comparison or parity evidence was supplied to this audit.

## Drone disposition and boundaries

Drone is now withdrawn from active-capability design by the owner. Existing3c41 Drone maps/frames are historical source facts, not a request to develop active Drone glyph pages. The incumbent firmware existing firmware owner owns active removal, stable IDs and retained saved Drone data. The next UI delta must consume that owner's admitted availability/result contract, distinguish saved retained-but-unavailable data, and avoid an automatic engine substitution or a new replacement CP1 writer. Exact unavailable-state wording/behavior is not selected here. Settings and reserved CP1 pending-sound/Keep work remain untouched.

The narrow next integration action is to replace only the generic Chicago substitutes with the retained source cue semantics, and add explicitly proposed Juno parameter cues from the exact16-control map while preserving geometry, controls and source value/scope. Carry those changed native states into the already admitted review route; this diagnosis supplies no new acceptance or emulator result.
