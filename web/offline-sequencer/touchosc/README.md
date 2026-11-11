# DnBOX TouchOSC companion source

This is an original portrait performance layout for current Hexler TouchOSC,
with twelve chromatic keys, four GM drum hits, a MIDI channel selector,
explicit Arm/Release controls and incoming-note feedback. It sends musical
Note On/Off messages on **connection 1 only**. It never sends SysEx, transport,
sample, project, installer or flash commands.

**Source delivery; runtime acceptance pending.** The owner assigned all builds
and tests to the Orchestrator's box. No `.tosc` was generated and no Python,
Lua, TouchOSC or device check ran in Codex. A generated file opening successfully
is still separate from MIDI and real-device acceptance. Row 105 remains partial
until the box has generated the layout, opened it in TouchOSC and recorded the
required runtime results. This companion does not implement the missing device
pattern/sound synchronization protocol.

## Build and check on the box

From the repository root, the Orchestrator runs:

```sh
python3 web/offline-sequencer/touchosc/pack.test.py
lua5.1 web/offline-sequencer/touchosc/controller.test.lua
mkdir -p build/touchosc
python3 web/offline-sequencer/touchosc/pack.py --output build/touchosc/DnBOX-companion.tosc
python3 web/offline-sequencer/touchosc/pack.py --output build/touchosc/DnBOX-companion.xml
```

The packer uses the Python standard library, produces deterministic output and
refuses to replace an existing output file. Keep existing qualified artifacts;
choose a fresh output path for a later candidate. The source embeds both Lua
scripts into typed `script` properties and writes a current-format `lexml`
document wrapped in zlib for `.tosc`. There is no dependency on TouchOSC Mk1.
TouchOSC 1.5.2.261 and later can also open the XML output directly. Both output
forms need acceptance in the actual application before delivery as a usable
project.

The Python checks exercise real packing, script embedding, unique IDs, bounds,
touch-target sizes and unchanged source. The Lua checks exercise the real root
script against a small TouchOSC API boundary: disarmed sends, duplicate held
events, exact note releases, channel changes, hold timeout and incoming feedback
without echo. They do not substitute for the actual TouchOSC runtime.

## Route and use

1. Open the generated document in current TouchOSC. Keep it disarmed.
2. Select the intended physical keyboard and configure TouchOSC MIDI connection
   1 with the appropriate send and receive endpoints. Endpoint names are routing
   hints, **not model/device identity**. This layout does not negotiate package
   identity or claim the selected endpoint is an Elite or Pro. Confirm the model
   and route through the existing owner-approved connection workflow.
3. On the keyboard, check its actual `GLO > MIDI` channel for the part to play.
   Set the layout's channel to match. The pinned source defaults are Synth 1
   channel 2, Synth 2 channel 3, 909 channel 10, 808 channel 11 and BREAK channel
   4; these settings are user-editable, so defaults are not device detection.
   Clip/Loopy mode changes note routing and must be considered separately.
4. Press **ARM**, then a key or GM hit. Note velocity is 100. Releasing a key
   sends Note Off on its original channel. Repeated held callbacks do not
   retrigger it. **RELEASE** releases this layout's active notes and disarms.
5. Changing the channel releases this layout's held notes and disarms it.
   Arm again after verifying the new route. A ten-second hold limit releases
   active notes and disarms if a touch release is lost. Release notes before
   switching applications, editing the layout, closing it or changing MIDI
   endpoints; a disconnected route cannot guarantee delivery of a Note Off.

Incoming Note On/Off messages on connection 1 and the selected MIDI channel
update the feedback line and matching key colours. They do not send messages
back, change button values or overwrite any editor draft. The feedback line
reports messages actually received, not a verified project snapshot or proof
that the keyboard echoes live input. Keyboard MIDI output and the route must
provide the feedback; the layout never fabricates it. Other connections,
channels and malformed note messages are ignored.

## Provenance and limits

The musical route follows the pinned firmware at
`bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2`:
`firmware/src/app/engine.c` receives melodic notes and GM kit notes on
`seq.ch[]`; `firmware/src/app/ui.c` defines the editable default channels.
The companion does not reuse the retired editor command table in
`web/EDITOR_PROTOCOL.md`. It cannot read/edit patterns, presets, sound
parameters, sample references or project data. No firmware changes are included.

Hexler documents the [root MIDI callbacks and sendMIDI API](https://hexler.net/touchosc/manual/script-examples),
[control notification API](https://hexler.net/touchosc/manual/script-objects-control),
[connection configuration](https://hexler.net/touchosc/manual/connections-midi)
and [XML opening in release 1.5.2.261](https://hexler.net/touchosc/releases).
Hexler does not publish the `.tosc` XML schema. The factual container/property
shapes used by this original writer were cross-checked against the fixture-based
[format investigation](https://github.com/rickprice/touch-osc-utilities-rs/blob/main/docs/FORMAT.md).
No third-party layout, script, source code, SDK or binary is copied here.

All files in this directory are GPL-3.0-only under the repository licence.
