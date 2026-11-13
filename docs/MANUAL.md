# DnBOX

This is the end-user manual for the SMK-37 Elite and the SMK-37 Pro.

The product name is exactly DnBOX. Use that spelling in every player-facing sentence.

## Contents

1. This manual and the firmware version
2. The panel
3. First sound
4. First pattern
5. First bass line: Chicago, then Miami
6. First break
7. Save, and why it waits
8. Step versus sound
9. The mixer, the effects and the master
10. Patterns, chains and song
11. Recording and free play
12. Knob motion, p-locks and chance
13. GLOBE settings
14. MIDI, USB audio and Bluetooth
15. Loopy clip mode
16. Install and update safety
17. Troubleshooting
18. Expert shortcuts
19. Parameters
20. Credits
- Appendix A. Same key, different job
- Appendix B. Names from the older FM-1 build
- Appendix C. Protocol identifiers that must stay the same

---

## 1. This manual and the firmware version

DnBOX turns an SMK-37 Elite or SMK-37 Pro into a live groovebox. It has a 909, an 808, a BREAK player, and two bass slots.

The bass slots can run Chicago, Miami, Juno or Drone. Chicago is the 303 engine. Miami is never a 303. Juno and Drone are not 303 engines.

Elite and Pro share the same playing surface. The USB product name and the Bluetooth name differ. The controls do not.

**Goal.** A first-time player can hear a sound, write a pattern, and save it. Expert shortcuts stay in chapter 18. This book does not remove them.

**Version this book describes.** This manual matches source `94197fc8` (full id `94197fc839b00aff47b98ca02f3c6396e4a6eabf`). That source sits after the published tag `v0.0.25.1`. The Git describe string is `v0.0.25.1-87-g94197fc83`.

The open-beta ledger has not allocated a new number. This book does not call the firmware 0.0.26.

A published package must print the same `X0X_VERSION` that the build used. About and the USB product use the same words: `DnBOX` plus that version. If a build does not pass a version, About shows `DEV`. About can also append a short Git id as `g` plus the SHA when the build provides it.

**How to read this book.** Start with chapters 3 to 7. Those chapters take you from a cold start to a saved pattern. Later chapters cover the rest of the instrument. Chapter 18 keeps expert muscle memory in one place.

A companion file, `web/MANUAL.md`, covers install, samples and project files in the browser. That file is not a second control manual.

This book describes the strings and behaviour in source at this SHA. It does not describe proposed screens.

---

## 2. The panel

Hold any one button for 900 ms, with no other key or turn. A help card names the printed legend and says what the button does here. Then let go. The release does nothing.

For BT, the help card comes at 400 ms. For every other button, the hold is 900 ms.

Why: many printed words are not the job. BT is Save, not Bluetooth. NOTE REPEAT opens the mixer. KNOB BANK opens Loopy clip mode. The help card uses the word on the plastic.

The table below is the glossary for this whole book. The left column is the printed SMK legend. The middle column is the DnBOX function name this book uses. Use one term for one thing.

| Printed on the SMK | Function name | What it does |
|---|---|---|
| FX | FX | Reverb and delay. With SEQ: panic (all notes off). |
| SCALE | SCALE (list / yes) | Opens this screen as a list. Confirms a question. In clip mode, SCALE is a real scale control. |
| CHORD | CHORD (accent) | Hold plus a key: accent. In clip mode, CHORD is a real chord control. |
| NOTE REPEAT | NOTE REPEAT — mixer (when clip mode is off) | When clip mode is off, NOTE REPEAT is the mixer (MIX), not note-repeat. Hold plus a key: slide on Chicago. In clip mode, a release of NOTE REPEAT alone is panic. With ARP: LATCH. The lamp means the mixer. |
| PARA | PARA (sound) | Sound of the selected part. On a drum part, white keys audition only. They do not write steps. |
| GLOBE | GLOBE (settings) | Settings. Press again to close. Bluetooth lives here. |
| PATCH | PATCH — HOME / SONG / back / no | A tap (release, with no modifier) toggles HOME and SONG: from HOME it opens SONG, and from SONG it opens HOME. From every other view, that same tap opens HOME first. In a list: back. In a question: no. Hold as a modifier. |
| BT | BT — Save (not Bluetooth) | A tap (release before 250 ms) saves, or queues the save until stop. At 250 ms, still held alone: the toast is `CHOOSE PASTE TARGET`, then a white key pastes. If you let go after 250 ms and before 400 ms, with no paste, that release still saves. At 400 ms the help card appears, and the release does nothing. Bluetooth is GLOBE, then BLUETOOTH. The lamp means unsaved, queued or failed, not wireless. |
| ARP | ARP (TB-3PO) | Line generator. Chicago or Miami only. |
| SEQ | SEQ (steps) | Step grid or piano roll. A cold start lands here on the 909. |
| SEQ PLAY | SEQ PLAY | Start or stop, on press. |
| SEQ REC | SEQ REC | Record on or off, on press. |
| OCTAVE− / OCTAVE+ | OCTAVE− / OCTAVE+ | The meaning depends on the screen. See Appendix A. |
| `<` `>` | `<` `>` (page / list value) | With no list: one page step, no hold-repeat. In a list: the value. Hold-repeat is 400 ms, then 80 ms. |
| KNOB BANK | KNOB BANK — Loopy clips | A single press (the moment you press it, not a hold) toggles Loopy clip mode. The toast is `LOOPY CLIPS` or `LP OFF`. |
| FADER BANK | FADER BANK | Bank A or bank B. The LED is on for bank B. |
| K1 | MASTER | Volume. This knob has no taught role. |
| K2 | K2 (page / chance / tempo) | Page or list row. Held-step chance. PATCH plus K2 sets tempo. |
| K3 | K3 (part) | Walks 909, then 808, then BREAK, then Chicago, then Miami. In a list: the value. |
| K4 | K4 (pattern) | Cue a pattern. On HOME, all five parts. On a part screen, this part. This also ends a chain. |
| K5–K8 | K5–K8 | The four values at the bottom of the screen. |
| 16 pads | Pads | The pads follow the 16 white keys. |
| Faders 1–4 | Faders, bank A or B | Bank A: levels for 909, 808, BREAK and Chicago. Bank B: Miami level, reverb return, delay return, and master FILTER. Soft pickup. |
| Pitch / mod wheels | Pitch wheel / mod wheel | On a Chicago-style slot: bend and cutoff. On Miami: bend and wobble depth. The wheels do not write the saved pots. |
| 37-key keybed | Keys | The middle 27 keys match the older 27-key layout. The outer ten keys are live. Some outer keys also page BREAK slices. |

