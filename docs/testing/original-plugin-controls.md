# Chicago and Miami original panel controls

The firmware pages follow the original **visible panel controls**, rather than
copying all public automation parameters into the editor. Chicago has 20 panel
controls; Miami has 16. Both also expose Pitch Bend and Damper to their host.
Those two endpoints belong to performance input and do not appear on synth or
effect pages.

This correction continues LX-REQ-032 from source
`16ef646d698445535c2b348ddc802467ad72ff56`. It preserves the accepted compact
seven-choice Chicago effect policy, original program/setter arithmetic, and
exact untouched factory values. The previous idle DSP repairs remain in place.

## Evidence and its limits

The audit started with the saved original-plugin evidence and the locally
retained source. Panel lettering was then checked against Korg's
[Chicago reference image](https://support.korguser.net/hc/article_attachments/115002777803),
[Miami reference image](https://support.korguser.net/hc/article_attachments/115002786986),
and [Gadget Guide](https://www.korguser.net/gadget/manual/gadgetguide/Gadget_GG_E1.pdf),
Chicago pages 18–20 and Miami pages 33–34. Korg publishes those images through
its [Synth gadgets help article](https://support.korguser.net/hc/en-us/articles/115001030803-Synth-gadgets).
They establish official visible lettering, but are not claimed to be new
captures of a running Mac AU 3.1.6 editor.

Saved Mac Miami 3.1.6 editor-construction evidence independently identifies 14
widget records and the SHAPE/RATE button groups, covering macro IDs 0–15. Its
Pitch Bend 16 and Damper 17 are absent from that main editor. Original resource
mapping, native setters, and captured AU selector-display transitions provide
the remaining parameter evidence. No new Mac plugin instance was run for this
audit. Continuous intermediate numeric text is derived from the recovered
control/native scales; Miami selector text has original captured transition
evidence.

Reference identities:

| Evidence | SHA256 |
|---|---|
| Official Chicago image | `230b6a202fdbfca28916bf3563c4d12f3478228c7f7b0142b61c56daa537661a` |
| Saved Chicago control inventory | `1b57d7550e3738ef43746cea34e31185092609d1a0983ddbf7ff70bd2f287b23` |
| Miami installed macro descriptor | `759d96b9459bb4230da084e4db4e118f470dac8f746b250292e139039b96f97e` |
| Miami installed macro mapping | `74eae09e79cdc6f2de00be2da2d0d9992d4852db5c080bb4d2a373e84c51fcbc` |

Restricted plugin resources, reference images, program tables and probe
binaries are not included in this source change. The existing
[external-input manifest](../../experiments/original_synth/EXTERNAL-INPUTS.json)
still describes the exact local build inputs.

## Chicago mapping

The table lists the immutable original macro ID. Firmware page order can
differ from macro order without changing the sound. FILTER uses two rows to
keep its five original controls together. Continuous macro amounts retain
percentage readouts; they are not claims of a measured frequency or time.

| Page | Visible control | Macro | Original/native destination | Display or selection |
|---|---|---:|---|---|
| OSCILLATOR | WAVE | 0 | Native 3/11, oscillator 1/3 levels | Continuous waveform mix and matching glyph |
| OSCILLATOR | GLIDE | 1 | Native 16, portamento | Amount |
| FILTER 1 | CUTOFF | 2 | Native 24, VCF cutoff | Amount |
| FILTER 1 | PEAK | 3 | Native 25, resonance | Amount |
| FILTER 1 | ENV | 4 | Native 26, bipolar VCF envelope amount | −100%…0%…100%; exact neutral at control 64 |
| FILTER 2 | BITE | 5 | Native 111, fixed Drive edit | Amount |
| FILTER 2 | GNAW | 6 | Native 7/39, oscillator 2 level and cross modulation | Coupled original macro |
| ENVELOPE | ATTACK | 7 | Native 28/32, filter/amp attack | Shared original envelope control |
| ENVELOPE | DECAY | 8 | Native 29/33, filter/amp decay | Shared original envelope control |
| ENVELOPE | SUSTAIN | 9 | Native 30/34, filter/amp sustain | Shared original envelope control |
| ENVELOPE | RELEASE | 10 | Native 31/35, filter/amp release | Shared original envelope control |
| AMPLIFIER | ENV/GATE | 11 | Native 107, amp envelope mode | ENV / GATE |
| AMPLIFIER | LEVEL | 12 | Native 96, master volume | Amount, applied once by the original output path |
| ARPEGGIATOR | ON | 13 | Native 62, enable | OFF / ON |
| ARPEGGIATOR | RANGE | 15 | Native 60, range selector | 1 OCT / 2 OCT / FULL |
| ARPEGGIATOR | MODE | 14 | Native 61, note-order selector | DOWN / UP / UP/DN |
| ARPEGGIATOR | SPEED | 16 | Native 54/53, timing fields | Ten musical divisions |
| EFFECT | TYPE | 17 | Native 113, selected effect; native 133 is an original no-op here | Seven admitted original choices |
| EFFECT | EDIT 1 | 18 | Original FX2 first edit mapping | Original macro amount |
| EFFECT | EDIT 2 | 19 | Original FX2 second edit mapping | Original macro amount |

The previous MODE text and arrow selected UP for native mode 0. The actual
selector uses native 0 for alternating direction, native 1 for descending, and
native 2 for ascending. The corrected text and glyph follow that behavior.
The public macro order is DOWN → UP → UP/DN. RANGE's third panel state is FULL;
the internal range value 3 must not be rendered as a third numbered octave.

SPEED has the following exact timing meanings, in its original order:

| Readout | Quarter-note duration |
|---|---:|
| 1/2 | 2 |
| 1/4D | 3/2 |
| 1/2T | 4/3 |
| 1/4 | 1 |
| 1/8D | 3/4 |
| 1/4T | 2/3 |
| 1/8 | 1/2 |
| 1/8T | 1/3 |
| 1/16 | 1/4 |
| 1/32 | 1/8 |

D and T are compact firmware notation for dotted and triplet values. The
durations are proved from the original timing fields and clock behavior; the
notation is not claimed to reproduce an unobserved plugin tooltip.

### Admitted Chicago effects

| Panel name | Original selected type | EDIT 1 behavior | EDIT 2 behavior |
|---|---:|---|---|
| OFF | 0 | Inactive | Inactive |
| CHO/FLG | 4 | Rate | Original coupled feedback/wet control |
| PHASER | 5 | Rate | Modulation depth |
| RING MOD | 10 | Frequency | Mix |
| COMPRESSOR | 18 | Sensitivity | Attack |
| DECIMATOR | 22 | Rate | Mix |
| EQ | 24 | Frequency | Gain |

The visible edit captions remain **EDIT 1 / EDIT 2**, as documented by Korg.
The behavior descriptions above explain their actual setters; they are not
invented claims about dynamic plugin captions. No visual evidence of those
active-effect captions was available. OFF dims the two inactive edits.

The accepted compact profile still omits the other original effect types and
the presets that require them. It does not silently substitute another effect.
It retains 55 admitted Chicago factory programs from the original set of 81.
Changing the selected effect follows the accepted cold policy: it discards the
previous selected effect's tail instead of retaining two effect states.

## Miami mapping

OUTPUT and XMOD follow the actual lettering in Korg's panel image. The manual
prose calls the output cluster MASTER. WOBBLE is a firmware page title for the
adjacent SHAPE/RATE controls; it is not a claim that the original panel has a
separate box with that heading.

| Page | Visible control | Macro | Original/native destination | Display or selection |
|---|---|---:|---|---|
| CARRIER | WAVE | 0 | Native 9/1, sub and carrier waveform | TRI / SAW / SQR |
| CARRIER | LEVEL | 1 | Native 3, carrier level | Amount |
| CARRIER | SUB | 2 | Native 11, sub level | Amount; no invented independent sub-wave control |
| CARRIER | PITCH | 3 | Native 2/10, carrier and sub semitone | −12…+12, 25 states |
| XMOD | WAVE | 6 | Native 5, modulator waveform | TRI / SAW / SQR / PUL |
| XMOD | LEVEL | 5 | Native 7, modulator level | Amount |
| XMOD | TUNE | 7 | Native 80, bipolar modulation routing | Signed unitless −5.00…+5.00, exact edited zero |
| XMOD | DEPTH | 8 | Native 39, cross-modulation depth | Amount |
| FILTER | CUTOFF | 9 | Native 24, VCF cutoff | Amount |
| FILTER | CRUSH | 10 | Native 110/25/111, fixed decimator controls and resonance | Coupled original macro |
| FILTER | WOBBLE | 12 | Native 81, squared modulation-depth mapping | Amount |
| WOBBLE | SHAPE | 11 | Native 43/49, modulation waveforms | TRI / DOWN / UP / SQR |
| WOBBLE | RATE | 13 | Native 48/47, modulation timing | Eight original divisions |
| OUTPUT | GLIDE | 4 | Native 16, portamento | Amount |
| OUTPUT | STEREO | 15 | Native 91, unison spread | Amount |
| OUTPUT | OUTPUT | 14 | Native 96, master volume | Amount |

RATE order is **1/2, 1/4, 1/8, 1/16, 1/3, 1/6, 1/12, 1/24**. The first four
and last four values are separate straight/triplet groups in the original
panel. XMOD DEPTH and WOBBLE are different macros. The latter's internal
automation alias is Wobble Depth; its visible caption must be WOBBLE.

TUNE's internal destination is a modulation intensity field. The recovered
display scale does not justify a semitone, cent or Hz suffix. PITCH is separately
derived from the actual quantized carrier semitone field.

## Control stepping, neutral and factory precision

Each encoder turn changes a selector by one original state. This applies to
normal editing and held-step parameter locks. Continuous controls retain the
existing accelerated encoder behavior. Patch and motion values remain seven
bits; the full original resource's 1001-position continuous edit resolution is
not claimed. All original selector states are reachable.

Two proved bipolar controls need a centered firmware conversion: Chicago ENV
(macro 4) and Miami TUNE (macro 7). For control byte q:

```text
q < 64:  normalized = q / 128
q >= 64: normalized = 0.5 + (q - 64) / 126
```

This preserves 0 and 1 endpoints and represents exact 0.5 at q64. The old
q/127 conversion could not reach neutral after editing either control. The
factory display byte uses the inverse centered scale, but an untouched factory
parameter still comes from its original binary32 value. Display projection,
patch restore and runtime editing use the same control conversion.

ENV displays the signed normalized amount derived from the native envelope
depth, so its neutral position visibly reads 0%. This is a percentage of the
proved bipolar amount, not a claim about original tooltip typography. Positive
ENV amounts omit a redundant plus sign so 100% fits the existing value area;
negative amounts retain their minus sign.

This intentionally changes the interpretation of already edited ENV/TUNE
bytes and their motion values. It does not add a compatibility mode for the
incorrect old law. Other macro laws and the original DSP setters are unchanged.
In particular, EQ Gain keeps the original integer effect mapping; no artificial
unity-gain state is inserted into the original effect table.

The baseline audit verified exact restoration of every admitted factory.
Rounding was material: 189 of the 540 Miami factory macro values differed from
their display-byte representations, and doing so to TUNE would alter 19 of its
30 factory programs. The renderer therefore caches a projection of the exact
program and applies only explicit edits or motion/lock previews.

## Performance input ownership

Chicago's original IDs remain Pitch Bend 20 and Damper 21; Miami's remain 16
and 17. The backend still has 22/18 endpoints. Removing controls from the pages
does not renumber the original mappings or shrink the runtime API.

Pitch Bend belongs to physical/MIDI performance input; Damper is handled
through MIDI CC64. New patch edits of those endpoints are refused. Valid old
patch bytes no longer bias restored bend or hold a pedal down: their base
values are neutral 0.5 and released 0. Former performance motion lanes must not
override, ramp or reset a held wheel/pedal. Ordinary motion lanes keep their
existing behavior and engine-identity protection.

An archived performance lane remains a used lane for engine-change protection
until it is explicitly removed. It cannot be reinterpreted as a different
engine's synthesis parameter. This change does not run a destructive part-clear
operation or silently remove other recorded notes and motion.

The physical scanner already used the correct original IDs. Its working owner
tracking, MIDI routing, and preset-adoption hold are retained. The former UI
Pitch Bend edit could persist a non-neutral base, which the physical center
then faithfully restored. That confirmed defect requires the patch/runtime
policy correction as well as removing the page entry.

The actual checks cover both physical wheels and incoming MIDI Pitch Bend and
CC64. The built-in pedal ADC currently has no route to these original engines,
and incoming CC1 is not implemented in the original MIDI branch. Those
pre-existing routes are unchanged here. The physical modulation wheel does
control Chicago CUTOFF and Miami WOBBLE. No physical-pedal acceptance is claimed.

## Scoped validation

- `tests/host/original_ui_test.sh` exercises actual encoder input, every panel
  control, selector order, held-step locks, exact factory projection, neutral
  values, native font widths, and actual rendered pages under normal and
  ASan/UBSan builds.
- `tests/host/original_sound_cell_test.sh` exercises the patch codec against
  original programs, centered conversions, all admitted factories, invalid
  values and performance-only endpoint rules.
- `tests/host/original_wheels_integration_test.sh` runs the actual scanner,
  command queue and original engine; a metadata-only or stub-only mapping test
  is insufficient to establish center restoration or runtime ownership.
- `tests/host/original_firmware_integration_test.sh` covers the affected complete
  application path. Unchanged DSP arithmetic/audio evidence is reused where
  its exact inputs remain valid.

The UI reuses the established shared card and glyph renderer from the saved
`6ae0928fe3264ee759c25c4f879eff4dc5dc3b20` presentation. Ordinary control strips
retain their value, motion and lock paths. The metronome follows the sequencer's
published quarter-note phase; tape reels follow actual delay activity and stop
when its signal and release window settle. Only visible phase changes enter
the strip redraw signature. These cues do not claim to plot measured audio.

Original SOUND pages use the established large values, bars and waveform-cell
geometry through a semantic adapter for the audited controls above. Their
29-by-19 waveforms consume the exact native projection already held in the
168-byte pool cache. Projection runs only when the selected sound or relevant
preview changes; animation time never enters its key. The Chicago ENVELOPE
touch view retains the stage highlight, lock label and four control cards. Its
caption explicitly identifies a control schematic in percentages; horizontal
spacing is illustrative, not milliseconds or an audio-envelope trace. Miami
gains no ADSR controls. Legacy oscillator and envelope equations are not used
for original-engine presentation.

PERF keeps raw meter calculations unchanged and widens only its display
storage, so loads above 255% cannot wrap to a small value. Native image, static
RAM and pool budgets must be checked on the final build. Software renders and
native builds do not establish hardware CPU percentages, installation success
or device acceptance.
