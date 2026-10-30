# X0X37 clip mode: arp, chord, scale and keybed MIDI out – analysis and design
Date: 2026-10-16. Analysis only. No code changes were made. Nothing was built, installed or committed.

## 1. What the attached firmware is
- File: attachments/681471b5...bin, 433 KB. Raw plaintext code image. No container header (byte 0 is code). Strings are readable, so the image is not encrypted.
- It is NOT the M-Vave stock firmware. It is a third-party custom firmware named "Oscuro" for the M-Vave SMK-37 Elite.
- Evidence: "SMK-37 Elite" (0x69c98), "SMK-37 Elite_900" (0x69c73), "FSMK-37 Oscuro USB" (0x69c0f, USB name), "OSCURO" (0x8bec, 0xda7c), version "0.68.0" (0x69fd8), "M-Vave updater" (0x69da7), "app.bin", "LOADER.BIN", "KSAPP1", "ota-".
- It has its own synth (patches, DX7, "Synth off"), a sequencer (Euclid, Probability, Ratchet), split zones and a mixer. Arp, chord and scale are only a part of it.
- Chip family: JieLi (AC79-class, same as SMK-37), from the loader names and the 0x01C3xxxx pointers. Confidence: high for "Oscuro 0.68.0 for SMK-37 Elite". Medium for the exact load address.
- No disassembly was done. There is no source. I did not trace the code paths; section 2 is from strings and data tables. Section 2 marks guesses.
- Licence: unknown. Do not copy code or binary blobs. Re-implement from the feature list. The tables below are plain music data (scales, chords).

## 2. Inventory of arp, chord and scale features (evidence)
### 2.1 Arp
- Engines: "ARP RHYTHM", "ARP EVOLVE", "ARP STRUM", "ARP HUMAN" (0x1ce4-0x1d17). Page list "Arp Rhythm Strum Human Evolve Freeze Reseed Revert" (0xdc6f).
- Main parameters (0x1d1e-0x1d46): Mode, Octave, Gate, Swing, Tempo, Rate, Latch, Steps.
- Rates (0x4f1c-0x4f57): 1/2, 1/4d, 1/2T, 1/4, 1/8d, 1/4T, 1/8, 1/16d, 1/8T, 1/16, 1/16T, 1/32, 1/32T (13 rates).
- Rate table at 0x6c341: 48,36,32,24,18,16,12,9,8,6,4,3,2. These are ticks at 24 PPQN (1/4 = 24). So the arp runs on a 24 PPQN clock, the same grid as MIDI clock.
- Directions/orders (0x4f5d-0x4fe2): Fwd, Rev, Pingpong, Walk, Dn Incl, Dn Excl, Thumb Up, Thumb UD, Pinky Up, Pinky UD, Converge, Diverge, Con+Div, Order (as played), User, Rand No Dup, Rand Drift, Rand Once. Plus "Random".
- Octave range: "+1 oct", "+2 oct", "+3 oct" (0x1e88). Guess: 1 to 4 octaves total.
- Strum: Strum steps, Spread (ms), Direction, Vel slope, Live strum, Window (ms), Repeat, Accents, Every; directions Down, Alternate, Random; curves Linear, Slowing, Speeding.
- Human: Amount, Late (ms), Velocity, Gate (%).
- Rhythm: Euclid, Hits, Length, Rotate, Chance (%), Accent.
- Evolve: Mutate (%), Bars, Gravity, Reach, Phrase, Seed; 1/2/4/8 bars; Freeze, Reseed, Revert; Live/Frozen.
- Clock: "Tempo owned by external clock", "Clock", "Clock Out", "Sync". So the arp follows external MIDI clock when present (guess from strings).
- UI hint: "holding Oct -", "tap a pad", "release to play" (latch/record hint, guess).