SEQ PLAY, SEQ REC and KNOB BANK act on the press. KNOB BANK needs only that single press. Almost every other button acts on release, and only if you did not already use it as a modifier.

On the step grid, with the keyboard off, the header shows the part chip and the word `SEQ`. A cold start shows the 909 and `SEQ`.

If record is on and the bass keyboard is on, that word is `SEQ KEYS`. If FREE PLAY is on, that word is `FREE PLAY`. On HOME the header word is `HOME`. On SONG it is `SONG`. In clip mode the left chip is `LP`.

On PARA, for a drum or a bass slot, the usual header is compact. It shows the part or engine name, page dots, the play icon, and a 2-pixel save dot if unsaved. It does not show the word `SEQ`. It does not show the word `SOUND`. It does not show the word `UNSAVED`. There is no `EDITING: SOUND` banner, and no `PROJECT SAVED` panel.

The PARA footer points you back to the steps: `SEQ: STEPS`. On a drum part it also says `BLACK: SELECT` and `WHITE: SOUND` (or `KEY SOUND OFF`). On a bass slot it says `KEYS: SOUND`.

---

## 3. First sound

The unit starts on the 909 step grid. The header shows the 909 chip and the word `SEQ`. Every part is empty. You are already on the right screen. You do not need to press SEQ.

Why: a cold start is ready for the first drum pattern. The first sound is a 909 kick on the grid.

Do not tap PATCH yet. A tap of PATCH (a release with no modifier) leaves SEQ and opens HOME. HOME keys make no sound.

Do not press KNOB BANK yet. A single press leaves this instrument and opens Loopy clip mode. The toast is `LOOPY CLIPS`.

If you tapped PATCH, you are on HOME. A second tap of PATCH opens SONG, not the grid. On HOME and SONG, a tap of PATCH toggles those two screens. Tap PATCH again for HOME, then press SEQ to return to the grid.

If you pressed KNOB BANK, press KNOB BANK once more. The toast is `LP OFF`.

While the loop does not run, press black key 1. This selects the bass drum (`BD`). You hear it.

Why: KEY SOUND defaults to `STOPPED`. A black key plays the drum only while the loop does not run, or while you record. While the pattern plays, a black key only selects the track. This avoids a stray hit on a live pattern.

Press white keys 1, 5, 9 and 13. These write steps on the selected track.

Why: on SEQ, a white key writes or removes a step. An empty step turns on immediately. A lit step turns off if you tap it for less than 300 ms. A longer hold keeps the step so you can set a p-lock.

Press SEQ PLAY. The loop starts. Press SEQ PLAY again to stop. Transport acts on the press.

---

## 4. First pattern

Stay on 909 SEQ.

Press black key 7. This selects the clap (`CP`). While stopped, you hear it. Press a white key to place a clap step.

Press black key 8. This selects the closed hat (`CH`) on the 909. Place hat steps the same way.

Why: each black key is one drum track. The 909 tracks in order are `BD`, `SD`, `LT`, `MT`, `HT`, `RS`, `CP`, `CH`, `OH`, `CR` and `RD`. The last row on the grid is accent (`AC`).

The 16 pads follow the 16 white keys. You can write the same steps on the pads.

On SEQ, the SEQUENCE page sets LENGTH and RATE on K5 and K6. The default rate names are `1/16`, `1/16T`, `1/32`, `1/8T`, `1/8` and `1/4`. Length can run to 64 steps. OCTAVE+ and OCTAVE− move the visible 16-step page (`1-16`, `17-32`, `33-48`, `49-64`).

Swing for the whole pattern sits on HOME, on the `PERFORM` page (K6), not on this SEQ page.

To mute a track from SEQ, hold PATCH and press that track's black key. If you release PATCH with no key, the view changes to HOME. See chapter 3 for how to return.

---

## 5. First bass line: Chicago, then Miami

Turn K3 clockwise from the 909. The part order is 909, then 808, then BREAK, then Chicago, then Miami.

From 909: one click is 808. Two clicks are BREAK. Three clicks are Chicago. Four clicks are Miami.

Why: the screen follows this display order, not an older "two 303s then a break" tour. Count the clicks against this order.

### Chicago

Chicago is the 303 engine. It is the default engine on bass slot B. On HOME, Chicago sits after BREAK, as the fourth of the five lanes. The default MIDI channel for bass slot B is 3.

On SEQ, the white keys are steps in a piano roll. Hold a step and turn K5–K8 to edit that step. CHORD plus a key writes an accent. NOTE REPEAT plus a key writes a slide. Accent and slide are audible on Chicago.

Press PARA to shape the sound. Chicago's first pages are OSCILLATOR and FILTER 1, then FILTER 2, ENVELOPE, AMPLIFIER, ARPEGGIATOR and EFFECT. The first FILTER 1 knobs are CUTOFF, PEAK and ENV. There is no leftover TONE or VOICE page from an older 303 engine.

