# X0X intended behavior (SMK-37 and FM-1)

## 1. Purpose and use

This file is the intended-behavior specification for X0X. Review agents and bug-hunt agents use it. It tells an intended behavior from a defect.

USE-01. Before you report a defect, compare the behavior with this file.
USE-02. Report a difference as a defect only if it contradicts a statement in this file.
USE-03. If this file does not cover a behavior, report it as "unspecified". Do not report it as a defect.
USE-04. If two statements in this file disagree, or the code and a source document disagree, see section 18 (Open questions). Do not select one yourself.
USE-05. On the SMK-37, SMK37.md, APC40_IMPLEMENTATION.md and the [USER] items have priority over docs/MANUAL.md. The manual is for the FM-1.
USE-06. An agent that changes intended behavior must update this file in the same task.
USE-07. A [CODE file:line] tag gives the line at the time of writing (2026-10-15). Lines can move. Search for the named function if the line does not agree.

Source tags:

- [MANUAL §n]: docs/MANUAL.md, section n.
- [SMK37]: SMK37.md.
- [APC]: APC40_IMPLEMENTATION.md, section "Implemented in source", unless the tag gives a version.
- [PADMAP]: PAD_MAP.md.
- [DESIGN file]: docs/plans/file.
- [CODE file:line]: we verified the statement in the code. Paths are relative to firmware/src unless given.
- [USER]: a decision of the owner. A [USER] item has priority over all other sources.

## 2. Global and hardware

### 2.1 Boards and builds

GEN-01. One source tree builds two boards: the FM-1 and the SMK-37. All SMK-37 code is inside BOARD_SMK37 conditions. [SMK37] [CODE x0x.c:11]
GEN-02. Changes for the SMK-37 must not change the FM-1 build. [SMK37]
GEN-03. The SMK-37 Pro build has the package identity "SMK-37 Pro_900". MODEL=elite gives "SMK-37 Elite_900". MODEL=mke gives "MKE-P37_900". [SMK37] [CODE x0x.c:47]
GEN-04. The Starrykey 37 Play uses the SMK-37 Pro build. [SMK37]
GEN-05. The installer refuses an FM-1 package on an SMK-37 and an SMK package on an FM-1. [SMK37]
GEN-06. The SMK-37 has no TRS MIDI input. The firmware does not use the SMK TRS jack. MIDI on the SMK-37 is USB only. [SMK37] [CODE x0x.c:55-65]
GEN-07. Bluetooth is for MIDI only. There is no Bluetooth audio. [USER]
GEN-08. Bluetooth MIDI is not available yet. [USER]
GEN-09. Bluetooth settings will be in the GLOBAL (GLO) settings menu. The BT button keeps its current function, or it can have a function only in clip mode. [USER]
GEN-10. On the SMK-37, the BT button is SAVE. [SMK37] [CODE app/panel.c:28-40]
GEN-11. X0X uses both processor cores when the second core works. The 808 and the break run on the second core. If the second core reads data incorrectly, X0X continues with one core and shows "1 CORE: BAD READS". [MANUAL §15]
GEN-12. When the audio load is near the limit, X0X can stop 303 oversampling and shorten 808 tails. The screen then shows GUARD. This is the intended behavior. [MANUAL §15]
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
GEN-29. When clip mode is off and no menu list is open, "<" and ">" move SELECT one step back or forward for each press. A hold does not repeat. They do nothing while you hold HOME. In a menu list, GEN-56 applies. [SMK37] [CODE app/main_fm1.c smk_arrows] [CODE app/smk_arrows.h]
GEN-30. FADER BANK switches the faders between bank A and bank B. The FADER BANK LED is on for bank B. [SMK37] [CODE app/main_fm1.c:607-613]
GEN-31. Bank A faders 1-4 set the levels of the 909, the 808, 303A and 303B. [SMK37] [CODE app/main_fm1.c:628-629]
GEN-32. Bank B faders 1-4 set the BREAK level, the reverb return, the delay return and the master FILTER. [SMK37] [CODE app/main_fm1.c:630-637]
GEN-33. A fader takes over when it reaches or crosses the saved value, within two raw counts. This applies after a bank change and after a knob or loaded project changes the target. Until then, moving it shows the fader number and the direction toward the saved value. The hint leaves the pattern or song bar and tempo visible. [SMK37] [CODE app/main_fm1.c smk_faders; app/ui.c draw_header]
GEN-34. A fader sets the same value as the related knob. The project keeps the value. With REC on, the fader records knob motion. [SMK37] [CODE app/main_fm1.c:641-642]
GEN-35. The pitch wheel changes the TUNE of the 303 on the screen by about +-1.5 semitones. It goes back when you release it. On other parts it does nothing. [SMK37] [CODE app/main_fm1.c:708-712]
GEN-36. The mod wheel opens the CUTOFF of the 303 on the screen from the saved value to the top. On other parts it does nothing. [SMK37] [CODE app/main_fm1.c:713-720]
GEN-36A. Changing the visible page or selected part sends no pitch-wheel or mod-wheel sound change. Each wheel gesture stays with its original 303 until the wheel returns to rest, including when that part is offscreen. A stationary wheel does not overwrite knob edits or sequencer motion. [USER] [CODE app/main_fm1.c smk_wheels]
GEN-37. The wheels change only the sound. They do not change saved values. [SMK37] [CODE app/main_fm1.c:698-705]
GEN-38. The keys and the pads play without velocity. [SMK37]
GEN-39. OCTAVE- and OCTAVE+ held at power-on start HARDWARE CALIBRATION. It teaches the buttons and K2 to K8 again. It does not teach K1. [SMK37] [CODE app/main_fm1.c:1039-1041] [CODE app/panel.c:100-101]
GEN-40. A KNOB BANK press toggles clip mode (section 15). A hold makes one change. [SMK37] [CODE app/main_fm1.c smk_frame]
GEN-41. The SMK-37 keybed has no LEDs. Black-key information (tracks, slices, part mutes) is on the screen only. [CODE app/plat_fm1.c:50-53]
GEN-42. The first start of X0X on an SMK-37 clears the data of another firmware in the store area. The screen then shows "OLD DATA CLEARED". [SMK37] [CODE storage.c:62-100] [CODE app/main_fm1.c:1049-1051]
GEN-43. The firmware never writes the calibration, the Bluetooth pairing or the key areas of the flash. [SMK37] [CODE ../hal/fm1_flash_map.h:24-83]
GEN-44. With FREE PLAY on and REC off, the keys play the 303 without writing notes, and the SMK pads stay in the step-grid layout. This is the current behavior of the existing 303 FREE PLAY feature. This file does not specify the pad layout in this case. The owner accepts it. [USER] [CODE app/main_fm1.c:758-759] [CODE app/ui.c:128] (Was OQ-06. Resolved 2026-10-15.)

### 2.3 FM-1 controls

