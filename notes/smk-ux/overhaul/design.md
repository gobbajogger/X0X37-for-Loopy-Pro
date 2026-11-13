# Phase 2 overhaul — three alternatives

**Status:** design proposal. Not a firmware patch, not a competing UI branch.
**Baseline:** `94197fc839b00aff47b98ca02f3c6396e4a6eabf`.
**Write set:** `/workspace/smk-ux/overhaul/` only. Firmware, `seq-chords/` (including `piano-roll.md`), FMB’s chord-namer, `presets-spec.md`, and `directions/frames/` are **not** edited.
**Renderer:** packed felucca (`tools/gen_font.py` FONT_LZ_CROP, Barlow, same advances as the device) and `directions/glyphs/shipped_glyphs.py` (decoded 26×19, `play_icon`, `night_mark`, `draw_wave_glyph`, drum and orig strips). No PIL TTF on any 240×240 frame. Geometry 20+69+69+82, AMBER night theme.
**Owner vision:** a firmware instrument — sound, musical capability, feel, usability, reliability. Judge on real musical tasks. Keep meaningful glyphs, add the missing Juno and Drone ones. Keep gestures, muscle memory, the full kit, and saved-data compatibility. Remove nothing. Chicago is the 303. Miami is never a 303 and is never labelled 303B.

## Chord labels (all three alternatives, owner 2026-11-13)

The owner **rejected** the §10 interval form (`C(3)`, `C(b2 2)`). It is **not drawn anywhere**. A separate revision of §10 is in progress; this work does **not** edit `piano-roll.md`.

Policy used on every SEQ frame:

- A **unique exact** compact name is shown (`Cmin7`, `C/E`, `Cadd9`, a major triad as bare `C` never `Cmaj`). Tier-3 exact names such as `Cadd9` are exact matches and are allowed.
- In **every other case** (dyad, cluster, two exact readings, anything unmatched) the screen shows **just the actual notes** (`put_note`, sharps only: `C3 E3`, `C3 C#3 D3`, `D3 G3 C4`). No forced name, no second reading, no `?`.
- The held line (232 px, FONT_B→S→XS, `ui.c` 5645–5659) always lists every onset pitch. When an exact name exists it is prefixed, then the notes.
- The 54 px NOTE cell shows the exact name, or the notes (FONT_S→XS; spaces drop only if the lane overflows). `C3xN` is used **only** as a last-resort overflow of the notes, and only on the honest **before** frames (shipped compact dump). It is not a chord name.
- Name the chord once on a held-step / onset, cache, invalidate on every input (`piano-roll.md` §9.8 F5). **Never** in the sequencer ISR, **never** stall the note scan (`smk-sentiment.md`: KEYS-screen dropout when the UI stalls).

Device-font fact, not a workaround: `gen_font.py` 183 `LOWER_KEEP = "abdkmnprstxz"` folds `i`, `e`, `o`, `h`, … on FONT_XS / S / B. The string `Cmin7` is what §9 requires; on the device it paints as **CmIn7**. Frames use that font as shipped. Hints and footnotes are FONT_T capitals so they do not inherit the fold.

## The three alternatives

These are information-architecture differences, not chrome variants. Every after-frame shares the unified 26×19 SOUND grammar, the chord policy above, first-play without stock hold-to-edit, and the preset journey (browse / audition / load / save / recover). Gestures and K4 (pattern cue, not a preset browser) stay.

**R — Instrument first.** Closest to the shipped instrument. First play is 909 SEQ with visible parameters. SOUND is one 4-row night matrix for all six engines (Juno and Drone gain 26×19; Chicago and Miami keep their own pictures; WAVE is a continuous saw↔pulse, never SAW/PUL names). Presets live on SOUND. Pitched SEQ uses the existing `draw_303` roll plus exact names. Recommended lean ship set.

**S — Score first.** 909 / 808 still open on the kit. Pitched engines (Juno, Drone, and a Chicago/Miami monophonic roll) **open on the piano-roll**, so the first musical object is a playable score. Stopped transport uses the documented hollow `play_icon` (same 9×11 footprint, Direction B proposal). Preset audition is in sequence context: you hear the preset against the roll that is already up. Heavier first-play change for Juno/Drone users; 909 muscle memory is unchanged.