Press ARP for TB-3PO, the line generator. On that screen, OCTAVE+ writes a new line. OCTAVE− mutates the line. TB-3PO runs on Chicago or Miami only.

### Miami

Turn K3 one more click to Miami. Miami is never a 303. It is the default engine on bass slot A. On HOME, Miami is the rightmost lane when both slots still have their defaults. The default MIDI channel for bass slot A is 2.

Miami ignores accent and slide. A CHORD or NOTE REPEAT flag that you store still has no Miami accent or slide sound.

Press PARA to shape Miami. The pages are CARRIER, XMOD, FILTER, WOBBLE and OUTPUT. The first FILTER knobs include CUTOFF, CRUSH and WOBBLE. The first sound you hear depends on this engine, not on Chicago's pages.

The two bass slots are not identical. They can run different engines. Their defaults are already different.

### Juno and Drone

Each bass slot can also run Juno or Drone. Choose the engine on the PARA SOUND page (`CHICAGO`, `MIAMI`, `DRONE`, `JUNO`).

ARP does not run on Juno or Drone. If you press ARP there, the toast is `TB-3PO: PICK CHICAGO OR MIAMI`.

Juno and Drone can record a held-key chord. If the chord does not fit, the toast is `CHORD NOT SAVED`.

Do not call Juno or Drone a 303.

### The bass keyboard

When the keyboard is on, the 37 physical keys play the selected bass slot. The keyboard is on in PARA, in record, or in FREE PLAY. FREE PLAY is a SEQ KEYS page. The keys play and write nothing. Strike velocity is in use on this path.

---

## 6. First break

From the 909, turn K3 two clicks clockwise to BREAK. Do not turn four clicks.

On SEQ, the white keys are the bar, not a drum track. This is a sample player. The feel is different from a 909 or 808 grid.

SEQ pages for BREAK include SEQUENCE (RATE: `1/16`, `1/8` or `1/4`), GROOVE and RETRIG.

PARA opens LOOPS. There you pick LOOP A and LOOP B, and you set slice counts (`A SLICES`, `B SLICES`: `4`, `8`, `16`, `32`).

Some outer keys page the BREAK slices. The middle keys still play the live path when the keyboard owns them.

To put your own loops on the unit, use the browser sample tools in `web/MANUAL.md`. This control book does not repeat that procedure.

---

## 7. Save, and why it waits

The printed legend on the Save button is **BT**. A tap of BT saves the project, or queues the save until it is safe.

Bluetooth is not this button. Bluetooth lives in GLOBE, in the BLUETOOTH section.

Why the wait: a flash write stops the audio. The unit does not write during play, during record, or while notes or clip activity are busy. If you tap BT then, the toast is `SAVES WHEN STOPPED`, or `SAVES WHEN QUIET`, or `REC OFF TO SAVE`.

The usual header can show `UNSAVED`, `SAVE AT STOP`, `SAVE WHEN QUIET` or `REC OFF TO SAVE`. If the left side is already full, that shrinks to `*`, `Q` or `!`. A 2-pixel amber (or red) dot stays at the right. On the PARA compact header, only that 2-pixel dot appears. There is no `SAVE QUEUED` title, and no `PROJECT SAVED` panel.

The unit writes only after you stop and the machine stays quiet. The toast is then `SAVED`, for 1300 ms. Failure toasts include `SAVE FAILED`, `NEWER DATA: NOT SAVED` and `MEMORY FULL: NOT SAVED`.

The BT lamp stays on while a save waits. It blinks for unsaved edits. It blinks fast after a save fails.

Autosave is on by default. After 4 s of quiet, while stopped, the unit can save on its own, at most every 20 s. The toast is `AUTOSAVED`. You can turn autosave off in GLOBE, under PROJECT.

To load the last saved project, open GLOBE, then PROJECT, then `LOAD SAVED`. If you have unsaved edits, the question is `LOAD SAVED PROJECT?` with `DISCARD UNSAVED EDITS`. SCALE is yes. PATCH is no.

The project does not save a chain. A chain is live only. See chapter 10.

To copy, hold BT for 250 ms, still alone. The toast is `CHOOSE PASTE TARGET`. Then press a white key to paste. If you let go after 250 ms and before 400 ms, with no paste target, that release still saves. At 400 ms the help card appears, and the release does not save. Hold BT plus OCTAVE to pick the copy bank (`P1-16` or `P17-32`).

---

## 8. Step versus sound

Keep this split. It is easy to confuse, and an older book was wrong here.

| | SEQ (steps) | PARA (sound) |
|---|---|---|
| Enter | Press SEQ. The SEQ lamp is on. The header word is `SEQ` (or `SEQ KEYS` / `FREE PLAY`). | Press PARA. The PARA lamp is on. |
| What you see | The drum kit grid, or a bass piano roll. | The sound pages. The usual header is compact: the part or engine name, and a 2-pixel save dot if unsaved. It does not show `SEQ`, `SOUND` or `UNSAVED`. |
| White keys on a drum part | Write or remove steps on the selected track. | **Audition only.** The white key plays the selected sound. It never writes a hidden step. The footer is `WHITE: SOUND` (or `KEY SOUND OFF`) and `SEQ: STEPS`. |
| White keys on a bass slot | Steps in the piano roll, when the keyboard is off. | The keys can play the sound. The footer is `KEYS: SOUND` and `SEQ: STEPS`. |
| Hold a step plus K5–K8 | A p-lock for that step. The tag is `P-LOCK STEP n`. | The same p-lock if you still hold the step. |

Why the audition rule: PARA white keys must not write steps. A player who opens PARA to change the sound must not also change the pattern. The step stays on SEQ. The sound stays on PARA.

