# SMC navigation and knob banks — isolated revision

The owner-tested base `e75fb2a584aa22978f352697a5e36fc9f886c3c5` has fixed
knob mappings. **Knob banks in this revision are new, not installed behavior.**
This revision separates controller navigation from keyboard navigation so the
musician can keep editing a drum kit or sound while exploring another keyboard
screen. Intentional controller navigation remains available under page lock.

## Context and state

Captured target context comprises exact parameter references (including bound
pattern/bar identity), view, part, page, drum selection/bank, mixer selection and
menu position. It is routing metadata, never a copy of parameter values. Keyboard
navigation cannot change those captured destinations. SMC navigation resumes that
context, applies the requested action, and captures its resulting destinations.
Confirmation temporarily parks controls without overwriting the previous capture.
Project reset clears the capture. Song structure edits continue to invalidate
removed captured bars.

The drum track bank (voices 1–8 or 9–11), the sound/page selection, and the knob
mapping bank are separate state. Changing knob banks never changes fader mapping.
An SMC page/mode/track change returns knobs to the context's default bank; keyboard
context changes do so only while following. Pickup invalidation stays with the
existing target/value observer. Merely locking/unlocking an unchanged destination
or changing only its knob bank does not clear fader pickup/history. Encoder
acceleration is keyed by the actual destination, including pattern/bar identity.
The independent fader-state regression owner retains the fader algorithm scope.

## Physical map

| Control | Action |
| --- | --- |
| Pause | Toggle follow/locked. LED blinks in follow; steady when locked. |
| Play | Toggle playback, including pending start. |
| Up / Down | Previous / next context-appropriate knob bank, with wrap. |
| Left / Right | Previous / next parameter page, with wrap. |
| Bank Left / Right in drum SEQ/MIX | Voices 1–8 / 9–11. |
| Channel Left / Right in drum SEQ/MIX or drum sound | Previous / next drum sound, at endpoints stays put. |
| SELECT 1–5 outside drum SEQ/MIX | Select part sound; repeated same drum selection opens its drum mixer. |
| SELECT in drum SEQ/MIX | Select that voice; repeat selected voice in drum MIX returns to parts MIX. |
| S6 outside drum SEQ/MIX | Select MIX; further distinct presses cycle MIX pages while in navigation context. |
| S7 outside drum SEQ/MIX | Global menu/back; retains menu ownership. |
| S8 outside drum SEQ/MIX | Select PART sound; further presses cycle its pages. |
| R6 / R7 outside drum SEQ/MIX | LIST/OK and existing HOME/SONG/BACK behavior. |
| R8 outside drum SEQ/MIX | Select HOME (one page). |
| SELECT 6 / 7 outside drum SEQ/MIX | First opens reverb / delay; further same-button presses cycle FX pages. |
| SELECT 8 outside drum SEQ/MIX | First opens MASTER; further same-button presses cycle MIX pages while in navigation context. |
| M6 / M7 / M8 outside drum SEQ/MIX | Existing reverb / delay / output mute/restore. |

**Drum SEQ and drum MIX are performance contexts.** Every populated strip,
including strip 8/voice 8, retains REC (select voice + toggle recording), held
SOLO, MUTE and SELECT. Bank 2 strips 4–8 remain inactive. Entering a drum MIX page
changes those rows back to drum performance meanings: a shortcut is never a
universal navigation layer. Use Left/Right to traverse pages there. No drum action
is silently reassigned. Physical held repeats do not count as distinct shortcut
presses; releases, queue-loss recovery, reconnect and reset rearm them.

Menus retain row/value arrows; clip SCALE/CHORD/ARP retains its existing arrows
and field ownership while following. A locked controller remains isolated when
the keyboard enters a clip page. MMC STOP retains STOP, separate from physical
Pause. Captions derive from the dispatch descriptors and captured action context.

## Knob banks

- HOME/parts MIX: default PAN (five pans plus existing decay/feedback/resonance),
  REV SEND, DLY SEND, then descriptor-derived useful parameter groups.
- Drum SEQ/MIX: PAN, REV SEND, DLY SEND, then successive voice parameter ordinals.
  Each populated strip names the actual parameter of its own drum voice; different
  voices can have different names or an inactive position in the same bank.
- Sound/FX/generator/master/other views: default visible page, then deduplicated
  groups of up to eight editable page and MORE descriptors. Fader-owned targets
  stay excluded. This exposes sound parameters such as Tune, Drive type, filter
  envelope, drum Drive/Dist, FX high-pass/ping and master compressor controls.

The native surface keeps separate knob/fader shapes, actual action captions,
drum abbreviations, active bank index/name and the touched parameter's full name.
The overlay expires or cancels on keyboard navigation; it does not restore stale
keyboard state. Screenshots are produced by the production native framebuffer
renderer in a headless host fixture, not by operating the user's GUI.

## Transport pattern navigation

Rewind/Forward (notes 0x5B/0x5C) navigate the playback pattern of the captured
part while locked, or selected part while following. During playback, each
separate press starts from that part's existing pending destination; reversing
back to the currently playing pattern cancels its pending cue. The existing
bar quantization, stopped service and bounded endpoints (0 and NPAT−1; no wrap
in the inspected owner-tested implementation) are retained. Physical held
replays do not accumulate presses. Existing scene/song cue state remains the
single authority; there is no separate SMC pending-pattern queue. These transport
buttons do not change drum track banks or knob banks. Bound locked editor knobs
keep their captured pattern destination when playback changes patterns; use an
intentional SMC editor/page navigation or unlock to retarget editing.

## Compatibility and acceptance

Only `smc_mixer.c` changes production behavior. There is no new stored project
format, controller firmware or transport protocol. Apply after the fader owner's
repair with a small overlap review around `smc_lock_toggle`; do not duplicate or
replace that owner's `smc_notice`/`smc_fader` state repair. Combined168/188/189
integration belongs to its existing owner. No GUI, device, flashing, merge or
binary publication is authorized here. Native budget evidence and independent
review accompany the final source receipt; an overflowing native image is not
installable and cannot proceed to rollout.