GEN-50. On the FM-1, the controls are as in the manual, section 3. [MANUAL §3]
GEN-51. Hold OCT- and OCT+ for 5 s to go into update mode. Release them before the countdown ends to cancel. [MANUAL §17] [CODE app/main_fm1.c:1075-1098]
GEN-52. Hold OCT- and OCT+ at power-on for calibration. [MANUAL §17]
GEN-53. After two crashes in a row at start, X0X starts in SAFE MODE. Safe mode has no sound, but USB works. The screen shows the last crash: PC, and one more line with the core (1 or 2), the vector and the debug status word. [MANUAL §17] [CODE app/main_fm1.c safe_main] (Changed 2026-10-15: the core line.)
GEN-54. A knob turn shows a readout box over the main area: the name on the left and the value on the right. The value uses the 23 px face. (Changed 2026-10-16: we removed the 31 px face for numbers to free 7.6 KB of flash for the clip mode screen. The owner approved.) [USER] [CODE app/ui.c draw_readout]
GEN-55. The 12 px and 15 px faces (XS, S and B) have their own glyphs for only 11 lower-case letters. They are a b d k m p r s t x z. Each other lower-case letter shows as its capital letter, also in sample names. The characters ` { | } ~ show as "?". The 10 px and 23 px faces do not change. (Added 2026-10-16: 4816 B of flash, with owner approval.) [USER] [CODE tools/gen_font.py LOWER_KEEP]
GEN-56. When a menu list is open (GLO, or the list of a page that SEL opens), "<" and ">" step the value of the selected row. They step it one step down or up, as ALGORITHM does. They do not move the row. This is also true in clip mode (CLP-27). Then only smk_arrows reads "<" and ">", and the clip keys ignore them (CLP-76).
  - Thus one press gives one step. A hold repeats the step: the first repeat comes 400 ms after the press, then one step each 80 ms. The repeat uses the 16 ms frame tick, so a step comes on the first frame at or after its time. The value stops at its first or last value (no wrap). At an end, a press keeps the value.
  - A wide value (for example 0-127) steps one at a time: the presses and the repeats are too slow for the knob acceleration. An action row (for example SAVE, CLEAR PATTERN, RESET PROJECT) and a section header do nothing. If "<" and ">" go down in the same frame, no step occurs. A press of one arrow while you hold the other gives one step and stops the repeat. While you hold HOME, they do nothing.
  - A question (O_ASK) is not a menu list: the arrows do nothing there. The function list_sub_arrow in ui.c is the hook for submenu rows (">" enters, "<" goes back). This build has no submenu rows. (Added 2026-10-16, a77, owner approval 21:20.) [USER] [CODE app/smk_arrows.h arw_route arw_frame] [CODE app/ui.c ui_menu_arrow] [CODE app/main_fm1.c smk_arrows]

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
DRM-10. The 909 hats and cymbals are samples. The synthesizer makes all other drum sounds. [MANUAL §5]
DRM-11. The 909 hi-hat, ride and crash samples are IMA ADPCM at 44.1 kHz in flash. They use the same codec as the loops (dsp/ima_adpcm.h). Each voice decodes forward as it plays. The output is bit-identical to playback of a fully decoded buffer.
  - The decoded samples are equal to the approved A/B study (adpcm_ab, 2026-10-15). The ADPCM error is white noise that follows the envelope (SNR 16.5-18.3 dB against the original PCM). This is the intended behavior. [USER] [CODE dsp/drum909.c:349] [CODE dsp/drum909.c:739] [CODE ../../tools/gen_drum_samples.py]
DRM-12. Cost of the cymbal decode: one decode for each source sample played. That is 1 decode for each output sample at pitch 1, and up to 4 at pitch x4, for each sounding cymbal voice. Estimate (not measured on the device): about 20-30 cycles for each decode. At 360 MHz, each sample has 8163 cycles. 3 cymbals at pitch 1 use about 1 % of them, and 4 voices at pitch x4 use about 4-6 %. The 909 runs on core 0. [CODE dsp/drum909.c:739-745] [DESIGN 2026-10-04-dual-core-and-clock.md]

## 4. 303A and 303B

BAS-01. There are two identical 303s. Each has its own sound and its own line. [MANUAL §6]
BAS-02. On SEQ, hold a step and turn the knobs. KNOB 1 sets the note, KNOB 2 the gate (REST, NOTE, TIE), KNOB 3 the accent and KNOB 4 the slide. [MANUAL §6]
BAS-03. A tie holds the previous note through the step. The note does not play again. [MANUAL §6]
BAS-04. With REC on, all 27 keys are a keyboard. A key that you play while you hold another key slides. [MANUAL §6]
BAS-05. FREE PLAY = ON lets the keys play the 303 without writing notes. Both 303s have FREE PLAY OFF at power-on. [MANUAL §6] [CODE app/ui.c:128]
BAS-06. A 303 line has a LENGTH of 1 to 64 steps, and a DIRECTION of forward, reverse, ping-pong or random. [MANUAL §6]
BAS-07. On the Miami SOUND matrix, a WAVE or SHAPE cell shows one cycle of the selected shape and the name TRI, SQR or PUL. The cycle comes from the real wave function, not from measured audio. The SOUND rows and the parameter labels have a raised face. [CODE app/ui.c draw_wave_glyph] [CODE app/ui_waveform.h] (Added 2026-10-30: port of #650.)

BAS-07. The 303 parts use only the Chicago and Miami engines. The bass303 engine and the legacy wobble DSP are removed. The BASS303_* parameter numbers stay as legacy IDs (dsp/bass_param_ids.h). [USER 2026-10-30] [CODE dsp/bass_param_ids.h]
BAS-08. A project saved by the bass303 engine loads safely. X0X does not crash, the 303 part gets the Chicago power-on sound, and X0X does not write over the old project. [USER 2026-10-30] [TEST tests/host/legacy_303_project_test.sh]
BAS-09. The host build needs the private original-synth inputs. Without them it stops with a clear error. There is no public fallback engine. [USER 2026-10-30] [CODE host/build_host.sh]

## 5. TB-3PO

TB3-01. TB-3PO writes 303 lines. Select a 303 and press ARP. [MANUAL §7]

TB3-02. OCT+ writes a new line. OCT- mutates the current line (about one quarter of the steps change). [MANUAL §7]

TB3-03. The TB-3PO knobs do not change the line that plays. They set the next OCT+ or OCT-. [MANUAL §7]

TB3-04. The same settings and seed always give the same line. [MANUAL §7]

## 6. BREAK

### 6.1 Playback

BRK-01. The break is a loop cut into 8 equal slices. [MANUAL §8] [CODE dsp/breaks.h:2-10]
BRK-02. The sequencer steps trigger slices on a tempo-synced 8-slice loop. This is the intended behavior. [USER] [CODE dsp/breaks.c:489-552]
BRK-03. The white keys are the 16 steps of the bar. A step that is on lets the break sound on that step. A step that is off closes the gate (2 ms ramp). The generator continues under a closed gate. [MANUAL §8] [CODE dsp/breaks.c:442-445]
BRK-04. Black keys 1-8 play the live slices 1-8. A live slice acts as a gate while you hold the key. [USER] [CODE dsp/breaks.c:442-445]
BRK-05. When you hold more than one slice key, the last key that you pressed plays. This is the intended behavior. [USER] [CODE dsp/breaks.c:159-179]
BRK-06. When you release all slice keys, the generator plays again. [MANUAL §8]
BRK-07. Live slices also play when the sequencer does not run. [CODE dsp/breaks.c:600]
BRK-08. Black key 9 plays reverse, key 10 half speed, key 11 stutter, while held. [MANUAL §8] [CODE dsp/breaks.c:467-475]
BRK-09. Hold a white key and press black key 1-8: that step always plays that slice of loop A. Press the same black key again to give the step back to the generator. [MANUAL §8] [CODE app/ui.c:2176-2188]
BRK-10. A held live slice key has priority over the slice of a step. [CODE dsp/breaks.c:536-537]
BRK-11. The loop follows the tempo by a change of playback speed. Thus the pitch follows the tempo. PITCH shifts the pitch on top. [MANUAL §8]
BRK-12. On MIDI, the break uses its own channel (default channel 4) for notes in. With 8 SLICES off, notes 36-43 play slices 1-8. With 8 SLICES on, every note plays a slice: the slice is the note number mod 8. A note on that channel does not also play the drums or a 303. USB, Bluetooth and TRS share the same hold ownership: a Note Off from one input does not release a slice another input still holds, and several notes of one wrap slice keep it until the last releases. Project reinit clears wrap note refcounts so a later Note Off cannot stick. The break does not send note messages. [MANUAL §16] [CODE app/engine.c brk_midi_legacy brk_midi_wrap engine_init]
BRK-13. The pattern saves the generator settings (COMPLEXITY, ANCHOR, ROLL, FILL, RETRIG, PHRASE, A/B LENGTH, B CHANCE). It also saves the steps, slices, loop choice and 8 SLICES. [MANUAL §8]

### 6.2 Loops and imported samples

BRK-20. Imported samples play only in BREAK mode. This is the intended behavior. [USER] [CODE app/engine.c:952-976]
BRK-21. The LOOP list has installed built-in loops first, then the imported loops as BR1, BR2 and so on. The default firmware has no built-in loops. [MANUAL §8] [CODE app/engine.c]
BRK-22. The factory loops are 909 GR and 909 FL. Each is 2 bars, IMA ADPCM at 44.1 kHz, with no length cut: 220,500 samples and 110,250 B. Together that is 220,500 B. They are not in the default image. They are an optional pack (`assets/factory-breaks/`). The installer can write that pack, or your own loops, into the free tail of the app code. The whole ADPCM is kept, so reverse still plays the loop. [USER] [CODE ../../tools/gen_builtin_break.c] [CODE ../../tools/break_pack.py]
BRK-23. Do not halve the length of a factory break. [USER]
BRK-24. An imported loop gets a length of 1, 2, 4 or 8 bars. The firmware selects the length that puts the loop tempo nearest to 120 BPM, inside 80-180 BPM. [CODE app/engine.c:821-833]
BRK-25. If a pattern names an empty user slot, that loop plays the first built-in loop when one is installed. With no built-in loop it is silent. [CODE app/engine.c]
BRK-26. Loops are IMA ADPCM at 44.1 kHz. The built-in loops, tools/upload_breaks.py, tools/import_breaks.py and the break page all use 44.1 kHz by default. 22.05 kHz is an explicit opt-in only: `--rate 22050` in the tools, or the "22 kHz (more room)" box on the page. A loop that the page reads back from the device keeps its rate. (Was OQ-07. Resolved 2026-10-15.) [USER] [CODE ../../tools/gen_builtin_break.c:134] [CODE ../../tools/upload_breaks.py:87-90] [CODE ../../web/x0x_breaks.js:13] [CODE ../../web/x0x_breaks.html:77]
BRK-27. The firmware reads the rate of each imported zone from the slot header. Thus it can play a 44.1 kHz zone. The bar guess (BRK-24) uses the zone rate. A host test checks both (tests/host/loop_bars_test.c, tests/scenarios/upload44.x0x). [CODE app/slots.c:10] [CODE app/plat_fm1.c:150]
BRK-28. Trim silence (sampleio.trim_silence, trimSilence in web/x0x_breaks.js) removes the samples at or below -60 dBFS (0.001) at the two ends. It keeps 1 ms before the first louder sample. It applies a 5 ms linear fade at the end. A file that is all silence stays unchanged. The tool and the page give the same bytes. [CODE ../../tools/sampleio.py:202] [CODE ../../web/x0x_breaks.js:78]
BRK-29. Trim is off by default for loops (upload_breaks.py `--trim`, the page's "Trim silence" box) because it changes the loop length and thus the bar guess. Trim is on by default for one-shots (fm1_sample_upload.py). `--no-trim` turns it off. [USER] [CODE ../../tools/upload_breaks.py:91] [CODE ../../tools/fm1_sample_upload.py:94] [CODE ../../web/x0x_breaks.html:75]

## 7. Patterns

PAT-01. There are 32 patterns. [MANUAL §9] [CODE seq/pattern.h:12]
PAT-02. A pattern holds the steps, length and rate of each part, the break settings, the swing, the knob motion, the p-locks and the probabilities. [MANUAL §9]
PAT-03. Sounds, effects and master settings are not part of a pattern. [MANUAL §9]
PAT-04. Each part can play a different pattern. [MANUAL §9]
PAT-05. While the sequencer plays, a new pattern starts at the end of the current bar. While it does not play, the new pattern starts immediately. [MANUAL §9]
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
REC-06. With REC on and the pattern in play, a drum hit or a 303 key records on the nearest step. In the first half of a step, it records on that step. After half of the step, it records on the next step. On an external clock, half of the step is half of its ticks.
  - Before the first step after PLAY or Start, it records on the first step. If the step did not play yet on this pass, it does not sound that hit again on this pass, because the key sounded it. The next pass plays it. The MIDI note out of that step still goes out.
  - [USER] [CODE seq/sequencer.c seq_rec_step seq_rec_skip] [CODE app/ui.c drum_key bass_kbd] (New 2026-10-16, a73. The owner approved it at 20:11. Before, a hit recorded on the step that played.)

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
SNG-05. INSERT BAR and DELETE BAR change only an existing selected bar. On the blank append row, they leave the song and save state unchanged and show NO BAR SELECTED. INSERT BAR on a 192-bar song leaves the song and save state unchanged and shows SONG FULL. [CODE app/ui.c run_action]
SNG-06. HOME cancels a confirmation and returns to the overlay that opened it, including its selected list row. SELECT still confirms the action and completes it as before. [CODE app/ui.c ask button_tap]

## 11. Effects

FX-01. There is one stereo reverb and one delay. Each part and each drum track has sends to both. [MANUAL §13]
FX-02. TAPE at the far left (DIGI) is a clean 12-bit delay. Higher TAPE values give a tape delay. [MANUAL §13]
FX-03. A delay longer than 2 s plays at half that length. [MANUAL §18]

## 12. Mix and master

MIX-01. LFO opens the mixer: PARTS, 909 MIX, 808 MIX, MASTER and WAVE. [MANUAL §14] (Changed 2026-10-25: WAVE sibling page.)
MIX-02. MASTER has DRIVE, COMP, PUMP and FILTER. WAVE uses the same four knobs and shows the post-master output waveform beside the GR bar. [MANUAL §14] (Changed 2026-10-25: WAVE.)
MIX-03. PUMP also works when COMP is 0 and when you mute the kick. [MANUAL §14]
MIX-04. FILTER is off in the middle. Left is a low-pass. Right is a high-pass. [MANUAL §14]

## 13. Settings and saving

SET-01. GLO has five sections: MIDI, AUDIO, KEYS AND SCREEN, PROJECT, SYSTEM. On the SMK-37 a sixth section, BLUETOOTH MIDI, follows SYSTEM (BLE-05). [MANUAL §15] [CODE app/ui.c open_global] (Changed 2026-10-15: the SMK-37 section is new.)
SET-02. SAVE saves the sounds, all 32 patterns, the song, the knob motion, the tempo and the settings. [MANUAL §15]
SET-03. X0X never saves while the sequencer plays. A SAVE during play shows "SAVES WHEN STOPPED" and "SAVE AT STOP" in the header, including in clip mode. Once X0X stops, a queued clip-mode save shows "SAVE WHEN QUIET" until clip input has been quiet for four seconds. A SAVE pressed while clip keys or encoders are active also queues with "SAVES WHEN QUIET". [MANUAL §15] [CODE app/ui.c save_project autosave draw_header]

SET-03A. A held external MIDI note postpones manual and automatic saves without cutting the note. Manual SAVE remains queued and shows "SAVES WHEN QUIET"; after release and any remaining busy condition, it saves once. Stopped REC arm shows "REC OFF TO SAVE". A storage failure remains "SAVE FAILED" with unsaved edits retained. [CODE app/ui.c save_project autosave] [TEST host/save_note_quiet_test.c]
SET-04. AUTOSAVE is ON by default. It saves when the sequencer does not run and nobody touched a control for 4 s. The header shows UNSAVED, or a small amber mark when space is tight, when an edit needs a save. The SAVE light flashes for unsaved changes and stays on for a queued save. A successful save clears the status. [MANUAL §15] [CODE app/ui.c autosave draw_header leds]
SET-05. If the project does not fit, X0X shows "MEMORY FULL: NOT SAVED" and keeps the last saved project. Any save failure keeps a red "SAVE FAILED" header mark. When space is tight, X0X shows a red mark instead. The SAVE light flashes fast until a save succeeds. [MANUAL §15] [CODE app/ui.c save_project autosave draw_header leds]
SET-06. Undo keeps a maximum of 32 steps. Undo does not cover the tempo or the settings. Power-off and successful project reset, load or import clear the history. Cancelling an action or rejecting an invalid staged project does not replace the live project. [MANUAL §3] [CODE app/undo.c:22] [CODE app/ui.c ui_project_end] (Changed 2026-10-20: project replacement starts a new history.)
SET-07. RESET PROJECT restores project parameters, sounds, all sequencers, song and motion to defaults while preserving uploaded sample slots. It stops playback and recording and releases notes using their old channels. The new project follows the ordinary save policy; reset restores AUTOSAVE to ON, so it can overwrite the saved project after the controls become quiet. [USER project task] [MANUAL §15] [CODE app/ui.c ui_project_reset] [CODE app/project.c project_reset_stage] (Changed 2026-10-20: replaces FACTORY RESET and supersedes the earlier statement that the saved project stays until a manual save.)
SET-08. Defaults of a new project: tempo 125 BPM, CLOCK SOURCE AUTO, CLOCK OUT on, NOTES OUT off, KEY SOUND STOPPED, theme amber. A new project keeps the device's BLUETOOTH and SMC role settings. [CODE app/project.c project_defaults_for] (Changed 2026-10-16, a74: CLOCK SOURCE. a75: CLOCK SOURCE AUTO is in the bt byte, MID-13. Changed 2026-10-20: preserve device role bits when replacing a project.)
SET-09. An update keeps the project. [MANUAL §1]
SET-10. GLO > LOAD SAVED loads the saved project through staging and validation. Unsaved edits require confirmation. A failed load leaves the live project and samples unchanged. Load and transfers require stopped, quiet input; Save waits for the existing quiet-save conditions. [USER project task] [CODE app/ui.c ui_project_load save_project] [CODE app/project.c project_load_stage]
SET-11. The Projects companion exports and imports .x0xproject files. Portable files preserve every populated slot's original 512-byte header and encoded sample data, with explicit empty slots and fixed slot order. They do not resample or remap audio. Project-only imports require every sample identity to match and do not mutate sample slots. Replacement of occupied slots requires confirmation and a saved, verified recovery backup. [USER project task] [DESIGN docs/PROJECT_FILES.md] [CODE ../../web/x0x_projects.js]
SET-12. A project transfer holds runtime input and audio. Import validates the project before sample writes and verifies exact sample readback before adoption. Failed transfers attempt rollback under the hold. If original sample images do not match, ABORT and timeout retain recovery mode; reconnecting can restore a portable backup without releasing the hold first. Power loss can still require that backup because sample writes are not power-failure atomic. [DESIGN docs/PROJECT_FILES.md] [CODE app/editor.c ed_project_end] [CODE ../../web/x0x_projects.js applyProjectImport applyRecoveryImport]
SET-13. Save and export preserve the stored clock-source preference while clip mode temporarily uses AUTO at runtime. [CODE app/project.c capture_settings]

## 14. MIDI and USB

MID-01. Over USB, the device shows as the MIDI device "X0X FM-1". This name is the same on the SMK-37. [MANUAL §16] [CODE usb.c:304] [APC 1.5-apc40]
MID-02. Default channels: 909 channel 10, 808 channel 11, 303A channel 2, 303B channel 3, BREAK channel 4. [MANUAL §16] [CODE seq/sequencer.c:28]
MID-03. GLO can set each part channel to 1-16 or OFF. OFF ignores incoming notes and sends no notes for that part. [MANUAL §15]
MID-04. 909 notes: 36 BD, 38 SD, 41/45/50 toms, 37 RS, 39 CP, 42 CH, 46 OH, 49 CR, 51 RD. 808: the same, with 56 CB and 49 CY. [MANUAL §16] [CODE app/engine.c:503-504]
MID-05. On a 303 channel, overlapping notes slide. Velocity 100 or more is an accent. [MANUAL §16] [CODE app/engine.c:509-513]
MID-06. In AUTO (the default, MID-13), X0X follows an incoming MIDI clock. Start, Stop and Continue start and stop the sequencer. [MANUAL §16] [CODE app/engine.c:474-478] The transport messages act in the order that they come. Exception: a Start or Continue less than one step after PLAY (MID-14). Thus Stop, then Start in one Bluetooth connection event stops X0X and starts it again from step 1. (Changed 2026-10-16, a72: before, the Stop cancelled the Start, and X0X stayed stopped.) [CODE seq/sequencer.c seq_stop]
  - In EXTERNAL the transport messages act the same. In INTERNAL X0X ignores them (MID-13). a73: Loopy Start starts X0X (owner test 20:37-20:39 PASS, ledger c3ee3ef).
MID-07. When clip mode is off, the original sequencer and MIDI input path must stay unchanged. Firmware changes stay inside clip mode unless the owner approves. [USER] (agent-rules 14)
  - One approved exception (the owner, in chat, 2026-10-16 00:45): the tempo hand-over in MID-12. It applies also when clip mode is off.
  - A second approved exception (the owner, 2026-10-16, a74): GLO > MIDI > CLOCK SOURCE (MID-13). It changes the clock input and the clock output when clip mode is off. AUTO keeps the path as it was.
  - A third approved exception (the owner, 2026-10-16 20:50, a74): the PLAY note (MID-14). It sends a note when clip mode is off, and it changes a MIDI Start or Continue just after PLAY.
  - A fourth approved exception (the owner, 2026-10-16 21:20, a77): "<" and ">" in a menu list step the value of the selected row (GEN-56). This applies also when clip mode is off.
  - A fifth approved exception (the owner, 2026-10-16 22:41): X0X follows the clock of one input (MID-15). This applies also when clip mode is off.
MID-12. In AUTO (the default, MID-13), the external MIDI clock stops when no tick comes for 0.5 s. Then the internal tempo takes the last measured external tempo, rounded to a whole BPM (20-275). Thus the tempo does not jump, and the TEMPO value and a later SAVE show that tempo. [USER] [CODE seq/sequencer.c seq_advance] (New 2026-10-16.)
  - The measured tempo is the mean tick period over the last 8 beats (24 ticks each). The smoothing is 1/4 for each beat. A change of more than 1/64 takes the new mean immediately. Bluetooth MIDI gives all the ticks of one connection event together, so X0X does not measure the time between two ticks. (Changed 2026-10-16, a72: before, X0X measured the time between two blocks with ticks. In the host test with Bluetooth bursts at 120 BPM, the kept tempo was 116-127 BPM.) [CODE seq/sequencer.c ext_ticks]
  - In EXTERNAL the tempo hand-over and the tempo measure are the same. But X0X does not go to its internal clock. It pauses and waits (MID-13). In INTERNAL X0X does not measure a clock.
MID-08. USB audio is a 2-channel input at 44.1 kHz. It carries the headphone signal. [MANUAL §16] [CODE usb.c:74]
MID-09. The firmware answers a Universal Device Inquiry as an Akai APC40 mkII (manufacturer 0x47, product 0x29). [APC] [CODE usb.c:200-216] [CODE usb.c:694-701]
MID-10. On all boards (also the FM-1), the board does not gate the APC40 identity reply and the APC40 introduction reply. The owner accepts both. They are not defects. [USER] [CODE usb.c:694-701] [CODE usb.c:702-710] (Was OQ-08. The owner resolved it on 2026-10-15.)
MID-11. A USB bus reset or a deconfiguration clears the stored APC clip state. [CODE usb.c:554-559] [CODE usb.c:1113-1117] [CODE app/main_fm1.c:339-359]
MID-13. GLO > MIDI > CLOCK SOURCE is the first row of MIDI, above CLOCK OUT. The values are AUTO, INTERNAL and EXTERNAL. The default is AUTO. SAVE and autosave keep it, as CLOCK OUT (SET-02).
  - It uses bits 1-2 of the settings byte bt (offset 27). Bit 3 of bt shows that bits 1-2 hold the value. Bit 0 stays BLUETOOTH (BLE-05). A load sets a value of 3 to AUTO.
  - A project without bit 3 comes from a74 or older. a74 kept the value 0-2 in the settings byte at offset 38, and offset 39 was 0. If offset 38 is 0-2 and offset 39 is 0, the load takes offset 38 as the value and sets offset 38 to 0. Else the load gives AUTO, and offsets 38 and 39 stay.
  - The clip keys use offset 38 for ck_x1 (ARP SWING and HUMAN) and offset 39 for ck_x2. Thus a stale a74 value 1 or 2 does not become ARP SWING 1 or 2. A project of X0X 0.10 or a73 loads AUTO. [USER] [CODE app/ui.c open_global] [CODE app/x0x.h PROJ_BT_CLK_IN] [CODE app/project.c project_clk_check] (New 2026-10-16, a74, with owner approval of the design and all recommendations. Changed 2026-10-16, a75: before, the value was at offset 38 only. Test: tests/scenarios/clkmove.x0x.)
- AUTO: X0X follows a clock when one comes in, and it uses its internal clock when no clock comes in. This is the behavior before a74 (MID-06, MID-12, LIM-11).
- INTERNAL: X0X ignores MIDI Clock, Start, Continue and Stop from USB and Bluetooth. PLAY starts and stops X0X. The steps and the swing use the TEMPO value. When CLOCK OUT is ON, X0X always sends its clock, Start, Continue and Stop, also when a clock comes in. If X0X followed a clock at the change, the next step of each part comes at the time of its clock tick.
- EXTERNAL: X0X plays only on the incoming clock. It never sends clock, Start, Continue or Stop. MIDI Start, Continue and Stop act as in AUTO. PLAY with no clock arms X0X: the first tick plays step 1. PLAY again cancels.
- EXTERNAL with no clock for 0.5 s while X0X plays: X0X pauses. The 303 notes and the break stop at once, and X0X keeps its step. When the ticks come back, X0X goes on from the next step. The tempo takes the last measured tempo (MID-12).
- EXTERNAL, X0X waits (it plays, and no clock came for 0.5 s). The play arrow at the top of the screen and the PLAY light blink slowly (about 1 time each second). The top line shows "WAITING FOR CLOCK" for 1.3 s when the wait starts.
- EXTERNAL: the tempo value at the top right is amber. It shows the last tempo that X0X measured, or the TEMPO value if no clock came yet. The TEMPO knob sets the internal tempo, as in AUTO with a clock.
- The change to EXTERNAL while X0X plays on its internal clock pauses X0X until the next tick. That tick is tick 0 of the step that plays (as LIM-11).
- Clip mode ignores this setting and always uses AUTO (CLP-56).
- MIDI notes, CC, SysEx and the APC40 feedback pass in all three modes. The clip-mode PLAY blink, the song position and the clip keys arp use the incoming clock in all modes. [CODE seq/sequencer.c seq_midi_rt clock_source seq_clock_waiting] [CODE app/engine.c midi_in] [CODE app/ui.c ui_frame leds draw_header]
MID-14. SMK-37, clip mode off: a PLAY press also sends note on 0x5B, velocity 127, on MIDI channel 16. The release sends note off 0x5B, velocity 0. Channel 16 is the channel of the Loopy Undo and Redo notes (CLP-52). The note goes to USB MIDI and to a connected Bluetooth MIDI central.
  - PLAY starts and stops X0X as before. NOTES OUT, CLOCK OUT and CLOCK SOURCE do not stop the note. HOME + PLAY (X0X redo) sends no note. In clip mode, PLAY sends only the APC PLAY note (CLP-50).
  - If clip mode starts or stops while you hold PLAY, the release sends the note off of the press. The owner uses the note in Loopy Pro for "Toggle Clock Pause". [USER] [CODE app/main_fm1.c smk_play_note_button] [CODE app/smk_apc40.h smk_apc40_play_note_packet] [CODE app/ui.c input] (New 2026-10-16, a74. The owner approved it at 20:50.)
- If PLAY started X0X less than one step ago, a MIDI Start or Continue does not start X0X again. X0X already plays step 1, so step 1 does not play two times. The next clock tick is tick 0 of the step that plays (LIM-11). A Start or Continue later than one step after PLAY acts as before (MID-06). [CODE seq/sequencer.c seq_advance]
- If PLAY stops X0X while it follows the Loopy clock, a later MIDI Stop from Loopy keeps X0X stopped.
- The default channels of the parts (MID-02: 10, 11, 2, 3, 4) and the clip keys (CLP-61: 2, range 2-15) are not 16. GLO can set a part channel to 16 (MID-03). Then that part's notes and the PLAY note share channel 16.
MID-15. In AUTO and EXTERNAL (MID-13), X0X follows the clock of one input only. The inputs are USB, Bluetooth and the TRS jack (FM-1 only). The first input that sends Clock, Start or Continue gets the clock. X0X ignores Clock, Start, Continue and Stop from the other inputs. The input keeps the clock until it sends no Clock, Start or Continue for 0.4 s.
  - A Stop of the input with the clock starts a time of 0.4 s. If that input sends no Start or Continue in this time, the next Start or Continue of another input after this time gets the clock. A Clock of another input does not get it then. A Stop alone gives no input the clock.
  - When the link of the input with the clock ends, that input loses the clock immediately. Then the next Clock, Start or Continue of another input gets it. A link ends at a Bluetooth disconnect, BLUETOOTH OFF or a Bluetooth controller failure. On USB, a link ends at a bus reset or when the USB frames stop for 100 ms (unplug or host asleep). (Added 2026-10-16 22:57.) [CODE usb.c midi_link_check] [CODE seq/sequencer.c seq_clk_drop]
  - If two inputs send in the same timer tick, the first message in the input queue wins (USB, then TRS, then Bluetooth). 0.4 s is more than 3 clocks at 20 BPM (125 ms each), and less than the 0.5 s of MID-12 and MID-13. Thus another input gets the clock before X0X goes to its internal clock (AUTO) or waits (EXTERNAL).
  - When another input gets the clock, the tempo measure (MID-12) starts again from the tempo that X0X has now. The first ticks of the new input give no tick period. Thus the tempo stays within 1 BPM after at most 2 beats, and a clock stop just after the change keeps the tempo. (Changed 2026-10-16 22:57: before, the gap stayed in the measure for about 10 beats, for example 109 BPM instead of 120.)
  - INTERNAL ignores the clock of all inputs, as before. It gives no input the clock. The clip-mode PLAY blink and the clip keys arp see the clock of the same input (clip mode uses AUTO, CLP-56).
  - Song Position follows the same input: X0X ignores a Song Position from another input while an input has the clock. A Song Position gets no clock and does not keep the clock. With no input that has the clock, and in INTERNAL, a Song Position passes from all inputs. (Changed 2026-10-16 22:57: before, Song Position passed from all inputs.)
  - Notes and other messages pass from all inputs. The input is in bits 4-5 of byte 0 of the MIDI in packet (0 USB, 1 Bluetooth, 2 TRS). [USER 2026-10-16 22:41] [CODE seq/sequencer.c seq_clk_take] [CODE app/engine.c midi_in] [CODE usb.c ep1_rx] [CODE ble.c ble_service] [CODE midi_uart.c um_put] [TEST tests/host/clk_lock_test.c] [TEST tests/scenarios/clklock.x0x] (New 2026-10-16. The owner approved it at 22:41. Before, each input counted, so two inputs gave two times the tempo, LIM-10.)

### 14.1 Bluetooth MIDI (SMK-37 only, roadmap step 5)

BLE-01. Bluetooth MIDI is BLE MIDI only (Bluetooth Low Energy, no Bluetooth audio). It needs a controller image in the Bluetooth block (FLS-13). The end user builds the image on their own computer from JieLi's files (ble/ctl/build_ctl.sh, pinned gitee commit d179b44, btctrler.a SHA-256 9ef42624...). X0X never ships the image or any JieLi Bluetooth file. The image is a separate program: the app talks to it only through standard HCI packets in a shared-RAM mailbox. [USER] [CODE ../hal/x0x_ble_abi.h] [CODE ble.c]
BLE-02. Without an image, or with an image that fails its check, the app starts exactly as before. Core 1 starts with its own stacks, no controller code runs, and X0X routes no interrupt to core 1. GLO shows NO IMAGE or BAD IMAGE. The check covers the header magic and version, the ABI, the size and the CRC-32. It also covers the init and poll entries (inside the image), the RAM window (0x01C01800-0x01C07EFF) and the interrupts (none that X0X uses on core 0). [USER] [CODE ble/ble_glue.h ble_img_check] [CODE ble.c ble_prepare]
BLE-03. The controller starts only at power-on, and only when BLUETOOTH is ON. Its init runs once on core 0 at power-on, before core 1 starts and before the audio starts (about 0.6 s). X0X feeds the watchdog before and after. Core 1 then runs the controller: its interrupts and its poll. The controller's interrupts stay off during the init. Its first poll on core 1 routes them to core 1 again and turns them on.
  - The RF init gets a fixed crystal trim of 11/11 (0x0B, the SDK default and the value of the working SMK-37 Oscuro firmware). Before a67 it got the trim that the boot left in WLA_CON12. The Bluetooth TX level is 6, as in the working SMK-37 Oscuro firmware (11 before a66). Level 5 did not change the core 0 failure of test 20.
  - While the controller runs, the core-0 guard for accesses to invalid bus addresses is off. The cause is that the RF calibration of the JieLi files touches the radio registers. The stack, write and PC guards stay on. While core 1 runs the controller, X0X closes the core 0 stacks to writes from core 1 and from DMA (write limit 3).
  - The timer 5 interrupt saves 8 words on the core 0 stack. While it runs, X0X closes these words to writes from core 0, core 1 and DMA (write limit 4). X0X checks them at 4 points, and records and sets back a change.
  - Core 1 watches core 0's ms counter: when it stops for 100 ms, core 1 records core 0's last branches (ETM), DBG_MSG and EMU_MSG. Core 1 also checks 8 KiB of unused RAM (0x01C72000-0x01C74000, set to all ones before the start) and records changed words.
  - X0X sets the stock supply before the RF init, with the interrupts off and core 1 not started. It uses the stock V15 order: DCDC13 on, LDO13 off, VDC14 1.60 V, VDDIOM and VDDIOW 3.2 V, SYSVDD 1.38 V. X0X records the rails and P3_ANA_CON0 before and after (DIAG). With BLUETOOTH OFF the boot supply does not change. The system clock stays 360 MHz.
  - The controller gets the host's HCI frames through the entry it registers with its transport (hci_h4_download_data), not through the library's H4 send. Then core 1 calls its poll entry between audio jobs.
  - If the start fails, or the image reports a failure later, X0X stops calling it, turns its interrupts off on core 1 and shows FAILED. If its start does not end in time, X0X holds core 1 and runs on one core. [CODE ble.c ble_start] [CODE ../hal/fm1_cpu1.h] (Changed 2026-10-15: 4 s, the bus guard, the stack write limit, the HCI entry and TX level 11 again. Also changed: the frame watch, the core 1 watch of core 0, the RAM canary and the stock supply setup. Changed 2026-10-16: TX level 6, crystal trim 11/11.)
BLE-04. Before X0X turns the flash off (erase, write, SPI read), it parks core 1. A parked core 1 waits in RAM with its interrupts off and runs no controller code. If core 1 does not park within 20 ms, X0X holds it (one core, no Bluetooth). Without a controller X0X parks nothing. [CODE ../hal/fm1_cpu1.h fm1_cpu1_park] [CODE ../hal/fm1_flash.h]
BLE-05. GLO > BLUETOOTH MIDI has three rows. The BLUETOOTH row sets OFF or ON (the default is OFF). X0X saves it in the old reserved byte of the settings. An older project loads OFF.
  - STATUS (read only): NO IMAGE, OFF, RESTART, STARTING, READY (advertising), CONNECTED, FAILED or BAD IMAGE. NAME (read only): "X0X-" and 4 hex digits. ON when the controller did not start at power-on shows "RESTART TO TURN ON". OFF while it runs drops the connection and stops advertising. [CODE app/ui.c] [CODE app/x0x.h settings_t.bt]
BLE-06. The unit advertises as a Bluetooth MIDI peripheral with a random static address. The address and the name come from a CRC of the factory record (USRTRIM, read only). Thus they stay the same for a unit across power cycles and differ between units. The unit accepts only one connection at a time. [CODE ble/ble_glue.h ble_addr_of] [CODE ble/ble_midi_host.c]
BLE-07a. X0X sends no connection parameter request (tests 31 and 34 made the iPhone link drop or slow down). DIAG shows the interval in use, the MTU that the central asked for and the pairing refusals. It also shows the rejected L2CAP requests, the end reason of the last link and the MIDI in drops. [CODE ble.c ble_h_link0] (Changed 2026-10-15, test 37.)
BLE-07. X0X does not pair and keeps no bonds. A pairing request gets "Pairing Not Supported". X0X writes nothing to BTIF (0xC0000). [CODE ble/ble_midi_host.c smp_in]
BLE-08. Bluetooth MIDI in acts like USB MIDI in. Notes, clock, Start, Stop and Continue go into the same input queue. Notes first go through the same APC40 clip and transport feedback handling (clip colours on the pads). X0X does not use the Bluetooth timestamps to schedule the messages. Up to 64 incoming messages wait between two service ticks. If more messages come, X0X drops them and counts them.
  - Incoming SysEx: a Universal Device Inquiry and the APC40 mkII introduction get the same replies as on USB, over Bluetooth. Loopy Pro can use the Bluetooth port as its APC40. X0X does not use other SysEx (it counts it).
  - When USB and Bluetooth both send clock, X0X follows one of them (MID-15). [CODE ble.c ble_service ble_sysex_in] (Changed 2026-10-15: APC40 over Bluetooth.)
BLE-09. Everything X0X sends as MIDI out (clock, Start, Stop, notes) also goes to a connected Bluetooth MIDI central that has notifications on. The APC40 identity and introduction replies to a SysEx that came over Bluetooth go to Bluetooth (BLE-08). In clip mode, three types of MIDI go to USB and to a connected Bluetooth MIDI central. They are the APC feedback replies, the pad clip-launch notes and the keybed notes (CLP-60). Thus Loopy Pro on the Bluetooth port can launch clips and get the keys. (Changed 2026-10-16: the keybed notes.)
  - The undo/redo history notes stay on USB only. The PLAY note of MID-14 goes to USB and to Bluetooth (added 2026-10-16, a74). (Changed 2026-10-15.) X0X does not mirror SysEx. [CODE app/plat_fm1.c plat_midi_out] [CODE app/main_fm1.c]
BLE-10. Every install writes the whole app slot, also the Bluetooth block. To keep Bluetooth, add the image again for each update. Use tools/add_ctl.py, tools/fm1_install.py --ctl FILE or the ctl.img field of the web installer (SMK-37 only). tools/fm1_install.py also uses ~/.config/x0x/ctl.img by itself, and --no-ctl skips it. Without the image, the block is empty after the install (BLE-02). [CODE ../../tools/add_ctl.py] [CODE ../../tools/fm1_install.py] [CODE ../../web/fm1pkg.js insertCtl]
BLE-11. tools/add_ctl.py and the web installer write the Bluetooth block only when the block is free. The block is free if it is all 0xFF or a controller image (X0BC header). It is also free if the package has the slot map record (FLS-14, the build limits the app code since then).
  - An older X0X package (for example 1.8-apc40) has app code in the block. tools/add_ctl.py and the web installer refuse such a package with a message, also for --remove. tools/fm1_install.py --ctl FILE refuses it too (exit badpkg). The cached ~/.config/x0x/ctl.img never stops an install: on such a package the installer skips it with a warning and installs the package without the image. (Added 2026-10-15.) [CODE ../../tools/add_ctl.py block_free] [CODE ../../tools/fm1_install.py with_ctl] [CODE ../../web/fm1pkg.js insertCtl]
BLE-12. A start record in RAM that a reset keeps (.noinit) protects the power-on. Before the init the app marks the start as trying. Two events block Bluetooth. One is a start that does not end READY (not in time, wrong job, not ready). The other is a power-on that finds the mark still set. The mark stays set after a crash, a hang or a power-off during the start or in the first 10 s after READY.
  - Then X0X does not start the controller at the next power-ons, and STATUS shows FAILED while BLUETOOTH stays ON. So a failing start ends in FAILED, not in a crash at each power-on. 10 s after READY the start counts as good, and X0X clears the mark.
  - Set BLUETOOTH OFF to clear the block (STATUS OFF). Then set ON again. The controller starts at the next power-on. Turn the unit off only when READY has shown for 10 s. (Added 2026-10-15.) [CODE ble/ble_glue.h ble_guard_*] [CODE ble.c ble_prepare ble_service plat_ble_set]
BLE-13. The editor command 33 DIAG (read only, web/EDITOR_PROTOCOL.md) returns the crash record and the boot guard counts. It also returns the start record of BLE-12 and the live Bluetooth state. The image writes its start stage, its time and its delay count to the mailbox (dbg words). It also writes the callers of the last delay and the stalls of the tick counter. After a reset during the start, the start record also keeps the reset source of that reset. If TIMER4 stands still during the start, core 0 does not wait for ever: the start fails (STATUS FAILED).
  - The first 1 KiB of RAM (0x01C00000) holds no code. X0X fills it before the start and checks it after the start. Thus DIAG shows whether the radio wrote there. The check after the start comes before the first P33 access. Core 0's wait records how it ended (result, TIMER4 stalls, time) in RAM that survives a reset.
  - When the image stops on a library reset request or assert, it records the caller and its heap use (peak, failed allocations). The start record also keeps WLA_CON12 (the crystal trim) before the start and after the init. DIAG also gives the HCI rings (written, read), the step of the host and the frames that the controller took and gave. It also gives the last frame from the controller (type, length, first 6 bytes) and the rails. It also gives the last Bluetooth tick of core 0 (time, core 1 poll count).
  - Editor command BT_TEST op 3 returns the timer 5 frame watch of the last boot. Op 4 returns the records of core 1 (core 0 seen stopping, the RAM canary). The mailbox and host values are raw: after a reset they are the last start's. (Added 2026-10-15.) [CODE app/editor.c ED_DIAG] [CODE ble.c ble_diag] [CODE ../../ble/ctl/src/ctl_main.c stage]
BLE-14. The editor command 34 BT_TEST (SMK-37 with Bluetooth, web/EDITOR_PROTOCOL.md) is for tools/ble_autotest.py. Op 1 clears the start block as BLUETOOTH OFF does. The BLUETOOTH setting does not change. Op 2 restarts the unit 200 ms after the reply (a chip reset, as after a crash). It never starts the update loader. (Added 2026-10-15.) [CODE app/editor.c ED_BT_TEST] [CODE app/main_fm1.c]
BLE-15. The editor command 38 INLOG (SMK-37, read only, web/EDITOR_PROTOCOL.md) is the inbound recorder. The firmware keeps the last 64 inbound clip messages in a ring (16 before 2026-10-16 23:00). Each entry has the time, the source, the status and the two data bytes. The source is 0 USB, 1 Bluetooth or 3 Bluetooth colour while notify is off (no cell change).
  - Source 3 is a Bluetooth note-on with a colour for a clip cell while notify is off: the cell did not change (CLP-18). The ring also keeps each change of the Bluetooth link (source 2: connected, notify on). It keeps only note-on and note-off messages for notes 0-98. Argument 0 also returns the 40 clip velocities and channels.
  - Argument 1 returns the 16 pad colours in the pad colour store and the 16 pad colours in the LED driver data. tools/ble_autotest.py --diag-only shows them as text and writes MISMATCH when a pad does not agree with its cell. (Added 2026-10-16, a76.) The reply with 64 entries has 732 bytes. The editor reply buffer (editor.c ed_out) has 732 bytes (tests/diag_size_test.py). [CODE app/smk_inlog.h] [CODE app/main_fm1.c smk_inlog_reply] [CODE ../../tools/inlog_fmt.py] [TEST tests/smk_inlog_test.c]
BLE-16. When a Bluetooth link ends, X0X sends All Notes Off, so no note hangs. [USER 2026-10-16] This All Notes Off is inside X0X only. X0X sends no MIDI message out for it: no CC 123 and no note-off, on USB or on Bluetooth (the owner decided this 2026-10-16 22:57). X0X releases each note that sounds from a Bluetooth note-on: the note of each 303 part and each held BREAK slice (notes 36-43). The notes from USB stay.
  - The drum notes are one-shot, so they do not hang. If a USB note-on came after the Bluetooth note-on on the same 303 part (a slide), USB holds that note, and it stays. This applies to a disconnect, to BLUETOOTH OFF and to a controller failure (STATUS FAILED). usb.c midi_link_check puts one link-end packet (CIN 0, input 1) into the MIDI in queue after the MIDI in of that tick. The engine then releases the notes.
  - The same packet frees the clock of Bluetooth (MID-15). CLP-70 ends the clip keys notes at a Bluetooth disconnect, as before. [CODE usb.c midi_link_check] [CODE ble.c ble_service] [CODE app/engine.c midi_in src_held] [TEST tests/scenarios/blenotes.x0x] (Was BLE-12 on smk37 before 2026-10-16 21:53. The code has it from 2026-10-16. Before, no code did it on integration 110d4c2.)
BLE-17. When the USB link ends, X0X releases each note that sounds from a USB note-on, as BLE-16 does for Bluetooth. This applies to the 303 notes and the held BREAK slices (notes 36-43). The notes from Bluetooth stay. If a Bluetooth note-on came after the USB note-on on the same 303 part (a slide), Bluetooth holds that note, and it stays.
  - The USB link ends at a bus reset, and when the USB frames stop for 100 ms (unplug or Mac asleep). This release is inside X0X only. X0X sends no MIDI message out for it. After the USB link comes back, new USB notes play as before.
  - usb.c midi_link_check puts one link-end packet (CIN 0, input 0) into the MIDI in queue. The same packet frees the clock of USB (MID-15). [USER 2026-10-16 23:06] [CODE usb.c midi_link_check] [CODE app/engine.c midi_in src_held] [TEST tests/scenarios/usbnotes.x0x] (New 2026-10-16.)

## 15. Clip mode (APC40 mkII emulation for Loopy Pro), SMK-37 only

### 15.1 Mode

CLP-01. Clip mode exists only on the SMK-37 build. [CODE app/main_fm1.c:170]
CLP-02. A KNOB BANK press toggles clip mode. A hold makes one change. The screen shows "LOOPY CLIPS" at entry and "LP OFF" at exit. [APC] [SMK37] [PADMAP] [CODE app/main_fm1.c smk_frame]
CLP-03. Clip mode is off at power-on. [CODE app/main_fm1.c:176]
CLP-04. In clip mode, the LCD main area shows the clip map. When a menu is open (for example GLO), the LCD shows the menu. When the menu closes, the clip map comes back. The mode chip at the top left reads "LP" (Loopy Pro). Where the X0X shows the pattern, the header shows the clock of the arp: EXT (amber) or INT (CLP-67).
  - The tempo stays. While a SCALE, CHORD or ARP page is open (CLP-75), the page covers the clip map. The bottom strip shows the KEYS panel (CLP-74). (Changed 2026-10-16: the menu and LP. Changed 2026-10-16: EXT/INT, the page and the KEYS panel.) When clip mode stops, the normal X0X view comes back at the next frame. [APC 1.5-apc40] [CODE app/ui.c:3788-3799]
CLP-05. LCD clip colours: a lit cell is a filled square. The colour comes from the table APC40_LCD565 (one RGB565 value for each velocity 0-127). tools/apc_pad_colors.py makes the table from tools/apc40mk2_akai_protocol_v1.2.csv (the Akai APC40 Mk2 Communications Protocol v1.2, table "RGB LEDs"). Each entry is the Akai colour as plain RGB565: each channel goes to the nearest 5, 6, 5 bit level. There is no gamma, no floor, no lift and no hand-picked value.
  - Dark Akai colours (for example 0x000059, velocity 46) are dark on the LCD too. Two different Akai colours give the same table value: 0x001900 and 0x001902 (velocities 23 and 27, 0x00C0). These two are idle shades, so the LCD shows 0x0400 and 0x0402 for them. The pads do not use this table.
  - An empty cell (velocity 0) is a small grey dot. A queued cell has a white centre square. A white frame marks the cells on the pads.
  - Exception, idle squares: Loopy sends a clip that does not play in the darkest shade of an Akai group (velocities 7, 11, 15 ... 59). The LCD shows half of the full shade of that group (velocity v − 2). The firmware halves each 8-bit Akai channel (round half up) and then makes RGB565 with the same rule. The table is APC40_LCD_IDLE565 (tools/apc_pad_colors.py --lcd-idle-h). [USER 2026-10-17 01:15] [CODE app/smk_clip_colour.h smk_clip_lcd565] [TEST tests/smk_clip_idle_test.py]
  - Velocities 0-6, the other shades and 60-127 do not change. Velocity 63 is not in a shade group.
  - Loopy sends the same shade for an empty slot (LIM-03), so an empty slot also shows the brighter idle square. SMK_CLIP_IDLE_LIFT 0 turns this exception off. (Changed 2026-10-17: was the plain darkest shade, about 10 % of the full shade.)
  - tests/apc40_tables_test.c makes sure that the table and the tool give the same values. tests/apc_palette_test.py examines the source, the hue order, the hue drift, the clashes and the INLOG replay. (Changed 2026-10-16: the approved table replaces the SMK_CLIP_LCD_FLOOR calculation. Changed 2026-10-16, apc-palette: plain RGB565 from the Akai source replaces the approved lifted table, Orchestrator decision 23:10, 91 entries change.) [APC] [CODE app/apc40_lcd565.h] [CODE app/main_fm1.c smk_clip_lcd_draw] [TEST tests/apc40_tables_test.c] [TEST tests/apc_palette_test.py] [TEST tests/apc40_lcd565_csv_test.py]

### 15.2 Feedback from Loopy

CLP-10. The firmware always stores APC clip colour feedback (notes 0-39) and transport feedback (notes 91 = PLAY, 93 = RECORD). This is also true when clip mode is off. [USER] [APC] [CODE app/main_fm1.c:250-270]
CLP-11. While clip mode is on, the APC40 LED feedback does not go to the sound engine. This is also true after an APC40 host (Loopy Pro) did the identity handshake on the input link that carried the handshake: USB owns USB messages, Bluetooth owns Bluetooth messages. The APC40 LED feedback is note-on and note-off for notes 0-98 on all MIDI channels and the knob ring CCs 16-31 and 48-63. The notes are the clip colours, the blink and pulse channels and the track, scene and transport LEDs. USB handshake ownership ends when X0X loses the USB host; Bluetooth ownership ends when its link or notification subscription ends. [USER] [CODE app/main_fm1.c smk_apc_usb_event] (Changed 2026-10-15, test 35: was notes 0-39, 91, 93 in clip mode only. Changed 2026-10-19: ownership is per input link.)
CLP-12. While clip mode is off and an input link has no APC40 handshake, its notes and CCs go to the sound engine as before, even if the other link has an APC40 host. On one link, an ordinary played note in the APC feedback range is indistinguishable from an LED note after the handshake and remains suppressed by CLP-11. [USER] [CODE app/engine.c] [CODE app/main_fm1.c smk_apc_usb_event]
CLP-13. Clip colours use the 128-colour APC palette. The firmware accepts them on MIDI channels 1-16. The channel does not change the colour. The pads use the owner-calibrated table APC40_PAD_OWNER.
  - The owner made this table on 2026-10-17 with PAD COLOURS (CLP-79) on build 89f680e. The export is tools/padcal/pads-owner-20261017.txt (sha256 54ecc6f8). 89 entries have owner values. The other 39 entries have the start values that the owner saw and kept. tools/apc_pad_colors.py --pad-override writes app/apc40_pad_owner.h from the export. Dev builds and release builds use the same table. [USER 2026-10-17] [CODE app/apc40_pad_owner.h] [TEST tests/apc40_pad_owner_test.py]
  - A lit clip pad sends the table level / 2. These are the bytes that the owner saw for the same entry in PAD COLOURS. The off half of a blink or a pulse sends 0. [CODE app/main_fm1.c smk_pads] [TEST tests/smk_apc40_violet_test.py]
  - Exception, idle pads: Loopy sends a clip that does not play in the darkest shade of an Akai group (velocities 7, 11, 15 ... 59). For these velocities the pad sends the full table level, 2 x the level / 2 of the other velocities. The table does not change. A table value of 0 stays 0. [USER 2026-10-17 01:19] [CODE app/smk_clip_colour.h smk_clip_pad_level] [TEST tests/smk_clip_idle_pad_test.py]
  - Loopy sends the same shade for an empty slot (LIM-03), so an empty slot pad also gets brighter. SMK_CLIP_IDLE_PAD_LIFT 0 turns this exception off. (Changed 2026-10-17: was level / 2.)
  - Changed 2026-10-17. The old table APC40_RGB had the Akai palette with 10 WS2812 corrections: 48-51 (a71) and 69, 80, 81, 93, 94, 116 (a76). Owner test of a74 (14fb352), 2026-10-16 21:18: FAIL. A violet clip from Loopy Pro still showed blue on the pads.
  - tests/apc_pad_source_test.py compares the live table with the Akai source. It is not in tests/run_tests.sh. Since 2026-10-17 it is evidence only, because the owner values are not Akai colours.
  - The channel gives the LED state: 0 solid, 1-5 one-shot, 6-10 pulse, 11-15 blink. One-shot and pulse show a steady colour, and X0X does not use the secondary colour of the protocol. app/apc40_pad_default.h is the start point for the pad calibration (gamma, white gains, trim and floor in tools/apc_colour_model.py). [TEST tests/apc_pad_source_test.py] [CODE app/apc40_pad_default.h]
CLP-14. Velocity 0 or a note-off clears the cell. The last colour from Loopy is the truth. Exception: an off less than 256 ms after an on can be the first flash of a queued clip. The cell keeps its colour.
  - If no on comes in 256 ms, the cell clears. Thus a queued clip does not blink fast before it gets the queued state (CLP-16). [APC 1.4-apc40] [CODE app/smk_apc40.h smk_apc40_cell_event smk_apc40_cells_age] (Changed 2026-10-16, a71.)
CLP-15. Only APC channels 11-15 (zero-based) mark a queued cell (a cell that blinks). Pulse channels (6-10) and one-shot channels (1-5) draw steady. [APC 1.1-apc40] [CODE app/smk_apc40.h:67-70]
CLP-16. Two off-to-on toggles within 256 ms set the queued state. The queued state stops 384 ms after the last flash message. [APC 1.4-apc40] [CODE app/smk_apc40.h:101-102]
CLP-17. A feedback burst is a page snapshot only if it obeys all of these conditions. It touches 8 or more cells and at least half of the lit cells. It has no cue state. It covers 3 or more rows and 3 or more columns. A snapshot clears the cells that it does not touch. [APC 0.9-apc40] [APC 1.0-apc40] [CODE app/smk_apc40.h:224-232]
CLP-18. Each clip cell has an owner link: the link (USB or Bluetooth) that wrote the cell last. The PLAY and REC LED feedback have an owner link too. When a link ends, the firmware clears the cells, the PLAY feedback and the REC feedback of that link only. The cells of the other link stay on the pads. The owner uses Loopy on the Mac (USB) and Loopy on the iPhone (Bluetooth) at the same time (owner, 2026-10-17 01:46).
  - Bluetooth: the link ends, or the central turns notify off (CCCD write 0). 300 ms later the firmware clears the Bluetooth cells again, if Loopy did not come back (link up and notify on).
  - USB: the cable is out, the host sleeps (usb.c usb.suspended), the host sets configuration 0, or a bus reset comes. The firmware clears the USB cells. The APC40 host state and the notes do not change at a cable-out or a sleep. The pads go dark also when the Mac sleeps (owner OK, 2026-10-17 01:46).
  - An APC40 introduction with mode 0x40 (generic mode, F0 47 7F 29 60 00 04 40 .. F7) gives the controller back. The firmware clears the cells of the link that sent it. Feedback after it shows as before.
  - A cleared cell has no colour and no queued state. If no cell stays lit, the latched layout and the pad colour store (lcd.c padled_rgb) clear too, and new feedback latches the layout again. If a cell stays lit, the layout does not change, so the pads of the other link do not move. New feedback from Loopy shows the colours again. Clip mode, the keybed and the sequencer do not change.
  - While notify is off, a Bluetooth colour note-on (velocity 1-127) for a clip cell (notes 0-39) does not change the cell. The inbound recorder keeps it as source 3 (BLE-15). An off (note-off, or note-on with velocity 0, on all channels) always goes to the cell.
  - The firmware does not drop the note itself. In clip mode or after the APC40 handshake, the note stays out of the sound engine. Else it goes to the sound engine, as before. All other Bluetooth notes and control changes (transport LEDs, track and scene LEDs, knob rings, played notes) work as before. X0X drops no note (rule 8 of the old AGENTS.md at integration 4d1608a). [USER] [CODE ble.c ble_service] [CODE app/main_fm1.c smk_apc_ble_event] [CODE app/smk_apc40.h smk_apc40_ble_colour_unheard] [CODE app/main_fm1.c smk_apc_ble_link] [CODE app/smk_apc40.h smk_apc40_lost_step] [TEST tests/smk_clip_ble_notify_test.c] [TEST tests/smk_clip_dark_test.sh]
  - (Added 2026-10-16, a71. Changed 2026-10-16, a76: the clear after 300 ms, the pad colour store and no clip cell writes from Bluetooth while notify is off.)
  - (Changed 2026-10-17, lane/mb-usb-padclear: the owner link of each cell, the USB clear, the clear at generic mode, and offs while notify is off. Then the owner link of PLAY and REC (owner, 02:00).) [TEST tests/host/smk_clip_usb_clear_test.sh] [CODE app/main_fm1.c smk_apc_link_clear] [CODE usb.c usb_poll]
  - Owner test of a74 (14fb352), 2026-10-16 21:18: FAIL. When Loopy Pro disconnected, the pads went dark, but the clip colours came back immediately.

### 15.3 Layout and pads

CLP-20. The firmware latches the layout 250 ms after the first feedback burst. At clip mode entry it latches at once from the stored feedback. [APC] [CODE app/main_fm1.c:209] [CODE app/main_fm1.c:458-464]
CLP-21. A latched layout cannot shrink. It can grow only to complete a 4x4 fold. 8-wide layouts stay as latched. [APC] [APC 1.0-apc40] [PADMAP] [CODE app/smk_apc40.h:427] (Was OQ-09. Resolved 2026-10-15: APC40_IMPLEMENTATION.md now agrees with the code.)
CLP-22. The firmware trims empty outer rows and columns. Empty cells inside the area stay in their positions. [APC]
CLP-23. A footprint of 3 or 4 rows and 4 or fewer columns folds onto one 8x2 view. [APC 1.1-apc40] [CODE app/smk_apc40.h:277-278]
CLP-24. 4x4 fold (1.7 mapping): source rows 0-1 go on pads 5-8 and 13-16. Source rows 2-3 go on pads 1-4 and 9-12. [APC 0.9-apc40] [APC 1.7-apc40]
CLP-25. In clip mode, the physical pad rows are upside down (vertical flip). Columns stay left to right. [APC]
CLP-26. OCT- and OCT+ move the local 8x2 view down and up by 2 rows. The move stays inside the latched footprint. [APC] [PADMAP] [CODE app/main_fm1.c:793-806] (Was part of OQ-02. Resolved 2026-10-15.)
CLP-27. In clip mode, KNOB BANK exits on press. FADER BANK changes the fader bank. "<" and ">" step the selected clip keys field when its page is open (CLP-76). With a menu list open, they step the menu value (GEN-56). Only smk_arrows reads them then, so one press gives one step.
  - With no menu and a SCALE, CHORD or ARP page on the screen, "<" and ">" step the selected field of the page. The hold repeat is the same, 400 ms then 80 ms (CLP-76). SCALE, CHORD, ARP and NOTE REPEAT go to the clip keys only (CLP-61). The firmware sends no APC navigation messages. [APC] [PADMAP] [CODE app/main_fm1.c smk_arrows] [CODE app/main_fm1.c ck_controls]
  - (Was OQ-02. Resolved 2026-10-15: PAD_MAP.md and BUILDING.md now agree with the code. Changed 2026-10-16: FADER BANK and < > work the page. Changed 2026-10-16, a77: a menu list in clip mode, owner approval 21:20.)
CLP-28. The four faders and FADER BANK work in clip mode. Bank A sets the 909, 808, 303 A and 303 B levels. Bank B sets BREAK level, reverb return, delay return and master filter. A fader takes over when it reaches or crosses the saved value. [CODE app/main_fm1.c smk_faders]
CLP-29. In clip mode, the update-mode hold (OCT- and OCT+ for 5 s) does nothing. [CODE app/main_fm1.c:1078-1083]
CLP-30. In clip mode, a pad press on a lit cell sends note-on (velocity 127) for the cell APC note on MIDI channel 1. The release sends note-off. [CODE app/main_fm1.c:823-839] [CODE usb.c:147-153]
CLP-31. A pad press on an unlit cell sends nothing. [APC "Spatial mapping"] [CODE app/main_fm1.c:830]
CLP-32. At clip mode exit, the firmware sends note-off for each pad note that is still on. [CODE app/main_fm1.c:841-852]
CLP-33. Pad presses in clip mode do not start X0X pad functions. In clip mode, the keybed does not control the X0X UI. It sends MIDI notes to Loopy Pro only (CLP-60). At clip mode entry, a 303 key note that still sounds stops. [USER] [CODE app/ui.c key_event] [CODE app/ui.c smk_clip_enter_cleanup] (Changed 2026-10-16: the keybed kept its X0X function before.)
CLP-34. In clip mode, while you hold a pad, X0X blocks the keybed white key that shares the same note bit. This is the intended behavior. Keep it. [USER] [CODE app/ui.c:2068-2071] (Was OQ-10. Resolved 2026-10-15.)

### 15.4 Pad brightness

CLP-40. A lit pad shows the Loopy colour at half level (palette value x 255 / 510). There is no brightness floor on the pads. The owner reports that this pad brightness is good. [USER] [APC] [CODE app/main_fm1.c:572-577]
CLP-41. Idle clip pads (clips that do not play) use the same half level as all lit pads. On 2026-10-15 we added a pad floor (SMK_APC40_PAD_FLOOR) and then removed it. It came from a misunderstanding: the dark problem was on the LCD (see CLP-05). [USER] [CODE app/main_fm1.c:574-577]
CLP-42. The colours of clips that play do not change on the pads or on the LCD. [USER] [CODE app/main_fm1.c:574-577] [CODE app/main_fm1.c:422-427]
CLP-43. The off phase of a queued blink is fully off. [USER] [CODE app/main_fm1.c:572] [CODE app/main_fm1.c:574-577]
CLP-44. A queued cell blinks once per beat (24 MIDI clock ticks), on for the first half. Without clock in the last 500 ms, it blinks 250 ms on and 250 ms off. [APC 1.5-apc40] [CODE app/main_fm1.c:297-310]

### 15.5 Transport and history

CLP-50. In clip mode, SEQ PLAY and SEQ REC send momentary APC PLAY (note 0x5B) and RECORD (note 0x5D). The release sends note-off. [APC] [CODE app/main_fm1.c:663 smk_clip_transport_button] [CODE app/smk_apc40.h] a73: X0X PLAY does not start or stop Loopy Pro (owner test 20:37-20:39 FAIL, ledger c3ee3ef).
CLP-51. In clip mode, the PLAY and REC LEDs follow Loopy feedback. A press sets the LED at once. Later feedback replaces it. [APC 0.7-apc40] [CODE app/ui.c:4024-4031]
CLP-52. In clip mode, PATCH held + SEQ REC sends note 0x70 on channel 16 (Loopy undo). PATCH held + SEQ PLAY sends note 0x71 on channel 16 (Loopy redo). The firmware then sends no APC transport note. [APC 1.8-apc40] [CODE app/smk_apc40.h:404-410]
CLP-53. The release of PATCH after a Loopy undo or redo does not change the X0X view. [APC 1.8-apc40] [CODE app/ui.c:2436]
CLP-54. With clip mode off, PATCH + SEQ REC and PATCH + SEQ PLAY are the X0X undo and redo. SEQ PLAY and SEQ REC are the X0X PLAY and REC. SEQ PLAY also sends the PLAY note (MID-14). [APC] [MANUAL §3]
CLP-55. MIDI clock, Start and Song Position drive the clip-mode blink phase. [APC 1.5-apc40] [CODE app/engine.c:461-473]
CLP-56. In clip mode, MIDI Clock, Start, Stop and Continue from Loopy also go to the X0X sequencer. Thus Loopy can start and stop the X0X sequencer in clip mode. This is the intended behavior, on condition that it does not conflict with external clock sync. Clip mode ignores GLO > MIDI > CLOCK SOURCE and always uses AUTO (MID-13), so this stays the same in all three modes.
  - See LIM-10 and LIM-11 for the known risks. [USER] [CODE app/engine.c:474-478] (Was OQ-05. Resolved 2026-10-15.)

### 15.1 Clip mode keybed (arp, chord and scale for Loopy Pro)

CLP-60. In clip mode, each keybed key sends a MIDI note. Key 0 (the lowest C) is note 48. The note goes through SCALE, then CHORD, then ARP, then out on the CLIP KEYS channel on USB and Bluetooth MIDI. Only the processed notes go out. [USER] [CODE app/main_fm1.c smk_ck_poll] [CODE app/smk_clip_keys.h]
CLP-61. GLO > MIDI > CLIP KEYS CH sets the keybed channel: 2 to 15, default 2. Channels 1 and 16 stay free for the APC40 emulation. X0X saves the value with the settings. An older project loads the default. [USER] [CODE app/ui.c open_global] [CODE app/project.c]
CLP-62. SCALE, CHORD and ARP are off when clip mode starts. One press on a stage button opens its settings page at once. Press the same button again to close it. Press another stage button to show that page. These presses do not change whether a stage is on. [CODE app/smk_clip_keys.h ck_page_press] [TEST tests/smk_clip_keys_test.c test_page]
CLP-63. In clip mode, knob 1 is always MASTER. With a stage page open, knob 2 selects a field and knob 3 edits it. The first field is ON; edit it to turn that stage on or off. Knob 4 directly edits ON without changing the selected field. Knobs 5 and 6 keep useful direct settings when present. The screen shows the selected field or value after a turn. A SCALE or CHORD change while keys sound ends the old notes and plays the new notes. [CODE app/main_fm1.c ck_controls] [TEST tests/host/smk_clip_knob_test.c K9]
CLP-64. SCALE has 24 scales:
  - Major, minor, Dorian, Phrygian, Lydian, Mixolydian, Locrian, harmonic minor.
  - Melodic minor, major pentatonic, minor pentatonic, chromatic, blues, major blues, harmonic major, Phrygian dominant.
  - Lydian dominant, altered, whole tone, diminished half-whole, diminished whole-half, bebop dominant, Hirajoshi, double harmonic.
  - Snap NEAR (default) moves a note to the nearest scale note. A tie goes down.
  - Snap FILTER plays no note for a key that is not in the scale. (Changed 2026-10-16: FILTER is back. Changed 2026-10-16: 13 more scales, after the first 11, so saved numbers keep their scale.) [USER] [CODE app/smk_clip_keys.h]
CLP-65. CHORD has 30 types:
  - MAJ, MIN, DIM, AUG, SUS2, SUS4, MAJ7, MIN7, DOM7, M7B5.
  - DIM7, MAJ9, MIN9, DOM9, SC TRIAD, SC 7TH, SC 9TH, MAJ6, MIN6, MMAJ7.
  - 7SUS4, AUG7, ADD9, MADD9, MAJ11, MIN11, DOM11, MAJ13, MIN13, DOM13.
  - The 11th chords have all 6 tones (1 3 5 7 9 11). The 13th chords have all 7 tones (1 3 5 7 9 11 13). X0X drops no tone.
  - The SC types stack thirds of the SCALE root and scale (also when SCALE is off). Inversion 0 to 3: that many of the lowest chord notes go up one octave (the top note always stays). STRUM (OFF, 10 to 70 ms, ARP off only): the chord notes of a key start one after the other, from low to high. The STRUM value is the time between two notes. A new key first plays all the notes that wait.
  - A release, or a change that changes the notes, stops the strum. (Changed 2026-10-16: SC types and inversion are back. Changed 2026-10-16: 13 more types after the first 17, and STRUM. Changed 2026-10-16: the 11th and 13th chords have all their tones, 7 notes at most.) [USER] [CODE app/smk_clip_keys.h] [CODE app/main_fm1.c ck_strum]
CLP-66. ARP plays the held notes one at a time. Orders: UP, DOWN, UP/DN, DN/UP, PLAYED, RANDOM (random from the time of the step). Rates: 1/2 to 1/32T on a 24 PPQN grid (24 ticks for each quarter note). Octaves: 1 to 4. Gate: 10 % to 100 %.
  - SWING 50 % to 71 % (steps of 3 %): each odd step starts late by rate x 6 % for each step of 3 %. X0X rounds the delay down to whole ticks. RHYTHM: OFF, or a Euclid pattern of hits in steps (2/3, 3/4, 2/5, 3/5, 3/7, 4/7, 5/7, 3/8, 5/8, 7/8, 4/9, 5/9, 5/12, 7/12, 5/16). A rest step plays no note, and the next hit plays the next note of the order. HUMAN 0 (OFF) to 7: each new note (key, chord, strum or arp) gets a random velocity change of up to 4 x HUMAN.
  - The note-on of the new note goes out 0 to 4 x HUMAN ms late at random (28 ms at most). Up to 24 note-ons wait. When 24 wait, a new one goes out immediately. A note-off never goes out before the note-on of its note: a waiting note-on goes out first. All notes off sends the waiting note-ons first.
  - The arp uses the first 8 keys that you press. One arp cycle holds 24 notes at most. (Changed 2026-10-16: DN/UP and true random are back. Changed 2026-10-16: SWING, RHYTHM, HUMAN. Changed 2026-10-16: HUMAN also moves the timing.) [USER] [CODE app/smk_clip_keys.h] [CODE app/main_fm1.c ck_arp]
CLP-67. The arp follows the incoming MIDI clock when a tick came in the last 0.5 s. Else it uses the internal tempo. It also runs when Loopy does not play. MIDI Start resets it to the first step on the next tick. A jump of the incoming clock count (Start, Song Position) gives no burst of steps. The arp sends no clock. [USER] [CODE app/main_fm1.c ck_arp]
CLP-68. NOTE REPEAT + ARP turns LATCH on or off. With LATCH on, the notes stay in the arp after the keys go up. The next key after all keys are up starts a new list. The ARP page also sets LATCH through knob 2 and knob 3. (Changed 2026-10-16: the page.) [USER] [CODE app/main_fm1.c ck_controls]
CLP-69. A key release ends exactly the notes that the key sent. The firmware finds them again from the key and the settings, and a settings change first ends the old notes. A note that two keys hold ends when the last of the two keys goes up. All notes use one channel. A change of CLIP KEYS CH first ends all notes on the old channel. (Changed 2026-10-16: no limit of 16 notes, and shared notes end with the last key.) [USER] [CODE app/main_fm1.c ck_release]
CLP-70. All notes off sends a note-off for each note that sounds. It occurs at clip mode exit, when a mode turns on or off and when LATCH goes off. It also occurs at MIDI Stop, at a CLIP KEYS CH change, at a USB reset and when Bluetooth disconnects. The arp list clears. [USER] [CODE app/main_fm1.c ck_all_off]
CLP-71. NOTE REPEAT alone is the panic button. It sends all notes off and then CC 123 (All Notes Off) on the CLIP KEYS channel. [USER] [CODE app/main_fm1.c ck_controls]
CLP-72. While you hold a pad, the keybed white key that shares its note bit plays no note (CLP-34 stays). [USER] [CODE app/main_fm1.c smk_ck_poll]
CLP-73. In clip mode, when no stage page is open, knobs 2-8 send CC 48-54 on channel 1 for Loopy Pro MIDI-learn. The value is relative: 1-63 is up and 65-127 is down. When a menu is on the screen (for example GLO), the knobs work the menu and send no CC. In clip mode the knobs do not change X0X values.
  - Knob 1 stays MASTER. (Added 2026-10-16. Changed 2026-10-16: also when a mode is on. Changed 2026-10-17: only when no stage has the knobs.) [USER] [CODE app/main_fm1.c ck_controls] [CODE app/plat_fm1.c plat_enc]
CLP-74. In clip mode with no menu, the bottom strip (82 px) is the KEYS panel. It is always on. It shows these items:
  - The SCALE, CHORD, ARP and LATCH chips. A chip shows filled when its function is on. A white bar is under the chip of the open page.
  - One octave with the scale notes lit, the root marked and the notes that sound in amber.
  - The root and scale name.
  - The chord name with the chord root of the first sounding key and the inversion (for example "C MIN7 /1"), or "NO CHORD".
  - The STRUM time.
  - One arp line (order, rate, octaves, gate, swing, human, rhythm).
  - With ARP on, a dot for each rhythm step (8 with RHYTHM OFF). The hits are brighter, and the step that plays now is white.
  - The X0X knob strip does not show in clip mode. (Added 2026-10-16.) [USER] [CODE app/main_fm1.c smk_ck_panel] [CODE app/ui.c draw_knobs]
CLP-75. A press of SCALE, CHORD or ARP opens its settings page over the clip map. The page stays open after release. It lists the stage fields and values, highlights the selected field and shows a keyboard labeled PLAY KEYS. The KEYS strip shows the stage name and ON/OFF. [CODE app/main_fm1.c ck_page_draw]
  - The keybed plays MIDI notes while the page is open. A held pad still blocks its white key. Page changes use knob 2, knob 3 and the arrows. Held keys light on the drawing. Key presses do not edit fields.
CLP-76. With a clip keys page open, knob 2 selects the field. Knob 3 and "<" and ">" step its value down or up without wrap. The ON field enables or disables the stage. FADER BANK keeps its normal fader-bank function. A second press of the open stage button closes the page.
  - Hold "<" or ">" to repeat after 0.4 s, then every 80 ms. Both arrows held give no step. An arrow held before a page opens or a menu closes does nothing until a new press. [CODE app/smk_clip_keys.h ck_rpt] [CODE app/main_fm1.c ck_controls]
CLP-77. The page shows a plain playable keyboard with no setting zones or value keys. The keybed plays while the page is open. [CODE app/main_fm1.c ck_page_draw] [CODE app/main_fm1.c smk_ck_poll]
CLP-78. X0X saves SWING, HUMAN, RHYTHM and STRUM with the settings, in two new bytes at the end of the settings. An older project loads them as 0: OFF (swing 50 %). A value out of range sets all clip keys settings to the defaults (CLP-61). (Added 2026-10-16.) [USER] [CODE app/project.c project_ck_check] [CODE app/x0x.h]
CLP-79. Pad calibration (GLOBAL > SYSTEM > PAD COLOURS, SMK-37, not in clip mode). The 16 pads show one page of 16 palette entries (8 pages: entries 0-127). The pads show the entries in reading order of the pad rows (pad index 0-7, then 8-15), at half level, as a lit clip pad. The LCD shows the Akai colour of each entry (APC40_LCD565) in the same layout. It also shows the entry number of the selected pad and a white frame on that pad.
  - A dot marks each entry with ok. The dot is black on a light square and white on a dark square.
  - The LCD square of the selected entry shows brighter, so that the hue of a dark entry is easier to see. The firmware shifts each RGB565 field left by s (gain 2, 4 or 8). s is the largest value up to 3 that keeps each field in its range. The other squares do not change. [USER 2026-10-17 01:42, 01:43] [CODE app/smk_padcal.h smk_padcal_boost_shift] [TEST tests/padcal_view_test.py]
  - The LCD shows the gain below the entry number ("BOOST X8"). SEQ REC shows the real level of the selected square ("REAL LEVEL"), and SEQ REC again shows the gain. The pads, the stored values, the knob steps and the export do not change.
  - The LCD also shows a name for the selected entry, for example "#47 BLUE DARK". The name is a hue family and a shade from the Akai reference colour. tools/apc_pad_colors.py --names-h makes the names (app/apc40_pad_names.h). These names are not Akai names and not Loopy Pro names. docs/padcal-names.csv has the full list with the owner values (PAD_MAP.md).
  - The mode starts with the orientation page. Only one pad is lit (white). The firmware takes this pad as top-left. The LCD shows "pad 1 top-left?". K2 moves the light to the next or previous pad (with wrap). SEQ PLAY confirms the lit pad and goes to page 1.
  - The correction tl (0-15) is the pad index of the top-left pad. It stays in RAM. Position i is on pad index i XOR tl. The pads and the pad presses use this rule.
  - The rule covers the four possible errors of a 2 x 8 grid. tl 0: as assumed. tl 8: rows swapped. tl 7: rows mirrored. tl 15: both.
  - PAD_MAP.md, section "Physical pad placement", gives the assumed placement and its status. It also gives the steps for a correction other than 0.
  - A pad press selects that pad. K2 / K3 / K4 change R / G / B of the selected entry and mark it ok. The step is 1 level for each detent below 32, and 1 + level / 32 above. The value stops at 0 and 255. SEQ PLAY marks the selected entry ok or not ok. OCTAVE- / OCTAVE+ go to the previous / next page (with wrap).
  - PATCH or GLOBE ends the mode on the release. Meanwhile the UI gets no buttons, keys or knobs. The start values come at run time from APC40_LCD565 with the gamma, gain and floor constants of app/apc40_pad_gamma.h (tools/apc_colour_model.py pad_default). The edits stay in RAM until power off: no settings byte, no flash save. The edits, the ok marks and tl stay when you open PAD COLOURS again. The orientation page then lights the pad of the saved tl.
  - At power off, X0X loses the calibration. In 89f680e, X0X also lost it when PAD COLOURS opened again. Since 1a07781, the edits stay when PAD COLOURS opens again, until power off.
  - The mode is in dev builds only (X0X_DEV_PADCAL=1, the default). The release build has X0X_DEV_PADCAL=0. It has no PAD COLOURS row, no pad calibration code and no INLOG args 2-5 export. It uses the owner-calibrated pad table of CLP-13 (export of 2026-10-17, sha256 54ecc6f8), as the dev build does. [USER 2026-10-17 00:01]
  - Editor command 38 INLOG, args 2-5, exports the 128 entries (R, G, B, ok) and tl. The export also works after the mode ends. tools/ble_autotest.py --padcal-export FILE writes them. tools/apc_pad_colors.py --pad-h OUT --pad-override FILE puts the entries marked ok over the start values.
  - The export file has the line "padcal_orient TL". tools/apc_pad_colors.py --pad-override warns if TL is not 0. [USER] [CODE app/smk_padcal.h] [CODE app/main_fm1.c smk_padcal_frame, smk_padcal_draw] [CODE app/ui.c open_global, run_action] [TEST tests/smk_padcal_test.c] [TEST tests/smk_padcal_wiring_test.py] [TEST tests/smk_padcal_export_test.py] (New 2026-10-16, apc-palette.)
CLP-80. In clip mode, the LCD main area shows "WAITING FOR LOOPY PRO" until a link has a Loopy host. The clip map then replaces this screen. [USER 2026-10-17 02:50] [CODE app/main_fm1.c smk_clip_wait, apc_own, smk_apc_link_clear] [CODE app/ui.c draw_main] [TEST tests/host/smk_clip_wait_test.sh]
CLP-82. In clip mode, when the open menu list changes (another view or page, for example a page list to GLO) while "<" or ">" is held, the hold repeat stops. An arrow that was already held in the old list does not edit the new list. Release and press it again to step a value (GEN-56). A new press that comes in the same frame as the change steps the new list at once (it is not lost). This is also true when a menu opens. The arrows never run an action row. Clip mode off is unchanged (MID-07). (New 2026-10-30, owner list NS008 and NS010.) [CODE app/smk_arrows.h arw_rearm arw_menu_id] [CODE app/main_fm1.c smk_arrows] [TEST tests/scenarios/clip-arrow-menu.x0x] [TEST tests/smk_arrows_test.c test_rearm]
  - A link (USB or Bluetooth) has a Loopy host after the first clip or PLAY / REC LED message on that link. The handshake alone does not set it, because the firmware does not get the link of the handshake.
  - The link has no host after its end, after the APC40 generic mode (mode 0x40) on that link, and after a USB host loss. These are the events that clear the clip cells of that link (CLP-18).
  - When no link has a host, the waiting screen comes back. A clip keys page (CLP-75) shows over the waiting screen. A menu shows over it too.
  - A Bluetooth clip message while notify is off does not set the host (CLP-18).

## 16. Flash and sample storage

FLS-01. SMK-37 flash map (layout 2):
  - App code 0x04120-0x8AFFF.
  - Bluetooth controller block 0x8B000-0x9AFFF (FLS-13).
  - X0X store 0x9D000-0xAFFFF (objects through 0xAAFFF, boot screen 0xAB000-0xACFFF, spare 0xAD000-0xAEFFF, owner tag 0xAF000).
  - VM spare 0xB0000-0xBAFFF (not a sample slot).
  - Update staging 0xBB000-0xBFFFF.
  - User sample slots 1 and 2 0xC2000-0xF3FFF.
  - 0xAB000 and 0xAC000 hold the boot screen (two copies). 0xAD000-0xAEFFF stays spare. 0xFE000-0xFEFFF stays unused on the SMK-37 (4 KiB is too small for a slot). [CODE ../hal/fm1_flash_map.h:25-68] [CODE app/boot_screen.h]
FLS-02. The SMK-37 has 2 user sample slots: slot 1 80 KiB at 0xC2000, slot 2 120 KiB at 0xD6000. They keep the flash offsets of the earlier layouts. The 44 KiB window at 0xB0000 is not a sample slot. The FM-1 has 2 slots of 80 KiB at 0xA0000 and 0xB4000. [CODE app/slots.c:15-39] (Changed 2026-10-26: the third slot was removed. 2026-10-15 had added it.)
FLS-03. Keep the imported-sample flash of the two slots. A space saving must not shrink slot 1 or slot 2. [USER]
FLS-04. The store-owner check at first start clears only 0x9D000-0xAEFFF. It does not clear the sample slots or the 44 KiB window at 0xB0000. [CODE storage.c:50-105]
FLS-05. A slot holds a header, up to 16 zones and IMA ADPCM data at offset 512. X0X plays zone 0 of each loop as a loop. [CODE app/slots.c:1-8]
FLS-06. The UI has PLAT_NSLOTS = 2 on the SMK-37 and on the FM-1. Each slot has its own size. The editor checks each write and each header against the size of its slot. [CODE app/slots.c:15-39] [CODE app/plat.h] (Changed 2026-10-26: was 3.)
FLS-07. IMA ADPCM at 44.1 kHz is the sample storage format: the 909 cymbals (DRM-11), the built-in loops (BRK-22) and the uploaded loops (BRK-26). 22.05 kHz is an opt-in for uploads only. fm1_sample_upload.py one-shots stay at 22.05 kHz (FM-1 tool, not maintained). [USER]
FLS-08. The SMK-37 store has 7 objects with A and B copies: settings, sound, two pattern objects, panel, song and motion. [CODE storage.c:50-60]
FLS-09. The protected ranges are head, BTIF, USRTRIM, stock USR resources and key_mac. The flash map refuses them at compile time. The flash driver refuses them again. [SMK37] [CODE ../hal/fm1_flash_map.h:56-83]
FLS-10. Slot backup: tools/x0x_slots.py backup and the break page's "Back up all slots" read every slot with SMP_INFO (15) and SMP_READ (16) only. They do not write to the device. The archive (.x0xslots) holds the slot count and size. For each slot, it holds the name, zone count, sample counts, rates, data length, header CRC and an image CRC.
  - A CRC-32 covers the whole file. Thus the CRC-32 of a full .x0xslots file is always 2144df1c (the CRC-32 residue). To compare two backups, compare the per-slot CRCs. The tool and the page write the same bytes. [USER] [CODE ../../tools/x0x_slots.py] [CODE ../../web/x0x_breaks.js]
FLS-11. Slot restore writes an archive back with SMP_BEGIN, SMP_WRITE and SMP_END, or SMP_ERASE for an empty slot, after the end user confirms. Then it reads the slots back and compares them. It takes the slot count and size from SMP_INFO. It skips an archive slot that the device does not have or that does not fit. [USER] [CODE ../../tools/x0x_slots.py] [CODE ../../web/x0x_breaks.html]
FLS-12. SMP_INFO reports the slot count and the smallest slot size in KiB (7 bits, maximum 127). For each slot, it reports the zone count, the name and the used KiB (maximum 127). Then the X0X extension follows: 'X' (0x58), 1, and data for each slot. The data is the flash offset, the size in 4 KiB sectors and the used KiB, each as two 7-bit bytes, low first.
  - An older tool ignores the extension and sees slots of the smallest size, so it never overfills a slot. x0x_slots.py, upload_breaks.py and the break loops page use the per-slot sizes. The archive stores the size and the flash offset of each slot. [CODE app/editor.c:252-288] [CODE ../../tools/x0x_slots.py] [CODE ../../web/x0x_breaks.js] (Changed 2026-10-15: the extension is new.)
FLS-13. The SMK-37 Bluetooth controller image (roadmap step 5) has one contiguous 64 KiB block at flash 0x8B000-0x9AFFF (XIP 0x02087000). The block is in the encrypted app slot (APP_SLOT 0x97400 does not change), above the app code. The app code can use up to 0x86EE0 B (552,672 B). build.py stops the build when the image is larger. A package from build.py has an empty block (0xFF).
  - tools/add_ctl.py and the installers put an image that the end user built there and write the app.bin, app area and package CRCs again (BLE-10). The image owns RAM 0x01C01800-0x01C07EFF. The app's RAM code must end below 0x01C01800 (app.ld checks it). The mailbox and the app's Bluetooth state are in BLERAM 0x01C7C400-0x01C7D3FF (not cleared at boot). [CODE ../hal/fm1_flash_map.h:63-81] [CODE ../../tools/fm1pkg_make.py] [CODE ../../tools/build.py] [CODE ../app.ld] (Changed 2026-10-15: the image and its RAM.)
FLS-14. An SMK-37 package carries its slot map (magic "X0X-SLOTMAP-1", per slot flash offset and size, CRC-32) at flash.bin 0x9CF00. A package without the map but with the Felucca loader is an older X0X with 2 slots of 80 KiB at 0xC2000. A package with neither is stock firmware, which has no X0X slots. [CODE ../../tools/fm1pkg_make.py] [CODE ../../web/x0x_breaks.js]
FLS-15. An install never writes the sample slots. A slot keeps its samples when the new firmware has a slot at the same flash offset. That slot must hold the header area and the used data. An upgrade from the 2 x 80 KiB layout keeps every slot. [USER] [CODE ../../tools/x0x_slots.py]
FLS-16. An install can lose samples when a slot moves, becomes smaller or disappears. Then the web installer and tools/fm1_install.py do these steps by themselves:
  - (1) They tell the end user which slots are at risk.
  - (2) After the end user confirms, they read every slot with SMP_INFO and SMP_READ into an .x0xslots archive and save it. The page downloads it. fm1_install.py writes --backup FILE or x0x-slots-before-install-DATE.x0xslots.
  - (3) They install.
  - (4) They connect to the new firmware again and write each slot at risk back. Old slot N goes to new slot N if it fits. Else it goes to the first other slot that is empty, not kept and large enough. Then they read the slots back to compare.
  - They report a slot that has no place. If the backup fails, they install nothing. If the write back fails, the archive stays (fm1_install.py exit 7). The message tells the end user to restore it with Restore on the break loops page or with tools/x0x_slots.py restore FILE. fm1_install.py --no-slots skips this. [USER] [CODE ../../tools/fm1_install.py] [CODE ../../web/x0x_breaks.js] [CODE ../../web/x0x_installer.html]
FLS-17. Installing this 2-slot firmware over a 3-slot unit keeps slots 1 and 2 in place (same flash offsets) and treats slot 3 as at risk. The install flow (FLS-16) saves it and writes it into a free remaining slot when one is large enough. A downgrade to an older X0X (2 x 80 KiB) cannot keep a slot 2 image larger than 80 KiB in place. A stock firmware install can erase the VM (the store and the 44 KiB window). Slots 1 and 2 are in stock USRFLASH, which the stock firmware can also use. The install flow saves every slot before a stock install. It tells the end user to restore the archive after the next X0X install. [USER] [CODE ../../tools/fm1_install.py]
FLS-18. The update loader drops a stale update record only where one matches (bytes +6 = 0x5441 and a nonzero CRC16). It looks only at a 4 KiB boundary − 256 in 0x9D000-0xBFFFF. Old bytes in the 44 KiB window at 0xB0000 can match by chance (about 2^-32 for each sector). The loader stays unchanged, because it must still drop the stock records. [CODE ../loader/ldr_core.c]
FLS-19. The boot screen can be changed from the installer page while X0X is running.
  - With nothing stored, the screen is the original words "X0X" and "909 808 303 303 BREAK" and no picture. The sound does not change.
  - A custom record replaces those two lines and may add a black-and-amber picture, 240 by 96 pixels, in the rows above the words. A color photo does not fit: 240 by 240 in full color is 115200 bytes, and one copy has room for 2924.
  - The top line is at most 12 characters. The second line is at most 21. Empty lines are refused. The previous record stays.
  - The new copy is written beside the old one. The header is last. A picture that is too big, the wrong size, cut off, or failing its check is not shown. Pulling the cable before the keyboard confirms leaves the previous screen.
  - If the keyboard cannot store it, the installer says so and does not claim the save. The previous screen stays.
  - On the SMK-37 the copies are 0xAB000 and 0xAC000. Clearing a foreign firmware's store also clears them. The original words come back. On the FM-1 the copies are 0x9F000 and 0xFE000. Sample slots are not used. [CODE app/boot_screen.h] [CODE app/editor.c] [CODE app/main_fm1.c] [CODE ../../web/boot_screen.js]

## 17. Known limitations

LIM-01. Fixed 2026-10-15. web/x0x_breaks.html had 3 slots hard-coded. It now reads the slot count and size from SMP_INFO. [CODE ../../web/x0x_breaks.html]
LIM-02. The APC protocol has no project-size data. The firmware infers the footprint from lit cells. A sparse 8-wide project with clips in 4 or fewer columns and 3-4 rows folds as a 4x4 grid. [APC 0.7-apc40] [APC "Spatial mapping"]
LIM-03. Loopy sends the same dim shade for empty slots and for filled clips that do not play. The firmware cannot tell them apart. Thus the brighter idle squares (CLP-05) and idle pads (CLP-13) also show for empty slots. [APC 1.4-apc40] [APC 1.5-apc40]
LIM-04. Clip mode cannot show clips outside the Loopy APC 8x5 box. Change the Loopy page to see them. [APC]
LIM-05. Old-page cells stay lit after a change to a page with fewer than 3 rows or 3 columns of clips. [APC 1.0-apc40]
LIM-06. Loopy sends its MIDI clock to the port "X0X FM-1". The APC40 mkII port gets no clock. [APC 1.5-apc40]
LIM-07. Nobody has verified the hardware LED colours and live clip launches end-to-end on the device. [APC "Verification performed"]
LIM-08. User sample room at 44.1 kHz is (slot size − 512) × 2 / 44100 s. On the SMK-37, slot 1 holds 3.69 s (81,408 B of data) and slot 2 holds 5.55 s (122,368 B). The total is 9.24 s (407,552 samples). The FM-1 has 7.38 s (2 x 80 KiB).
  - With the 22.05 kHz opt-in the room is double (about 18.5 s on the SMK-37). A loop is one piece: it must fit in one slot. [MANUAL §8] [CODE app/slots.c:15-39] (Changed 2026-10-26: the third slot is gone. Was about 11.26 s on the SMK-37.)
LIM-09. tools/build_smk_release.sh builds Pro and Elite packages only. It does not build MKE-P37. [CODE ../../tools/build_smk_release.sh:17-18]
LIM-10. X0X tells the inputs USB, Bluetooth and TRS apart and follows the clock of one input (MID-15). On the SMK-37 the USB port has one cable. Thus the firmware cannot separate two sources on USB (for example Loopy and a DAW on the port "X0X FM-1"). Each tick of the two counts, the sequencer then runs at about double tempo, and a Start or Stop from either source acts.
  - Send clock to the port "X0X FM-1" from one source only. CLOCK SOURCE INTERNAL ignores all clock sources (MID-13). (Changed 2026-10-16: before, USB and Bluetooth together also counted twice.) [CODE app/engine.c midi_in] [CODE seq/sequencer.c seq_clk_take] [CODE usb.c ep1_rx]
LIM-11. In AUTO (the default, MID-13), while X0X follows an external clock, it sends no Clock, Start or Stop. X0X mutes CLOCK OUT, so no clock loop occurs. A MIDI Start that arrives when no clock came in the last 0.5 s starts X0X on its internal clock. CLOCK OUT is on by default (SET-08).
  - In clip mode, X0X does not echo a MIDI Start, Continue or Stop (seq midi_req). Until the first incoming tick it can still send internal clock ticks (0xF8) to the USB host. With clip mode off, X0X sends Start (0xFA) and clock ticks back to the USB host until the first incoming tick, as before (agent-rules 14). [CODE app/engine.c:472-475 midi_in] [CODE seq/sequencer.c:378-379] [CODE seq/sequencer.c:392-393] [CODE seq/sequencer.c:470] [CODE seq/sequencer.c:446-455] [CODE seq/sequencer.c:511-512] If the clock then starts while X0X plays, the first tick is tick 0 of the step that plays.
  - The next step comes 6 ticks later (16ths). (Changed 2026-10-16, a72: before, the first tick played the next step at once. Thus X0X played one step ahead of the clock.) [CODE seq/sequencer.c seq_advance] INTERNAL always sends CLOCK OUT when CLOCK OUT is ON. EXTERNAL never sends clock, Start or Stop (MID-13).
LIM-13. Bluetooth MIDI (BLE-01 to BLE-16) has only part of its hardware tests (Stage 3: docs/plans/2026-10-15-ble-stage3-test-plan.md). The Mac link passed 3 rounds of 600 s with no disconnect (a59). The return path to the Mac works (a60). The iPhone link works on a67 (94285b5, owner test 2026-10-16 19:18).
  - BLE-16 is in code (2026-10-16), with a host test only. The RF calibration values are the AC79 SDK demo board values, not the unit's own. X0X gives clocks to the image (system 360 MHz, HSB 180 MHz, LSB 60 MHz). We measured them on an FM-1, not on an SMK-37. [CODE ../hal/fm1_ble.h]
LIM-14. The controller's poll and its interrupts run on core 1 between and inside audio jobs. A long poll can delay an audio job past the 8 ms limit, and then X0X holds core 1 (one core, no Bluetooth). Long flash operations park core 1, and a long park can drop the Bluetooth connection. Units that cannot run core 1 (fm1-x0x#10) have no Bluetooth. [CODE ../hal/fm1_cpu1.h]
LIM-12. We maintain only the SMK-37 build. We do not maintain the FM-1 build. Its link step fails because the XIP region overflows by about 648 B. The owner accepts this. It is not a defect. [USER]

### SMC-Mixer navigation and pickup

SMC-01. The SMC uses the main mixer faders, including the master filter on fader 8. Drum MIX and drum SEQ pages instead use two banks for all eleven drum tracks. SELECT chooses the kit and track within the bank. In HOME and main MIX PARTS, knobs 1–5 edit part pan, knob 6 reverb decay, knob 7 delay feedback and knob 8 master filter resonance. In 909 MIX, 808 MIX, 909 SEQ and 808 SEQ, knobs edit corresponding voice Pan in the active bank. Fader destinations remain unchanged. Ordinary knob rows already on any active fader are inactive; the overview explains ON FADER n. GLOBAL/PERFORM K3 and MIX/MASTER K4 edit filter resonance. Each control type has unique destinations. Unused positions are inactive. [CODE firmware/src/app/smc_mixer.c]

SMC-02. SMC navigation reaches part sound pages, sequence, generator, FX, MIX, song and deep menus. Outside drum MIX and drum SEQ, R6 opens or activates a menu; R7 backs out or cancels; S7 opens Global. On drum MIX and drum SEQ, their strips follow SMC-05. Open menus keep their own priority. Only explicit R6 confirms a question. Help is only a card: SMC controls keep working under it, including fader pickup, and only R7 closes it. Confirmation alone parks faders and encoders so only confirm/cancel apply. [Mapping](docs/SMC_MIXER.md) defines the complete button map. [CODE firmware/src/app/smc_mixer.c]

SMC-03. Outside the three mixer modes in SMC-01, encoders edit visible fields. Open menus take precedence over the mixer modes. Clip-grid encoders cannot edit the hidden X0X page. Clip navigation reaches SCALE, CHORD and all nine ARP fields. [CODE firmware/src/app/main_fm1.c smk_ck_smc_nav]

SMC-04. The temporary SMC screen shows eight strips in physical left-to-right order, with each encoder above its fader and buttons beside the fader at the physical M/S/R positions. Captions show their dispatched actions: filter-strip PART/HOME, context-dependent LIST/OK and SONG/HOME/BACK, and drum M/S/R. X means blocked by target lock; -- means an unused drum position. Knob arcs follow the mapped parameter value, with a center reference for bipolar parameters. The lower readout retains the exact parameter/value/unit and fader pickup direction; the static transport/navigation glyph footer is omitted. Drum SEQ strip titles use the authoritative displayed kit voice short names in both banks (909 bank 2: OH/CR/RD; 808 bank 2: CY/OH/CH), including captured targets while locked; drum MIX titles and other modes retain their existing labels. The selected lane has a solid title highlight; touched knob/fader focus retains its separate control highlight and value readout. There is no on-screen Select button. Selection follows the current part or the main MIX channel editor selection, or the displayed kit’s selected drum within the bank, including while targets are locked; a selection outside a locked bank has no highlighted lane. LOCK in the header identifies captured targets. Physical Select and its repeated-press mixer toggle remain unchanged. All eight positions remain visible, including inactive positions. A compact lower readout names the touched destination and formatted value with page/kit/bank context. PICKUP feedback gives the direction to the displayed target; ACTIVE means pickup is acquired. It expires after 1.2 seconds of inactivity. Navigation cancels it. Target refresh, repeated absolute samples and zero encoder deltas do not open or extend it. A changed target or external value resets pickup and crossing history. The fader's own write survives the following feedback frame. Native framebuffer renders and host tests do not prove physical LCD behavior. [CODE firmware/src/app/smc_mixer.c smc_ui_draw]

SMC-05. On 909 MIX, 808 MIX, 909 SEQ and 808 SEQ pages, Record, Solo, Mute and SELECT follow the drum bank on every strip. MIX uses its visible kit; SEQ uses the selected drum part. Record selects that kit and drum and toggles the existing recording mode. SELECT selects the same target without toggling recording. Solo isolates the drum while held; multiple Solo targets combine. A release clears its original target after a page, kit or bank change. Solo does not change saved mute state and clears on disconnect, blocked input and project reset. Stock User MMC retains its latch fallback because that preset has no release message. The five unused strips in bank 2 are inactive and unlit. Other pages retain the existing whole-part Record/Solo behavior. [USER 2026-10-21 individual-drum choice and SEQ follow-up] [CODE firmware/src/app/smc_mixer.c firmware/src/app/engine.c]

SMC-06. On drum MIX and drum SEQ, the SMK pads edit the steps of the selected kit and drum while the current view remains open. Step lights follow that drum. SMC Record and SELECT focus both the visible kit and its selected drum so pad edits reach the requested sound. Main MIX and FX retain their existing pad behavior. [USER 2026-10-21: "It can select a drum to seq on smk pads" and SEQ follow-up] [CODE firmware/src/app/ui.c]

SMC-07. Accepted SMC fader, encoder and action feedback uses a temporary full control screen without a yellow SMC notice. After 1200 ms without activity, the underlying screen is redrawn. Presentation does not change its view, page, selection, menu or clip state. Repeated activity extends the interval. Panel input and intentional SMC navigation dismiss the screen immediately; no later restoration can overwrite that navigation. Help retains its card and confirmation retains its input lock. Clip-editor encoders show their actual field; clip-grid encoders remain inactive. [USER full SMC UI and return request, 2026-10-25] [CODE firmware/src/app/smc_mixer.c smc_ui_active smc_ui_draw]

SMC-08. PLAY toggles sequencer playback, including a start queued before the audio loop runs. Holding a DAW Note button does not toggle repeatedly; release permits the next press. Outside drum MIX/SEQ, selecting another part retains ordinary sound-page selection. SELECT on the already-selected 909/808 opens its individual-sound mixer. In drum MIX, SELECT chooses another sound normally; pressing SELECT for the already-selected sound returns to main MIX. SELECT navigation does not open the temporary control screen or change transport. [CODE firmware/src/app/smc_mixer.c smc_button_apply]

SMC-09. Physical Pause on the owner's captured NoteOn preset (90 5D 7F press, 90 5D 00 release) toggles captured SMC target-page lock once per press; release and held repeats do not retoggle. It captures full control targets, including drum bank and pattern/song identity, without opening a page or retaining an overlay. Locked SELECT and other target-changing SMC navigation are parked; keyboard navigation cannot redirect the captured controls. While unlocked, the mixer follows the keyboard's current page; while locked, it keeps its own page and targets (SMC-11). Unlock returns the mixer to the keyboard's current page and resets pickup. [USER 2026-10-30] Link loss/reconnect retains captured targets and rearms press edges; project reset clears the lock. Play remains play/pause; User MMC34 retains transport STOP because the capture established NoteOn input only. The existing Mackie NoteOn feedback route sends 90 5D 7F/00 at 512 ms phases while following and steady 7F while locked. Its physical Pause LED address remains unproven and requires native lamp observation on the exact candidate/preset before a hardware-pass claim. [USER retained Pause capture 2026-10-25] [CODE firmware/src/app/smc_mixer.c smc_lock_button smc_frame]

SMC-10. An SMC knob's quick spin travels less far than the panel knob curve. A slow detent, and the first three detents of a spin, still move one value each. After that, detents closer than 45 ms move 2 values. Detents closer than 12 ms move 3 on a pot whose range is 100 or more, and 2 on a shorter pot. A wide pot is not doubled, so tempo does not jump by 16. A message that already bundles many detents multiplies only its first three; the rest count one each. A very fast 63-detent flick therefore moves 69 on a 0-127 pot, not 504. A turn that is not quick stays one value per detent. Named switches and short ranges (max under 24) stay one step. Values still stop at the ends; they do not wrap. Both directions use the same distance. The knob work stays in the main loop, so a spin while the sequencer plays does not take the audio render's silence path. Which parameter sits on which knob is unchanged. The device's own panel knobs keep the older curve. [USER 2026-10-26: quick SMC spins are too quick] [CODE firmware/src/app/ui.c accel firmware/src/app/smc_mixer.c smc_knob]

SMC-11. SMC mixer page lock. While unlocked, the mixer follows the keyboard's current page; mixer-side PAGE navigation changes that shared page. While locked, each SMC peer retains its captured mixer page when the keyboard page changes. Mixer-side navigation and its own UI popup remain available during lock and move lock_page without moving the saved fader/knob targets. Fader-button SELECT LEDs follow the mixer page, including locked page changes. Lock stores lock_part, lock_view, lock_page, lock_bank and lock_kit independently for each peer; a saved part's controls remain bound until unlock, when the mixer again follows the keyboard page. [USER 2026-10-29] [USER 2026-10-30]

## 18. Open questions

These are disagreements between the code and a source document, or behavior that needs an owner decision. Do not report them as defects. Do not fix them without an owner decision.

OQ-11. The GLOBAL menu has no "forget pairing" row. X0X never pairs (and keeps no bonds) (BLE-07), so there is nothing to forget. Needs an owner decision: keep it so, or add pairing (then BTIF writes and a forget row are needed). [USER task 2026-10-15]
OQ-12. tools/fm1_install.py adds ~/.config/x0x/ctl.img to an SMK-37 install by itself when the file exists and the package has a free Bluetooth block (BLE-11). It says so. Needs an owner decision: keep this default, or add the image only with --ctl. [CODE ../../tools/fm1_install.py with_ctl]

Resolved open questions (do not use these IDs again):

- OQ-01: see GEN-40.
- OQ-02: see CLP-26 and CLP-27.
- OQ-03: see CLP-28.
- OQ-04: see LIM-08.
- OQ-05: see CLP-56, LIM-10 and LIM-11.
- OQ-06: see GEN-44.
- OQ-07: see BRK-22, BRK-26 and FLS-07. The 22.05 kHz in DESIGN 2026-10-03-x0x-design.md is the old plan.
- OQ-08: see MID-10.
- OQ-09: see CLP-21.
- OQ-10: see CLP-34.
