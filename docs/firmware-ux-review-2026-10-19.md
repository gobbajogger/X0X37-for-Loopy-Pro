# Keyboard firmware UX review — 19 October 2026

## Scope and evidence

This review follows the SMK-37 as a musician plays it: keys, pads, faders, transport, clip mode, recording, and saving. It covers firmware behavior and the keyboard display, not a companion website. I read the control paths in `firmware/src/app/ui.c` and `main_fm1.c`, the sequencer and project storage code, `BEHAVIOR.md`, and the other active project's integration changes. Host tests exercise input and persistence; they do not replace a keyboard and Loopy Pro session.

The review branch contains fixes for note and step ownership, song mute persistence, fader pickup, queued save feedback and clip-safe saving, memory-full save preflight, copy/paste source ownership, and per-link APC MIDI ownership. Each fix has a focused host regression. The active SMC MIDI/Bluetooth thread owns its separate integration work. The final source revision `c565164` built for Elite and Pro; the controller source is unchanged from its independently validated build.

Final image margins are tight: Elite has 232 bytes of code space free and Pro has 248 bytes. Both have 556 bytes of main RAM free and 8,436 bytes in the project pool, 244 bytes above the 8 KiB guard. The controller image has 44 bytes of code space and 544 bytes of RAM free. These builds prove link and image checks, not musical behavior on the keyboard.

The 303 velocity format enlarged the project and undo shadow. Integration now uses a 16,128-byte undo history pool to preserve the 8 KiB runtime margin. A stress test retains 24 whole-pattern replacement undo entries instead of 30 with the former 20,480-byte pool; one-byte edits still reach the existing 32-entry limit. This is a deliberate capacity cost, not a change to saved project data.

## What the player experiences

| Moment | Before this review | Fix or current state | Remaining ambiguity |
| --- | --- | --- | --- |
| Play a 303 key, change octave or part, release the key | The sound could remain held because release used the new note/part | Release remembers the physical key and original voice; tests cover octave, part, REC, free play, and clip entry | Verify the real hardware MIDI route and sustained sound together |
| Hold a 303 step, then navigate | The old preview could keep sounding, and key-up could edit a different step | Preview owns its part/pattern and cancels on navigation | Step preview is audible only when the key-sound rule allows it; a player may expect every held step to sound |
| Record a song mute and save | The mute change did not mark the project dirty | The bar mute now marks dirty and survives reboot in a scenario test | The SONG screen still needs a device check for the visible mute state during a live bar change |
| Switch a fader bank | First small movement could jump the target to the physical fader value | The fader waits until it meets or crosses the stored value; a short hint shows the direction | Check the hint and takeover feel on the keyboard, including a target changed by a knob |
| Hold SAVE and choose another pattern | “COPIED” suggested a frozen clipboard, but paste read live source patterns later | UI now asks for a paste target and remembers the five source slots through replacement confirmation | Pattern *contents* remain live until confirmation; playback or generator edits during a prompt can change what is pasted |
| Save while the sequencer or clips are active | Save could appear delayed with little persistent explanation; clip activity could overlap flash work | UNSAVED / queued / failed marks, SAVE LED states, and a clip activity quiet guard for manual and automatic saves | REC armed while stopped suppresses autosave until REC is turned off; help states this, but the status mark does not explain the cause |
| Save a project that no longer fits | The first objects could be rewritten before stream capacity failed | A preflight detects the memory-full case before any storage write | Other I/O failures can still leave a partially updated project |
| Keep a 303 step held while selecting another part on SMC | A temporary parameter lock could remain audible on the old 303 after the page changed | The held step now cancels before part selection; host regression passes | Confirm the old sound returns on release and the new part remains selected |
| Play notes into X0X after leaving clip mode | APC ownership could consume ordinary MIDI notes 0–98 from both links while Loopy stayed connected | Handshake ownership is now per input link; focused host tests pass | Same-link APC feedback and playable notes are indistinguishable by note/channel alone; test USB and Bluetooth together |

## Ambiguities still worth resolving

### 1. Clip transport ownership is easy to misread

In clip mode the PLAY and REC lamps follow Loopy/APC feedback, while the X0X sequencer and its REC arm remain separate state. Entering clip mode does not necessarily stop an armed X0X recording. A performer looking only at the two lamps can infer that X0X stopped recording when those lamps switched owners. A device test should cover enter/exit clip mode with X0X PLAY and REC in each state. The display should name both transport owners if the two states can be active together.