### 2.2 Scale
- Menu "SCALE"/"SCL" with Base (root), Scale, Fix, Shift (0x4c4f-0x4c68). Key snap options near 0x4ffd: "Near" and "Filter" (guess: Near = snap to nearest scale note, Filter = drop out-of-scale notes). "Follow scale" (0x68e23) in the sequencer.
- Scale table at 0x6c368: records of 8 bytes {u32 name pointer, u32 12-bit mask, bit 0 = root}. Examples: 0x0FFF Chromatic, 0x0AB5 Major {0,2,4,5,7,9,11}, 0x05AD Minor {0,2,3,5,7,8,10}, 0x06AD Dorian, 0x0AD5 Lydian, 0x0295 Maj Penta, 0x04A9 Min Penta, ...
- Scale names (0x69a3d-0x69b90): Chromatic, (Major, Minor, Phrygian – short names, inferred from masks), Dorian, Lydian, Maj Penta, Min Penta, World I-IV, Mixolydian, Locrian, Harm Minor, Mel Minor, Harm Major, Lydian Dom, Altered, Major Blues, Bebop Dom, Whole Tone, Dim H-W, Dim W-H, Augmented, Neap Minor, Phrygian Dom, Hung Minor, Double Harm, Enigmatic, Spanish 8, Prometheus, Hirajoshi, Iwato, Pelog, Egyptian, User 1-4, Host. About 40 entries. "Host" is a guess: scale sent by the host.
- Shift -7..+7 (0x5009-0x502d). Guess: transpose in scale degrees.

### 2.3 Chord
- Menu "CHORD"/"CHD": Chord, Type, Voicing (0x4c6e-0x4c83). Modes guess: Fixed, Triad (scale-aware), Chan, Poly (0x5040-0x5063).
- Qualities: Maj, Min, Dim, Aug, 7th, 9th, 11th, 13th (0x5030-0x5053). Names: maj7 m7b5 dim7 aug7 sus2 sus4 7sus4 mMaj7 madd9 add9 maj9 m9 maj11 m11 maj13 m13 m6 (0xdb85, 0xdbba).
- Interval table at 0x6c353: triads {0,4,7} {0,3,7} {0,3,6} {0,4,8}, then extension intervals 11,10,9,10,14,17,21 (maj7, b7, 6, 7, 9, 11, 13).
- Sequencer step type "Note / Chord" (0x6a347).
- Note path hook: not traced. Guess: keybed -> split/zone -> scale -> chord -> arp -> synth + USB/BLE out, with separate "Key Channel" and "Pad Channel" settings (0x4c9d, 0x4ca9).

## 3. What X0X already has
- No arp, chord or keybed scale snap. The ARP button (B_ARP) opens TB-3PO (ui.c:1166, 2355). TB-3PO has 6 scales (MIN, PHRY, HARM, MPEN, DOR, MAJ; seq/tb3po.c:7-15). It writes 303 lines only.
- IMPORTANT: X0X has no keybed-to-MIDI-out path today. key_event (ui.c:2094) feeds the X0X UI (patterns, tracks, 303 notes). The only MIDI note outputs are: the sequencer echo (seq.send_notes, NOTES OUT, default off), clip pads (main_fm1.c:823-852, ch 1), APC PLAY/REC (ch 1) and undo/redo (ch 16). midi_send_note (usb.c:147) is hard-wired to channel 1 and USB only.
- So "keybed passes through to Loopy" must be built new. In clip mode, the keybed currently still drives X0X (CLP-33). Section 4 changes this in clip mode only.
- Channel settings today: settings_t.midi_ch[5] (x0x.h:36) per part, in and out, 0 = default, 1-16, 17 = OFF. Defaults 909=10, 808=11, 303A=2, 303B=3, BREAK=4 (MID-02). No keybed channel.
- Clock: engine.c:461-478 already gets MIDI Clock/Start/SPP (24 PPQN) for the clip blink phase and the sequencer. Internal tempo exists (bpm_x10).
- Out: plat_midi_out (plat_fm1.c:132) mirrors to BLE (BLE-09). Clip feedback stays USB only.
- Settings storage: settings_t is saved in the project (SAVE/AUTOSAVE). It has a spare byte rsv_bright (unused). Adding bytes at the end changes the object size; project.c must load older (shorter) objects with defaults (the bt byte did this).
- Flash: last smk37 build (build/smk37/x0x.elf, maybe stale): code+data end about 0x85060. Limit 0x86EE0 (FLS-13). Free about 7.8 KB. This is tight.

## 4. Design (clip mode only)
### 4.1 Rules
- All new code runs only when clip_grid_mode = 1 (BOARD_SMK37). With clip mode off, the path stays exactly as now (MID-07, agent-rules 10).
- Keep CLP-34: a held pad blocks its shared white key. The block stays first in key_event.
- Do not copy Oscuro code. Write new, small code.