**L — Live rail.** Same SOUND and SEQ as R, plus a **2 px state rail** under the 20 px header that **never times out**. Amber if unsaved or waiting, accent if playing and clean, dim if idle. `UNSAVED`, `SAVE AT STOP`, and `SAVES WHEN STOPPED` stay visible for as long as they are true. `SAVED` is only a successful flash write; a RAM capture is never that word. The rail sits in the existing header (y=18–19), so the 20+69+69+82 fight is not reopened. This is mostly **policy**, not new pixels.

## Recommendation

Ship **R’s SOUND grammar and first play**, adopt **L’s unsaved-never-timeout** (0 B if it is a policy on the existing badge — do not add a second dirty flag), and use **S’s piano-roll open** for pitched SEQ (the roll is already designed in `piano-roll.md`; this is not a new view and not a second `held_chord_get`). Do **not** implement the rejected interval form (saves the ~200 B §10 interval estimate). Measure the namer, the kind-map, and any factory appends **at link** before a count ships. Elite headroom is already **248 B** and the Tier-1 namer is **285 B measured**.

---

## Changes

Every change below is for the recommended ship set unless a row names S or L only. Each row is the smallest coherent design that answers the problem, not a label tweak.

### 1. One SOUND grammar for six engines

- **Problem.** 909 / 808 / Chicago / Miami already have 26×19 pictures in a 4-row night matrix (`drum_strip_glyph` `ui.c` 8044–8197, `orig_strip_glyph` 7846–7959). Shipped Juno / Drone are name + percent + a 42 px bar, because `orig_strip_slot` 7734–7740 returns 0 for them. The two grammars make Juno and Drone look like a different product. Flipping that slot test alone would paint Juno with Miami’s pictures (`directions/glyphs/grammar.md`).
- **Evidence.** `original_sound.c` 38–57 (four Juno, four Drone); `juno6.c` 41 (WAVE is `saw*(1-macro[0])+pulse*macro[0]`, continuous); `drone_string.h` 64–66 (REGISTER is a continuous three-rank mix, not a 1-of-n switch). Sentiment: novices feel lost even with the manual (`smk-sentiment.md`, r/synthesizers 20 Apr 2026).
- **User impact.** Every engine’s SOUND page is the same kind of page. A player who has learned 909 TUNE can read Juno WAVE and Drone REGISTER. Miami stays Miami (never labelled 303B); Chicago stays the 303.
- **Smallest design.** A kind-specific **add-only** map: Juno WAVE → existing `draw_wave_glyph` shape 1; Juno SUB → Miami SUB (`orig_strip` c=2, chi=0); Juno filter / LEVEL → Chicago CUTOFF / PEAK / ENV / LEVEL; Juno tails → `dg_tail`; plus the already-drawn add-only pictures (WIDTH duty `0.1+x·0.8` `juno6.c` 79, HPF, SUSTAIN, Drone REGISTER). 16 audible Juno pots fill the 4-row matrix on one page. Before-frames keep the honest bars.
- **HOST ESTIMATE.** Kind-map table **16 B** + dispatch **80–200 B**. Unique pictures already in the phase-1 port: **200–300 B** if the unique functions and two 9×9 identities land (identities still have no honest header slot — 6–7 px gap — and stay off the frames). 0 additional RAM if no new cache. CPU: idle SOUND is the existing idle painters; no note-scan work.
- **Acceptance.** On a JUNO POLY SOUND screenshot at the implementation SHA, all 16 cells show a 26×19 picture, WAVE is a mix (not the words SAW and PUL), and a Miami screenshot is labelled `MIA`, never `303B`. Task: a first-time Juno user names WAVE, CUTOFF, and CHORUS from the pictures, 0 errors, without the manual.

### 2. Clear states: sound, step, save / dirty, playing, queued

