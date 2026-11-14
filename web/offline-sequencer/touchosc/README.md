# DnBOX TouchOSC companion source

This is an original portrait performance layout for current Hexler TouchOSC,
with twelve chromatic keys, four GM drum hits, a MIDI channel selector,
explicit Arm/Release controls and incoming-note feedback. It sends musical
Note On/Off messages on **connection 1 only**. It never sends SysEx, transport,
sample, project, installer or flash commands.

**Partial runtime qualification.** PR #910 source has passing incoming-feedback
qualification in genuine Linux TouchOSC 1.5.4.269 through loopback Bridge MIDI.
The later native lifecycle qualification found a lost Note Off across editor
mode and a silent first tap after focus timeout. This candidate adds a supported
return-time recovery safeguard and clears stale note values on disarm. Seven
Lua lifecycle regressions and one new packing check pass. **The changed artifact
has not been qualified in TouchOSC:** the official runtime download returned
HTTP 403 in this stage. Native tag persistence and MIDI sending from `init()`
remain NOT_RUN. This is a source candidate, not a runtime-qualified fix.

Row 105 remains partial. Actual OS endpoint disappearance/reconnection,
independent-touch held-note RELEASE/channel changes, multitouch, mobile/macOS
and real-device behavior remain unqualified. The companion does not implement
the missing device pattern/sound synchronization protocol.

## Build and check

From the repository root, run in the authorized test environment:

```sh
python3 web/offline-sequencer/touchosc/pack.test.py
lua5.1 web/offline-sequencer/touchosc/controller.test.lua
lua5.1 web/offline-sequencer/touchosc/lifecycle.test.lua
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
   active notes and disarms if a touch release is lost while scripts run.
   Disarm also resets note values so a cancelled pointer cannot consume the
   first tap after re-arming. **Press RELEASE before editing, saving, closing,
   switching applications or changing MIDI endpoints.** A disconnected route
   cannot guarantee delivery of a Note Off.

Incoming Note On/Off messages on connection 1 and the selected MIDI channel
update the feedback line and matching key colours. They do not send messages
back, change button values or overwrite any editor draft. The feedback line
reports messages actually received, not a verified project snapshot or proof
that the keyboard echoes live input. Keyboard MIDI output and the route must
provide the feedback; the layout never fabricates it. Other connections,
channels and malformed note messages are ignored.

Incoming feedback has an observation window: initialization, **RELEASE**, either
channel control (including at channel 1/16 limits), and a local held-note timeout
clear the feedback line and all key colours. The neutral line means no incoming
MIDI has been observed in the new window. **ARM** retains feedback from the
current window. Incoming notes can still update feedback while disarmed; they
never become locally held notes or cause RELEASE to send Note Offs for them.
Receive-only highlights have no independent timeout. During normal play, local
and incoming events keep the existing last-event colour behavior.

The layout cannot detect a downstream receiver outage or replay a Note Off
after it has removed the local held-note entry. A lost Note Off can leave a
downstream note sounding, and a later RELEASE cannot recover that cleared entry.
Release notes before changing the route or endpoints. Loopback receiver
restoration does not prove OS endpoint reconnection or delivery while
disconnected.

## Editor-mode safeguard and its limit

Hexler documents `init()` when returning from editor to control-surface mode,
but no pre-stop callback. The qualified baseline loses the outstanding Note Off
when Ctrl+E stops scripts: the ten-second script timeout does not run in editor
mode. **There is no bounded release guarantee while the editor is open.**
Release before entering it; if a note is already stuck, use the receiver's own
note recovery before continuing. Closing, crashes, document replacement and
network editor updates are not covered by the return safeguard.

The candidate stores the original channel of each locally started note in its
native note control's reserved `tag` property (`dnbox-held:1` through
`dnbox-held:16`). On the next `init()`, only valid markers on the allowed note
controls can create releases. It sends those outstanding OFFs, clears their
markers, resets note x values and stays disarmed. This return-time behavior is
API-boundary tested and still needs native qualification. It cannot execute
while scripts are stopped. Do not edit the reserved tags or save with notes
held: tags are editable native document properties and can be saved with the
layout. Newly generated documents have empty tags.

An ordinary release, RELEASE, channel change or timeout retires its marker on
the existing send attempt. Later initialization does not retry that OFF, even
if the receiver missed it. No transport replay, endpoint recovery protocol or
received-note release queue is added. Incoming MIDI never acquires markers.

## Provenance and limits

The musical route follows the pinned firmware at
`bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2`:
`firmware/src/app/engine.c` receives melodic notes and GM kit notes on
`seq.ch[]`; `firmware/src/app/ui.c` defines the editable default channels.
The companion does not reuse the retired editor command table in
`web/EDITOR_PROTOCOL.md`. It cannot read/edit patterns, presets, sound
parameters, sample references or project data. No firmware changes are included.

Hexler documents the [root MIDI callbacks and sendMIDI API](https://hexler.net/touchosc/manual/script-examples),
[control callbacks and notification API](https://hexler.net/touchosc/manual/script-objects-control),
[native tag and value properties](https://hexler.net/touchosc/manual/script-properties-and-values),
[connection configuration](https://hexler.net/touchosc/manual/connections-midi)
and [XML opening in release 1.5.2.261](https://hexler.net/touchosc/releases).
Hexler does not publish the `.tosc` XML schema. The factual container/property
shapes used by this original writer were cross-checked against the fixture-based
[format investigation](https://github.com/rickprice/touch-osc-utilities-rs/blob/main/docs/FORMAT.md).
No third-party layout, script, source code, SDK or binary is copied here.

All files in this directory are GPL-3.0-only under the repository licence.