If you hold a step and then try to leave PARA for SEQ, the toast is `RELEASE STEP FIRST`. Release the step, then press SEQ.

A turned knob replaces the bottom of the screen for 900 ms with a large readout. For a drum, the name looks like `BD  DECAY`. There is no banner that says `EDITING: SOUND` or `EDITING: THIS STEP ONLY`.

---

## 9. The mixer, the effects and the master

### The mixer (NOTE REPEAT)

When clip mode is off, NOTE REPEAT opens the mixer (MIX). Press NOTE REPEAT again for the next MIX page. The NOTE REPEAT lamp means you are on MIX, not that note-repeat is on.

The MIX pages are PARTS, 909 MIX, 808 MIX, MASTER and WAVE.

On PARTS, black keys 1 to 5 pick the part in display order: 909, 808, BREAK, Chicago, Miami. The knobs are LEVEL, PAN, REV and DLY. The footer is `BLACK 1-5: PART` and `PATCH+BLK: MUTE`.

909 MIX and 808 MIX set Level, Pan, Rev and Dly for the selected drum track.

MASTER starts with Drive, a one-knob COMP, PUMP and FILTER. Further knobs include THRESH, RATIO, ATTACK, RELEAS, MAKEUP, MIX, PUMP BY, RESO, LIMIT, Dist, GLUE and Volume. WAVE shows the same first four knobs with a scope.

K1 is MASTER volume at all times.

### Faders

Press FADER BANK to switch bank A and bank B. The LED is on for bank B. A bank change rearms pickup and toasts `FADERS:` plus the short labels.

Bank A sets levels for the first four displayed parts: 909, 808, BREAK and Chicago.

Bank B sets the fifth displayed part (Miami, when that slot still runs Miami), then reverb return, delay return, and master FILTER.

The faders use soft pickup. Until a fader reaches the saved value, or crosses it, the header shows `PICK Fn UP` or `PICK Fn DOWN`. Pattern and tempo stay visible.

### FX

Press FX for reverb and delay. Press FX again for the next page.

REVERB knobs include DECAY and TONE. DELAY knobs include TIME, FEEDBACK, TONE and TAPE (from clean digital into worn tape). Further knobs include REV LOW CUT, REV RETURN, DLY LOW CUT, DLY RETURN and PING-PONG.

Each part's send levels live on the mixer, not on this FX screen.

### SMC

If you have an SMC mixer on the link, the HOME footer shows `SMC F1-5: PART LEVEL` and `F6: REV  F7: DLY  F8: FILT`. The SMC has its own pickup words. With no SMC mixer on the link, the HOME footer is `WHITE: PATTERN`, `TWO: CHAIN` and `BLACK: MUTE`. Until you press a button, the footer can say `HOLD ANY BUTTON: WHAT IT DOES`.

---

## 10. Patterns, chains and song

There are 32 patterns, in two banks: 1 to 16 and 17 to 32. On HOME or SONG, OCTAVE+ shows 17–32. OCTAVE− shows 1–16.

### HOME

A tap of PATCH (a release with no modifier) opens HOME from most views. The header word is `HOME`. The screen shows all five parts of the current pattern, in display order: 909, 808, BREAK, Chicago, Miami.

On HOME, a white key (or a pad) selects a pattern. Two white keys make a **chain**. A live chain shows a small amber `CHN`. An amber underline marks chained slots. A chain is live only. The project does not save it.

Why: a chain is a performance tool. A saved project must not surprise you with a chain that you did not mean to keep.

One pattern selection ends the chain. K4 also ends the chain. K4 cues a pattern. A pending cue shows a NEXT glyph and a blinking pattern number. After the cue plays, that number is the current pattern.

On HOME, black keys mute parts in **display order**. Black key 1 mutes 909. Black key 2 mutes 808. Black key 3 mutes BREAK. Black key 4 mutes Chicago (the fourth displayed part). Black key 5 mutes Miami (the fifth displayed part, by default).

The HOME `PERFORM` page has TEMPO, `SWING`, FILTER and PUMP. Default tempo is 125 bpm. The tempo range is 20 to 275 bpm. PATCH plus K2 also sets tempo from other screens.

A cue, a mute or a pattern change from HOME does not play a sound on the keys. To write steps again, press SEQ.

### SONG

If you are already on HOME, a tap of PATCH opens SONG. The header word is `SONG`. From SONG, a tap of PATCH returns to HOME. That is the toggle: HOME, then SONG, then HOME. There is no PATCH path straight back to SEQ. Press SEQ for the grid.

On SONG, white keys pick the pattern for a song bar. SCALE opens a list with bar length, insert, delete and clear. Song length can run to 192 bars.

If you press SEQ PLAY while the SONG screen is up, song mode stays on even after you leave SONG.

---

## 11. Recording and free play

Press SEQ REC to turn record on or off. This acts on press. The toast is `RECORD ON: KEYS + KNOBS` or `RECORD OFF`. A red record dot shows in the header. The SEQ REC lamp is on while record is on.

On a bass slot, the keys play and write notes while record is on. On a drum part, black keys and knob moves record.

CHORD plus a key writes an accent (on Chicago, an accent is audible). NOTE REPEAT plus a key writes a slide (on Chicago, a slide is audible). On the SEQ keyboard, NOTE REPEAT plus OCTAVE writes a tie. CHORD plus OCTAVE writes a rest.

FREE PLAY sits on the SEQ KEYS page. The keys play the bass slot and write nothing. The header mode is `FREE PLAY`. The footer can say `FREE PLAY: KEYS PLAY`. Strike velocity is in use.

External MIDI on a part's channel can record that part while record is on. This book does not claim a trial on a specific host.

If a recorded chord does not fit, the toast is `CHORD NOT SAVED`. Other record toasts include `NOTE SOUND NOT SAVED` and `REC NEEDS FWD/REV`.