- **Problem.** Header overcrowding already fights part chip, mode, pattern, cue, tempo, `play_icon`, and a dirty mark (`grammar.md` layout problem 5). Unsaved can disappear behind a short display timeout. Queued pattern and playing can be missed. Sentiment: unsaved must not hide; latency and missed notes matter more than decoration (`smk-sentiment.md` §§1, 6).
- **Evidence.** Existing words already in the firmware: `SAVES WHEN STOPPED`, `SAVE AT STOP`, `SAVED`. `play_icon` `ui.c` 5153–5162. `header_pattern_badge` / cue. Stock KEYS-screen dropout (r/midi PSn505, Heise) is the warning against a UI that stalls the scan.
- **User impact.** A glance tells you whether you are on SOUND or a held STEP, whether the pattern is dirty, whether transport is running, and whether a pattern is queued. No decorative delay.
- **Smallest design.** Mode word `SOUND` / `SEQ` / `STEP n`. Filled `play_icon` in the part-colour / AMBER when playing, dim when stopped (S uses the documented hollow icon, same footprint). Pattern badge `P1` plus `Q:n` **only** for a pattern cue (K4). Dirty is the word `UNSAVED`, never `SAVED`. L adds the 2 px rail. Toasts are outline pills; the clock stays.
- **HOST ESTIMATE.** Mode and badge reuse existing header sites: **0–40 B**. L’s rail is **~20 B** of fill in the existing 20 px header, or **0 B** if an already-specified Direction B signal bar is used. No new ISR work.
- **Acceptance.** In a 5-second glance test, 8/8 players correctly call playing, queued (`Q:2` = next pattern, not a preset), and dirty. 0 players call a RAM capture “saved”.

### 3. Unsaved never times out (L, recommended for every ship)

- **Problem.** The before-frame `05-dirty-timeout` shows the honest shipped failure: transport is running, the edit is live, and `UNSAVED` has left the header. Players lose work or double-save.
- **Evidence.** `smk-sentiment.md` implication: unsaved must not hide behind a short display timeout. Existing words `SAVE AT STOP` / `SAVES WHEN STOPPED`.
- **User impact.** If the pattern is dirty, the mark stays until a successful flash write or the edit is discarded. Power-loss recover shows the last flash write, and says a RAM capture is not `SAVED`.
- **Smallest design.** Policy on the existing badge: do not start a timeout against `UNSAVED`. `SAVED` toast only after `flash` succeeds. Recover uses `SAVES WHEN STOPPED` plus the line `RAM CAPTURE IS NOT SAVED`.
- **HOST ESTIMATE. 0 B** if the timeout is removed or not started (a policy change). Do not add a second dirty flag.
- **Acceptance.** After 10 s of idle display with a dirty pattern, `UNSAVED` is still visible (task time 0, errors 0). After a simulated power-loss, the recover screen never shows `SAVED` for a RAM-only capture.

### 4. SEQ chord labels, exact-match only

- **Problem.** A held Juno chord is musically a `Cmin7` (or `C/E`, or `Cadd9`), but the shipped NOTE cell falls back to a `C3x4` dump, and the rejected §10 form would have written `C(3)` / `C(b2 2)`. Players who praised chord names (`smk-sentiment.md`) would be shown a forced name or a code.
- **Evidence.** `piano-roll.md` §9 (exact pitch-class match, `min` not `m`, bare `C`, slash `Root+suffix/Bass`, sharps, never `?`); §9.8 F1–F9. Owner steering 2026-11-13: compact names only on exact matches; otherwise the actual notes. Chicago / Miami are monophonic (`engine.c` 1085–1088); the chord labels belong on Juno / Drone, which already share `draw_303` (`ui.c` 5572–5668, 6888–6889).
- **User impact.** A `Cmin7` is named. A `C/E` is named. A `Cadd9` is named. A dyad is `C3 E3`. A cluster is `C3 C#3 D3`. An ambiguous set is `D3 G3 C4`, not a forced `Gsus4/D`. The roll itself is the existing `night_mark` (5 px, width law `ui.c` 5617–5618), one mark per stored pitch, TIE connectors at onset pitches only (`sequencer.c` 278–287).
- **Smallest design.** Call the same `held_chord_of` helper the bounded display fix occupies. Exact-match table is the namer’s T1+T2, plus T3 exact names if they ship; **no** interval shortener, **no** second-reading line. Cache on onset; invalidate on input. FONT_S for names that contain lowercase (`Cmin7` is 37 px, fits 54).
- **HOST ESTIMATE.** Tier 1 namer **285 B measured** (already over Elite). Tier 2 **+46 B**. Tier 3 exact names **+82 B HOST** (from §10 Rec), **without** the interval ~200 B and without the second-reading ~72 B, because those are not shipping. CPU: one name per onset, not per frame, not in the ISR.
- **Acceptance.** On the seven SEQ fixtures (Cmin7, C/E, Cadd9, bare C, dyad, cluster, ambiguous), 0 frames contain `C(`, `b2`, or a forced name. Players name the exact matches in <3 s and read the notes on the others, 0 errors. A 16-step SEQ at 140 BPM shows no additional note-scan dropout versus the bounded roll alone.

