# Chicago and Miami replacement profile

Owner-approved scope, 29 October 2026, LX-REQ-032.

The default `X0X_ORIGINAL_SYNTH=1` build replaces the earlier bass303 and WBL
implementations. Each synth part can run Chicago or Miami. Both parts own stable
DSP state and render stereo; a single 256-frame workspace is reused serially.
Explicit `X0X_ORIGINAL_SYNTH=0` is a reference build configuration. It does not
add compatibility or old DSP state to the replacement image.

## Chicago FX and factory programs

The selected-state FX policy offers Bypass, Chorus, Phaser, Ring, Compressor,
Decimator and EQ. The UI displays `Comp` and `Decim` to fit the existing controls.
The seven-position `FxType` control maps explicitly to the original selector.
Edit1 and Edit2 keep the original range and arithmetic. Each part owns 4096 bytes
of stereo Chorus history and a scalar-state union. No sample/send buffer is
borrowed for this state.

Selecting another type discards the previous type's state and tail, constructs
the new kernel, applies current edits and tempo, and enters the existing initial
transition from dry input. Returning to a type constructs it afresh. An ordinary
edit keeps the selected type's history. This is a distinct firmware policy;
it does not reproduce the plugin's shared history across inactive FX types.

The retained reference identifies effect-local reset event codes, but does not
establish their host/MIDI provenance or retiming. Firmware leaves that separate
event stream empty and uses its explicit cold selection/adoption lifecycle.
It does not infer note-to-FX-reset events. The selected kernels and their explicit
queue interface can be compared exactly; complete original host reset/transport
timing is not established by that comparison.

The factory browser offers all 55 original Chicago programs whose selected FX
is in this set, in original source order. Ensemble, Short Delay, BPM Delay,
Hall and Gate Reverb are omitted along with 26 programs that request them.
No program is silently assigned a different effect. The complete original
program tables remain reference data; compact indices map to their exact entries.
Miami retains its 30 original factory programs.

Untouched parameters use their original binary32 factory values. Display
rounding does not change the program. User edits use the firmware control domain;
Chicago FxType is 0..6 and the other exposed controls are 0..127. Patch schema 2
and project sound/stream revision 13 distinguish these meanings from the earlier
diagnostic format. Earlier cells and projects are rejected without an automatic
rewrite. Projects must be created or explicitly reset for this profile.

## Factory loops outside the application

Native and host builds use an empty built-in break bank by default. The separate
stock deliverable preserves the saved 44.1kHz ADPCM bytes for CRAMEN, TASTY and
HOTPANT. The keyboard already enumerates zones inside its two user sample slots:

| Slot | Contents | Complete image bytes | Capacity bytes |
| --- | --- | ---: | ---: |
| 1 | CRAMEN | 35807 | 81920 |
| 2 | TASTY, HOTPANT | 96289 | 122880 |

The three zones appear in order as BR1, BR2 and BR3. These files use the user
sample slots. They are separate from the firmware package, and choosing to
restore them replaces the contents of the selected slots. Preserve the existing
sample backup before an explicitly authorized device operation. Preparing the
files does not write a device. Empty slots leave the BREAK part empty while the
other instruments remain available.

The stock packer validates the exact source bank, slot bounds, headers, names,
sample counts, rate and CRC. It copies ADPCM directly and does not normalize,
resample or re-encode it. Native firmware packages retain the existing sample
map and exclude sample-slot payloads.

## Build and proof

The public source excludes twelve external coefficient/factory-data headers.
Ten are required by the active profile; two support alternate reference paths.
See the [input contract](../experiments/original_synth/EXTERNAL-INPUTS.md) and its
exact path/hash manifest. Supply and verify those local inputs before building.
This source tree alone is not a complete original-engine data closure.

Use the pinned local JieLi compiler and SDK described in BUILDING.md. Build Elite
and Pro sequentially from one frozen source checkout, then build the matching
controller. Keep `X0X_FACTORY_BREAKS=0`, `X0X_WOBBLE=0` and `X0X_DEV_WOBBLE=0`.
Original DSP compilation requires strict arithmetic, with the project's software
FMA path on the target. The application code limit, static RAM, pool reserve and
controller limits are unchanged.

Focused proof includes exact factory restoration, all seven FX selections,
stereo kernel/backend comparisons, queued-input ownership, type/preset resets,
both synth parts, UI names, project roundtrip and malformed-format refusal.
Native BUILD_ONLY proof records actual linked resources, model identity,
controller pairing and package bytes. Those results do not measure hardware
CPU load, audio latency, stack high-water or plugin startup parity. Installation,
hardware acceptance and release publication remain separate outcomes.