---

## 12. Knob motion, p-locks and chance

Hold a step and turn K5–K8. This writes a p-lock for that step only. The knob tag shows `P-LOCK STEP n`. On a bass step, the roll also shows a `STEP n` line with the note and velocity.

Hold a step and turn K2 to set chance (probability) for that step. On a bass step, hold the step and turn K3 to set velocity.

BT plus a knob forgets that knob's motion. BT plus a knob on a p-lock removes the lock.

With record on, knob moves can record motion. The motion plays back with the pattern.

A chance miss is a rest for both the internal sound and MIDI out.

---

## 13. GLOBE settings

Press GLOBE to open settings. The list title is `GLOBAL`. Press GLOBE again to close and return to the previous view. PATCH in a subsection goes back one level. PATCH at the root closes.

SCALE opens a screen as a list, runs a list row, or answers yes to a question.

The six sections are `MIDI + CLOCK`, `BLUETOOTH`, `AUDIO`, `KEYS + SCREEN`, `PROJECT` and `SYSTEM`.

**MIDI + CLOCK.** `CLOCK SOURCE` is `AUTO`, `INTERNAL` or `EXTERNAL`. `CLOCK OUT` and `NOTES OUT` are on or off. Each part has a MIDI channel (see chapter 14). On the SMK there is also `CLIP KEYS CH` for clip mode.

**BLUETOOTH.** `BLUETOOTH` is on or off. `BT ROLE` is `DEVICE` or `SMC HOST`. `SMC PRESET` is `DAW` or `USER`. `STATUS` and `NAME` are read only.

The name is the advertised Bluetooth name (chapter 14). If the controller only starts at power-on, a change can toast `RESTART TO TURN ON`. A later flush can toast `BLUETOOTH SAVED`. This settings bit is separate from the project save on the BT button.

**AUDIO.** `OUTPUT` is `NORMAL` or `LOUD`. `USB AUDIO` is `FOLLOWS KNOB` or `FULL`. `TUNE` is a global bass fine-tune for the slots together. Each part has an on/off row (`909`, `808`, `BASS A`, `BASS B`, `BREAK`). `QUEUE MUTES` and `NOTE LOCK` also live here.

**KEYS + SCREEN.** `LIGHTS` is `OFF`, `ON` or `KEYS`. `KEY SOUND` is `STOPPED` or `ALWAYS`. `THEME` is `GREEN`, `AMBER`, `CYAN`, `RED`, `MONO` or `LIGHT`.

**PROJECT.** `SAVE PROJECT`, `LOAD SAVED`, `AUTOSAVE`, `CLEAR PATTERN` and `RESET PROJECT`. Reset asks `RESET PROJECT?` with `SAMPLES ARE KEPT`.

**SYSTEM.** `PERFORMANCE`, `ABOUT DnBOX` and `BATTERY` (read only). About shows `ABOUT DnBOX` and a line that starts `DnBOX ` plus the version.

---

## 14. MIDI, USB audio and Bluetooth

### What the computer shows

The USB product string is `DnBOX ` plus the version plus ` SMK-37 Elite`, or the same with ` SMK-37 Pro`. The USB manufacturer string is `X0X`. Hosts typically show the product string. This pass did not check the exact words on each operating system.

There is no TRS MIDI IN on the SMK. The SMK TRS jack is a MIDI OUT jack. The firmware does not use it. Do not look for a MIDI IN jack on this keyboard.

### MIDI channels

Default channels stay with the slot, not with the engine name:

| Slot | Default engine | Default channel |
|---|---|---|
| 909 | 909 | 10 |
| 808 | 808 | 11 |
| Bass A | Miami | 2 |
| Bass B | Chicago | 3 |
| BREAK | BREAK | 4 |

A channel of OFF sends and takes no notes. On a bass slot, the on-screen row uses the engine display name plus ` MIDI CH`. The slot itself does not move. Saved data and MIDI maps keep these slot IDs.

### Clock

If the unit waits for an external clock, SEQ PLAY blinks slowly and the toast is `WAITING FOR CLOCK`. Tempo in the header is amber when the clock is external.

### USB audio

USB audio carries the master output, and the host can also send audio back. `USB AUDIO` in GLOBE selects `FOLLOWS KNOB` or `FULL`. The USB product string is the same as for MIDI. This pass did not check exact host wording.

### Bluetooth

Bluetooth MIDI is in GLOBE, not on the BT button.

The advertised and GATT name is `SMK-37 Elite-` plus four hex digits, or `SMK-37 Pro-` plus four hex digits, for example `SMK-37 Pro-A1B2`. The four-hex suffix comes from a stable address. The `X0X-` prefix is not the SMK name.

The BLE-MIDI service UUID and characteristic UUID are the standard BLE-MIDI values. See Appendix C. Those identifiers must stay the same.

The unit answers APC Device Inquiry over Bluetooth, with the same identity as over USB.

### Wheels and MIDI

On Chicago, the pitch wheel bends pitch, and the mod wheel opens cutoff, without writing saved pots. On Miami, the mod wheel moves wobble depth. A gesture stays with the bass slot it started on. Record can record bend motion.

---

## 15. Loopy clip mode

A single press of KNOB BANK (the moment you press it, not a hold) enters clip mode. The toast is `LOOPY CLIPS`. The header shows `LP`. This leaves the normal instrument UI.

Why this chapter sits after the first-path chapters: that one press leaves the normal instrument. SCALE, CHORD, ARP and NOTE REPEAT then belong to the Loopy clip keybed. Know how to leave before you use it live.

A single press of KNOB BANK again returns. The toast is `LP OFF`.

In clip mode, NOTE REPEAT is not the mixer:

