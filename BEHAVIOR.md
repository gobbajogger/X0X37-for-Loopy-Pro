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
GEN-21. The 10 outer keys (C3 to E3, G#5 to C6) do nothing. [SMK37] [CODE app/main_fm1.c:766-767]
GEN-22. The 16 pads are the 16 white keys as a step grid. Steps 1-8 are on the top row. Steps 9-16 are on the bottom row. On HOME and SONG, the pads are the patterns. [SMK37] [PADMAP] [CODE app/main_fm1.c:756-762]
GEN-23. On a 303 with REC on, the pads are the 303 keyboard. Low notes are on the bottom row. [SMK37] [PADMAP]
GEN-24. The pads show the colour of the part on the screen. A lit pad is at half level. A dim pad is at one eighth level. [SMK37] [CODE app/main_fm1.c:582-592]
GEN-25. The pad LED chain order is {0,15,14,13,12,11,10,9,1,2,3,4,5,6,7,8}. Changes must not change this table or the non-grid row swap. [PADMAP]
GEN-26. K1 is MASTER. K2, K3 and K4 are SELECT, ALGORITHM and PRESETS. K5 to K8 are KNOB 1 to KNOB 4. [SMK37] [CODE app/panel.c:28-40]
GEN-27. FX, SCALE, CHORD, NOTE REPEAT, PARA and GLOBE are FX, SEL, ENV, LFO, EDIT and GLO. PATCH is HOME. BT is SAVE. [SMK37]
GEN-28. ARP and SEQ are ARP and SEQ. SEQ PLAY and SEQ REC are PLAY and REC. OCTAVE- and OCTAVE+ are OCT- and OCT+. [SMK37]
GEN-29. "<" and ">" move SELECT one step back or forward. They do nothing while HOME is held. [SMK37] [CODE app/main_fm1.c:738]
GEN-30. FADER BANK switches the faders between bank A and bank B. The FADER BANK LED is on for bank B. [SMK37] [CODE app/main_fm1.c:607-613]
GEN-31. Bank A faders 1-4 set the levels of the 909, the 808, 303A and 303B. [SMK37] [CODE app/main_fm1.c:628-629]
GEN-32. Bank B faders 1-4 set the BREAK level, the reverb return, the delay return and the master FILTER. [SMK37] [CODE app/main_fm1.c:630-637]
GEN-33. A fader has an effect only after it moves. This is also true after a bank change. [SMK37] [CODE app/main_fm1.c:621-626]
GEN-34. A fader sets the same value as the related knob. The project keeps the value. With REC on, the fader records knob motion. [SMK37] [CODE app/main_fm1.c:641-642]
GEN-35. The pitch wheel changes the TUNE of the 303 on the screen by about +-1.5 semitones. It goes back when you release it. On other parts it does nothing. [SMK37] [CODE app/main_fm1.c:708-712]
GEN-36. The mod wheel opens the CUTOFF of the 303 on the screen from the saved value to the top. On other parts it does nothing. [SMK37] [CODE app/main_fm1.c:713-720]
GEN-37. The wheels change only the sound. They do not change saved values. [SMK37] [CODE app/main_fm1.c:698-705]
GEN-38. The keys and the pads play without velocity. [SMK37]
GEN-39. OCTAVE- and OCTAVE+ held at power-on start HARDWARE CALIBRATION. It teaches the buttons and K2 to K8 again. K1 is not taught. [SMK37] [CODE app/main_fm1.c:1039-1041] [CODE app/panel.c:100-101]
GEN-40. A short KNOB BANK press does nothing. A KNOB BANK hold of more than 1 s toggles clip mode (section 15). [SMK37] [CODE app/main_fm1.c:772-792] (Was OQ-01. Resolved 2026-10-15: SMK37.md now agrees with the code.)
GEN-41. The SMK-37 keybed has no LEDs. Black-key information (tracks, slices, part mutes) is on the screen only. [CODE app/plat_fm1.c:50-53]
GEN-42. The first start of X0X on an SMK-37 clears the data of another firmware in the store area. The screen then shows "OLD DATA CLEARED". [SMK37] [CODE storage.c:62-100] [CODE app/main_fm1.c:1049-1051]
GEN-43. The firmware never writes the calibration, the Bluetooth pairing or the key areas of the flash. [SMK37] [CODE ../hal/fm1_flash_map.h:24-83]
GEN-44. With FREE PLAY on and REC off, the keys play the 303 without writing notes, and the SMK pads stay in the step-grid layout. This is the current behaviour of the existing 303 FREE PLAY feature. The pad layout in this case is unspecified. The user accepts it. [USER] [CODE app/main_fm1.c:758-759] [CODE app/ui.c:128] (Was OQ-06. Resolved 2026-10-15.)

### 2.3 FM-1 controls

GEN-50. On the FM-1, the controls are as in the manual, section 3. [MANUAL §3]
GEN-51. Hold OCT- and OCT+ for 5 s to go into update mode. Release them before the countdown ends to cancel. [MANUAL §17] [CODE app/main_fm1.c:1075-1098]
GEN-52. Hold OCT- and OCT+ at power-on for calibration. [MANUAL §17]
GEN-53. After two crashes in a row at start, X0X starts in SAFE MODE. Safe mode has no sound, but USB works. [MANUAL §17] [CODE app/main_fm1.c:1155-1160]

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
DRM-11. The 909 hi-hat, ride and crash samples are IMA ADPCM at 44.1 kHz in flash. They use the same codec as the loops (dsp/ima_adpcm.h). Each voice decodes forward as it plays. The output is bit-identical to playback of a fully decoded buffer. The decoded samples are equal to the approved A/B study (adpcm_ab, 2026-10-15). The ADPCM error is white noise that follows the envelope (SNR 16.5-18.3 dB against the original PCM). This is intended. [USER] [CODE dsp/drum909.c:349] [CODE dsp/drum909.c:739] [CODE ../../tools/gen_drum_samples.py]
DRM-12. Cost of the cymbal decode: one decode for each source sample played. That is 1 decode for each output sample at pitch 1, and up to 4 at pitch x4, for each sounding cymbal voice. Estimate (not measured on the device): about 20-30 cycles for each decode, so 3 cymbals at pitch 1 take about 1 % and 4 voices at pitch x4 take about 4-6 % of the 8163 cycles for each sample at 360 MHz. The 909 runs on core 0. [CODE dsp/drum909.c:739-745] [DESIGN 2026-10-04-dual-core-and-clock.md]

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
BRK-12. On MIDI, notes 36-43 on the BREAK channel play slices 1-8. [MANUAL §16] [CODE app/engine.c:519-520]
BRK-13. The generator settings (COMPLEXITY, ANCHOR, ROLL, FILL, RETRIG, PHRASE, A/B LENGTH, B CHANCE) and the steps, slices and loop choice are saved with the pattern. [MANUAL §8]

### 6.2 Loops and imported samples

BRK-20. Imported samples play only in BREAK mode. This is intended. [USER] [CODE app/engine.c:952-976]
BRK-21. The LOOP list has the built-in loops first ("909 GR", "909 FL"), then the imported loops as BR1, BR2 and so on. [MANUAL §8] [CODE app/engine.c:859-882]
BRK-22. The factory (built-in) breaks are 2 bars long, IMA ADPCM at 44.1 kHz, without a length cut: 2 x 220,500 samples, 220,500 B of flash. [USER] [CODE ../../tools/gen_builtin_break.c:14-15] [CODE ../../tools/gen_builtin_break.c:134]
BRK-23. Do not halve the length of a factory break. [USER]
BRK-24. An imported loop gets a length of 1, 2, 4 or 8 bars. The firmware selects the length that puts the loop tempo nearest to 120 BPM, inside 80-180 BPM. [CODE app/engine.c:821-833]
BRK-25. If a pattern names an empty user slot, loop A plays the first built-in loop. [CODE app/engine.c:971-972]
BRK-26. Loops are IMA ADPCM at 44.1 kHz. The built-in loops, tools/upload_breaks.py, tools/import_breaks.py and the break page all use 44.1 kHz by default. 22.05 kHz is an explicit opt-in only: `--rate 22050` in the tools, or the "22 kHz (more room)" box on the page. A loop that the page reads back from the device keeps its rate. (Was OQ-07. Resolved 2026-10-15.) [USER] [CODE ../../tools/gen_builtin_break.c:134] [CODE ../../tools/upload_breaks.py:87-90] [CODE ../../web/x0x_breaks.js:13] [CODE ../../web/x0x_breaks.html:77]
BRK-27. The firmware reads the rate of each imported zone from the slot header. Thus it can play a 44.1 kHz zone. The bar guess (BRK-24) uses the zone rate. A host test checks both (tests/host/loop_bars_test.c, tests/scenarios/upload44.x0x). [CODE app/slots.c:10] [CODE app/plat_fm1.c:150]
BRK-28. Trim silence (sampleio.trim_silence, trimSilence in web/x0x_breaks.js): remove the samples at or below -60 dBFS (0.001) at the two ends, keep 1 ms before the first louder sample, and apply a 5 ms linear fade at the end. A file that is all silence stays unchanged. The tool and the page give the same bytes. [CODE ../../tools/sampleio.py:202] [CODE ../../web/x0x_breaks.js:78]
BRK-29. Trim is off by default for loops (upload_breaks.py `--trim`, the page's "Trim silence" box) because it changes the loop length and thus the bar guess. Trim is on by default for one-shots (fm1_sample_upload.py; `--no-trim` turns it off). [USER] [CODE ../../tools/upload_breaks.py:91] [CODE ../../tools/fm1_sample_upload.py:94] [CODE ../../web/x0x_breaks.html:75]

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

SET-01. GLO has five sections: MIDI, AUDIO, KEYS AND SCREEN, PROJECT, SYSTEM. On the SMK-37 a sixth section, BLUETOOTH MIDI, follows SYSTEM (BLE-05). [MANUAL §15] [CODE app/ui.c open_global] (Changed 2026-10-15: the SMK-37 section is new.)
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
MID-04. 909 notes: 36 BD, 38 SD, 41/45/50 toms, 37 RS, 39 CP, 42 CH, 46 OH, 49 CR, 51 RD. 808: the same, with 56 CB and 49 CY. [MANUAL §16] [CODE app/engine.c:503-504]
MID-05. On a 303 channel, overlapping notes slide. Velocity 100 or more is an accent. [MANUAL §16] [CODE app/engine.c:509-513]
MID-06. X0X follows an incoming MIDI clock. Start, Stop and Continue start and stop the sequencer. [MANUAL §16] [CODE app/engine.c:474-478]
MID-07. When clip mode is off, the original sequencer and MIDI input path must stay unchanged. Firmware changes stay inside clip mode unless the user approves. [USER] (agent-rules 10) Approved exception (user, 2026-10-16 00:45): when the external MIDI clock stops (0.5 s with no tick), the internal tempo takes the last measured external tempo (whole BPM). Nothing must jump. This applies to the main sequencer too. [USER] (Agreed 2026-10-16, in progress as of 72ff628.)
MID-08. USB audio is a 2-channel input at 44.1 kHz. It carries the headphone signal. [MANUAL §16] [CODE usb.c:74]
MID-09. The firmware answers a Universal Device Inquiry as an Akai APC40 mkII (manufacturer 0x47, product 0x29). [APC] [CODE usb.c:200-216] [CODE usb.c:694-701]
MID-10. On all boards (also the FM-1), the APC40 identity reply and the APC40 introduction reply are not gated by the board. The user accepts both. They are not defects. [USER] [CODE usb.c:694-701] [CODE usb.c:702-710] (Was OQ-08. Resolved 2026-10-15 by the user.)
MID-11. A USB bus reset or a deconfiguration clears the stored APC clip state. [CODE usb.c:554-559] [CODE usb.c:1113-1117] [CODE app/main_fm1.c:339-359]

### 14.1 Bluetooth MIDI (SMK-37 only, roadmap step 5)

BLE-01. Bluetooth MIDI is BLE MIDI only (no Bluetooth audio). It needs a controller image in the Bluetooth block (FLS-13). The user builds the image on their own computer from JieLi's files (ble/ctl/build_ctl.sh, pinned gitee commit d179b44, btctrler.a SHA-256 9ef42624...). X0X never ships the image or any JieLi Bluetooth file. The image is a separate program: the app talks to it only through standard HCI packets in a shared-RAM mailbox. [USER] [CODE ../hal/x0x_ble_abi.h] [CODE ble.c] Planned: the JieLi BLE-only host from the official SDK (built locally, never bundled) replaces our own host. Classic Bluetooth and Bluetooth audio are removed. [USER 2026-10-16 00:08] (Agreed 2026-10-16, not working yet as of a27da68.)
BLE-02. Without an image, or with an image that fails its check, the app starts exactly as before: core 1 starts with its own stacks, no controller code runs, no interrupt is routed to core 1. GLO shows NO IMAGE or BAD IMAGE. The check covers the header magic and version, the ABI, the size, the CRC-32, the init and poll entries (inside the image), the RAM window (0x01C01800-0x01C07EFF) and the interrupts (none that X0X uses on core 0). [USER] [CODE ble/ble_glue.h ble_img_check] [CODE ble.c ble_prepare]
BLE-03. The controller starts only at power-on, and only when BLUETOOTH is ON. Its init runs once on core 1 while core 0 waits in RAM (at most 1.5 s, before the audio starts). Then core 1 calls its poll entry between audio jobs. If the start fails, or the image reports a failure later, X0X stops calling it, turns its interrupts off on core 1 and shows FAILED. If its start does not end in time, core 1 is held and X0X runs on one core. [CODE ble.c ble_start] [CODE ../hal/fm1_cpu1.h]
BLE-04. Before X0X turns the flash off (erase, write, SPI read), it parks core 1: core 1 waits in RAM with its interrupts off and runs no controller code. If core 1 does not park within 20 ms, X0X holds it (one core, no Bluetooth). Without a controller nothing is parked. [CODE ../hal/fm1_cpu1.h fm1_cpu1_park] [CODE ../hal/fm1_flash.h]
BLE-05. GLO > BLUETOOTH MIDI has three rows. BLUETOOTH OFF/ON (default OFF, saved in the settings byte that was reserved; an older project loads OFF). STATUS (read only): NO IMAGE, OFF, RESTART, STARTING, READY (advertising), CONNECTED, FAILED or BAD IMAGE. NAME (read only): "X0X-" and 4 hex digits. ON when the controller did not start at power-on shows "RESTART TO TURN ON". OFF while it runs drops the connection and stops advertising. [CODE app/ui.c] [CODE app/x0x.h settings_t.bt]
BLE-06. The unit advertises as a BLE MIDI peripheral with a random static address. The address and the name come from a CRC of the factory record (USRTRIM, read only), so they stay the same for a unit across power cycles and differ between units. One connection at a time. [CODE ble/ble_glue.h ble_addr_of] [CODE ble/ble_midi_host.c]
BLE-07. ABI 3 build: the Bluetooth image holds the BLE host from the JieLi SDK. If the central asks for pairing, X0X accepts it with Just Works pairing and keeps the bond. X0X keeps the bonds in its own flash sector at 0xFE000 (FL_BOND). X0X writes them 2 s after a change. X0X never writes BTIF (0xC0000). (Changed 2026-10-16.) [CODE ble.c ble_idle] [CODE ../../ble/ctl/src_host/host_app.c]
BLE-07a. X0X asks the central for new connection parameters. The a63-15 build asks for an interval of 15-30 ms. The a63-7 build asks for an interval of 7.5-15 ms. Both builds ask for latency 0 and a supervision timeout of 2000 ms. X0X sends a maximum of 3 requests in total: 1 first request and 2 retries. The first request goes about 1.5 s after the central turns on notifications (CCCD on). Each retry goes about 1 s after the request before it. X0X stops the retries when the central confirms the update. When the CCCD goes from off to on again in the same connection, the count goes back to 0, so X0X can send 3 more requests. A new connection also sets the count to 0. DIAG shows the request result, the number of updates, the setup ring (connect, MTU, first ATT, CCCD, request, update, disconnect) and the disconnect reason. [CODE ../../ble/ctl/src_host/host_app.c hx_tick] (Changed 2026-10-16, a62, a62b and a63. Before: no request, test 37.) (Not verified on hardware as of d9844a4.) (Owner decision 2026-10-16 17:41. Replaces the earlier no-request decision.)
BLE-08. BLE MIDI in goes into the same input queue as USB MIDI (and TRS on the FM-1), so it acts exactly like USB MIDI in (notes, clock, Start, Stop, Continue). The BLE timestamps are not used for scheduling. Incoming SysEx is not used (it is counted). LIM-10 applies: clock from USB and Bluetooth together counts twice. [CODE ble.c ble_service] When the frame ring from the image to the app is full, the image keeps one frame and sends it again at the next poll. The image drops a frame only when a second frame comes while one frame waits. c2h_drops counts these drops. (Changed 2026-10-16, a62.)
BLE-09. X0X sends all of its MIDI out (clock, Start, Stop, notes) to USB and to a connected BLE MIDI central with notifications on. The APC40 feedback and the keybed notes go out on both USB and BLE (60a7835). The APC40 identity and introduction replies to a SysEx from Bluetooth go to Bluetooth. The undo and redo history notes stay on USB only. X0X does not send SysEx to BLE. [CODE app/plat_fm1.c plat_midi_out] [CODE app/main_fm1.c] (Agreed 2026-10-16, not working yet as of a27da68.)
BLE-10. Every install writes the whole app slot, also the Bluetooth block. To keep Bluetooth, add the image again for each update: tools/add_ctl.py, tools/fm1_install.py --ctl FILE (it also uses ~/.config/x0x/ctl.img by itself; --no-ctl skips it), or the web installer's ctl.img field (SMK-37 only). Without it the block is empty after the install (BLE-02). [CODE ../../tools/add_ctl.py] [CODE ../../tools/fm1_install.py] [CODE ../../web/fm1pkg.js insertCtl]
BLE-11. After a power cycle or a lost link, the keyboard reconnects by itself to the last bonded device when that device is in range. [USER 2026-10-16 00:08] (Agreed 2026-10-16. Not verified on hardware as of d9844a4 (a63). In the a61 test the iPhone connected without pairing, so X0X kept no bond.)
BLE-12. When a BLE link ends, X0X sends All Notes Off, so no note hangs. [USER 2026-10-16] [CODE plat_ble_link_lost (planned)] (Agreed 2026-10-16. Not verified on hardware as of d9844a4 (a63).)

## 15. Clip mode (APC40 mkII emulation for Loopy Pro), SMK-37 only

### 15.1 Mode

CLP-01. Clip mode exists only on the SMK-37 build. [CODE app/main_fm1.c:170]
CLP-02. A KNOB BANK hold of more than 1 s toggles clip mode. The screen shows "LOOPY CLIPS" at entry and "LP OFF" at exit. [APC] [SMK37] [PADMAP] [CODE app/main_fm1.c:772-788]
CLP-03. Clip mode is off at power-on. [CODE app/main_fm1.c:176]
CLP-04. In clip mode, the LCD main area shows the clip map. When clip mode stops, the normal X0X view comes back at the next frame. [APC 1.5-apc40] [CODE app/ui.c:3788-3799]
CLP-05. LCD clip colours: a lit cell is a filled square. The firmware maps the brightest channel of the Loopy colour from 0-255 to 128-255 (SMK_CLIP_LCD_FLOOR). The hue does not change. Full (playing) colours do not change, and idle clips stay darker than playing clips. An empty cell (velocity 0) is a small grey dot. A queued cell has a white centre square. A white frame marks the cells on the pads. [USER] [CODE app/main_fm1.c:367] [CODE app/main_fm1.c:412-440]

### 15.2 Feedback from Loopy

CLP-10. The firmware stores APC clip colour feedback (notes 0-39) and transport feedback (notes 91 = PLAY, 93 = RECORD) at all times, also when clip mode is off. [USER] [APC] [CODE app/main_fm1.c:250-270]
CLP-11. While clip mode is on, incoming note-on and note-off for notes 0-39, 91 and 93 do not go to the sound engine. This applies on all MIDI channels. [USER] [CODE app/main_fm1.c:256-268] [CODE app/engine.c:481-498]
CLP-12. While clip mode is off, these notes go to the sound engine as before. [USER] [CODE app/engine.c:481-498]
CLP-13. Clip colours use the 128-colour APC palette. The firmware accepts them on MIDI channels 1-16. The channel does not change the colour. [APC]
CLP-14. Velocity 0 or a note-off clears the cell. The last colour from Loopy is the truth. [APC 1.4-apc40]
CLP-15. Only APC channels 11-15 (zero-based) mark a queued cell (a cell that blinks). Pulse channels (6-10) and one-shot channels (1-5) draw steady. [APC 1.1-apc40] [CODE app/smk_apc40.h:67-70]
CLP-16. Two off-to-on toggles within 256 ms set the queued state. The queued state stops 384 ms after the last flash message. [APC 1.4-apc40] [CODE app/smk_apc40.h:101-102]
CLP-17. A feedback burst is a page snapshot only if it touches 8 or more cells, has no cue state, touches at least half of the lit cells, and covers 3 or more rows and 3 or more columns. A snapshot clears the cells that it does not touch. [APC 0.9-apc40] [APC 1.0-apc40] [CODE app/smk_apc40.h:224-232]

### 15.3 Layout and pads

CLP-20. The firmware latches the layout 250 ms after the first feedback burst. At clip mode entry it latches at once from the stored feedback. [APC] [CODE app/main_fm1.c:209] [CODE app/main_fm1.c:458-464]
CLP-21. A latched layout cannot shrink. It can grow only to complete a 4x4 fold. 8-wide layouts stay as latched. [APC] [APC 1.0-apc40] [PADMAP] [CODE app/smk_apc40.h:427] (Was OQ-09. Resolved 2026-10-15: APC40_IMPLEMENTATION.md now agrees with the code.)
CLP-22. The firmware trims empty outer rows and columns. Empty cells inside the area stay in their positions. [APC]
CLP-23. A footprint of 3 or 4 rows and 4 or fewer columns folds onto one 8x2 view. [APC 1.1-apc40] [CODE app/smk_apc40.h:277-278]
CLP-24. 4x4 fold (1.7 mapping): source rows 0-1 go on pads 5-8 and 13-16. Source rows 2-3 go on pads 1-4 and 9-12. [APC 0.9-apc40] [APC 1.7-apc40]
CLP-25. In clip mode, the physical pad rows are vertically flipped. Columns stay left to right. [APC]
CLP-26. OCT- and OCT+ move the local 8x2 view down and up by 2 rows. The move stays inside the latched footprint. [APC] [PADMAP] [CODE app/main_fm1.c:793-806] (Was part of OQ-02. Resolved 2026-10-15.)
CLP-27. In clip mode, "<", ">", a short KNOB BANK press and FADER BANK do nothing. The firmware sends no APC navigation messages. [APC] [PADMAP] [CODE app/main_fm1.c:735-737] [CODE app/main_fm1.c:602-603] (Was OQ-02. Resolved 2026-10-15: PAD_MAP.md and BUILDING.md now agree with the code.)
CLP-28. In clip mode, the faders do nothing. [CODE app/main_fm1.c:602-603] (Was OQ-03. Resolved 2026-10-15: the code comment at app/main_fm1.c:599 now agrees.)
CLP-29. In clip mode, the update-mode hold (OCT- and OCT+ for 5 s) is disabled. [CODE app/main_fm1.c:1078-1083]
CLP-30. In clip mode, a pad press on a lit cell sends note-on (velocity 127) for the cell APC note on MIDI channel 1. The release sends note-off. [CODE app/main_fm1.c:823-839] [CODE usb.c:147-153]
CLP-31. A pad press on an unlit cell sends nothing. [APC "Spatial mapping"] [CODE app/main_fm1.c:830]
CLP-32. At clip mode exit, the firmware sends note-off for each pad note that is still on. [CODE app/main_fm1.c:841-852]
CLP-33. Pad presses in clip mode do not start X0X pad functions. The keybed keeps its normal function. [APC] [CODE app/ui.c:2068-2071] Changed: in clip mode the keybed sends MIDI notes to Loopy Pro only. It does not control the X0X UI. A held pad still blocks its white key (CLP-34). [USER 2026-10-16] (Agreed 2026-10-16, in progress as of 72ff628.)
CLP-34. In clip mode, while a pad is held, the keybed white key that shares the same note bit is blocked. This is intended. Keep it. [USER] [CODE app/ui.c:2068-2071] (Was OQ-10. Resolved 2026-10-15.)

### 15.4 Pad brightness

CLP-40. A lit pad shows the Loopy colour at half level (palette value x 255 / 510). There is no brightness floor on the pads. The user reports that this pad brightness is good. [USER] [APC] [CODE app/main_fm1.c:572-577]
CLP-41. Idle clip pads (clips that do not play) use the same half level as all lit pads. A pad floor (SMK_APC40_PAD_FLOOR) was added and then removed on 2026-10-15. It came from a misunderstanding: the dark problem was on the LCD (see CLP-05). [USER] [CODE app/main_fm1.c:574-577]
CLP-42. The colours of clips that play do not change on the pads or on the LCD. [USER] [CODE app/main_fm1.c:574-577] [CODE app/main_fm1.c:422-427]
CLP-43. The off phase of a queued blink is fully off. [USER] [CODE app/main_fm1.c:572] [CODE app/main_fm1.c:574-577]
CLP-44. A queued cell blinks once per beat (24 MIDI clock ticks), on for the first half. Without clock in the last 500 ms, it blinks 250 ms on and 250 ms off. [APC 1.5-apc40] [CODE app/main_fm1.c:297-310]

### 15.5 Transport and history

CLP-50. In clip mode, SEQ PLAY and SEQ REC send momentary APC PLAY (note 0x5B) and RECORD (note 0x5D). The release sends note-off. [APC] [CODE app/main_fm1.c:527-557]
CLP-51. In clip mode, the PLAY and REC LEDs follow Loopy feedback. A press sets the LED at once. Later feedback replaces it. [APC 0.7-apc40] [CODE app/ui.c:4024-4031]
CLP-52. In clip mode, PATCH held + SEQ REC sends note 0x70 on channel 16 (Loopy undo). PATCH held + SEQ PLAY sends note 0x71 on channel 16 (Loopy redo). The firmware then sends no APC transport note. [APC 1.8-apc40] [CODE app/smk_apc40.h:404-410]
CLP-53. The release of PATCH after a Loopy undo or redo does not change the X0X view. [APC 1.8-apc40] [CODE app/ui.c:2436]
CLP-54. With clip mode off, PATCH + SEQ REC and PATCH + SEQ PLAY are the X0X undo and redo. SEQ PLAY and SEQ REC are the X0X PLAY and REC. [APC] [MANUAL §3]
CLP-55. MIDI clock, Start and Song Position drive the clip-mode blink phase. [APC 1.5-apc40] [CODE app/engine.c:461-473]
CLP-56. In clip mode, MIDI Clock, Start, Stop and Continue from Loopy also go to the X0X sequencer. Thus Loopy can start and stop the X0X sequencer in clip mode. This is intended, on condition that it does not conflict with external clock sync. See LIM-10 and LIM-11 for the known risks. [USER] [CODE app/engine.c:474-478] (Was OQ-05. Resolved 2026-10-15.)
CLP-57. GLO > MIDI has CLIP KEYS CH: MIDI channel 2 to 15, default 2. The clip keys notes use this channel on USB and BLE. Channels 1 and 16 stay free for the APC40 emulation. [USER 2026-10-16] (Agreed 2026-10-16, in progress as of 72ff628.)
CLP-58. In clip mode, GLOBAL opens on the internal screen over the clip view. The clip view comes back when GLOBAL closes. [USER 2026-10-16] (Agreed 2026-10-16, in progress as of 72ff628.)
CLP-59. In clip mode, the mode label on the screen reads "LP". [USER 2026-10-16] (Agreed 2026-10-16, in progress as of 72ff628.)
CLP-73. In clip mode with no SCALE, CHORD or ARP mode on (and no menu on the screen), knobs 2-8 send CC 48-54 on channel 1 for Loopy Pro MIDI-learn. The value is relative: 1-63 is up and 65-127 is down. Knob 1 stays MASTER. (Added 2026-10-16.) [USER] [CODE app/main_fm1.c ck_controls] When SCALE, CHORD or ARP has the knobs, knobs 2 to 5 set that mode. Note: CLP-60 to CLP-72 are only in the clip-keys branch copy of BEHAVIOR.md (flash_review_clipkeys).

## 16. Flash and sample storage

FLS-01. SMK-37 flash map (layout 2): app code 0x04120-0x8AFFF, Bluetooth controller block 0x8B000-0x9AFFF (FLS-13), X0X store 0x9D000-0xAFFFF (spare 0xAB000-0xAEFFF, owner tag 0xAF000), user sample slot 3 0xB0000-0xBAFFF, update staging 0xBB000-0xBFFFF, user sample slots 1 and 2 0xC2000-0xF3FFF. 0xFE000-0xFEFFF stays unused (4 KiB is too small for a slot). [CODE ../hal/fm1_flash_map.h:25-68] (Changed 2026-10-15: the store ended at 0xBAFFF, the slots at 0xE9FFF.)
FLS-02. The SMK-37 has 3 user sample slots: slot 1 80 KiB at 0xC2000, slot 2 120 KiB at 0xD6000, slot 3 44 KiB at 0xB0000. Slots 1 and 2 keep the flash offsets of the earlier 2 x 80 KiB layout. The FM-1 has 3 slots of 80 KiB. [CODE app/slots.c:15-34] (Changed 2026-10-15: was 2 x 80 KiB.) Planned: slot 3 becomes 16 KiB (about 0.7 s at 44.1 kHz) to make room for the Bluetooth host. Slots 1 and 2 do not change. [USER 2026-10-16 00:08] (Agreed 2026-10-16, not working yet as of a27da68.)
FLS-03. Imported-sample flash must be preserved. A space saving must not take flash from the sample storage. [USER]
FLS-04. The store-owner check at first start clears only 0x9D000-0xAEFFF. It does not clear the sample slots. Slot 3 starts at 0xB0000, after the owner tag. [CODE storage.c:50-105]
FLS-05. A slot holds a header, up to 16 zones and IMA ADPCM data at offset 512. X0X plays zone 0 of each loop as a loop. [CODE app/slots.c:1-8]
FLS-06. The UI has PLAT_NSLOTS = 3 on the SMK-37 and on the FM-1. Each slot has its own size. The editor checks each write and each header against the size of its slot. [CODE app/slots.c:15-34] [CODE app/editor.c:180-250] (Changed 2026-10-15: slot 3 was always empty on the SMK-37.)
FLS-07. IMA ADPCM at 44.1 kHz is the sample storage format: the 909 cymbals (DRM-11), the built-in loops (BRK-22) and the uploaded loops (BRK-26). 22.05 kHz is an opt-in for uploads only. fm1_sample_upload.py one-shots stay at 22.05 kHz (FM-1 tool, not maintained). [USER]
FLS-08. The SMK-37 store has 7 objects with A and B copies: settings, sound, two pattern objects, panel, song and motion. [CODE storage.c:50-60]
FLS-09. The protected ranges are head, BTIF, USRTRIM, stock USR resources and key_mac. The flash map refuses them at compile time. The flash driver refuses them again. [SMK37] [CODE ../hal/fm1_flash_map.h:56-83]
FLS-10. Slot backup: tools/x0x_slots.py backup and the break page's "Back up all slots" read every slot with SMP_INFO (15) and SMP_READ (16) only. They do not write to the device. The archive (.x0xslots) holds the slot count and size, and for each slot the name, zone count, sample counts, rates, data length, header CRC and an image CRC. A CRC-32 covers the whole file. The tool and the page write the same bytes. [USER] [CODE ../../tools/x0x_slots.py] [CODE ../../web/x0x_breaks.js]
FLS-11. Slot restore writes an archive back with SMP_BEGIN, SMP_WRITE and SMP_END, or SMP_ERASE for an empty slot, after the user confirms. Then it reads the slots back and compares them. It takes the slot count and size from SMP_INFO. It skips an archive slot that the device does not have or that does not fit. [USER] [CODE ../../tools/x0x_slots.py] [CODE ../../web/x0x_breaks.html]
FLS-12. SMP_INFO reports the slot count, the smallest slot size in KiB (7 bits, maximum 127) and per slot the zone count, the name and the used KiB (maximum 127). Then the X0X extension follows: 'X' (0x58), 1, and per slot the flash offset and the size in 4 KiB sectors and the used KiB, each as two 7-bit bytes, low first. An older tool ignores the extension and sees slots of the smallest size, so it never overfills a slot. x0x_slots.py, upload_breaks.py and the break loops page use the per-slot sizes. The archive stores the size and the flash offset of each slot. [CODE app/editor.c:252-288] [CODE ../../tools/x0x_slots.py] [CODE ../../web/x0x_breaks.js] (Changed 2026-10-15: the extension is new.)
FLS-13. The SMK-37 Bluetooth controller image (roadmap step 5) has one contiguous 64 KiB block at flash 0x8B000-0x9AFFF (XIP 0x02087000). The block is in the encrypted app slot (APP_SLOT 0x97400 is unchanged), above the app code. The app code may use up to 0x86EE0 B (552,672 B). build.py stops the build when the image is larger. A package from build.py has an empty block (0xFF). tools/add_ctl.py and the installers put a user-built image there and write the app.bin, app area and package CRCs again (BLE-10). The image owns RAM 0x01C01800-0x01C07EFF. The app's RAM code must end below 0x01C01800 (app.ld checks it). The mailbox and the app's BLE state are in BLERAM 0x01C7C400-0x01C7D3FF (not cleared at boot). [CODE ../hal/fm1_flash_map.h:63-81] [CODE ../../tools/fm1pkg_make.py] [CODE ../../tools/build.py] [CODE ../app.ld] (Changed 2026-10-15: the image and its RAM.)
FLS-14. An SMK-37 package carries its slot map (magic "X0X-SLOTMAP-1", per slot flash offset and size, CRC-32) at flash.bin 0x9CF00. A package without the map but with the Felucca loader is an older X0X with 2 slots of 80 KiB at 0xC2000. A package with neither is stock firmware, which has no X0X slots. [CODE ../../tools/fm1pkg_make.py] [CODE ../../web/x0x_breaks.js]
FLS-15. An install never writes the sample slots. A slot keeps its samples when the new firmware has a slot at the same flash offset that holds the header area and the used data. An upgrade from the 2 x 80 KiB layout keeps every slot. [USER] [CODE ../../tools/x0x_slots.py]
FLS-16. When an install would lose samples (a slot moves, shrinks or disappears), the web installer and tools/fm1_install.py do this by themselves: (1) they tell the user which slots are at risk; (2) after the user confirms, they read every slot with SMP_INFO and SMP_READ into an .x0xslots archive and save it (the page downloads it; fm1_install.py writes --backup FILE or x0x-slots-before-install-DATE.x0xslots); (3) they install; (4) they reconnect to the new firmware, write each slot at risk back (old slot N to new slot N if it fits, else to the first other slot that is empty, not kept and large enough), and read them back to compare. A slot that has no place is reported. If the backup fails, nothing is installed. If the write back fails, the archive stays and the message tells the user to restore it with Restore on the break loops page or tools/x0x_slots.py restore FILE (fm1_install.py exit 7). fm1_install.py --no-slots skips this. [USER] [CODE ../../tools/fm1_install.py] [CODE ../../web/x0x_breaks.js] [CODE ../../web/x0x_installer.html]
FLS-17. A downgrade to an older X0X (2 x 80 KiB) cannot keep slot 3 or slot 2 data larger than 80 KiB in place. The install flow (FLS-16) saves them and writes slot 3 back into a free older slot when it fits. A stock firmware install can erase the VM (the store and slot 3). Slots 1 and 2 are in stock USRFLASH, which the stock firmware can also use. The install flow saves every slot before a stock install and tells the user to restore the archive after X0X is installed again. [USER] [CODE ../../tools/fm1_install.py]
FLS-18. The update loader drops a stale update record only where one matches (bytes +6 = 0x5441 and a nonzero CRC16) at a 4 KiB boundary − 256 in 0x9D000-0xBFFFF. Sample data in slot 3 could match by chance (about 2^-32 for each sector). The loader stays unchanged, because it must still drop the stock records. [CODE ../loader/ldr_core.c]

## 17. Known limitations

LIM-01. Fixed 2026-10-15. web/x0x_breaks.html had 3 slots hard-coded. It now reads the slot count and size from SMP_INFO. [CODE ../../web/x0x_breaks.html]
LIM-02. The APC protocol has no project-size data. The firmware infers the footprint from lit cells. A sparse 8-wide project with clips in 4 or fewer columns and 3-4 rows folds as a 4x4 grid. [APC 0.7-apc40] [APC "Spatial mapping"]
LIM-03. Loopy sends the same dim shade for empty slots and for filled clips that do not play. The firmware cannot tell them apart. [APC 1.4-apc40] [APC 1.5-apc40]
LIM-04. Clip mode cannot show clips outside the Loopy APC 8x5 box. Change the Loopy page to see them. [APC]
LIM-05. Old-page cells stay lit after a change to a page with fewer than 3 rows or 3 columns of clips. [APC 1.0-apc40]
LIM-06. Loopy sends its MIDI clock to the port "X0X FM-1". The APC40 mkII port gets no clock. [APC 1.5-apc40]
LIM-07. Nobody has verified the hardware LED colours and live clip launches end-to-end on the device. [APC "Verification performed"]
LIM-08. User sample room at 44.1 kHz is (slot size − 512) × 2 / 44100 s. On the SMK-37: slot 1 3.69 s (81,408 B of data), slot 2 5.55 s (122,368 B), slot 3 2.02 s (44,544 B), about 11.26 s in total. The FM-1 has about 11 s (3 x 80 KiB). With the 22.05 kHz opt-in the room is double (about 22.5 s on the SMK-37). A loop is one piece: it must fit in one slot. [MANUAL §8] [CODE app/slots.c:15-34] (Was OQ-04. Resolved 2026-10-15. Values changed 2026-10-15 for 44.1 kHz and for the SMK-37 layout 2.) The slot 3 value changes when slot 3 becomes 16 KiB (FLS-02).
LIM-09. tools/build_smk_release.sh builds Pro and Elite packages only. It does not build MKE-P37. [CODE ../../tools/build_smk_release.sh:17-18]
LIM-10. The firmware does not tell MIDI clock sources apart. All Clock, Start, Stop and Continue messages go into one input queue. If two sources send clock (for example Loopy and a DAW on the USB port, or USB and TRS on the FM-1), each tick counts. The sequencer then runs at about double tempo, and a Start or Stop from either source acts. On the SMK-37 the USB port has one cable, so the firmware cannot separate two USB sources. Send clock to the port "X0X FM-1" from one source only. [CODE app/engine.c:475-478] [CODE seq/sequencer.c:520] [CODE seq/sequencer.c:397-424] [CODE usb.c:754-757] [CODE midi_uart.c:39]
LIM-11. While X0X follows an external clock, it sends no Clock, Start or Stop (CLOCK OUT is muted), so no clock loop occurs. A MIDI Start that arrives when no clock came in the last 0.5 s starts X0X on its internal clock. CLOCK OUT is on by default (SET-08). In clip mode, X0X does not echo a MIDI Start, Continue or Stop (seq midi_req). Until the first incoming tick it can still send internal clock ticks (0xF8) to the USB host. With clip mode off, X0X sends Start (0xFA) and clock ticks back to the USB host until the first incoming tick, as before (agent-rules 10). [CODE app/engine.c:469-472] [CODE seq/sequencer.c:378-379] [CODE seq/sequencer.c:392-393] [CODE seq/sequencer.c:470] [CODE seq/sequencer.c:446-455] [CODE seq/sequencer.c:511-512]
LIM-13. Bluetooth MIDI (BLE-01 to BLE-12) has only part of its hardware tests (Stage 3: docs/plans/2026-10-15-ble-stage3-test-plan.md). On 2026-10-16 the Mac connected over BLE MIDI and passed 3 rounds of 600 s with no disconnect (a59). The return path from the keyboard to the Mac works (a60). The iPhone link drops after about 2 min (a61). BLE-07, BLE-07a, BLE-11 and BLE-12 are not verified on hardware. The RF calibration values are the AC79 SDK demo board values, not the unit's own. The clocks given to the image (system 360 MHz, HSB 180 MHz, LSB 60 MHz) were measured on an FM-1, not on an SMK-37. [CODE ../hal/fm1_ble.h]
LIM-14. The controller's poll and its interrupts run on core 1 between and inside audio jobs. A long poll can delay an audio job past the 8 ms limit, and then X0X holds core 1 (one core, no Bluetooth). Long flash operations park core 1, and a long park can drop the BLE connection. Units that cannot run core 1 (fm1-x0x#10) have no Bluetooth. [CODE ../hal/fm1_cpu1.h]
LIM-12. Only the SMK-37 build is maintained. The FM-1 build is not maintained. Its link step fails because the XIP region overflows by about 648 B. The user accepts this. It is not a defect. [USER]

## 18. Open questions

These are disagreements between the code and a source document, or behaviour that needs a user decision. Do not report them as defects. Do not fix them without a user decision.

OQ-11. The GLOBAL menu has no "forget pairing" row. X0X never pairs or bonds (BLE-07), so there is nothing to forget. Needs a user decision: keep it so, or add pairing (then BTIF writes and a forget row are needed). [USER task 2026-10-15] Update 2026-10-16: the user chose pairing with bonding (BLE-07, BLE-11). A forget row is still open.
OQ-12. tools/fm1_install.py adds ~/.config/x0x/ctl.img to an SMK-37 install by itself when the file exists (and says so). Needs a user decision: keep this default, or add the image only with --ctl. [CODE ../../tools/fm1_install.py with_ctl]

Resolved open questions (do not reuse these IDs): OQ-01 (see GEN-40), OQ-02 (see CLP-26 and CLP-27), OQ-03 (see CLP-28), OQ-04 (see LIM-08), OQ-05 (see CLP-56, LIM-10, LIM-11), OQ-06 (see GEN-44), OQ-07 (see BRK-22, BRK-26, FLS-07; the 22.05 kHz in DESIGN 2026-10-03-x0x-design.md is the old plan), OQ-08 (see MID-10), OQ-09 (see CLP-21), OQ-10 (see CLP-34).
