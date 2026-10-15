# X0X intended behaviour (SMK-37 and FM-1)

## 1. Purpose and use

This file is the intended-behaviour specification for X0X. Review agents and bug-hunt agents use it. It tells an intended behaviour from a defect.

USE-01. Before you report a defect, compare the behaviour with this file.
USE-02. Report a difference as a defect only if it contradicts a statement in this file.
USE-03. If this file does not cover a behaviour, report it as "unspecified". Do not report it as a defect.
USE-04. If two statements in this file disagree, or the code and a source document disagree, see section 18 (Open questions). Do not select one yourself.
USE-05. On the SMK-37, SMK37.md, APC40_IMPLEMENTATION.md and the [USER] items have priority over docs/MANUAL.md. The manual is for the FM-1.
USE-06. An agent that changes intended behaviour must update this file in the same task.
USE-07. A [CODE file:line] tag gives the line at the time of writing (2026-10-15). Lines can move. Search for the named function if the line does not agree.

Source tags:

- [MANUAL §n]: docs/MANUAL.md, section n.
- [SMK37]: SMK37.md.
- [APC]: APC40_IMPLEMENTATION.md, section "Implemented in source", unless a version is given.
- [PADMAP]: PAD_MAP.md.
- [DESIGN file]: docs/plans/file.
- [CODE file:line]: the statement was verified in the code. Paths are relative to firmware/src unless given.
- [USER]: a decision of the user. A [USER] item has priority over all other sources.

## 2. Global and hardware

### 2.1 Boards and builds

GEN-01. One source tree builds two boards: the FM-1 and the SMK-37. All SMK-37 code is inside BOARD_SMK37 conditions. [SMK37] [CODE x0x.c:11]
GEN-02. Changes for the SMK-37 must not change the FM-1 build. [SMK37]
GEN-03. The SMK-37 Pro build has the package identity "SMK-37 Pro_900". MODEL=elite gives "SMK-37 Elite_900". MODEL=mke gives "MKE-P37_900". [SMK37] [CODE x0x.c:47]
GEN-04. The Starrykey 37 Play uses the SMK-37 Pro build. [SMK37]
GEN-05. The installer refuses an FM-1 package on an SMK-37 and an SMK package on an FM-1. [SMK37]
GEN-06. The SMK-37 has no TRS MIDI input. The firmware does not use the SMK TRS jack. MIDI on the SMK-37 is USB only. [SMK37] [CODE x0x.c:55-65]
GEN-07. Bluetooth is for MIDI only. There is no Bluetooth audio. [USER]
GEN-08. BLE MIDI is not available yet. [USER]
GEN-09. Bluetooth settings will be in the GLOBAL (GLO) settings menu. The BT button keeps its current function, or it can have a function only in clip mode. [USER]
GEN-10. On the SMK-37, the BT button is SAVE. [SMK37] [CODE app/panel.c:28-40]
GEN-11. X0X uses both processor cores when the second core works. The 808 and the break run on the second core. If the second core reads data incorrectly, X0X continues with one core and shows "1 CORE: BAD READS". [MANUAL §15]
GEN-12. When the audio load is near the limit, X0X can stop 303 oversampling and shorten 808 tails. The screen then shows GUARD. This is intended. [MANUAL §15]
GEN-13. The output sample rate is 44.1 kHz. [CODE seq/sequencer.h:37] [CODE dsp/breaks.h:22]
GEN-14. Use 44.1 kHz wherever possible. Do not halve a sample rate. [USER]

### 2.2 SMK-37 controls (clip mode off)