### 4.2 Where it inserts
- In key_event (ui.c), after the CLP-34 block: if clip mode is on, send the key to a new module `clip_keys` and return. The X0X UI does not see the key. (This changes CLP-33 "keybed keeps its normal function". Decision D1.)
- Velocity: use the same key velocity curve value as the X0X UI gets (check the key event carries velocity; if not, use the raw velocity from fm1_in).
- Chain: raw key note (plus octave offset) -> SCALE (snap or filter) -> CHORD (expand to 1-4 notes) -> ARP (if on) -> output.
- Output: a new function midi_send_ch(status, ch, note, vel) that goes through plat_midi_out (USB + BLE). Do not change midi_send_note (pads, ch 1).

### 4.3 Note-off safety (stuck notes)
- Keep a table: for each physical key, the notes it sent (up to 4) and the channel used. A note-off sends off for exactly those notes on that channel, even if scale, chord, octave or channel changed while held.
- Arp keeps its own "currently sounding" note and channel. Gate end sends its off.
- On any of these, send note-off for all sounding notes: clip mode exit, scale/chord/arp/channel change while notes sound (or let held keys finish – store-and-release is enough), arp off, latch off, MIDI Stop (arp only), USB reset/deconfigure (usb.c:554, 1113), BLE disconnect (BLE part only cannot be selective; send offs to both).
- Fallback: a "panic" on long press (e.g. hold KNOB BANK + OCT-) sends CC123 All Notes Off on the keybed channel.
- OCT-/OCT+ in clip mode already move the clip view (main_fm1.c ~800). Octave change must not move held notes (store-and-release covers it).

### 4.4 Arp clock
- Base grid 24 PPQN (same as Oscuro and MIDI clock). Rate table 48,36,32,24,18,16,12,9,8,6,4,3,2 ticks.
- Source rule: if a MIDI clock tick arrived in the last 0.5 s (same rule as LIM-11), step on incoming ticks (Loopy clock). Else step on the X0X internal tempo (proj bpm_x10), tick generator in the 2 kHz or frame timer.
- MIDI Start resets arp phase to step 0. Stop: arp stops and sends off (if not latched to free-run; decision D4).
- Arp runs in the main loop / timer context, not the audio ISR. plat_midi_out is ISR-safe already ("audio ISR or main loop").
- No clock is sent by the arp. CLOCK OUT behaviour is unchanged.

### 4.5 Keybed MIDI channel setting (new requirement)
- Name: KEYS CH (keybed out channel in clip mode).
- Menu: GLO > MIDI section, new row "CLIP KEYS CH" after the part channels. Reason: GLO MIDI already holds the channel rows (MID-03), and GLO is where the user wants settings (GEN-09). Alternative: a clip-mode settings page (see 4.6). Decision D2.
- Values: 2-15 (see clash rule). Optional OFF only if the user wants it. Proposed default: channel 2.
- Clash with APC40 emulation: X0X sends APC notes on channel 1 (pads, PLAY 0x5B, RECORD 0x5D) and channel 16 (undo 0x70, redo 0x71). Loopy reads the APC40 device's notes as controls. If keybed notes go out on ch 1 or 16 from the same USB port, Loopy may treat key notes 0-39, 91, 93, 112, 113 as clip launches or transport.
  - Rule: the menu skips channels 1 and 16. Range 2-15. Default 2. (User to confirm, D3.)
  - Even on ch 2-15, Loopy may map any channel of the APC device. The Loopy side must route this device's notes on the KEYS channel to an instrument/MIDI track, and its APC40 profile must only listen on ch 1/16. This must be tested on the device (LIM-07 says nothing is verified end-to-end yet).
  - Better isolation (later): a second USB MIDI cable/port for the keybed. This changes the USB descriptor (outside clip mode, needs approval and more flash). Not proposed now.
- Persistence: store in settings_t. Option A: reuse rsv_bright (unused byte) – no size change, old projects load value 0 = "default" (map 0 -> 2). Option B: append a new byte after bt – needs project.c short-object handling like bt. Proposal: append new bytes (A is tempting but the name says reserved; ask). Store as 0 = default, 1-16.
- Also persist (same block, about 8 bytes): scale root, scale id, snap mode, chord type, arp on, arp mode, rate, octaves, gate, latch.
- Saved by SAVE/AUTOSAVE like other settings. Settings are not in undo (SET-06).
- Incoming notes on the KEYS channel: no change. CLP-11 filtering stays as is.

