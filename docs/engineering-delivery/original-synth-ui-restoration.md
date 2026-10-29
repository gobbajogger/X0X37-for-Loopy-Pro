# Original synth visual controls

The original Chicago/Miami profile previously replaced the established sound
cards with sequential `CONTROLS` rows and bypassed waveform, ARP and damper
glyphs. This presentation repair restores semantic rows and visual controls
for the original macro IDs. It changes no DSP, preset bytes, project format,
control range, stock slot, HOME surface or MIX surface.

The native-scale proposal was independently reviewed before implementation.
The actual candidate renders were then independently inspected. Screenshots
establish the software's presentation, not responsiveness on an overloaded
device; the separate runtime repair and native/hardware validation retain
their own scope.

## Control mapping

Numbers below are the unchanged original macro IDs. SOUND keeps engine,
factory and live status. Every 22 Chicago and 18 Miami control appears once.

| Engine | Row | Logical knob columns1–4 |
|---|---|---|
| Chicago | TONE / OSC | Cutoff2, Peak3, EnvAmt4, Wave0 |
| Chicago | CHARACTER | Bite5, Gnaw6, Glide1, AmpMod11 |
| Chicago | ENVELOPE | Attack7, Decay8, Sustain9, Release10 |
| Chicago | OUTPUT | Level12, Pitch bend20, empty, empty |
| Chicago | LIVE ARP | Enable13, Direction14, Octaves15, Rate16 |
| Chicago | FX / DAMPER | Type17, Edit1/18, Edit2/19, Damper21 |
| Miami | CARRIER | Wave0, Level1, Sub2, Pitch3 |
| Miami | X-MOD | Wave6, Level5, Tune7, Depth8 |
| Miami | WOBBLE / FILTER | Shape11, Depth12, Rate13, Cutoff9 |
| Miami | PITCH / FX | Glide4, Pitch bend16, Crush10, empty |
| Miami | OUTPUT | Output14, Stereo15, empty, Damper17 |

Chicago uses four rows on its first screen and the existing three-row second
screen for ARP, FX/DAMPER and SOUND. Miami keeps its four-row layout and sparse
OUTPUT/SOUND second screen. Physical SMK captions still use K5..K8, including
`KNOBS5-6,8` for Miami OUTPUT. Existing row navigation and empty knobs remain
unchanged.

Chicago's envelope macros jointly affect VCF/VCA stages, so `ENVELOPE` is the
appropriate label. Amp mode displays ENV/GATE from native107. The small stage
drawings are qualitative parameter diagrams. They do not reuse the legacy
engine's full-screen timing graph or claim its timing laws. Miami exposes no
ADSR or bass-split macro in this profile, so neither is invented.

Continuous parameters retain the existing q127 percent formatter. The original
native states supply discrete labels: Miami carrier TRI/SAW/SQR, X-MOD
TRI/SAW/SQR/PUL, wobble TRI/DOWN/UP/SQR, Chicago ARP direction/range/enable,
amp mode and damper. The existing power, clock, pedal and arrow primitives,
native fonts and panel surfaces are reused.

## Original effect editors

Labels come from the selected original kernel setters, not legacy FX mappings.

| FX | Edit1 | Edit2 |
|---|---|---|
| Bypass | EDIT1, inactive | EDIT2, inactive |
| Chorus | RATE | FB/WET, coupled feedback/wet |
| Phaser | RATE | MOD DEPTH |
| Ring | FREQ | MIX |
| Comp | SENS | ATTACK |
| Decim | RATE | MIX |
| EQ | FREQ | GAIN |

Only Bypass dims both editors. Comp Attack and Decim Mix remain active. Source
contracts are `chicago_modulation.c`, `chicago_dynamics.c`, and
`chicago_smallfx.c` through `miami_decimator_adapter.c`.

## Shape projection and resource limits