- SCALE is a real scale control.
- CHORD is a real chord control.
- ARP belongs to the clip keybed.
- NOTE REPEAT alone, on release, is panic: all notes off, and control change 123. The toast is `PANIC`.
- NOTE REPEAT plus ARP toggles LATCH. The toast is `LATCH ON` or `LATCH OFF`.
- PATCH plus SEQ REC and PATCH plus SEQ PLAY are Loopy history, not the DnBOX undo and redo. PATCH release does not change the DnBOX view.
- Loopy owns the SEQ PLAY and SEQ REC lamps. SEQ PLAY also sends the clip play note unless you hold PATCH.

Do not treat NOTE REPEAT as the mixer while clip mode is on.

DnBOX still identifies itself to Loopy as an APC40 mkII. That identity is a protocol identifier. See Appendix C. It must stay the same. Loopy mappings depend on that identity, not on the USB product string.

---

## 16. Install and update safety

Install from a browser that provides Web MIDI with SysEx. The current installer site is <https://gobbajogger.github.io/X0X37-for-Loopy-Pro/install/>. The companion file `web/MANUAL.md` describes the browser steps, sample backup and project files.

**Check the model printed on the keyboard**, Elite or Pro, not only the USB name. An older Elite can report Pro. A shared USB name does not prove the physical model.

A wrong-model write can make the keyboard unusable. Cancel writes nothing. The installer already warns in these words. This book repeats that warning because a name mix-up has bricked a unit.

The package identity that admits a write is `SMK-37 Elite_900` or `SMK-37 Pro_900` (release builds append a version code). That identity is not the everyday USB product string. Do not choose Elite or Pro by reading the USB product name.

USB VID and PID, OTA SysEx, and the stock update-record ids must stay the same. See Appendix C. This book does not propose a rename.

While the unit is in the update loader, the USB product is `SMK-37 Elite Update` or `SMK-37 Pro Update`.

Stop playback and recording before an install. Keep USB connected until the operation ends. If samples are at risk, back them up first, as the installer asks.

MKE-P37 is the same hardware with a third name: USB product `DnBOX ` plus version plus ` MKE-P37`, package identity `MKE-P37_900`, Bluetooth `MKE-P37-` plus four hex digits. Starrykey 37 Play uses the Pro package.

---

## 17. Troubleshooting

**No sound after you press keys.** You are likely on HOME or SONG. HOME keys select patterns and mute parts. They do not play sound. Press SEQ to return to the grid. If you are on SONG, press PATCH for HOME, then press SEQ.

**The screen says `LOOPY CLIPS` or shows `LP`.** Press KNOB BANK again for `LP OFF`.

**The toast is `RELEASE STEP FIRST`.** Release the held step, then press SEQ.

**BT did not write immediately.** If the loop was running or you were recording, the unit queues the save. The toast is `SAVES WHEN STOPPED` or a quiet or record variant. Stop, and let the unit go quiet. Then look for `SAVED`.

**You wanted Bluetooth and pressed BT.** BT is Save. Open GLOBE, then BLUETOOTH.

**The toast is `TB-3PO: PICK CHICAGO OR MIAMI`.** The selected slot is Juno or Drone, or another engine that is not Chicago or Miami. Pick Chicago or Miami, or leave ARP.

**The toast is `WAITING FOR CLOCK`.** The unit waits for an external clock. Set `CLOCK SOURCE` in GLOBE, or start the host clock.

**The screen says `DnBOX CRASH`.** The firmware halted. The title is `DnBOX CRASH`, not an older product name. Power off and on.

If the unit starts twice and fails, you can reach `DnBOX SAFE MODE`. In safe mode, nothing that makes sound runs. Use the update loader and a verified package for your printed model.

**An install made the unit unusable.** See chapter 16. Use a verified package for the model printed on the keyboard. Do not pick the package from the USB name alone.

**Faders do nothing at first.** They wait for pickup. Move each fader to the saved position, or across it. The header shows `PICK Fn UP` or `PICK Fn DOWN` until then.

**You opened PARA to change a drum pattern and the unit wrote no step.** PARA white keys audition only. Press SEQ, then write the steps.

---

## 18. Expert shortcuts

These shortcuts stay. They are part of live muscle memory. This book documents them. It does not ask for their removal.

Transport is on press. Almost every other button is on release. A hold that you use as a modifier consumes the release, so the view does not change. A PATCH release with no modifier does change the view. That is why a missed mute combo can land you on HOME.

| Shortcut | What it does |
|---|---|
| PATCH + SEQ REC | Undo (DnBOX). In clip mode: Loopy undo. |
| PATCH + SEQ PLAY | Redo (DnBOX). In clip mode: Loopy redo. |
| PATCH + K2 | Tempo. |
| PATCH + white key | Select a pattern from any screen. |
| PATCH + black key | Mute a track or a mixer channel. |
| BT + white key | Paste, after a copy hold. |
| BT + knob | Forget that knob's motion, or remove a p-lock. |
| BT + SEQ REC | Ask to clear this part. |
| Hold a step + K5–K8 | P-lock. |
| Hold a step + K2 | Chance. |
| Hold a bass step + K3 | Velocity. |
| CHORD + key | Accent. |
| NOTE REPEAT + key | Slide on Chicago. |
| NOTE REPEAT + OCTAVE | Tie, on the SEQ keyboard. |
| CHORD + OCTAVE | Rest, on the SEQ keyboard. |
| FX + SEQ | Panic, all notes off. The view does not change. |
| Two white keys on HOME | Chain (live only, not saved). |
| KNOB BANK | Toggle Loopy clip mode (`LOOPY CLIPS` / `LP OFF`). |
| K4 | Cue a pattern and end a chain. |
| Fader move after pickup | Set the same value as the matching knob. |
| Pitch and mod wheels | Bend or open a filter-related control, without writing saved pots. |