### 4.6 UI controls (no clash)
Clip mode now uses: pads (clips), KNOB BANK hold (toggle), OCT-/OCT+ (view), SEQ PLAY/REC (APC transport), PATCH+PLAY/REC (undo/redo). It ignores <, >, short KNOB BANK, FADER BANK, faders. Free in clip mode: SCALE, CHORD, ARP, NOTE REPEAT, FX, PARA, knobs, faders.
- SCALE button (B_SEL) short press: scale on/off. Hold + knobs: K1 root, K2 scale, K3 snap mode (Near/Filter).
- CHORD button (B_ENV) short: chord on/off. Hold + knobs: K1 type, K2 voicing/inversion.
- ARP button short: arp on/off. Hold + knobs: K1 mode, K2 rate, K3 octaves, K4 gate. Double-press or ARP+NOTE REPEAT: latch.
- Need to confirm these buttons reach plat_buttons in clip mode and do not open X0X pages (they must be hidden from the X0X UI in clip mode, like OCT). Also the LCD: show a 1-line status (e.g. "ARP 1/16 UP 2O | C MIN | CH 2") on the clip screen.
- Faders stay unused (CLP-28).
- Keep the feature set small first: scales ~16 (masks are 2 bytes each), chords 8-12, arp modes Up/Down/UpDown/Order/Random, 6-8 rates, 1-4 octaves, gate, latch. Skip Evolve/Human/Rhythm/Strum at first.

### 4.7 Flash and RAM cost (estimate)
- Code: scale snap ~0.4 KB, chord ~0.4 KB, arp + clock ~1.5-2 KB, channel out + note table + panic ~0.5 KB, UI/menus/LCD text ~1.5-2 KB. Total about 4-5.5 KB.
- Tables: 16 scales x (2 B mask + name) ~0.2 KB; chords ~0.1 KB.
- RAM: ~200-300 B (37 keys x 4 notes, arp held-note list 16-32, state).
- Free flash is about 7.8 KB (from a maybe-stale ELF). Fits, but little margin. Build check (build.py) stops on overflow. Keep the first version small. Verify free space with a fresh build first.

### 4.8 Risks
- R1: Loopy may read keybed notes as APC controls (channel clash). Mitigation: ch 2-15 only, Loopy routing, device test.
- R2: Stuck notes in Loopy on mode change, settings change, USB reset or BLE drop. Mitigation: per-key sent-note table, all-off on exit, panic.
- R3: Clock jitter: arp ticks from incoming clock arrive via the 2 kHz USB ISR; fine for 1/32. Clock handover (external lost) may cause one late/double step.
- R4: Flash overflow into the BLE block (seen before, progress log). Must re-check the limit at every build.
- R5: CLP-33 change: the user loses X0X keybed functions in clip mode. Needs approval (BEHAVIOR.md update in the same task, USE-06).
- R6: BLE mirror doubles traffic; arp at 1/32 with chords is fine for BLE-MIDI but check the BLE queue (DIAG limit noted in log).
- R7: Licence of Oscuro unknown; re-implement only.

## 5. Open decisions for the user
- D1: In clip mode, should the keybed send MIDI notes to Loopy instead of driving X0X (changes CLP-33)? Or both (X0X sound + MIDI)?
- D2: Where does KEYS CH live: GLO > MIDI (proposed) or a clip-mode settings page (e.g. GLO pressed in clip mode)?
- D3: Default keybed channel (proposed 2) and range (proposed 2-15, skip 1 and 16 for APC).
- D4: Arp clock: follow Loopy clock when present, else internal tempo (proposed)? Should arp run when Loopy is stopped?
- D5: Feature scope v1: which scales, chords and arp modes (proposed small set) and whether Strum/Human/Evolve come later.
- D6: Send processed notes only (proposed), or also raw notes on a second channel?
- D7: Scale snap default: Near or Filter.
- D8: Settings persistence: new bytes at the end of settings_t (proposed) or reuse rsv_bright.
- D9: Button mapping in 4.6 OK?