### 2. “WAITING FOR LOOPY PRO” is a feedback detector

The clip waiting screen disappears after the first clip or transport feedback message. It does not prove that a USB or Bluetooth link is absent or that Loopy is not running. A silent, connected Loopy session can therefore look disconnected. This wording is already an owner-approved behavior, so a change needs a keyboard/Loopy check and an explicit decision about whether the screen should report link state, feedback state, or both.

### 3. Clip grid position is hard to recover at a glance

The clip LCD draws colored squares and a white viewport frame, but no track or scene number. OCT paging changes the pad viewport and the physical rows are vertically flipped. After moving pages, a player must infer where the pads are from the frame and remembered layout. Small row/column or page labels would make navigation safer, especially when Loopy supplies an 8×4 grid across two pages. Check legibility on the 240 px display before adding text.

### 4. Flash quiet time needs an activity definition

The clip save guard checks the raw keybed and pending clip encoders for both manual and automatic saves, then waits four quiet seconds. It covers the controls that were making the flash save audible during a clip performance. While X0X plays, the queued save says `SAVE AT STOP`; after X0X stops it says `SAVE WHEN QUIET`. External MIDI activity and outgoing notes should be checked on hardware to establish whether they also need to postpone flash.

### 5. PLAY to Loopy still needs round-trip proof

A previous owner device check reported that clip PLAY did not start or stop Loopy. The code sends a momentary APC transport note, and host tests can check that packet, but they cannot establish Loopy's route or control assignment. Capture the outgoing MIDI packet and Loopy's returned PLAY feedback on the actual USB and Bluetooth paths before declaring the transport fixed.

### 6. The keybed changes meaning by screen and arm state

White and black keys select patterns and mutes on HOME, edit steps in SEQ/PART, play or record notes under 303 FREE PLAY/REC, and play Loopy notes in clip mode. The footer describes several roles, but the same physical key can make or edit data depending on REC and screen. A short, always-visible `KEYS: STEPS / PLAY / WRITE / LOOPY` state, or an equivalent LED convention, would reduce wrong-mode presses. Validate that it fits alongside save status and pattern/clock information.

### 7. Incoming SMC REC needs the same step cleanup as panel REC

The SMC integration initially toggled `ui.rec` directly. Panel REC first cancels a held 303 step preview because the key-up will change meaning from step edit to keyboard play. The review branch now calls the same cleanup in the SMC path. This needs a device check with a held step and the SMC REC control after the combined image is built.

### 8. SMC part selection can outlive a temporary 303 lock

Changing part from SMC rebuilds the knob pages. A held 303 step can own a temporary parameter-lock preview, and releasing that preview restores controls from the *current* page. If part selection happens first, the old part's sound may stay at the preview value. The review branch now cancels the held step before changing part. Verify the host-tested fix on a stopped 303 with a held step, a sound-knob turn, and an SMC part select.

### 9. APC feedback and ordinary MIDI notes need separate ownership

CLP-11 intentionally suppresses APC feedback after a host handshake, including outside clip mode. The old receive path applied that ownership to incoming notes 0–98 on *both* links, which could swallow playable notes from a different MIDI keyboard. The review branch now scopes ownership to the link that sent the handshake and clears it when that link ends. A keyboard and APC host on the *same* USB or Bluetooth link remain ambiguous because APC LED feedback uses all MIDI channels and overlaps the playable note range. That requires a routing choice or a distinct MIDI source identifier, then a device test with both active.

## Release checks

1. Have the integration owner merge this review branch after the active SMC MIDI/Bluetooth work settles, then validate the exact integrated commit.
2. Re-run the host suite and Pro, Elite, controller image/headroom gates in the supported Linux build environment. The final review revision passed focused host checks and local image builds.
3. On the keyboard, test note release across mode changes, SMC part selection during a held 303 lock, fader pickup in both banks, clip PLAY/REC round trips, clip paging, APC feedback alongside played MIDI on both links, and save timing during sustained notes and external MIDI.
4. Only then package an image for installation. No device installation was part of this review.

The local Mac host run passed the affected UI and scenario tests. Its repository-wide result also reports unavailable reference DSP source trees and Linux-specific `flock`/Bash checks; those are environment limits, not a passing release gate. A repeat on the project's supported build environment is required.