### 5. First play: sound, then sequence, then save, without stock hold-to-edit

- **Problem.** Stock Patch-hold-and-release and hold-to-edit are disliked (`smk-sentiment.md`). Dot 01 was a HOME that could not play a first sound. Parameters should be visible without a hold. First play must work on-device alone, no Midi Suite.
- **Evidence.** Evolved 01 as 909 SEQ is the right first page for a box that boots on 909. 909 BD factory Tune pot 34 → τ ≈ 12.96 ms (`drum909.c` 80–82), displayed as **13 ms**, not the old mock’s “14 Hz”. Attack 13 → 10 %.
- **User impact.** Power on, see a 16-step pattern, see TUNE / ATTACK / DECAY with real glyphs and honest units, press PLAY, hear it. K2 SOUND, K3 STEP, K4 CUE are named, not remapped. No new gesture.
- **Smallest design.** R and L: 909 SEQ is the first screen, with a FONT_T hint `PLAY TO HEAR  K2 SOUND  K3 STEP  K4 CUE` in the gap above the knob band (not a new control). S keeps that for 909 and adds `01-first-play-juno`: Juno opens on the roll, NOTE `C`, hollow stopped icon, `PLAY TO HEAR`. Save uses `SAVE AT STOP` while playing, `SAVED` only after flash.
- **HOST ESTIMATE.** Hint is a `say()` / a static string: **~40–80 B**. No new code path for the 909 SEQ itself. S’s Juno-opens-on-roll is a default-page choice per kind: **~20–40 B**.
- **Acceptance.** A novice, on-device, with the manual closed, plays the factory 909 pattern and can see the decay time in <30 s, 0 hold-to-edit attempts required. A Juno-first S user sees a roll and a `C` and can play a chord in the same bound.

### 6. Preset journeys: browse, audition, load, save, recover

- **Problem.** At `94197fc8` Juno and Drone have **4** factory presets each against **81** Chicago and **30** Miami (`original_sound.h` 15, 21; `presets-spec.md` inventory). There is no working user bank; import returns `-1`. Players need to audition without committing, load without erasing flash during play, and recover honestly after power-loss.
- **Evidence.** `presets-spec.md` §§1–5, 3.1 deferred save, 4 live-play. §3A plugin compatibility is **BLOCKED** and is not claimed. K4 remains pattern cue. Flash does not erase during play. Factory-export default OFF pending rights.
- **User impact.** Browse shows shipped 0–3 at full ink and any proposed 4+ marked `PROPOSAL, NOT IMPLEMENTED`. Audition hears without committing (`AUD`). Load commits; if playing, the preset **waits at stop** (`LOAD AT STOP  SOFT`) — this is **not** the `Q:` pattern-cue badge. Save while playing is `SAVE AT STOP`. Recover is `SAVES WHEN STOPPED` plus `RAM CAPTURE IS NOT SAVED`.
- **Smallest design.** One SOUND-page list, existing select gestures, no new browser mode, no Midi Suite. Proposed slots are documentation in the mock; Main Builder implements only after a flash-reserve measurement and a rights record (`overhaul/presets/juno-drone-factory.md`).
- **HOST ESTIMATE.** List and audition on the existing preset path: **unmeasured**, likely small if the path is completed as `presets-spec` already describes (the user-bank work is that spec, not this overhaul). New factory *data* is §7.
- **Acceptance.** Task: audition JUNO SOFT, reject it, leave JUNO POLY loaded — 0 accidental commits. Task: load while playing, confirm the running sound does not change until stop, and flash is not erased mid-bar. Task: recover after power-loss, confirm the last **flash** write, not a RAM capture, is what comes back.

### 7. Juno and Drone factory appends (proposal, not implemented)

