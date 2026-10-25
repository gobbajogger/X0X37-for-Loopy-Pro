# SMC control context lock — private source stage

Base: installed combined PR144, `3862175d5815d2bd505ab04ca1c8e1868761a2e7`.
Owned isolated branch: `lane/smc-pause-context-lock-20261025`.

## Reason and intended effect

SMC controls currently follow the keyboard's live page and selection. A musician
who navigates the keyboard can therefore redirect the surface while performing.
The requested Pause toggle captures the control destinations, allowing the
keyboard UI to move independently; unlocking follows the current UI again.
The lock concerns control ownership. The temporary SMC display still expires
after 1.2 seconds and returns to the current keyboard UI.

The owner additionally requested a blinking Pause light while following and a
steady light while locked, to make ownership visible without reading the LCD.
Neither the physical Pause input nor its LED address is established in the
available labelled evidence. This stage must preserve existing transport and
leave physical activation and that light unbound until identification is proven.
Expected behavior is distinguished from host-tested behavior in the final receipt.

## Bounded plan and ownership

| Owner | Action | Done evidence | Stop condition |
| --- | --- | --- | --- |
| Mapping investigator | Check existing map, PR18 and earlier captures | Exact provenance or narrow identification gap | No guessed physical identity or hardware commands |
| Implementation worker | Freeze complete destinations and expose neutral lock transition | Reviewed source and production-state tests | Unsafe implicit target, unrelated transport change or budget failure |
| Target reviewer | Review pickup, pattern, song, kit and selection ownership | Independent disposition on the exact diff | A control can silently redirect |
| Independent test worker | Exercise production routing, state and UI transitions | Commands, exits and meaningful regression/negative evidence | No host result labelled physical proof |
| Native builder | Compile exact private Elite/Pro/controller inputs | Actual package hashes, models and budgets | No publication, installation or compiler-input substitution |
| Coordinator | Reconcile evidence, isolate and verify Git delivery | Draft PR and remote full SHA/tree | Protected refs and existing stage owners stay untouched |

## Physical identity evidence

The current `smc_protocol.h` maps 43 stock buttons. `smc_mixer.c` treats note
`0x5D` as STOP, `0x5E` as PLAY and `0x5F` as REC. Its note-`0x5D` light follows
stopped transport. Keyboard PLAY/PAUSE in `ui.c` is sequencer transport.

Old draft PR18, head `d29f365df8b60436749b6712ca9d605d2f912708`, calls the
stock STOP note Pause without a labelled physical observation. It is prior art,
not proof of the requested button. Note 53 / CC53 is an owner-history lead only.
The existing USB capture has 5,394 unlabelled messages and no note53, CC53 or
note93; the BLE capture is also unlabelled. Maintained device investigation docs
already warn that button-to-CC correspondence requires a labelled capture.
Raw captures and owner preset bytes are not part of this source change.

Existing direct-SMC BLE output supports note velocity 127/0 with success-only
state caching and reconnect invalidation. No documented native blink command
or physical Pause LED address was found. Once identified, a 500 ms nonblocking
phase can reuse that cache (at most two changes per second), independently of
sequencer playback. This is a future implementation choice, not implemented
or hardware-verified LED behavior.

Minimal identification needed: one labelled physical Pause press/release in
the current preset, raw message bytes, and that button's confirmed feedback
note/CC. Do not substitute the existing STOP event.

## Target policy

The implementation captures full fader and encoder parameter identities,
including pattern and song-bar identity where ordinary UI references resolve
through live selection. Main mixer engine identities remain global sound
parameters. Existing sound-motion recording follows the active sequencer
pattern; this lock does not change automation recording ownership.

Drum mute/Solo use the captured kit and exact eight-strip bank. Main strip
mute/Solo retain their existing fixed part/FX targets. Held Solo release keeps
the target from its original press, including across lock transitions.
Target-changing SMC navigation, bank/channel selection, SELECT and row REC
are parked during the lock. Global PLAY/STOP/REC retain their existing behavior;
global REC still uses the current keyboard part. Rewind/forward act on the
captured part's current playback pattern, while captured editor fields remain
bound to their original pattern. Unlocking discards old pickup crossing history
and resolves current UI targets. Confirmation keeps its input priority; R7 may
still dismiss Help.

| Input while locked | Scope |
| --- | --- |
| Faders 1–8 | Captured main/FX/drum parameter; empty drum-bank strips stay inactive |
| Knobs 1–8 | Captured visible refs, complete kit/voice/parameter and pattern/song identity; originally unassigned positions stay inactive |
| Mute row | Captured kit/bank, or existing fixed main part/FX mute |
| Solo row | Captured kit/bank or fixed main part; original press target owns release; existing User MMC latch fallback retained |
| Record / SELECT rows | Parked, because they change keyboard selection/record context |
| Bank/channel/view/page and main-row navigation shortcuts | Parked until unlock |
| PLAY / STOP / global REC | Existing global transport/record behavior, unchanged |
| Rewind / forward | Captured part's current playback pattern, bounded as before |
| Confirmation / Help | Existing confirmation confirm/cancel and Help-dismiss priority |

Entering the lock in a clip editor/grid is intentionally refused, because its
fields have separate stage-dependent ownership. Locking an ordinary page and
then navigating the keyboard into a clip view keeps the captured ordinary
targets. The song-bar navigation field is inactive while locked; removed song
bars must not be recreated by a stale bound reference. Song insert/delete/clear,
length change and successful undo invalidate captured song-bar knobs until
unlock. This conservatively includes append and undo of unrelated content.
Project reset drops the runtime lock. Reconnect retains the captured context,
clears the held button edge, resets pickup/crossing history and acceleration,
and resynchronizes the existing success-cached feedback. No Pause feedback is
added. Existing UI/device behavior still requires physical testing after binding.

The neutral `smc_lock_button(pressed)` main-loop entry point toggles only on a
rising edge, ignoring repeated presses and releases. It is deliberately called
by host tests only at this stage. No current MIDI event activates it. Native
linker removal of an unreferenced entry point must be disclosed separately from
host coverage and any diagnostic link that explicitly retains it.

No protected merge, binary publication, access change, GUI or device operation
is authorized by this document or private build evidence.
