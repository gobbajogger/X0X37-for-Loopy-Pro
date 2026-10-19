# Keyboard firmware UX review — 19 October 2026

## Scope and evidence

This review follows the SMK-37 as a musician plays it: keys, pads, faders, transport, clip mode, recording, and saving. It covers firmware behavior and the keyboard display, not a companion website. I read the control paths in `firmware/src/app/ui.c` and `main_fm1.c`, the sequencer and project storage code, `BEHAVIOR.md`, and the other active project's integration changes. Host tests exercise input and persistence; they do not replace a keyboard and Loopy Pro session.

The review branch contains fixes for note and step ownership, song mute persistence, fader pickup, queued save feedback and clip-safe saving, memory-full save preflight, and copy/paste source ownership. The final image-budget review is still in progress. Each fix has a focused host regression. The active SMC MIDI/Bluetooth thread owns its separate integration work.

## What the player experiences

| Moment | Before this review | Fix or current state | Remaining ambiguity |
| --- | --- | --- | --- |
| Play a 303 key, change octave or part, release the key | The sound could remain held because release used the new note/part | Release remembers the physical key and original voice; tests cover octave, part, REC, free play, and clip entry | Verify the real hardware MIDI route and sustained sound together |
| Hold a 303 step, then navigate | The old preview could keep sounding, and key-up could edit a different step | Preview owns its part/pattern and cancels on navigation | Step preview is audible only when the key-sound rule allows it; a player may expect every held step to sound |
| Record a song mute and save | The mute change did not mark the project dirty | The bar mute now marks dirty and survives reboot in a scenario test | The SONG screen still needs a device check for the visible mute state during a live bar change |
| Switch a fader bank | First small movement could jump the target to the physical fader value | The fader waits until it meets or crosses the stored value; a short hint shows the direction | Check the hint and takeover feel on the keyboard, including a target changed by a knob |
| Hold SAVE and choose another pattern | “COPIED” suggested a frozen clipboard, but paste read live source patterns later | UI now asks for a paste target and remembers the five source slots through replacement confirmation | Pattern *contents* remain live until confirmation; playback or generator edits during a prompt can change what is pasted |
| Save while the sequencer or clips are active | Save could appear delayed with little persistent explanation; clip activity could overlap flash work | Persistent UNSAVED / queued / failed marks, SAVE LED states, and clip activity quiet guard | REC armed while stopped suppresses autosave until REC is turned off; help states this, but the status mark does not explain the cause |
| Save a project that no longer fits | The first objects could be rewritten before stream capacity failed | A preflight detects the memory-full case before any storage write | Other I/O failures can still leave a partially updated project |

## Ambiguities still worth resolving

### 1. Clip transport ownership is easy to misread

In clip mode the PLAY and REC lamps follow Loopy/APC feedback, while the X0X sequencer and its REC arm remain separate state. Entering clip mode does not necessarily stop an armed X0X recording. A performer looking only at the two lamps can infer that X0X stopped recording when those lamps switched owners. A device test should cover enter/exit clip mode with X0X PLAY and REC in each state. The display should name both transport owners if the two states can be active together.

### 2. “WAITING FOR LOOPY PRO” is a feedback detector

The clip waiting screen disappears after the first clip or transport feedback message. It does not prove that a USB or Bluetooth link is absent or that Loopy is not running. A silent, connected Loopy session can therefore look disconnected. This wording is already an owner-approved behavior, so a change needs a keyboard/Loopy check and an explicit decision about whether the screen should report link state, feedback state, or both.

### 3. Clip grid position is hard to recover at a glance

The clip LCD draws colored squares and a white viewport frame, but no track or scene number. OCT paging changes the pad viewport and the physical rows are vertically flipped. After moving pages, a player must infer where the pads are from the frame and remembered layout. Small row/column or page labels would make navigation safer, especially when Loopy supplies an 8×4 grid across two pages. Check legibility on the 240 px display before adding text.

### 4. Flash quiet time needs an activity definition

The clip save guard checks the raw keybed and pending clip encoders, then waits four quiet seconds. It covers the controls that were making the flash save audible during a clip performance. External MIDI activity and outgoing notes should be checked on hardware to establish whether they also need to postpone flash. “SAVE WHEN QUIET” is the current display wording; it should match the actual audible-safe condition.

### 5. PLAY to Loopy still needs round-trip proof

A previous owner device check reported that clip PLAY did not start or stop Loopy. The code sends a momentary APC transport note, and host tests can check that packet, but they cannot establish Loopy's route or control assignment. Capture the outgoing MIDI packet and Loopy's returned PLAY feedback on the actual USB and Bluetooth paths before declaring the transport fixed.

### 6. The keybed changes meaning by screen and arm state

White and black keys select patterns and mutes on HOME, edit steps in SEQ/PART, play or record notes under 303 FREE PLAY/REC, and play Loopy notes in clip mode. The footer describes several roles, but the same physical key can make or edit data depending on REC and screen. A short, always-visible `KEYS: STEPS / PLAY / WRITE / LOOPY` state, or an equivalent LED convention, would reduce wrong-mode presses. Validate that it fits alongside save status and pattern/clock information.

### 7. Incoming SMC REC needs the same step cleanup as panel REC

The SMC integration initially toggled `ui.rec` directly. Panel REC first cancels a held 303 step preview because the key-up will change meaning from step edit to keyboard play. The review branch now calls the same cleanup in the SMC path. This needs a device check with a held step and the SMC REC control after the combined image is built.

## Release checks

1. Rebase this review branch after the active SMC MIDI/Bluetooth work settles; resolve overlapping clip and UI files with the integration owner.
2. Run the host suite, focused firmware tests, Pro and Elite builds, and the project image/headroom gates on the exact combined commit.
3. On the keyboard, test note release across mode changes, fader pickup in both banks, clip PLAY/REC round trips, clip paging, and save timing during sustained notes and external MIDI.
4. Only then package an image for installation. No device installation was part of this review.