- **Problem.** Four Juno and four Drone factory sounds cannot cover the §5.2 live-play roles. Existing 0–3 already sit near J-PAD, J-PLUCK, J-BRASS and D-ENS / D-WASH / D-AIR / organ; those are **not** retuned and **not** duplicated.
- **Evidence.** Exact pots and DSP laws cited in `overhaul/presets/juno-drone-factory.md` (`original_sound.c` 31–57, `juno6_set` 66–86, `original_runtime.c` 101, `drone_string.h` 64–70).
- **User impact.** A recommended first-ship subset — JUNO SOFT, STR, STAB, BASS, LEAD, and Drone Horizon — adds a swell pad, strings, a poly stab, a bass, a lead, and a very slow drone, all original programs, all append-only at indices 4+.
- **Smallest design.** Append-only tables. Zero new slots is acceptable. A subset is acceptable. Full eight-plus-two menu is a ceiling, not a target. Pending §5.3 render evidence and a rights/provenance record. Main Builder owns implementation.
- **HOST ESTIMATE.** **32 B** per Juno append, **20 B** per Drone append (compact `.rodata` row + name + pointer). Recommended subset **180 B**. Full menu **276–296 B**. Competes with the 285 B namer against 248 B Elite. Measure first. **0** ISR cost (`memcpy` at init, `original_sound.c` 227–230).
- **Acceptance.** BugBot’s §5.3 package (held 4 beats at 120 BPM, stab eighth, chorus listen, Juno C4 / Drone C3). A role that fails a listen is not that role, even if the pot table is in range. Indices 0–3 remain byte-identical.

### 8. What this overhaul does not do

- No competing firmware UI branch, no gesture remap, no kit cut, no saved-data break.
- No §3A plugin compatibility claim (BLOCKED).
- No “best in class” or zero-latency claim. No decorative transition delays.
- No edit of `piano-roll.md`. No interval-form implementation.
- No retune or rename of Juno / Drone 0–3, Chicago 0–80, or Miami 0–29.
- 9×9 Juno / Drone identities stay off the header (no honest slot). They remain on the phase-1 glyph sheet.

---

## Frame index

Native 240×240 1× and nearest-neighbour 3×. Shared honest **before** (shipped language, felucca, real glyphs; Juno / Drone bars; unsaved can time out; NOTE `C3xN` dump; four factory only). After: `frames/{R,S,L}/`.

| Stem | Before | After (R,S,L) | What it shows |
|---|---|---|---|
| `01-first-play` | 909 SEQ, no hint | 909 SEQ, `PLAY TO HEAR` hint, params visible | First play |
| `01-first-play-juno` | — | S only: Juno opens on the roll, NOTE `C` | S first play |
| `02-sequence` | playing 909 | playing 909 | Playing state |
| `03-sound-909` / `808` / `chicago` / `miami` | 26×19 | 26×19, same grammar | Six-engine SOUND |
| `03-sound-juno` / `drone` | 42 px bars | 26×19 (Juno all 16 pots) | The grammar split, then the unify |
| `03-sound-juno-b2` | — | same Juno page, CHORUS selected | Chorus as a first-class cell |
| `04-step` | held step | `STEP n`, params, no hold-to-edit | Step state |
| `05-dirty-timeout` / `05-dirty` | dirty mark **gone** | `UNSAVED` + `SAVE AT STOP`; L has an amber rail | Save / dirty |
| `06-saved` | `SAVED` | `SAVED` only after flash, stopped | Save success |
| `07-queued` | `Q:2` | `Q:2` = **pattern** cue; hint on K4 | Queued |
| `seq-juno` (before) | notes + `C3x4` dump | — | Honest shipped NOTE |
| `seq-juno-cmin7` / `inv` / `add9` / `maj` | — | `Cmin7`, `C/E`, `Cadd9`, bare `C` | Exact names |
| `seq-juno-dyad` / `cluster` / `amb` | — | `C3 E3`, `C3 C#3 D3`, `D3 G3 C4` | Notes only |
| `preset-browse` | 4 factory, user bank dead | 0–3 shipped, 4+ `PROPOSAL` | Browse |
| `preset-audition` / `load` / `load-playing` / `save` / `recover` | — | AUD, LOAD, `LOAD AT STOP`, `SAVE AT STOP`, `SAVES WHEN STOPPED` | Journeys |

Overview sheets: `sheet-before.png`, `sheet-R.png`, `sheet-S.png`, `sheet-L.png` (host PIL captions around 1× device frames).

## Flash budget

See `budget.md`. Headline: Elite **248 B**, namer **285 B measured** already over. Recommended UI adds the kind-map and optional unique glyphs; recommended factory subset **180 B**; interval form **0 B** (rejected). Measure at link. Ranked cuts: keep WAVE, SUB, filter, tails, LFO, chorus, LEVEL, ms/Hz text; drop WIDTH, REGISTER, identities if needed; ship fewer factory appends; do not shrink the reserve.