OCTAVE+ at power-on together with OCTAVE− starts HARDWARE CALIBRATION. That screen re-teaches every button and K2–K8. It never teaches K1.

A live bass note cancels if the owner, the arp, or the context changes. This is so a held key does not keep sounding in the wrong place.

---

## 19. Parameters

These names come from the control tables at this SHA. They are the words on the PARA and FX pages. They are not an older TONE / VOICE list.

### Chicago (the 303 engine)

| Page | K5–K8 |
|---|---|
| OSCILLATOR | WAVE, GLIDE |
| FILTER 1 | CUTOFF, PEAK, ENV |
| FILTER 2 | BITE, GNAW |
| ENVELOPE | ATTACK, DECAY, SUSTAIN, RELEASE |
| AMPLIFIER | ENV/GATE, LEVEL |
| ARPEGGIATOR | ON, RANGE, MODE, SPEED |
| EFFECT | TYPE, EDIT 1, EDIT 2 |
| SOUND | engine, preset, status |

The ARPEGGIATOR knob order on the page is ON, RANGE, MODE, SPEED.

### Miami (never a 303)

| Page | K5–K8 |
|---|---|
| CARRIER | WAVE, LEVEL, SUB, PITCH |
| XMOD | WAVE, LEVEL, TUNE, DEPTH |
| FILTER | CUTOFF, CRUSH, WOBBLE |
| WOBBLE | SHAPE, RATE |
| OUTPUT | GLIDE, STEREO, OUTPUT |
| SOUND | engine, preset, status |

### Juno

| Page | K5–K8 |
|---|---|
| OSCILLATOR | WAVE, SUB, WIDTH, LEVEL |
| FILTER | CUTOFF, RES, ENV, HPF |
| ENVELOPE | ATTACK, DECAY, SUSTAIN, RELEASE |
| LFO/CHORUS | LFO RATE, LFO DEPTH, CHO RATE, CHORUS |
| SOUND | engine, preset, status |

### Drone

| Page | K5–K8 |
|---|---|
| STRING | REGISTER, BRIGHTNESS, FLUTTER |
| ENVELOPE | ATTACK, RELEASE, LEVEL |
| SOUND | engine, preset, status |

### 909 and 808

909 tracks: `BD`, `SD`, `LT`, `MT`, `HT`, `RS`, `CP`, `CH`, `OH`, `CR`, `RD`, then the kit. 808 tracks: `BD`, `SD`, `LT`, `MT`, `HT`, `RS`, `CP`, `CB`, `CY`, `OH`, `CH`, then the kit.

PARA shows that track's sound knobs, four at a time. The named rank order is Tune, Tone, Attack, Sound, Decay, Snappy, Tail, P.Dpth, Pitch. Not every track has every name. For 909 BD, the first PARA page is Tune, Attack, Decay and P.Dpth.

Level, Pan, Rev and Dly for a drum track live on the mixer (909 MIX / 808 MIX). Drive and Dist live in the SCALE list, not on the first PARA page. A KIT page has an ACCENT depth for the kit.

### BREAK

SEQ: SEQUENCE (RATE), GROOVE, RETRIG, and further groove fields in the list. PARA: LOOPS (`LOOP A`, `LOOP B`, `B CHANCE`, and the BREAK level), plus slice counts.

### FX and master

FX REVERB: DECAY, TONE. FX DELAY: TIME, FEEDBACK, TONE, TAPE. Further: REV LOW CUT, REV RETURN, DLY LOW CUT, DLY RETURN, PING-PONG.

MASTER first row: Drive, COMP, PUMP, FILTER. Further: THRESH, RATIO, ATTACK, RELEAS, MAKEUP, MIX, PUMP BY, RESO, LIMIT, Dist, GLUE, Volume.

HOME `PERFORM`: TEMPO (20–275 bpm, default 125), `SWING`, FILTER, PUMP.

### TB-3PO (ARP), Chicago or Miami only

GENERATE: DENS, ACCENT, SLIDE, OCTS. SCALE: ROOT, SCALE, OCTAVE, MUTATE. OCTAVE+ writes a new line. OCTAVE− mutates it.

### SEQ pages

Drums: LENGTH, RATE. Bass: LENGTH, RATE, DIR (`FWD`, `REV`, `PING`, `RND`), TRANSP, then PRESET (`PAT SOUND`) and KEYS (`FREE PLAY`).

---

## 20. Credits

DnBOX firmware is free software under the GNU General Public License, version 3. If you share the firmware, you must also share its source.

The 909 path comes from **9W9** by athousanddetails, based on **ER-99** by Matthew Cieplak. The 808 path comes from **8W8** by athousanddetails. The send reverb, delay and master stage are a port of 9W9 work, under GPL-3.0. The factory break generator comes from **BB Gen** by mestela.

Chicago is the 303-style engine in this firmware. Miami, Juno and Drone are also in this source. This book does not credit an older, removed 303 engine as the current bass sound.

The default UI font is Barlow Semi Condensed (SIL Open Font License).

M-VAVE and SMK-37 are trademarks of their owners. TR-808, TR-909 and TB-303 are Roland trademarks. DnBOX has no tie to M-VAVE, Roland or Hügelton Instruments.

---

## Appendix A. Same key, different job

The same physical control has more than one meaning. This table is the single map. The behaviour stays.

**White keys.** They mean steps on SEQ, patterns on HOME, and song-bar patterns on SONG. They also audition sound on PARA drums, and play bass notes when the keyboard is on. They take a paste while you hold BT. Two white keys on HOME set the ends of a chain.

**Black keys.** They select drum tracks, BREAK slices, and part mutes on HOME, in display order. They also pick a mixer channel on MIX. Hold PATCH and press a black key to mute.

