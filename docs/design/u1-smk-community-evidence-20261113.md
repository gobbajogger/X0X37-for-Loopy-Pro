# U1 SMK firmware user evidence — 2026-11-13

Evidence protocol: public comments are individual, self-selected accounts, **not representative user research**. Claims below describe what authors wrote, not independently verified behavior. No fabricated usage count, satisfaction scores, analytics or YouTube audience metrics. This is firmware workflow research, NOT an installer redesign.

## First-party product baseline
- M-VAVE publishes SMK-37 Pro and Elite V016 dated 2026-08-12: https://www.m-vave.com/download . Its stock capabilities differ from DnBOX and are not presumed to be current DnBOX behavior.
- Third-party SMK-37 emulator documentation says stock PATCH mode displays chord names when pressing a chord, but the stock firmware emits MIDI rather than audible emulator sound. The emulator explicitly warns it has not been hardware checked: https://synthhub.roomi-fields.com/virtual/fm-1/emu/smk37/ . **Design implication:** concise musical chord naming is already a recognizable use case; DnBOX must improve it by retaining exact recorded notes and disclosing interpretation, not replace it with generic labels.

## Community observations with traceable sources

1. **Controller/synth dual-use friction and firmware switching.** In an October 12 2026 r/MVaveFM1 post, a user values SMK37 as a Bluetooth MIDI controller and likes the creative custom firmware concept, but reports being unable to keep their preferred MIDI controller workflow while running custom synth firmware. The post also reports knob audio clicks and lost patch data as individual experiences; neither is DnBOX proof. https://www.reddit.com/r/MVaveFM1/comments/1x402o8/midi_controller_and_custom_firmware_for_the_smk37/ . **Design action:** explicitly show the current controller/instrument role and make mode switching/feedback intelligible; avoid a UI that makes sound selection appear to change the user's MIDI setup. Installer failures belong to W1, not U1.

2. **SMK hardware-native control surfaces matter.** The October 11 r/synthesizers SMK port-kit post describes Snoop37's velocity keys, pad colour, fader banks and wheel usage, plus Makkuro37's drone voice imagery and 96 presets; these are **author descriptions**, not performance benchmarks. A commenter asked for controller mode because the SMK keys/controls are important. https://www.reddit.com/r/synthesizers/comments/1x3fzyz/guide_to_port_mvave_fm1s_firmwares_to_the_bigger/ . **Design action:** use full 37-key range musically where the mode promises it; visibly map physical pad/key/fader functions; use meaningful, sober engine glyphs to reinforce identity but never substitute glyph for a sound label.

3. **Immediate mapping feedback rather than hidden state.** An October 10 r/MvaveSmk author wants to view current encoder digital state before moving it, and more knob/pad banks; their proposed no-synth firmware is a different product goal. https://www.reddit.com/r/MvaveSmk/comments/1x2gezu/smk37_pro_would_you_be_interested_in_a_midi/ . **Design action:** display exact target and saved/current value on focus and adjustment, with no forced movement or page jump. DnBOX must not drop synth/Bluetooth on the basis of that single user's preference.

4. **Physical shortcuts are discoverable by printed legend.** On November 6, a DumbMIDI user specifically requested preserving GLO + one of the first 16 note keys for quick channel changes, matching printed labels. This is a proposed shortcut, not proof it works in DnBOX. https://www.reddit.com/r/MvaveSmk/comments/1xp0ng8/new_firmware_dumbmidi/ . **Design action:** prefer direct, explained hardware shortcuts and contextual help over deep menus; resolve conflicts with note/record mode deliberately.

5. **Performance gestures are part of song creation.** The October 20 FMX post proposes a dedicated PERFORM layer with momentary breakdown/filter/FX and quantized change application; these are author-stated planned/demo features, not measured user satisfaction. https://www.reddit.com/r/MVaveFM1/comments/1xaelhk/fmx_wip_update_new_ui_auto_song_mode_at_530_loop/ . **Design action:** clear queued-vs-applied state and one-gesture performance controls, with audio ownership tests and no page-entry stall. Do not copy FMX implementation.

6. **Custom firmware interest is not permission to discard original controller use.** The October 29 discussion asks whether custom firmware retains normal MIDI controller functionality while improving synth sounds; replies describe cross-firmware limitations and alternatives without establishing the current DnBOX feature matrix. https://www.reddit.com/r/MvaveSmk/comments/1xizvc1/any_way_to_have_a_cfw_that_can_switch_back_to/ . **Design action:** explicit in-firmware modes and role labels; test actual project mode coexistence, not promise external compatibility.

7. **Budget hardware should emphasize a usable third octave.** A September 2 SMK37 discussion cites 37-key portability and mentions usefulness of extra range; small sample and model variation limit inference. https://www.reddit.com/r/synthesizers/comments/1w55xfw/mvave_smk37_looking_for_feedback_from_users/ . **Design action:** never show a '37-key record' workflow that silently converts most keys into editing shortcuts.

## Video boundary
The SMK port-kit author linked a YouTube demonstration at https://www.youtube.com/watch?v=HqUAT-hULgU . This **link is verified from the author's thread**, but the actual video sequence/timestamps were not independently watched or transcribed for this checkpoint. No claimed video observations. Existing source-grounded use of video is therefore pending.

## Why these findings deserve only bounded weight
- Most samples are developers and enthusiasts with self-selected posting incentives, not a representative onboarding cohort.
- A Reddit vote is not product usage, and even a detailed hardware report is not a reproducible DnBOX defect.
- No search-trend volume series or Google Trends export was retrieved. Search-interest comparisons remain NOT_RUN. Do not infer market rank from posts or catalog listings.
- Product-specific references (Baud Girl, Snoop37, Makkuro37, FMX, DumbMIDI) are comparative **workflow** references, not instructions to recreate their look, redistribute their code or import features without resource proof.

## Concrete evaluation matrix for owner review
- Novice first sound: fewer than or equal to three intentional actions; correctly named engine and exact current preset.
- Musical entry: note record/play access to all 37 keys; explicit transition back to 16-step editing.
- Sound editing: initial control focus displays part, engine, parameter and exact value; no accidental synth/pattern navigation.
- SEQ: spell `Cmin7` for C Eb G Bb; `C/E` for E G C; a defensible extended/slash chord for its actual voicing; retain actual notes for every case.
- Performance: visible applied versus pending changes; note-off remains tied to physical ownership across menu navigation.
- Preset: factory/user/import scope, edited status, expected compatibility and rejected import reason; no unproven Chicago/Miami plugin file interoperability.