`ui_original.h` owns a 164-byte selected-part cache. Its 101-byte view contains
three 29-point wave cues plus native discrete state. There are no allocations,
new framebuffer, DSP owner or audio-thread operations. The local program union
is 208 bytes; the frame's preview array is 22 bytes. The exact host GCC-O2 stack
frames reported by `-fstack-usage` are 112 bytes for `original_ui_prepare`,
48 for `ui_original_update`, and 368 for `ui_original_project`. Calls into the
existing program restore/initializer have their own frames; these host numbers
are not a target stack high-water claim. Target stack and memory margins belong
to the integrated native build check.

The cache key contains the original 32-byte cell, selected voice, relevant
motion/lock values and explicit preview mask. Untouched factory floats are
restored exactly. A preview equal to the rounded factory byte still overrides
the exact factory float explicitly. A cache hit performs comparisons, without
program restore, native voice compilation, soft FMA or waveform sampling.

Only eight Chicago and six Miami preview parameters can alter a field read by
the cached projection. The other visual amounts read `matrix_value()` directly.
The dependency masks are documented beside their definitions. Their exclusions
are checked over every q127 value at factory 0 and endpoints/midpoints at all 85
admitted factories, preventing stale motion/lock cues.

Wave points use small integer geometry equivalent to the original waveform
evaluator at reference increment `0x02000000` (128 samples per cycle), without
external modulation. That reference makes the triangle correction zero and
the saw edge coefficient exactly 0.5. Pulse width follows the native
`[0.01,0.99]` clamp before complement. Its asymmetric amplitudes and zero line
use one affine projection; they are not normalized into a square. Chicago's
Wave cue combines the two oscillators controlled by that macro using their
actual gains and shapes. It is not the complete modulated voice or output scope.

The Chicago blend prehalves Q14 amplitudes before multiplying by Q15 gains.
Each term is bounded by 2^29, and the sum by 2^30, so signed 32-bit math suffices.
The resulting loss is below the independently tested one-pixel projection
tolerance. Drawing uses cached integer points; there is no per-pixel floating
point evaluation. Other glyphs are labelled control schematics, without
invented Hz, milliseconds, spectrum or loudness measurements.

MIX-WAVE was already active for the original profile. It still reads the actual
post-master ring and retains its existing 10Hz latch and surfaces.

The identical diagnostic compiled against the frozen baseline and candidate
produces byte-identical HOME, MIX PARTS, MIX MASTER, idle WAVE and sounding WAVE
PNGs. Model-caption renders show `KNOBS 5-8`, sparse `KNOBS 5-6,8`, and the
existing `< >` / `PARA` legends.

A sequential host comparison of complete `ui_frame()` calls, three trials of
500 frames at each fixed state, gave these medians. It measures a host render
cost; it does not establish device CPU usage or audio deadline margins.

| Screen | Baseline | Candidate |
|---|---:|---:|
| Miami first | 237.8µs | 208.1µs |
| Chicago first | 227.2µs | 217.5µs |
| Chicago second | 171.7µs | 202.3µs |

## Focused validation

Run `ASAN_OPTIONS=detect_leaks=0 sh tests/host/original_ui_test.sh OUT`.
The UI test uses actual host unity code and production original DSP units.
Normal and ASan/UBSan runs pass 313,912 checks with byte-identical PNGs.
The bounded checks cover:

- Independent original voice-compiler and waveform/MG comparisons for all 85
  factory programs and q127 selector/control sweeps, within one pixel.
- Native pulse-width clamp boundaries, cache hits, invalid cells/preview masks
  and excluded-macro dependency checks.
- Every 22/18 parameter through the real knob input path, preserving all other
  parameter bytes; exact-once row mapping and sparse knob captions.
- Actual held-step and running motion previews, restoration on release/Stop,
  and preservation of untouched factory cells.
- Actual pages, first/last factory names, four Miami shape tuples, ARP states,
  all seven FX names/editors, live SOUND waiting text and a nonzero MIX-WAVE ring.

The existing full-app test changes only its FX row-location assertion to the
reviewed row 5, column 0. Its audio, MIDI, project and Stop-edge checks are retained
for the integrator's candidate-pinned app run. No new full firmware or device
test is performed by this UI lane.