**OCTAVE±.** They move step pages, and they switch pattern banks 1–16 and 17–32. They also set the bass keyboard octave, and the octave of a held step. On TB-3PO they write a new line or mutate it. While you hold BT, they pick the copy bank.

**PATCH.** A tap (unused release) toggles HOME and SONG. From any other view, that tap opens HOME. The same button is also back, no, and a modifier.

**NOTE REPEAT.** When clip mode is off, NOTE REPEAT is the mixer. When clip mode is on, a release of NOTE REPEAT alone is panic. With ARP in clip mode: LATCH. Held with a key: slide on Chicago.

**BT.** A tap (release before 250 ms) saves. At 250 ms, still held alone, the toast is `CHOOSE PASTE TARGET`. A release after 250 ms and before 400 ms, with no paste, still saves. At 400 ms the help card appears, and the release does nothing.

**SCALE.** List and yes, except in clip mode, where SCALE is scale.

---

## Appendix B. Names from the older FM-1 build

If you used the older FM-1 build, the printed legends and the book names were different. This appendix is the only place this manual uses those older names. New player text uses the SMK printed legends and the glossary in chapter 2.

| Older book or Felucca name | SMK printed legend | Use this now |
|---|---|---|
| EDIT | PARA | PARA (sound) |
| HOME (the button) | PATCH | PATCH. The HOME screen still uses the header word `HOME`. |
| SAVE | BT | BT — Save |
| GLO | GLOBE | GLOBE |
| SEL | SCALE | SCALE (list / yes) |
| ENV | CHORD | CHORD (accent) |
| LFO, MIX, MASTER (as the button) | NOTE REPEAT | NOTE REPEAT — mixer |
| ALGORITHM | K3 | K3 (part) |
| PRESETS | K4 | K4 (pattern) |
| SELECT (the knob) | K2 | K2 |
| KNOB 1–4 | K5–K8 | K5–K8 |
| PLAY, REC | SEQ PLAY, SEQ REC | SEQ PLAY, SEQ REC |
| OCT- / OCT+ | OCTAVE− / OCTAVE+ | OCTAVE− / OCTAVE+ |

The older book also used two player names for the bass slots, and it called both of them 303 engines. That was wrong for this firmware. Chicago is the 303 engine. Miami is never a 303. This book uses Chicago, Miami, Juno, Drone, Bass A and Bass B.

The older book used FM-1 panel drawings and an FM-1 install URL. This book is for the SMK-37 Elite and Pro. It has no FM-1 panel figure.

---

## Appendix C. Protocol identifiers that must stay the same

Player-facing names can say DnBOX. The identifiers below are what installers, M-UPGRADE, Loopy and the editor protocol use. They must stay the same. This book describes what the unit uses today. It does not propose a rename.

| Identifier | Elite | Pro | Must stay? |
|---|---|---|---|
| USB product (app) | `DnBOX <version> SMK-37 Elite` | `DnBOX <version> SMK-37 Pro` | Keep the `SMK-37` token. Do not use this string to choose a package. |
| USB manufacturer (app) | `X0X` | `X0X` | User-visible on some hosts. Changing it can change the port name. This book does not change it. |
| USB product (update loader) | `SMK-37 Elite Update` | `SMK-37 Pro Update` | Prefer to keep. The loader manufacturer string is `Hügelton Instruments`. |
| USB VID / PID (app) | `0x1209` / `0x0001` | Same | Yes. Never change. Shared by Elite and Pro. Not a model test. |
| USB VID / PID (loader) | `0x1209` / `0x0002` | Same | Yes. |
| Package / OTA identity | `SMK-37 Elite_900` (release adds a version code) | `SMK-37 Pro_900` | Yes. This admits a package. |
| Loader OTA identity | `ota-SMK-37 Elite_900` | `ota-SMK-37 Pro_900` | Yes. Hosts look for `ota-`. |
| Stock update-record parm id | `ota-SMK-37 Elite_016` | `ota-SMK-37 Pro_016` | Yes. So a factory package still applies. |
| Handshake SysEx | Query `F0 00 32 45 …`, command `0x11`. The identity reply is the package id. | Same mechanism | Yes. Port names must not gate discovery. |
| M-UPGRADE upgrade SysEx | `F0 22 24 35 7F F7` | Same | Yes. Soft key `F0 22 24 35 7D F7`. |
| BLE advertised / GATT name | `SMK-37 Elite-` plus four hex | `SMK-37 Pro-` plus four hex | The four-hex suffix and address derivation must stay. |
| BLE-MIDI service UUID | `03B80E5A-EDE8-4B33-A751-6CE34EC4C700` | Same | Yes. Standard BLE-MIDI. |
| BLE-MIDI characteristic UUID | `7772E5DB-3868-4112-A1A9-F2669D106BF3` | Same | Yes. Standard BLE-MIDI. |
| APC / Loopy identity | Manufacturer `0x47` (Akai), family `0x29` (APC40 mkII), firmware 1.2, device id `0x7F`. Introduction `F0 47 7F 29 61 …`. | Same | Yes. Loopy mappings depend on this, not on the USB product. |
| Editor INFO version string | `"X0X "` plus the version (24-char field) | Same | Treat as protocol until tools change with it. |
| Storage magic | `"X0X0"` and data family `"X0X"` | Same | Yes. |
| About, crash, boot | `ABOUT DnBOX`, `DnBOX CRASH`, `DnBOX SAFE MODE`, boot `DnBOX` and `909 808 CHICAGO MIAMI` | Same | Display. Already spelled DnBOX. The boot subtitle does not list BREAK, Juno or Drone. |

The onboard package handshake, not the USB display name, is the safe way to identify a unit for a write.