GEN-20. Keys F3 to G5 (the middle 27 keys) are the 27 FM-1 keys. [SMK37]
GEN-21. The 10 outer keys (C3 to E3, G#5 to C6) do nothing. [SMK37] [CODE app/main_fm1.c:762-763]
GEN-22. The 16 pads are the 16 white keys as a step grid. Steps 1-8 are on the top row. Steps 9-16 are on the bottom row. On HOME and SONG, the pads are the patterns. [SMK37] [PADMAP] [CODE app/main_fm1.c:752-758]
GEN-23. On a 303 with REC on, the pads are the 303 keyboard. Low notes are on the bottom row. [SMK37] [PADMAP]
GEN-24. The pads show the colour of the part on the screen. A lit pad is at half level. A dim pad is at one eighth level. [SMK37] [CODE app/main_fm1.c:578-588]
GEN-25. The pad LED chain order is {0,15,14,13,12,11,10,9,1,2,3,4,5,6,7,8}. Changes must not change this table or the non-grid row swap. [PADMAP]
GEN-26. K1 is MASTER. K2, K3 and K4 are SELECT, ALGORITHM and PRESETS. K5 to K8 are KNOB 1 to KNOB 4. [SMK37] [CODE app/panel.c:28-40]
GEN-27. FX, SCALE, CHORD, NOTE REPEAT, PARA and GLOBE are FX, SEL, ENV, LFO, EDIT and GLO. PATCH is HOME. BT is SAVE. [SMK37]
GEN-28. ARP and SEQ are ARP and SEQ. SEQ PLAY and SEQ REC are PLAY and REC. OCTAVE- and OCTAVE+ are OCT- and OCT+. [SMK37]
GEN-29. "<" and ">" move SELECT one step back or forward. They do nothing while HOME is held. [SMK37] [CODE app/main_fm1.c:734]
GEN-30. FADER BANK switches the faders between bank A and bank B. The FADER BANK LED is on for bank B. [SMK37] [CODE app/main_fm1.c:603-609]
GEN-31. Bank A faders 1-4 set the levels of the 909, the 808, 303A and 303B. [SMK37] [CODE app/main_fm1.c:624-625]
GEN-32. Bank B faders 1-4 set the BREAK level, the reverb return, the delay return and the master FILTER. [SMK37] [CODE app/main_fm1.c:626-633]
GEN-33. A fader has an effect only after it moves. This is also true after a bank change. [SMK37] [CODE app/main_fm1.c:617-622]
GEN-34. A fader sets the same value as the related knob. The project keeps the value. With REC on, the fader records knob motion. [SMK37] [CODE app/main_fm1.c:637-638]
GEN-35. The pitch wheel changes the TUNE of the 303 on the screen by about +-1.5 semitones. It goes back when you release it. On other parts it does nothing. [SMK37] [CODE app/main_fm1.c:704-708]
GEN-36. The mod wheel opens the CUTOFF of the 303 on the screen from the saved value to the top. On other parts it does nothing. [SMK37] [CODE app/main_fm1.c:709-716]
GEN-37. The wheels change only the sound. They do not change saved values. [SMK37] [CODE app/main_fm1.c:694-701]
GEN-38. The keys and the pads play without velocity. [SMK37]
GEN-39. OCTAVE- and OCTAVE+ held at power-on start HARDWARE CALIBRATION. It teaches the buttons and K2 to K8 again. K1 is not taught. [SMK37] [CODE app/main_fm1.c:1035-1037] [CODE app/panel.c:100-101]
GEN-40. A short KNOB BANK press does nothing. A KNOB BANK hold of more than 1 s toggles clip mode (section 15). [SMK37] [CODE app/main_fm1.c:768-788] (Was OQ-01. Resolved 2026-10-15: SMK37.md now agrees with the code.)
GEN-41. The SMK-37 keybed has no LEDs. Black-key information (tracks, slices, part mutes) is on the screen only. [CODE app/plat_fm1.c:50-53]
GEN-42. The first start of X0X on an SMK-37 clears the data of another firmware in the store area. The screen then shows "OLD DATA CLEARED". [SMK37] [CODE storage.c:62-100] [CODE app/main_fm1.c:1045-1047]
GEN-43. The firmware never writes the calibration, the Bluetooth pairing or the key areas of the flash. [SMK37] [CODE ../hal/fm1_flash_map.h:24-83]
GEN-44. With FREE PLAY on and REC off, the keys play the 303 without writing notes, and the SMK pads stay in the step-grid layout. This is the current behaviour of the existing 303 FREE PLAY feature. The pad layout in this case is unspecified. The user accepts it. [USER] [CODE app/main_fm1.c:754-755] [CODE app/ui.c:128] (Was OQ-06. Resolved 2026-10-15.)

### 2.3 FM-1 controls

GEN-50. On the FM-1, the controls are as in the manual, section 3. [MANUAL §3]
GEN-51. Hold OCT- and OCT+ for 5 s to go into update mode. Release them before the countdown ends to cancel. [MANUAL §17] [CODE app/main_fm1.c:1071-1094]
GEN-52. Hold OCT- and OCT+ at power-on for calibration. [MANUAL §17]
GEN-53. After two crashes in a row at start, X0X starts in SAFE MODE. Safe mode has no sound, but USB works. [MANUAL §17] [CODE app/main_fm1.c:1151-1156]

## 3. 909 and 808

DRM-01. Each drum machine has 11 tracks, one on each black key. [MANUAL §5]
DRM-02. The 909 keys 1-11 are BD, SD, LT, MT, HT, RS, CP, CH, OH, CR, RD. [MANUAL §5]
DRM-03. The 808 keys 1-11 are BD, SD, LT (or conga), MT, HT, RS (or claves), CP (or maracas), CB, CY, OH, CH. [MANUAL §5]
DRM-04. On both machines, the closed hat stops the open hat. [MANUAL §5]
DRM-05. While the pattern plays, a black key selects a track without sound. GLO > KEY SOUND = ALWAYS changes this. With REC on, you always hear the key. [MANUAL §5] [MANUAL §10]
DRM-06. A white key turns an empty step on. A tap on a step that is on turns it off. [MANUAL §5]
DRM-07. Hold ENV and press white keys to set accents. An accent makes all drums on that step louder. [MANUAL §5]
DRM-08. A pattern has a maximum of 64 steps. The white keys show 16 steps at a time. OCT+ and OCT- select the group of 16. [MANUAL §5] [CODE seq/pattern.h:13]
DRM-09. Hold HOME and press a black key to mute a track. X0X does not save track mutes. [MANUAL §5]
DRM-10. The 909 hats and cymbals are samples. All other drum sounds are synthesized. [MANUAL §5]

## 4. 303A and 303B

BAS-01. There are two identical 303s. Each has its own sound and its own line. [MANUAL §6]
BAS-02. On SEQ, hold a step and turn the knobs: KNOB 1 sets the note, KNOB 2 the gate (REST, NOTE, TIE), KNOB 3 the accent, KNOB 4 the slide. [MANUAL §6]
BAS-03. A tie holds the previous note through the step. The note does not play again. [MANUAL §6]
BAS-04. With REC on, all 27 keys are a keyboard. A key played while another key is held slides. [MANUAL §6]
BAS-05. FREE PLAY = ON lets the keys play the 303 without writing notes. Both 303s have FREE PLAY OFF at power-on. [MANUAL §6] [CODE app/ui.c:128]
BAS-06. A 303 line has a LENGTH of 1 to 64 steps, and a DIRECTION of forward, reverse, ping-pong or random. [MANUAL §6]

## 5. TB-3PO

TB3-01. TB-3PO writes 303 lines. Select a 303 and press ARP. [MANUAL §7]
TB3-02. OCT+ writes a new line. OCT- mutates the current line (about one quarter of the steps change). [MANUAL §7]
TB3-03. The TB-3PO knobs do not change the line that plays. They set the next OCT+ or OCT-. [MANUAL §7]
TB3-04. The same settings and seed always give the same line. [MANUAL §7]

## 6. BREAK

### 6.1 Playback

BRK-01. The break is a loop cut into 8 equal slices. [MANUAL §8] [CODE dsp/breaks.h:2-10]
BRK-02. The sequencer steps trigger slices on a tempo-synced 8-slice loop. This is intended. [USER] [CODE dsp/breaks.c:489-552]
BRK-03. The white keys are the 16 steps of the bar. A step that is on lets the break sound on that step. A step that is off closes the gate (2 ms ramp). The generator continues under a closed gate. [MANUAL §8] [CODE dsp/breaks.c:442-445]
BRK-04. Black keys 1-8 play the live slices 1-8. A live slice acts as a gate while you hold the key. [USER] [CODE dsp/breaks.c:442-445]
BRK-05. When you hold more than one slice key, the last key that you pressed plays. This is intended. [USER] [CODE dsp/breaks.c:159-179]
BRK-06. When you release all slice keys, the generator plays again. [MANUAL §8]
BRK-07. Live slices also play while the sequencer is stopped. [CODE dsp/breaks.c:600]
BRK-08. Black key 9 plays reverse, key 10 half speed, key 11 stutter, while held. [MANUAL §8] [CODE dsp/breaks.c:467-475]
BRK-09. Hold a white key and press black key 1-8: that step always plays that slice of loop A. Press the same black key again to give the step back to the generator. [MANUAL §8] [CODE app/ui.c:2176-2188]
BRK-10. A held live slice key has priority over the slice of a step. [CODE dsp/breaks.c:536-537]
BRK-11. The loop follows the tempo by a change of playback speed. Thus the pitch follows the tempo. PITCH shifts the pitch on top. [MANUAL §8]
BRK-12. On MIDI, notes 36-43 on the BREAK channel play slices 1-8. [MANUAL §16] [CODE app/engine.c:515-516]
BRK-13. The generator settings (COMPLEXITY, ANCHOR, ROLL, FILL, RETRIG, PHRASE, A/B LENGTH, B CHANCE) and the steps, slices and loop choice are saved with the pattern. [MANUAL §8]

### 6.2 Loops and imported samples

BRK-20. Imported samples play only in BREAK mode. This is intended. [USER] [CODE app/engine.c:948-972]
BRK-21. The LOOP list has the built-in loops first ("909 GR", "909 FL"), then the imported loops as BR1, BR2 and so on. [MANUAL §8] [CODE app/engine.c:855-878]
BRK-22. The factory (built-in) breaks are 2 bars long. [USER] [CODE ../../tools/gen_builtin_break.c:14-15]
BRK-23. Do not halve the length of a factory break. [USER]
BRK-24. An imported loop gets a length of 1, 2, 4 or 8 bars. The firmware selects the length that puts the loop tempo nearest to 120 BPM, inside 80-180 BPM. [CODE app/engine.c:817-829]
BRK-25. If a pattern names an empty user slot, loop A plays the first built-in loop. [CODE app/engine.c:967-968]
BRK-26. Target format for loops: IMA ADPCM at 44.1 kHz. [USER] Current state: built-in loops and uploaded loops are IMA ADPCM at 22.05 kHz. See OQ-07. [CODE ../../tools/gen_builtin_break.c:133-134] [CODE ../../web/x0x_breaks.js:9]
BRK-27. The firmware reads the rate of each imported zone from the slot header. Thus it can play a 44.1 kHz zone. [CODE app/slots.c:10] [CODE app/plat_fm1.c:150]

## 7. Patterns

PAT-01. There are 32 patterns. [MANUAL §9] [CODE seq/pattern.h:12]
PAT-02. A pattern holds the steps, length and rate of each part, the break settings, the swing, the knob motion, the p-locks and the probabilities. [MANUAL §9]
PAT-03. Sounds, effects and master settings are not part of a pattern. [MANUAL §9]
PAT-04. Each part can play a different pattern. [MANUAL §9]
PAT-05. While the sequencer plays, a new pattern starts at the end of the current bar. While it is stopped, the new pattern starts at once. [MANUAL §9]
PAT-06. Hold two white keys on HOME to make a chain. A single pattern selection ends the chain. [MANUAL §9]
PAT-07. Hold SAVE to copy. Press a white key to paste. If the target has notes, X0X asks first. [MANUAL §9]
PAT-08. X0X does not save part mutes on HOME. A song keeps its mutes. [MANUAL §9]
PAT-09. The 909 length sets the bar length. The 909 pattern holds the swing for all parts. Swing goes from 50 % (straight) to 75 %. [MANUAL §9] [CODE seq/pattern.h:75]

## 8. Recording

REC-01. REC toggles recording. [MANUAL §10]
REC-02. Drums: with REC on and the pattern in play, black keys record hits. [MANUAL §10]
REC-03. 303 step input: with REC on and the pattern stopped, each key fills the next step from step 1. [MANUAL §10]
REC-04. With REC on and the pattern in play, a sound knob records knob motion. [MANUAL §10]
REC-05. On the SMK-37 with clip mode on, SEQ REC does not toggle X0X recording. It sends APC RECORD (section 15). [APC] [CODE app/ui.c:2442-2447]

## 9. Knob motion, p-locks and probability

MOT-01. Motion records only on the steps where you turned the knob. X0X fills short gaps. Playback glides from step to step. [MANUAL §11]
MOT-02. A knob turn without REC overrides the motion for one pass of the pattern. [MANUAL §11]
MOT-03. Hold SAVE and turn a knob to clear its motion. [MANUAL §11]
MOT-04. When the sequencer stops, each knob goes back to its own value. [MANUAL §11]
MOT-05. On EDIT, hold a step that is on and turn a sound knob to make a p-lock. On SEQ, a held step makes no p-lock. [MANUAL §11]
MOT-06. Hold a step and turn SELECT to set its chance: 100 % to 5 % in steps of 5. Past 5 %, cycles from 1:2 to 8:8. [MANUAL §11] [CODE seq/sequencer.h:56-57]
MOT-07. A skipped drum hit sends no MIDI note. A skipped 303 step is a rest. [MANUAL §11]
MOT-08. There is room for 160 motion lanes across all patterns. When it is full, the screen shows MOTION FULL. [MANUAL §11] [CODE seq/motion.h:25]

## 10. Song

SNG-01. A song has a maximum of 192 bars. [MANUAL §12] [CODE seq/sequencer.h:52]
SNG-02. A song bar can repeat 1 to 8 times. [MANUAL §12] [CODE seq/sequencer.h:68]
SNG-03. PLAY on the SONG screen plays the song from the selected bar. PLAY on another screen plays the patterns. [MANUAL §12]
SNG-04. REC and PLAY on the SONG screen record the song bar by bar. The recording replaces the old bars. [MANUAL §12]

## 11. Effects

FX-01. There is one stereo reverb and one delay. Each part and each drum track has sends to both. [MANUAL §13]
FX-02. TAPE at the far left (DIGI) is a clean 12-bit delay. Higher TAPE values give a tape delay. [MANUAL §13]
FX-03. A delay longer than 2 s plays at half that length. [MANUAL §18]

## 12. Mix and master

MIX-01. LFO opens the mixer: PARTS, 909 MIX, 808 MIX and MASTER. [MANUAL §14]
MIX-02. MASTER has DRIVE, COMP, PUMP and FILTER. [MANUAL §14]
MIX-03. PUMP also works when COMP is 0 and when the kick is muted. [MANUAL §14]
MIX-04. FILTER is off in the middle. Left is a low-pass. Right is a high-pass. [MANUAL §14]

## 13. Settings and saving

SET-01. GLO has five sections: MIDI, AUDIO, KEYS AND SCREEN, PROJECT, SYSTEM. [MANUAL §15]
SET-02. SAVE saves the sounds, all 32 patterns, the song, the knob motion, the tempo and the settings. [MANUAL §15]
SET-03. X0X never saves while the sequencer plays. A SAVE during play shows "SAVES WHEN STOPPED" and saves at stop. [MANUAL §15] [CODE app/ui.c:1302]
SET-04. AUTOSAVE is ON by default. It saves when the sequencer is stopped and nothing was touched for 4 s. [MANUAL §15] [CODE app/ui.c:4169]
SET-05. If the project does not fit, X0X shows "MEMORY FULL: NOT SAVED" and keeps the last saved project. [MANUAL §15] [CODE app/ui.c:4189]
SET-06. Undo keeps a maximum of 32 steps. Undo does not cover the tempo or the settings. Power-off clears the history. [MANUAL §3] [CODE app/undo.c:22]
SET-07. FACTORY RESET does not delete the saved project until you save. [MANUAL §15]
SET-08. Defaults of a new project: tempo 125 BPM, CLOCK OUT on, NOTES OUT off, KEY SOUND STOPPED, theme amber. [CODE app/project.c:36-48]
SET-09. An update keeps the project. [MANUAL §1]

## 14. MIDI and USB

MID-01. Over USB, the device shows as the MIDI device "X0X FM-1". This name is the same on the SMK-37. [MANUAL §16] [CODE usb.c:304] [APC 1.5-apc40]
MID-02. Default channels: 909 channel 10, 808 channel 11, 303A channel 2, 303B channel 3, BREAK channel 4. [MANUAL §16] [CODE seq/sequencer.c:28]
MID-03. GLO can set each part channel to 1-16 or OFF. OFF ignores incoming notes and sends no notes for that part. [MANUAL §15]
MID-04. 909 notes: 36 BD, 38 SD, 41/45/50 toms, 37 RS, 39 CP, 42 CH, 46 OH, 49 CR, 51 RD. 808: the same, with 56 CB and 49 CY. [MANUAL §16] [CODE app/engine.c:499-500]
MID-05. On a 303 channel, overlapping notes slide. Velocity 100 or more is an accent. [MANUAL §16] [CODE app/engine.c:505-509]
MID-06. X0X follows an incoming MIDI clock. Start, Stop and Continue start and stop the sequencer. [MANUAL §16] [CODE app/engine.c:470-474]
MID-07. When clip mode is off, the original sequencer and MIDI input path must stay unchanged. Firmware changes stay inside clip mode unless the user approves. [USER] (agent-rules 10)
MID-08. USB audio is a 2-channel input at 44.1 kHz. It carries the headphone signal. [MANUAL §16] [CODE usb.c:74]
MID-09. The firmware answers a Universal Device Inquiry as an Akai APC40 mkII (manufacturer 0x47, product 0x29). [APC] [CODE usb.c:200-216] [CODE usb.c:694-701]
MID-10. On all boards (also the FM-1), the APC40 identity reply and the APC40 introduction reply are not gated by the board. The user accepts both. They are not defects. [USER] [CODE usb.c:694-701] [CODE usb.c:702-710] (Was OQ-08. Resolved 2026-10-15 by the user.)
MID-11. A USB bus reset or a deconfiguration clears the stored APC clip state. [CODE usb.c:554-559] [CODE usb.c:1113-1117] [CODE app/main_fm1.c:341-361]

## 15. Clip mode (APC40 mkII emulation for Loopy Pro), SMK-37 only

### 15.1 Mode

CLP-01. Clip mode exists only on the SMK-37 build. [CODE app/main_fm1.c:170]
CLP-02. A KNOB BANK hold of more than 1 s toggles clip mode. The screen shows "LOOPY CLIPS" at entry and "LP OFF" at exit. [APC] [SMK37] [PADMAP] [CODE app/main_fm1.c:768-784]
CLP-03. Clip mode is off at power-on. [CODE app/main_fm1.c:176]
CLP-04. In clip mode, the LCD main area shows the clip map. When clip mode stops, the normal X0X view comes back at the next frame. [APC 1.5-apc40] [CODE app/ui.c:3788-3799]

### 15.2 Feedback from Loopy

CLP-10. The firmware stores APC clip colour feedback (notes 0-39) and transport feedback (notes 91 = PLAY, 93 = RECORD) at all times, also when clip mode is off. [USER] [APC] [CODE app/main_fm1.c:252-272]
CLP-11. While clip mode is on, incoming note-on and note-off for notes 0-39, 91 and 93 do not go to the sound engine. This applies on all MIDI channels. [USER] [CODE app/main_fm1.c:258-270] [CODE app/engine.c:477-494]
CLP-12. While clip mode is off, these notes go to the sound engine as before. [USER] [CODE app/engine.c:477-494]
CLP-13. Clip colours use the 128-colour APC palette. The firmware accepts them on MIDI channels 1-16. The channel does not change the colour. [APC]
CLP-14. Velocity 0 or a note-off clears the cell. The last colour from Loopy is the truth. [APC 1.4-apc40]
CLP-15. Only APC channels 11-15 (zero-based) mark a queued cell (a cell that blinks). Pulse channels (6-10) and one-shot channels (1-5) draw steady. [APC 1.1-apc40] [CODE app/smk_apc40.h:67-70]
CLP-16. Two off-to-on toggles within 256 ms set the queued state. The queued state stops 384 ms after the last flash message. [APC 1.4-apc40] [CODE app/smk_apc40.h:101-102]
CLP-17. A feedback burst is a page snapshot only if it touches 8 or more cells, has no cue state, touches at least half of the lit cells, and covers 3 or more rows and 3 or more columns. A snapshot clears the cells that it does not touch. [APC 0.9-apc40] [APC 1.0-apc40] [CODE app/smk_apc40.h:224-232]

### 15.3 Layout and pads

CLP-20. The firmware latches the layout 250 ms after the first feedback burst. At clip mode entry it latches at once from the stored feedback. [APC] [CODE app/main_fm1.c:209] [CODE app/main_fm1.c:444-450]
CLP-21. A latched layout cannot shrink. It can grow only to complete a 4x4 fold. 8-wide layouts stay as latched. [APC] [APC 1.0-apc40] [PADMAP] [CODE app/smk_apc40.h:427] (Was OQ-09. Resolved 2026-10-15: APC40_IMPLEMENTATION.md now agrees with the code.)
CLP-22. The firmware trims empty outer rows and columns. Empty cells inside the area stay in their positions. [APC]
CLP-23. A footprint of 3 or 4 rows and 4 or fewer columns folds onto one 8x2 view. [APC 1.1-apc40] [CODE app/smk_apc40.h:277-278]
CLP-24. 4x4 fold (1.7 mapping): source rows 0-1 go on pads 5-8 and 13-16. Source rows 2-3 go on pads 1-4 and 9-12. [APC 0.9-apc40] [APC 1.7-apc40]
CLP-25. In clip mode, the physical pad rows are vertically flipped. Columns stay left to right. [APC]
CLP-26. OCT- and OCT+ move the local 8x2 view down and up by 2 rows. The move stays inside the latched footprint. [APC] [PADMAP] [CODE app/main_fm1.c:789-802] (Was part of OQ-02. Resolved 2026-10-15.)
CLP-27. In clip mode, "<", ">", a short KNOB BANK press and FADER BANK do nothing. The firmware sends no APC navigation messages. [APC] [PADMAP] [CODE app/main_fm1.c:731-733] [CODE app/main_fm1.c:598-599] (Was OQ-02. Resolved 2026-10-15: PAD_MAP.md and BUILDING.md now agree with the code.)
CLP-28. In clip mode, the faders do nothing. [CODE app/main_fm1.c:598-599] (Was OQ-03. Resolved 2026-10-15: the code comment at app/main_fm1.c:599 now agrees.)
CLP-29. In clip mode, the update-mode hold (OCT- and OCT+ for 5 s) is disabled. [CODE app/main_fm1.c:1074-1079]
CLP-30. In clip mode, a pad press on a lit cell sends note-on (velocity 127) for the cell APC note on MIDI channel 1. The release sends note-off. [CODE app/main_fm1.c:819-835] [CODE usb.c:147-153]
CLP-31. A pad press on an unlit cell sends nothing. [APC "Spatial mapping"] [CODE app/main_fm1.c:826]
CLP-32. At clip mode exit, the firmware sends note-off for each pad note that is still on. [CODE app/main_fm1.c:837-848]
CLP-33. Pad presses in clip mode do not start X0X pad functions. The keybed keeps its normal function. [APC] [CODE app/ui.c:2068-2071]
CLP-34. In clip mode, while a pad is held, the keybed white key that shares the same note bit is blocked. This is intended. Keep it. [USER] [CODE app/ui.c:2068-2071] (Was OQ-10. Resolved 2026-10-15.)

### 15.4 Pad brightness

CLP-40. A lit pad shows the Loopy colour at half level. [APC] [CODE app/main_fm1.c:560]
CLP-41. Idle clip pads (clips that do not play) have a minimum brightness. The brightest channel is at least 48 of 255. The hue does not change. [USER] [APC 1.8-apc40 follow-up] [CODE app/main_fm1.c:211] [CODE app/main_fm1.c:563-569]
CLP-42. The colours of clips that play do not change. [USER] [CODE app/main_fm1.c:566]
CLP-43. The off phase of a queued blink is fully off. [USER] [CODE app/main_fm1.c:558] [CODE app/main_fm1.c:566]
CLP-44. A queued cell blinks once per beat (24 MIDI clock ticks), on for the first half. Without clock in the last 500 ms, it blinks 250 ms on and 250 ms off. [APC 1.5-apc40] [CODE app/main_fm1.c:299-312]

### 15.5 Transport and history

CLP-50. In clip mode, SEQ PLAY and SEQ REC send momentary APC PLAY (note 0x5B) and RECORD (note 0x5D). The release sends note-off. [APC] [CODE app/main_fm1.c:513-543]
CLP-51. In clip mode, the PLAY and REC LEDs follow Loopy feedback. A press sets the LED at once. Later feedback replaces it. [APC 0.7-apc40] [CODE app/ui.c:4024-4031]
CLP-52. In clip mode, PATCH held + SEQ REC sends note 0x70 on channel 16 (Loopy undo). PATCH held + SEQ PLAY sends note 0x71 on channel 16 (Loopy redo). The firmware then sends no APC transport note. [APC 1.8-apc40] [CODE app/smk_apc40.h:404-410]
CLP-53. The release of PATCH after a Loopy undo or redo does not change the X0X view. [APC 1.8-apc40] [CODE app/ui.c:2436]
CLP-54. With clip mode off, PATCH + SEQ REC and PATCH + SEQ PLAY are the X0X undo and redo. SEQ PLAY and SEQ REC are the X0X PLAY and REC. [APC] [MANUAL §3]
CLP-55. MIDI clock, Start and Song Position drive the clip-mode blink phase. [APC 1.5-apc40] [CODE app/engine.c:461-469]
CLP-56. In clip mode, MIDI Clock, Start, Stop and Continue from Loopy also go to the X0X sequencer. Thus Loopy can start and stop the X0X sequencer in clip mode. This is intended, on condition that it does not conflict with external clock sync. See LIM-10 and LIM-11 for the known risks. [USER] [CODE app/engine.c:470-474] (Was OQ-05. Resolved 2026-10-15.)

## 16. Flash and sample storage

FLS-01. SMK-37 flash map: X0X store 0x9D000-0xBAFFF, update staging 0xBB000-0xBFFFF, user sample slots 0xC2000-0xE9FFF. [CODE ../hal/fm1_flash_map.h:25-36]
FLS-02. The SMK-37 has 2 user sample slots of 80 KiB each. The FM-1 has 3. [CODE app/slots.c:15-24]
FLS-03. Imported-sample flash must be preserved. A space saving must not take flash from the sample storage. [USER]
FLS-04. The store-owner check at first start clears only 0x9D000-0xAEFFF. It does not clear the sample slots. [CODE storage.c:62-100]
FLS-05. A slot holds a header, up to 16 zones and IMA ADPCM data at offset 512. X0X plays zone 0 of each loop as a loop. [CODE app/slots.c:1-8]
FLS-06. The UI keeps PLAT_NSLOTS = 3 on the SMK-37. Slot 3 is always empty there. The editor refuses an upload to slot 3. [CODE app/slots.c:17-18] [CODE app/editor.c:180-229]
FLS-07. IMA ADPCM at 44.1 kHz is the preferred sample storage format. [USER]
FLS-08. The SMK-37 store has 7 objects with A and B copies: settings, sound, two pattern objects, panel, song and motion. [CODE storage.c:50-60]
FLS-09. The protected ranges are head, BTIF, USRTRIM, stock USR resources and key_mac. The flash map refuses them at compile time. The flash driver refuses them again. [SMK37] [CODE ../hal/fm1_flash_map.h:56-83]

## 17. Known limitations

LIM-01. web/x0x_breaks.html has 3 slots hard-coded. The SMK-37 has 2. This is a known fault. [USER] [CODE ../../web/x0x_breaks.html:95] [CODE ../../web/x0x_breaks.js:13]
LIM-02. The APC protocol has no project-size data. The firmware infers the footprint from lit cells. A sparse 8-wide project with clips in 4 or fewer columns and 3-4 rows folds as a 4x4 grid. [APC 0.7-apc40] [APC "Spatial mapping"]
LIM-03. Loopy sends the same dim shade for empty slots and for filled clips that do not play. The firmware cannot tell them apart. [APC 1.4-apc40] [APC 1.5-apc40]
LIM-04. Clip mode cannot show clips outside the Loopy APC 8x5 box. Change the Loopy page to see them. [APC]
LIM-05. Old-page cells stay lit after a change to a page with fewer than 3 rows or 3 columns of clips. [APC 1.0-apc40]
LIM-06. Loopy sends its MIDI clock to the port "X0X FM-1". The APC40 mkII port gets no clock. [APC 1.5-apc40]
LIM-07. Nobody has verified the hardware LED colours and live clip launches end-to-end on the device. [APC "Verification performed"]
LIM-08. User sample room on the SMK-37 is about 14.8 s at 22.05 kHz (2 slots). The FM-1 has about 21 s (3 slots). [MANUAL §8] [CODE app/slots.c:15-24] (Was OQ-04. Resolved 2026-10-15: MANUAL §8 now gives both values.)
LIM-09. tools/build_smk_release.sh builds Pro and Elite packages only. It does not build MKE-P37. [CODE ../../tools/build_smk_release.sh:17-18]
LIM-10. The firmware does not tell MIDI clock sources apart. All Clock, Start, Stop and Continue messages go into one input queue. If two sources send clock (for example Loopy and a DAW on the USB port, or USB and TRS on the FM-1), each tick counts. The sequencer then runs at about double tempo, and a Start or Stop from either source acts. On the SMK-37 the USB port has one cable, so the firmware cannot separate two USB sources. Send clock to the port "X0X FM-1" from one source only. [CODE app/engine.c:471-474] [CODE seq/sequencer.c:519] [CODE seq/sequencer.c:397-424] [CODE usb.c:754-757] [CODE midi_uart.c:39]
LIM-11. While X0X follows an external clock, it sends no Clock, Start or Stop (CLOCK OUT is muted), so no clock loop occurs. A MIDI Start that arrives when no clock came in the last 0.5 s starts X0X on its internal clock. CLOCK OUT is on by default (SET-08). If CLOCK OUT is on, X0X then sends Start (0xFA) and clock ticks back to the USB host until the first incoming tick. A host that also listens for clock on this port can receive this echo. [CODE seq/sequencer.c:378-379] [CODE seq/sequencer.c:446-455] [CODE seq/sequencer.c:510-511]

## 18. Open questions

These are disagreements between the code and a source document, or behaviour that needs a user decision. Do not report them as defects. Do not fix them without a user decision.

OQ-07. Sample rate target: the user confirmed the target IMA ADPCM at 44.1 kHz [USER, confirmed 2026-10-15]. This stays open until the flash and tool work is done. Current state: built-in loops and the upload tools use 22.05 kHz. DESIGN also says 22.05 kHz. [USER] vs [CODE ../../tools/gen_builtin_break.c:133-134] [CODE ../../web/x0x_breaks.js:9] [DESIGN 2026-10-03-x0x-design.md]

Resolved open questions (do not reuse these IDs): OQ-01 (see GEN-40), OQ-02 (see CLP-26 and CLP-27), OQ-03 (see CLP-28), OQ-04 (see LIM-08), OQ-05 (see CLP-56, LIM-10, LIM-11), OQ-06 (see GEN-44), OQ-08 (see MID-10), OQ-09 (see CLP-21), OQ-10 (see CLP-34).
