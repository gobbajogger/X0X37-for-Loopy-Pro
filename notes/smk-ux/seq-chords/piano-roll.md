# B chord/note view (piano-roll-like SEQ)

**Date:** 2026-11-13, America/Chicago.
**Baseline:** `94197fc839b00aff47b98ca02f3c6396e4a6eabf`.
**This is not a separate design.** It is the chord/note view inside the existing A/B/C overhaul, drawn first in **Direction B (Live signals)** — the recommended language in `/workspace/smk-ux/directions/directions.md`. A and C variants use the **same data path** and the **same 16-column roll**, and only change marks, header, and held-step chrome. Gestures, live-note (`ui_live_key_owner`, `live_boundary`), shortcuts, streams 13/14/15, and engine voice counts are untouched.

**Method.** Design proposals on top of the diagnosis at `diagnosis.md`. Frames are **PROPOSAL**, not observed output, not a firmware patch. Firmware Main Builder is the sole writer of the bounded display fix, on `lane/fmb-seq-chord-display` (off `94197fc8`, existing SEQ visuals only). Second Builder is cancelled. The branch is **INFERRED**: a `git fetch origin lane/fmb-seq-chord-display` on 2026-11-13 (America/Chicago) returned `couldn't find remote ref`. The change map names the functions diagnosis says that fix must occupy, cross-checked against the Juno/Drone appendix, and shows the overhaul **building on** them rather than adding a second chord reader.

**Patterns** are the same five as the diagnosis (Juno on 303B, 303A not Juno, length 16, rate 1/16, transpose 24, root C, scale MAJ, example seed `00A3`). MIDI: C3=48, E3=52, G3=55, A3=57, B3=59, D4=62, F4=65.

---

## 1. What B is doing here

Direction B already has a family: **fill** = selected, playing, or briefly confirmed; **amber outline** = waiting, always named; **hollow triangle** = stopped; **quiet 1 px rail** that turns amber when something is waiting; the clock never leaves.

On SEQ, a recorded held-key chord is just more of that family, applied to the roll that `draw_303` already draws:

- Every stored pitch gets an existing `night_mark` (5 px, same width law, same 16 columns). A triad is three fills; a single note is one. “Chord vs single” and overlap become visible without a new widget or a new view.
- Duration stays **per step**, as stored: one gate and one endpoint nibble cover every pitch of that onset. TIE connectors run at **every** pitch of the onset chord, looked up at `note_start_of`, never at a TIE index (TIE steps do not carry lanes — `sequencer.c` 278–287).
- The pitch window uses those same extras, so a six-note Juno chord is not clipped (diagnosis fault 2).
- A held step is **waiting**: the header gets B’s amber outline pill `STEP n` (same mark as `SAVE AT STOP`); the rail goes amber; the held column’s bars keep the source **white fill** (already `s == ui.held_step` at 5616) and gain a 1 px amber outline. The held line **names every pitch**.
- Stopped = hollow triangle; playing = filled. No `STOP`/`PLAY` word.
- `CHORD NOT SAVED` is the designed refusal (`say()` at 3355, 3364, 4450). B does not change when it fires. It does apply B’s toast rule: **the clock stays.** The long string is an outline pill with a 3 px part-colour tick, same treatment as `SAVES WHEN STOPPED`. The roll under it already shows the six notes that *were* saved.

No new SEQ mode, no piano-key sidebar, no per-note gate editing (the store has none). AC/SL rows stay per-step.

---

## 2. ui.c change map — bounded fix, then overhaul

`lane/fmb-seq-chord-display` was **not readable**. Everything in this section about that branch is **INFERRED** from `diagnosis.md` §6, for a display-only patch that uses **existing SEQ visuals only**. The Juno/Drone appendix (`directions/engines-juno-drone.md`) lists `draw_303` “5572–5918”, `held_chord_*` 3348–3493, `bass_write_held_chord` 3348, and piano-roll comments at 5557 and 5918. Cross-check: **`draw_303` is 5572–5668**; the 5572–5918 span overruns PARA and Break (`draw_param_widget` 5671, `draw_part_sound` 5740, `draw_break` 5836) and must not be treated as roll territory. The 5556–5558 comment is on `note_start_of`. The 5918–5919 comment is **`bass_content_bits16`** (5920–5938), used for HOME-lane glyphs (5957, 6043), gates only — **leave it alone**. `ui.c` 3348–3503 is the write/edit path (`bass_write_held_chord` 3348–3370, `held_note_by` 3472–3503), **not** in the display bound. See `diagnosis.md` §6.

### 2.1 What the bounded fix occupies (INFERRED)

| Site at `94197fc8` | Function | What the fix is inferred to do |
|---|---|---|
| `ui.c` **5556–5570**, new helper beside `note_start_of` / `held_note_of` | **`held_chord_of`** (name INFERRED) | `held_chord_get(lanes, pat, b, note_start_of(bp,s), bp->step[start].note, out)`. One reader. Never looks up a TIE index. `held_note_of` itself can stay (root). |
| `ui.c` **5572–5668** `draw_303` | the whole draw | The only SEQ roll Juno and Drone use (6888–6889). |
| `ui.c` **5581–5596** | window | Walk extras from that helper so `lo`/`hi` include every stored pitch. Floor of 14 st stays. |
| `ui.c` **5614–5638** | marks, ties, p-lock notch, slide | A `night_mark` per stored pitch, same `width` / `fc` / height 5. TIE connectors (`5622–5633`) at **each** of those y values. Slide preview (`5635–5637`) may stay root-to-next-root (303 slide is into the next **step**); extending it is optional and still this site, not a new function. |
| `ui.c` **5645–5659** | held / WRITE line | List every onset pitch (`put_note` in pitch order). If `FONT_B` overflows 232 px, drop to `FONT_S` then `FONT_XS` — still this line, no new banner. |
| `ui.c` **8378–8381** `draw_knobs` | held-step NOTE cell | Same helper. `FONT_M` cannot hold six names in a 54 px cell: compact `C3x3` / `C3x6` (root + TOTAL note count; no "+" = augmented, no "/" = slash chord) (INFERRED layout), with the full list on the 5645 line. GATE / ACCENT / SLIDE unchanged (they are already per-step). |
| **Not** in the bound | Appendix write/edit `held_chord_*` **3348–3503** (`bass_write_held_chord` 3348–3370, `held_note_by` 3472–3503); `bass_content_bits16` 5920–5938; PARA/Break inside the appendix’s 5572–5918 span; `seq_rec_bass`; streams 13/14/15; `JUNO6_VOICES` / `DRONE_STRING_VOICES`; Chicago/Miami monophonic paths | Recording, refusal, HOME-lane bits, and engine polyphony stay as they are. Related edit-path note (not in the bound): `held_note_by` already calls `held_chord_get` at `ui.held_step` (3484), which misses extras if that index is a TIE. |

### 2.2 How A/B/C build on it — they do not duplicate it

All three overhaul options **call the same helper** the bounded fix introduces. They must not add a second `held_chord_get` walk, a second window, or a parallel roll.

```
stored lanes ──► held_chord_of (bounded, INFERRED) ──► lo/hi, marks, ties, labels, NOTE cell
                                              │
                                              ├─ A: leave those marks as source night_mark;
                                              │     held line already lists every name.
                                              │     Header and play icon as in Direction A
                                              │     (revealed strings; clock stays on A's
                                              │     save path; this view does not add pills).
                                              │
                                              ├─ B: same marks; white fill on a held column
                                              │     plus 1 px amber outline; header pill
                                              │     STEP n; amber rail; hollow/filled
                                              │     triangle; CHORD NOT SAVED keeps the clock.
                                              │     (header_proposed / signal_bar already
                                              │     in the B overhaul — not a chord reader.)
                                              │
                                              └─ C: same marks; part-colour spine and
                                                    hairline; held step turns the edge
                                                    amber and the 5645 line into a full-width
                                                    amber plate (black text, all names);
                                                    tickets for CHORD NOT SAVED; play icon
                                                    as shipped.
```

**A** is the bounded fix as the player sees it, in Direction A’s “already-written pixels” rule: more `night_mark` bars, fuller labels, source play icon, source (or A-general) header. No amber rail, no pill, no hollow triangle.

**B** (this document, recommended) is that roll **plus** the Live-signals family already specified for save, held step, and transport. Chord-specific work in B is styling and naming at sites the bounded fix already touches (5616 `fc`, 5645 line, header around 5418–5502 and 5345–5369), not a new data path.

**C** is the same roll in the Two-rooms language: spine, hairline, amber plate, tickets. The plate **is** the 5645 held line, restyled, not a second string. Play icon stays shipped.

If Firmware Main Builder’s actual diff occupies these sites, the overhaul patches **on top** of `lane/fmb-seq-chord-display`. If the branch lands elsewhere, update this map; do not reimplement `held_chord_get` at onset.

---

## 3. Label width (B’s “always named” vs 240 px)

A six-name `FONT_B` string `STEP 1  C3 E3 G3 B3 D4 F4 V100` is tight on 240 px (diagnosis §7, INFERRED). B’s rule is: the object is always named, not that every name is `FONT_B`.

- Held / WRITE line: try `FONT_B`, then `FONT_S`, then `FONT_XS`, left at x=4, y=122. Names are `put_note` in stored (strictly increasing) order.
- Header pill stays the short `STEP n` (already shown to fit with `JUNO B` + `SEQ` left of the pattern badge at x=132).
- NOTE cell: one pitch = `FONT_M` `C3`; more = compact `C3x3` (INFERRED), full list on the line above.

---

## 4. Frames

All **PROPOSAL**. 240×240 at 1×; nearest-neighbour 2×. Native strips 20+69+69+82. Barlow roles. RGB565-safe, night + AMBER. Header chip **`JUNO B`**, unheld tag **`P1 SEQUENCE`**, held tag **`JUNO STEP n`**, knobs **LENGTH / RATE / DIRECTION / TRANSP**, scale **`C MAJ`**.

| View | 1× | What it shows |
|---|---|---|
| B P1 Sustained triad | `frames/1x/B-01-sustained-triad.png` | C3–E3–G3 fills + ties across four steps; playhead; filled triangle. Faults 1 and 4 closed. |
| B P2 Overlapping chords | `frames/1x/B-02-overlapping-chords.png` | C triad then E-minor; shared E3/G3 visible in both onsets (new onset, not a TIE). Fault 5 closed. |
| B P3 Chord + single, held | `frames/1x/B-03-chord-and-single.png` | White fills + amber outline on the triad; A3 a single fill; pill `STEP 1`; rail amber; line `STEP 1  C3 E3 G3 V100`; NOTE `C3x3`; hollow triangle. Faults 1 and 3 closed. |
| B P4 Six-note limit | `frames/1x/B-04-voice-limit.png` | Window includes F4; all six fills. Fault 2 closed. |
| B P5 Seventh key refused | `frames/1x/B-05-beyond-limit.png` | `CHORD NOT SAVED` outline pill, 3 px cyan tick, pattern and tempo remain; six fills still on the roll. |
| A P1 / A P3 | `frames/1x/A-01-sustained-triad.png`, `A-03-chord-and-single.png` | Same roll data as B; source marks and header; no pill, rail, or hollow triangle. P3 names the chord on the held line only. |
| C P1 / C P3 | `frames/1x/C-01-sustained-triad.png`, `C-03-chord-and-single.png` | Same roll; spine + hairline (cyan, amber when held); P3 amber plate with the names; shipped play icon. |
| Sheets | `sheet-B.png`, `sheet-B-2x.png`, `sheet-variants.png`, `sheet-variants-2x.png` | B’s five patterns; A/B/C of P3 (and P1 on the variants sheet). |

2× copies sit in `frames/2x/`.

---

## 5. What this is not

- Not a new view or a remap. SEQ stays SEQ. PARA, PATCH, NOTE REPEAT, K4, live-note, and the shortcut list stay.
- Not a per-note editor. The store has one gate, one endpoint, one velocity per step.
- Not a Drone voice-limit fix. A five-note Drone chord still **saves** (store is 6; `DRONE_STRING_VOICES` is 4). The roll will show five; stealing is engine work, out of this bound.
- Not a reimplementation of Firmware Main Builder’s patch. If `lane/fmb-seq-chord-display` becomes readable, this map should be checked against its actual functions and lines.
- Not observed on a device. CPU and blit cost are not measured; an implementation plan must include them.

Elite and Pro interaction is identical in source; these frames are that shared UI.

§§6–8 complete the original brief: data facts from source, other engines, and options with flash/RAM tradeoffs.

---

## 6. Data facts (from source, `94197fc8`)

All file:line cites are this SHA, read-only. HOST ESTIMATE means struct layout, not a linked size.

### Polyphony, per engine

| Engine | Kind | Real voice count | Recording / play |
|---|---|---|---|
| **Juno** | `ORIGINAL_SOUND_JUNO` **3** (`original_sound.h` 12) | **`JUNO6_VOICES 6`** (`dsp/juno6.h` 7). This is engine polyphony, not a UX “six user notes” cap. | `seq_bass_poly` is true (`engine.c` **1085–1088**: `kind >= 2 && kind <= 3`). Held-chord write is offered when `engine_bass_sound_bank(...) >= 2` (`ui.c` 3393). |
| **Drone** | `ORIGINAL_SOUND_DRONE` **2** (same header, 11) | **`DRONE_STRING_VOICES 4`** (`dsp/drone_string.h` 32). | Same `seq_bass_poly` path. The **store can hold 6** (below); the engine has **4** voices. A five- or six-note Drone chord **saves**. Voice stealing is engine work, not a display invention. |
| **Chicago** | `ORIGINAL_SOUND_CHICAGO` **0** (9). Chicago is the **303 engine**. | Monophonic / unison: `seq_bass_poly` is false (`engine.c` 1085–1088, “Chicago/Miami preserve mono/unison”). | `bass_write_held_chord` is not called (3393). One pitch per step. |
| **Miami** | `ORIGINAL_SOUND_MIAMI` **1** (10). **Never called a 303.** | Monophonic / unison, same test. | Same as Chicago. |

Do **not** treat these as note-chord counts: `CHI_ENS_VOICES 3` (`chicago_ens.h` 12) is an ensemble sweep; `ORIGINAL_MIAMI_BACKEND_VOICES 2` (`dsp/original_miami_backend.h` 9) is a backend macro.

### What a step stores (sizes)

- **Primary**, in `bstep_t` (`pattern.h` **29–33**): `uint8_t note` (MIDI), `uint8_t flags`, `uint8_t velocity`. **Three bytes** per step (HOST ESTIMATE; all `uint8_t`, no padding). One step in each of `bpart_t.step[NSTEPS]` (`pattern.h` 51–58), `NSTEPS` 64 (`pattern.h` 15).
- **`flags`** (`pattern.h` 24–32, 112): low two bits are the gate (`G_REST`, `G_NOTE`, `G_TIE`, enum at 21; `bstep_gate` 112). `BS_ACCENT` 0x04, `BS_SLIDE` 0x08 (slide **into** the next **step**). The high nibble is the **endpoint**: `BS_END_SHIFT` 4. **0** = legacy gate; **1..15** = captured release fraction of **this playback slot** (`pattern.h` 24–25).
- **Velocity**: 1..127; **0 means default 100** (`BASS_VELOCITY_DEFAULT`, `pattern.h` 23, 35–38). One velocity for the whole step, including every extra pitch.
- **Extras** (up to **`HELD_CHORD_NOTES - 1` = 5**): sparse motion-lane pool, `HELD_CHORD_TARGET = 0xfc`, **no pattern or song layout change** (`held_chord.h` **4–8**, 21–32). Each extra is one `uint8_t` MIDI value in `lane_t.val[step]`, or `MOT_NONE` (`0xFF`). `held_chord_get` returns the root plus extras, **strictly increasing**; a decrease, duplicate, or `MOT_NONE` **stops** the list (29). Maximum stored notes: **6**.
- **`lane_t`** (`motion.h` **25–36**): 8 header bytes + `val[NSTEPS]` 64 = **72 bytes** (HOST ESTIMATE). Pool `NLANE` 160, shared with motion and note-lock. A full six-note chord uses up to five lanes (360 B of **existing** pool, not new RAM).

### Duration is per step, not per note

There is **no** per-note gate, endpoint, or velocity in the chord lanes (lanes store pitch only).

- One gate and one endpoint nibble on `bstep_t` cover every pitch of that step’s chord.
- A **TIE** (`G_TIE`) continues the sounding onset (`sequencer.c` 305, comment 302; `note_start_of` / `held_note_of`, `ui.c` 5556–5570). An orphan TIE is a rest, never an onset (5556–5558).
- On **REC while playing**, a held chord that crosses a step boundary writes **TIE steps with the root only**: `flags = G_TIE | 0xF0u`, and **no chord lanes** on those TIE steps (`sequencer.c` **278–287**). Extras exist only on the **onset**. The correct lookup is `held_chord_get(..., note_start_of(bp, s), root)`, never the TIE index.

**Any duration the roll draws must come from this stored data, never invented.** The existing width law is already that data: `end = st->flags >> BS_END_SHIFT`, `width = end ? (end * 12 + 14) / 15 : 12` (`ui.c` **5617–5618**). TIE connectors (`5622–5633`) are the stored `G_TIE`, drawn at each onset pitch’s y once the bounded fix loops them. A proposal must not invent a per-note length, a guessed release, or a bar that outlives the stored gate/endpoint run.

### Six-note store vs four-voice Drone

`HELD_CHORD_NOTES 6` (`held_chord.h` 8) is the store and admission cap (`held_chord_write` 39; refusal toasts `CHORD NOT SAVED`, `ui.c` 3355, 3364, 4450). Juno’s six engine voices match that cap. Drone’s **four** voices do not. The roll shows **what is stored**. It must not hide a fifth Drone note, and it must not pretend the engine has a fifth voice. Closing that mismatch is out of the display bound.

---

## 7. Other engines

The 16-column pattern overview and the playback position (playhead bar, `ui.c` **5611–5613**; HOME lanes, `draw_home` 5996, `bass_content_bits16` 5920–5938 at 6043; pattern badge and tempo in the header) **must be preserved** on every option below. No new SEQ mode.

| Part | Reason | What the view is |
|---|---|---|
| **Chicago** (the 303 engine, kind 0) | Monophonic (`seq_bass_poly` false, `engine.c` 1085–1088). One `bstep_t` pitch, one gate, one endpoint, one velocity. No extras are written (3393). It **needs only pitch plus length**, which `draw_303` already draws. It already **is** on that function (`is_303()`, **6888–6889**). A bounded fix that calls `held_chord_get` at onset returns **n = 1** and leaves the picture as today. Do not invent chord bars or per-note duration. Miami is never this engine. | Existing SEQ roll (pitch + length). Stay on `draw_303`. |
| **Miami** (kind 1, **never called a 303** or 303B) | Same monophonic fact and the same `draw_303`. Same reason: pitch plus length is the stored data. `ORIGINAL_MIAMI_BACKEND_VOICES` is not a UX chord. | Existing SEQ roll. Stay on `draw_303`. |
| **909 / 808 drums** | Stored as hit/accent **bit rows**, not pitches (`dpart_t`, `pattern.h` 60–66; `sm_get`). A piano roll would not fit that data and would delete the kit overview, which must stay (`draw_drum` **5511–5549**; directions and Dot’s README). | Keep the **12-row grid**. Playhead and selected row stay as they are. |
| **BREAK** | A **slice** model: `brkpart_t.steps` (which 16ths may sound), `slice[16]` (0 = generator, 1..32 = stored slice intent), `slice_mode` and rate groups (`pattern.h` **71–78**, 90–101). That is not a pitch roll. `draw_break` (**5836**) already shows this. | Keep the slice view. Do not force BREAK onto `draw_303`. |

Chicago and Miami sharing `draw_303` with Juno and Drone is a **reason to keep one function**, not a reason to build a second roll. If Main Builder’s fix stays inside `draw_303`, those mono engines keep pitch + length automatically. See option 4 below.

---

## 8. Options and tradeoffs

Budgets at this SHA, as Main Builder measured and as cited in `/workspace/smk-ux/directions/engines-juno-drone.md`: **248 B Elite / 256 B Pro** flash above the 8192 B floor, and **148 B** pool headroom. Device `.pool` free must stay ≥ 8192. Figures below are **ESTIMATE** (Thumb-2, from the size of similar `ui.c` draw branches, **not** a linked size) or **HOST ESTIMATE** (structs, string literals, TTF). Main Builder must measure at link. Preset work and Juno/Drone engine-page units already share this headroom; stacked totals can exceed 248 B. Remove no feature. **No draw-path CPU regression is allowed.** Streams 13/14/15 and the pattern layout are not changed. Elite and Pro interaction stays identical.

“On top of Main Builder’s fix” means sites **beyond** the inferred display-only bound (`draw_303` 5572–5668, helper at 5556–5570, `draw_knobs` 8378–8381). That bound is still **INFERRED** until `lane/fmb-seq-chord-display` is readable.

| # | Option | Flash (ESTIMATE) | RAM (pool) | String bytes | vs 248 / 256 / 148 | Draw-path CPU | Readability at 240×240 | Muscle memory | Saved-data | ui.c on top of Main Builder’s fix |
|---|---|---|---|---|---|---|---|---|---|---|
| **1** | **Bounded fix only** — existing SEQ visuals: `held_chord_get` at onset, expand `lo`/`hi`, `night_mark` and TIE connectors at every stored pitch, fuller labels. Duration only from `flags` / `G_TIE` (5617–5618, 5622–5633). | **80–200 B**. Helper + window/mark/label loops + compact NOTE (`C3x3`). High end is tight against Elite. | **0** new pool. Stack `uint8_t notes[6]` and (recommended) five cached lane indices. Do not declare a cache in `X0X_POOL`. | **0** new user strings. `CHORD NOT SAVED` and `put_note` already exist. Optional `"x"` literal: **2 B** HOST ESTIMATE. | Mid / fits if the low–mid end lands; measure before stacking with presets or engine-page units. 0 of 148 B pool. | **Main risk.** A naive per-step `held_chord_get` inlines up to five `NLANE` 160 scans, twice (window and marks), **per band**. That can regress the SEQ blit. **Required:** resolve the five lane indices **once per draw**, then read `val[onset]` per column. Two bands must not double a pool walk. | Good. The diagnosis frames already show the faults; this option is those faults closed in the shipped mark language. Six names still need the FONT_B → S → XS fallback (§3). | Small and beneficial: more bars, same gestures, same 16-column overview and playhead. | Compatible: display only. 6-vs-4 Drone mismatch stays visible, not “fixed” by hiding notes. | **None** — this **is** the fix (INFERRED sites in §2.1). |
| **2** | **Bounded fix plus B restyle** — option 1’s roll, then Direction B marks: white fill + amber outline on a held column, `STEP n` pill, amber rail, hollow/filled triangle, `CHORD NOT SAVED` keeps the clock. Same data path. This document’s recommended picture. | **Incremental 40–120 B** on top of (1) if B’s header primitives (rail, pill, hollow triangle) already land in the general B overhaul; **160–360 B all-in** if this option also pays for those marks. High all-in can **exceed 248 B Elite**. | **0** new pool (same stack as 1). 2 px rail and 1 px outlines are existing strip pixels, not a new framebuffer. | **0** new strings. Reuse `STEP`, `CHORD NOT SAVED` (already in the image, 16 B including NUL), `SEQ`. | Incremental fits on mid (1). All-in: tight or over Elite; Pro 256 B is only 8 B kinder. Measure at link; if over, keep option 1’s roll and drop only the hollow triangle (already a stated B fallback in `directions.md`). | Low incremental if (1) cached the lanes. Outlines and a header pill are a handful of `cv_rect`s. Toast-with-clock is a header blit, not a second roll pass. Still no regression allowed: do not re-walk the pool. | Best of the four for a held chord: named pill, named line, visible extras, clock stays (see `frames/1x/B-03`, `B-05`). Still 20+69+69+82. | Display-only. Experts will notice the hollow triangle (B’s known risk). Gestures unchanged. If the triangle is rejected, the rest of (2) can stand. | Same as (1). | **Header and play icon only**, not a second reader: toast / pickup path **5345–5369**, part chip and mode **5418–5502**, unsaved / clock **5450–5502**, play icon **5495–5498** (and the icon helper **5153–5162**), optional 1 px outline at the existing mark site **5616**. The 5645 line changes **ink**, not data. |
| **3** | **A full roll for Juno/Drone** — richer than “more `night_mark`s on the existing 14-st, 16-col `draw_303`”: e.g. a key gutter, an always-wide window, a Juno/Drone-only branch or a new draw function. Still must **not** invent duration, and must **keep** the 16-column overview and playhead. | **300–700 B** ESTIMATE. Likely **exceeds 248 B Elite**; may exceed **256 B Pro**. Needs a flash decision and a measured link before any plan to implement. | **0** only if it stays inside the existing 20+69+69+82 `cv_begin` strips. A new roll buffer would spend the **148 B** pool and is **out**. | Prefer **0** new user strings. New captions would add flash 1:1 and are not required to show stored pitches. | Over budget unless something else yields. Do not take from presets or from option 1. | **High risk** if a second pass, a tall keybed, or a per-frame window walk is added on top of (1). A Juno-only branch that still calls a naive `held_chord_get` doubles the danger. Easy to slow SEQ while playing. | Uncertain at 1×. A gutter steals columns from the 16-step overview (forbidden if it hides playback position). A wider window helps P4 (already solved in (1) by expanding `lo`/`hi`). | Larger display change. A new “roll room” can be misread as a new mode. No remap is allowed to accompany it. | Compatible only if it remains display-only. Temptation to add per-note lengths would **break** the store (lanes have no duration) and is disallowed. | On top of (1): a heavier rewrite of **5572–5668**, possibly a new `draw_*` and a kind test at **6888–6889**. That leaves the “existing SEQ visuals only” bound and can drift into PARA/Break if the 5572–5918 appendix span is misread — don’t. |
| **4** | **Extending it to Chicago/Miami** — same visual language (1, 2, or 3) on the mono engines. | **0–20 B** if work stays in `draw_303` (they already use it, 6888–6889). `held_chord_get` returns 1; the picture stays pitch + length. **0–40 B** only if (3) had introduced a Juno/Drone-only branch that this option then removes. | **0**. | **0**. | Negligible against 248 / 256 / 148. | Negligible, **if** the lane cache from (1) is skipped when `seq_bass_poly` is false (one pitch, no pool walk). Do not start walking 160 lanes on a Chicago pattern. | The existing roll is already the right density for one note. A “full” roll (3) on Chicago/Miami adds chrome they do not need. | None beyond whatever (1)–(3) already did: same function, same gestures. | Compatible. Do not start writing extras on mono engines, and do not change streams. | **None** if (1)–(2) stay in `draw_303`. If (3) split the path, this option only deletes that split at **6888–6889**. |

### Recommendation

**Take option 1 now, then option 2**, in that order.

Firmware Main Builder’s bounded fix (option 1) is the one that closes diagnosis faults 1–5, stays on existing SEQ visuals, and is the only display writer. It must land first, with a **once-per-draw** lane lookup (CPU) and a **measured** flash at link against 248 B Elite / 256 B Pro. Duration only from stored `flags` and `G_TIE`.

**Option 2** is the recommended overhaul picture (this document’s B chord/note view). It builds on that helper; it does not add a second reader. Prefer paying only the incremental 40–120 B by sharing Direction B’s already-planned header marks. If the all-in cost exceeds Elite headroom, keep (1) and the named held line, and drop the hollow triangle first.

**Do not take option 3 now.** It is likely over the Elite flash budget, it is the CPU and overview risk, and option 1 already puts every stored pitch on the 16-column roll (P4’s window included).

**Option 4 is not a separate project.** Chicago (the 303 engine) and Miami (never a 303) already share `draw_303` and need only pitch plus length. Leave them on that function. Do not invent duration or chord lanes for them. Drums keep the grid; BREAK keeps slices; the 16-column overview and playhead stay.

§9 adds chord notation on this same B view (display-only; pitches and playback never change).

---

## 9. Chord notation — what will play

**Date:** 2026-11-13, America/Chicago.
**Baseline:** `94197fc839b00aff47b98ca02f3c6396e4a6eabf`.
**This is a design proposal**, not a firmware patch, not observed on a device. It sits on the Direction B chord/note view in §§1–8. It does not redesign every engine. **Pitches and playback never change.** The namer only writes a label.

The input is the **sounding** set — `held_chord_get` at the onset — plus the **effective transpose**, reduced to a 12-bit pitch-class mask. Octaves fold. The lowest sounding note is the bass. Spelling follows `put_note`. A compact symbol is shown next to the notes so the player can see what will play. This section is the **final naming spec** sent to Firmware Main Builder: where it conflicts with earlier notes in this folder (`C3+2`, `?` candidates, “C minor 7”), **this wins**.

Host-side code and frames: `chord_name.py` (namer + acceptance tests), `render_chord_notation.py`, `frames/1x/N-*.png` (native 240×240) and `frames/3x/N-*.png` (nearest-neighbour 3×), `sheet-notation.png`, `host-size/` (JieLi size probe).

---

### 9.1 Source — the sounding set and the effective transpose

The input set is the notes that **sound**, after the same transpose already applied to playback and to the existing labels, then folded to pitch classes.

**What is in the set**

1. **Recorded pitches at the onset.** `held_chord_get(lanes, pat, b, note_start_of(bp, s), bp->step[start].note, out)` (`held_chord.h` **21–32**). Root plus extras, strictly increasing, at most 6. Look up the **onset**, never a TIE index: TIE steps do not carry lanes (`sequencer.c` **278–287**; `note_start_of`, `ui.c` **5556–5570**). This is the same helper the bounded display fix is inferred to introduce (§2.1).
2. **Effective transpose, then clamp.** Every pitch, root and extras, is `note + (int)bp->transpose - 24`, clamped 0..127. That is already what playback does (`sequencer.c` **309** for the root, **358–361** for extras) and what the held line and the NOTE cell already pass to `put_note` (`ui.c` **5651**, **8381**). `bpart_t.transpose` is “semitones + 24 (24 = none), applied when played” (`pattern.h` **56**); default 24 (`pattern.c` **28**); the SEQ knob `TRANSP` is 0..48, centre 24 (`ui.c` **467**, `R_BTRANS` **640**, **770**). Each bass part on each pattern has its own byte — there is no second track-level layer on top of it.
3. **A 12-bit pitch-class mask.** `pcs |= 1u << (n % 12)` over the sounding MIDI. Octaves fold. The lowest sounding MIDI note is the bass (`notes[0]` after the store’s increasing order, then the same transpose). The symbol has **no octave**.

**What is not applied at playback, and is not in the namer**

These were searched at this SHA and are **not** a MIDI transpose of the sounding set. Marked clearly so they are not silently folded in.

| Offset | Where | Role at this SHA |
|---|---|---|
| Pattern / part transpose | `bp->transpose` (above) | **The** effective transpose. In the input. |
| Track transpose (a second layer) | — | **Not found.** The part’s `bp->transpose` is the only semitone offset. |
| Global semitone transpose | — | **Not found.** |
| `FINETUNE` / `engine_bass_tune` | `ui.c` **1252–1263**, `engine.c` **1010–1019**, `engine.h` **246** | A frequency offset in “steps”, shown in Hz. The comment is “Chicago and Miami”. It does **not** change MIDI note numbers. Not in the namer. |
| Generator `ROOT` / `SCALE` / `OCTAVE` / `oct_range` / `base_oct` | `pattern.h` **43–46**, `tb3po.c` **8–16**, **52–83**, **118**; `GEN_P` at `ui.c` **441–446** | TB3PO **generator** only. They write stored MIDI when a line is generated. They are not applied again at playback. The unheld 5645 line already shows `C MAJ` as that generator setting, not as a playback key. |
| Keyboard `ui.oct` | `kbd_note` `ui.c` **3295**; OCT buttons **4141–4153** | Octave at **entry**. The stored MIDI already includes it. Not a playback offset. |
| Held-step OCTAVE | `held_note_by(±12)` at `ui.c` **4114–4116** | Writes the **store**. The namer then sees the new stored pitches. No extra offset. |
| Clip-mode scale / chord (`ck_*`) | `main_fm1.c`, `smk_clip_keys.h` | Clip keys, not SEQ playback. |

**Drone, sounding vs stored.** The store holds 6 (`HELD_CHORD_NOTES`, `held_chord.h` **8**). Drone has **4** voices (`DRONE_STRING_VOICES`, `dsp/drone_string.h` **32**). All six can be sent (`seq_bass_chord_on` → `original_runtime_key`, `engine.c` **1110–1114**; extras in stored order after the root, `sequencer.c` **357–367**). On a fresh poly onset the previous sequence keys are released first (`engine.c` **1129–1132**). Voice steal (`drone_string_note`, `drone_string.h` **124–156**): re-trigger a held pitch, else a free slot, else prefer RELEASE, then lowest envelope, then lowest age. **INFERRED**, not device-observed, for a cold start / same-tick onset: the first four triggered pitches take the voices; the fifth and sixth steal the two oldest (the root and the next), so the **highest four** sound and the **lowest two** are stored but silent. The namer uses only the sounding four. The two dropped marks are drawn in the existing dim colour (`C_DIM`, `gfx.c` **54**). This steal is **state-dependent** (a previous note still ringing changes the pick). The N-15 frame is an EXAMPLE SET, labelled INFERRED. Closing the 6-vs-4 mismatch is still out of the display bound (§5, §6).

**Spelling.** `put_note` (`ui.c` **388–392**) writes `NOTE_N[(n % 12 + 12) % 12]` plus `n / 12 - 1`. `NOTE_N` is `X0X_NOTE_N` from `note_names.h`: **sharps only**, `C C# D D# E F F# G G# A A# B`. There are no flats. An input described as “C Eb G Bb” sounds the same pitches and is labelled `Cmin7`, with the notes written `C3 D#3 G3 A#3`. The store is not respelled.

---

### 9.2 Naming rules

Compact device names, never long words. Minor is always `min`, never `m` or `M`, so `Cmin7` cannot be read as a major or as a single letter:

`C` · `Cmin` · `C5` · `C7` · `Cmaj7` · `Cmin7` · `Cdim` · `Csus4` · `Csus2` · `Caug` · `C6` · `Cmin7b5` · `Cdim7`

**A major triad is the bare root `C`, never `Cmaj`.** Chosen and justified: (1) this is the final spec; (2) players say “C”, not “Cmaj”, for a major triad, and `maj` is reserved for `Cmaj7` so the two cannot be confused; (3) `C` is 12 px at FONT_M and always fits the 54 px NOTE cell, while `Cmaj` is already 46 and `Cmaj7` overflows FONT_M (below).

**Templates.** Exact pitch-class match only. Intervals relative to the candidate root.

Tier 1, required, in this order:

| Suffix | Intervals | Shown as (root C) |
|---|---|---|
| *(empty)* | {0,4,7} | `C` |
| `min` | {0,3,7} | `Cmin` |
| `5` | {0,7} | `C5` |
| `7` | {0,4,7,10} | `C7` |
| `maj7` | {0,4,7,11} | `Cmaj7` |
| `min7` | {0,3,7,10} | `Cmin7` |

Tier 2, optional, in this order: `dim` {0,3,6}, `sus4` {0,5,7}, `sus2` {0,2,7}, `aug` {0,4,8}, `6` {0,4,7,9}, `min7b5` {0,3,6,10}, `dim7` {0,3,6,9}.

`add9`, `min6`, and the clip-mode extra shapes (`maj9`, `9`, `7(#9)`, …) are **not** in this set. They show as notes.

**Match.** Try each present pitch class as root, **bass first, then upward**. Rotate the 12-bit mask, require an **exact** match, first match wins.

- A reading with the bass as root always wins: `C E G A` is **`C6`** with Tier 2, and **`Amin7/C`** with Tier 1 only (bass C is not a Tier 1 template; the one other match is A as `min7`).
- `aug` and `dim7` take the bass as root, with **no slash** (the set is symmetric; bass-first makes `E G# C` → `Eaug`, not `Caug/E`).
- No bass-root match, and **two or more** matches on other roots: show the **notes**, no symbol. Example: `D G C` with Tier 2 is both `Csus2` and `Gsus4` from non-bass roots → notes, NOTE `D3x3`. An inversion is **not** ambiguous by itself (`E G C` is `C/E`). Two equal templates are.
- No match at all: notes only. A cluster `C C# D`, a major third `C E`, a hexachord that is not an exact template, an octave double `C4 C5`.
- **Never `?`.**

**Slash.** `Root+suffix/Bass` when the root is not the bass, and only then. The bass is always a pitch class of the set (it is the lowest sounding note). No octave in the symbol. `E G C` → `C/E`. `A# C D# F#` → `Cmin7b5/A#`. Spread voicings and octave doubles reduce to pitch classes; if the bass is the root, there is no slash (`C3 G3 C4 E4` → `C`).

A non-chord-tone bass (clip-mode’s `C/D` for D under C E G, or a five-class set) is **not** produced: the match is exact on the full mask, so a foreign bass adds a bit and the template misses. The asked string `Cmin7b5/G#` is **not** an output of these rules: `G# C D# F#` is **`G#7`** (Tier 1, bass-root). The third inversion of `Cmin7b5` is `Cmin7b5/A#`. The longest **produced** slash in this template list is `A#min7b5/G#`. Widths for `Cmin7b5/G#` are still given below, as asked.

**Single note.** The note only (`C3`). No symbol.

**Drone.** Name only what actually sounds. Stored-but-silent notes are drawn in `C_DIM` on the roll, and (if they fit) in `C_DIM` after the held-line velocity.

**Deterministic, for live play.** Bass first, then upward, exact match, first hit. The bass is the lowest bar on the roll and the lowest pitch the player hears; naming from that is the expected live reading, and it is stable under inversion for `aug` and `dim7`. When two non-bass templates fit equally, we show the notes rather than pick a winner. No history, no “preferred key”, no scale. The same stored set with the same transpose always yields the same label.

**Do not call `ck_chord_detect` / `ck_chord_format` for this.** That detector (`smk_chord_detect.h`, used from `main_fm1.c` **1773–1774** for clip mode) uses `m` not `min`, hides a slash when the bass is a core tone (`E G C` → `C`, not `C/E`), scores a single winner for `C E G A`, allows an omitted fifth, rejects dyads (so no `C5`), and has 33 types. Including the header in `ui.c` would also **duplicate** its tables (they are `static` in a header already compiled into `main_fm1.c`). SEQ needs the smaller exact-match namer in this section.

**FONT_M fold (existing behaviour, not a new rule).** `glyph()` (`gfx.c` **162–163**) folds `a–z` to `A–Z` when the face has no lowercase (`FONT_M` is 32–95). The written name is still `Cmin7`, `C3x3`, `Eaug`. The NOTE cell at FONT_M **paints** `CMIN7`, `C3X3`, `EAUG`. FONT_S and FONT_XS have lowercase and paint the written form. Extending FONT_M with lowercase would cost far more than Elite’s 248 B (FONT_M’s packed data is already about 2854 B from `gen_font.py`). Do not extend the font. The mockups paint the folded form at FONT_M so 1× matches the device.

---

### 9.3 Placement on the 240×240 B view

Nothing is removed. The symbol is **added** at two sites the bounded fix already touches. No new strip, no roll-tag widget (a third site would cost flash the Elite does not have).

**NOTE cell** (held step, 54 px night panel, `ui.c` **8378–8381**; panel is `i*60+3` by 54, `render_diagnosis.py` knob_strip). The **symbol**, in FONT_M, then S, then XS, then dropping `/Bass` and retrying those fonts. Never cut a name. Unrecognised, or an octave double of a single pitch class (`C4 C5`): compact `C3x3` / `C4x2` (bass with octave, `put_note`, plus the **total** sounding count). Mono stays `C3`. A recognised chord with a doubled root still gets the symbol (`C3 G3 C4 E4` → `C`, not `C3x4`). The `"x"` is a count, not a sharp and not a transpose; `+` is still avoided (augmented / transpose). Compact at FONT_M paints `C3X3`.

**Held line** (`ui.c` **5645–5659**): `STEP n  <symbol>  <notes low to high>  V###`, plus the existing `TIE` / `ACC` / `SLIDE` if those flags are set (nothing removed). Font is tried B, then S, then XS (more notes need the smaller face). If XS still does not fit 232 px (x=4), the notes become `C3x4` (same compact) and **the symbol stays**. If there is no symbol, the line is `STEP n  <notes>  V###`. Names are `put_note` in low-to-high (stored) order, after the effective transpose.

**Widths, HOST MEASURED from the packed Barlow advances** (`tools/gen_font.py`, FONTSET=barlow, the tables `text_w()` / `tw()` use on the device). PIL TTF in the mockups can differ by a few pixels; **these packed figures are the ones FMB should use**. FONT_M widths are for the folded (uppercase) glyphs the device paints.

Longest asked string, `Cmin7b5/G#` (not a matcher output; see §9.2):

| Face | Device packed | PIL TTF (mockups) | Fits 54 px NOTE? | Fits 232 px held? |
|---|---:|---:|---|---|
| FONT_M | **119** | 115 | no | yes (but NOTE uses M only for the cell) |
| FONT_S | **75** | 74 | no | yes |
| FONT_XS | **61** | 60 | **no** | yes |
| FONT_B | **79** | 76 | no | yes |

None of M/S/XS fit 54 px. After dropping `/G#`, `Cmin7b5` is **82 / 51 / 42** at M / S / XS: FONT_S fits. This is why the fallback **must** drop `/Bass` and must not clip.

Longest **produced** slash, `A#min7b5/G#`: **135 / 85 / 70 / 90** at M / S / XS / B. After the drop, `A#min7b5` is **98 / 61 / 51**: FONT_XS (51) fits 54. Frame N-18.

Other NOTE-cell figures FMB needs (device packed):

| Written | M | S | XS | NOTE cell does |
|---|---:|---:|---:|---|
| `C` | 12 | 8 | 6 | FONT_M |
| `Cmin` (paints `CMIN`) | 46 | 30 | 24 | FONT_M |
| `C5` / `C7` / `C6` / `C3` | 24 | 15 | 12 | FONT_M |
| `C/E` | 34 | 22 | 17 | FONT_M |
| `Cmaj7` | **63** | 41 | 33 | FONT_S (M is 9 px over) |
| `Cmin7` | **58** | 37 | 30 | FONT_S (M is 4 px over) |
| `Cmin7b5` | 82 | **51** | 42 | FONT_S, after `/Bass` drop if slashed |
| `Amin7/C` | 81 | **51** | 42 | FONT_S |
| `C3x3` (paints `C3X3`) | 48 | 29 | 24 | FONT_M |
| `G#7` | 39 | 25 | 20 | FONT_M |
| `A#min7b5` | 98 | 61 | **51** | FONT_XS |

Held line, 232 px, device packed: `STEP 1  C  C3 E3 G3  V100` is 155 at B (fits). `STEP 1  Cmin7  C3 D#3 G3 A#3  V100` is 225 at B (fits). `STEP 1  Cmaj7  C3 E3 G3 B3 D4 F4  V100` overflows B (245) and S (234), fits XS (184) — the compact-notes fallback is not needed until a longer symbol plus six sharp names (e.g. `STEP 64  G#min7b5  G#2 C3 D#3 F#3 A#3 B3  V127` is 240 at XS, one px over; then notes become `G#2x6` and the symbol stays).

---

### 9.4 Mockups

All **PROPOSAL**. Native 240×240 at 1×, nearest-neighbour 3×. Same strips (20+69+69+82), Barlow roles, RGB565-safe night + AMBER, Direction B chrome, as the earlier B frames. Labels checked at 1×: every NOTE cell and every held line in this set is readable; FONT_XS on N-14 and N-18 is tight but complete (no clipped name); 3× confirms. FONT_M paints the folded form (`EAUG`, `C3X6`, `D3X3`).

Recorded note sets are taken from Main Builder’s host frames at `/workspace/seq-chord-001/evidence/before/` (`measurements.txt`) and from the SR / B frames in this folder. Sets with no recorded example are labelled **EXAMPLE SET**.

| Frame (1×) | 3× | Set | Kind | Source | Symbol | NOTE (font, painted) | Held line |
|---|---|---|---|---|---|---|---|
| `frames/1x/N-01-held-maj.png` | `frames/3x/N-01-held-maj.png` | 48,52,55 = C3 E3 G3 | recorded | `before-selected-chord.png`, `before-triad.png`; same set as SR-01 / B-03 | `C` | `C` FONT_M | `STEP 1  C  C3 E3 G3  V100` FONT_B |
| `frames/1x/N-02-held-maj7.png` | `frames/3x/…` | 48,52,55,59 | recorded | `before-tetrad.png` | `Cmaj7` | `Cmaj7` FONT_S | FONT_B, all four names |
| `frames/1x/N-03-held-hex.png` | 3× | 48,52,55,59,62,65 | recorded | `before-hexachord.png`; SR-04 / B-04 | *(notes)* | `C3x6` FONT_M, paints `C3X6` | six names, FONT_B |
| `frames/1x/N-04-held-mono.png` | 3× | 48 = C3 | recorded | `before-mono-control.png` | *(none)* | `C3` FONT_M | `STEP 1  C3  V100` |
| `frames/1x/N-05-held-dyad.png` | 3× | 48,52 | recorded | `before-dyad.png` | *(notes)* | `C3x2` paints `C3X2` | `C3 E3` |
| `frames/1x/N-06-inv-C-over-E.png` | 3× | 52,55,60 = E3 G3 C4 | **EXAMPLE SET** | no recorded inversion at this SHA | `C/E` | `C/E` FONT_M | inversion, not ambiguous |
| `frames/1x/N-07-spread-maj.png` | 3× | 48,55,60,64 | **EXAMPLE SET** | no recorded spread | `C` | `C` FONT_M | doubles fold; bass is the root |
| `frames/1x/N-08-CEGA-tier2.png` | 3× | 48,52,55,57 | **EXAMPLE SET** | no recorded C E G A | `C6` | `C6` FONT_M | Tier 2 on (recommended default) |
| `frames/1x/N-09-CEGA-tier1.png` | 3× | same | **EXAMPLE SET** | same set, Tier 1 only | `Amin7/C` | `Amin7/C` FONT_S | bass is not a T1 template; one other match |
| `frames/1x/N-10-cluster.png` | 3× | 48,49,50 | **EXAMPLE SET** | no recorded cluster | *(notes)* | `C3x3` paints `C3X3` | never `?` |
| `frames/1x/N-11-power-C5.png` | 3× | 48,55 | **EXAMPLE SET** | no recorded C G | `C5` | `C5` FONT_M | Tier 1 dyad |
| `frames/1x/N-12-min7.png` | 3× | 48,51,55,58 | **EXAMPLE SET** | “C Eb G Bb” in | `Cmin7` | `Cmin7` FONT_S | notes `C3 D#3 G3 A#3` (`put_note`) |
| `frames/1x/N-13-oct-double.png` | 3× | 60,72 | **EXAMPLE SET** | no recorded C4 C5 | *(notes)* | `C4x2` paints `C4X2` | |
| `frames/1x/N-14-min7b5-slash.png` | 3× | 46,48,51,54 | **EXAMPLE SET** | 3rd inversion | `Cmin7b5/A#` | `Cmin7b5` FONT_S (`/A#` dropped) | FONT_XS, full names |
| `frames/1x/N-15-drone-overflow.png` | 3× | stored 48,52,55,59,62,65; sounding 55,59,62,65 | **EXAMPLE SET**, steal **INFERRED** | `drone_string.h` **124–156**, not a device frame | `G7` (sounding only) | `G7` FONT_M | `G3 B3 D4 F4`; `C3 E3` in `C_DIM`. Header `DRONE B` |
| `frames/1x/N-16-aug-bass.png` | 3× | 52,56,60 | **EXAMPLE SET** | aug takes the bass | `Eaug` | paints `EAUG` FONT_M | no slash |
| `frames/1x/N-17-sus-ambiguous.png` | 3× | 50,55,60 | **EXAMPLE SET** | two non-bass matches | *(notes)* | `D3x3` paints `D3X3` | `D3 G3 C4` |
| `frames/1x/N-18-longest-slash.png` | 3× | 44,46,49,52 | **EXAMPLE SET** | longest produced slash | `A#min7b5/G#` | `A#min7b5` FONT_XS | FONT_XS |

Sheets: `sheet-notation.png` (1× frames, RECORDED / EXAMPLE SET tags), `sheet-notation-3x.png`.

---

### 9.5 Label budget — HOST MEASURED, and a check of the 90–140 B figure

**The Tier 1 HOST ESTIMATE of 90–140 B is not realistic. This section does not change the naming rules to chase it.**

A complete namer (exact-match loop, format `Root+suffix[/Bass]`, `put_s` / `NOTE_N` left as externs already in `ui.c`) was compiled with the **same JieLi pi32v2 clang 4.0.1** the app uses, at `-Os -ffunction-sections -fdata-sections -fno-builtin -mcpu=r3`, with `__attribute__((minsize, noinline))` like `UI_DRAW` (`ui.c` **5179**). Source and a reproduce script: `host-size/seq_chord_name.c`, `host-size/measure.sh`, `host-size/measurements.txt`. Unlinked object sizes (HOST MEASURED). LTO on a final image can move this by a few tens of bytes; it will not cut it in half.

| Item | Flash | Notes |
|---|---|---|
| Tier 1 **code** | **254 B** (0xfe) | `.text.seq_chord_symbol`. 248 B at `-Oz`. |
| Tier 1 masks | 12 B | 6 × `uint16` |
| Tier 1 suffixes | 19 B | `"\0min\0" "5\0" "7\0" "maj7\0min7"` (adjacent string literals; never `\05` or `\07`, which are octal escapes) plus the final NUL |
| **Tier 1 total** | **285 B** (279 B at `-Oz`) | **Above 90–140 B.** Code alone is already 254 B. |
| Tier 2 incremental | **+46 B** | +14 B masks, +32 B suffixes; **code unchanged**. ≈ 6.6 B/entry. The **6–10 B per Tier 2 entry estimate is realistic** (data only). |
| New user-facing strings beyond those suffixes | 0 | Compact `"x"` may already be paid by the bounded fix (2 B, §8). |
| Pool (`X0X_POOL`) | **0** | Cache is a small BSS buffer (`char[16]` plus a few bytes of keys: step, n, mask), not a pool object. Do not declare it in the motion pool. |
| `put_s`, `NOTE_N` / `X0X_NOTE_PACK` | 0 new | Already in `ui.c`. |

**Versus Elite 248 B and the 80–200 B display fix.** Tier 1 at **285 B already exceeds** the Elite’s 248 B flash headroom, **before** the bounded fix. Desk range for both together is 365–485 B, well over 248 (and over Pro’s 256). **Do not stack this on Firmware Main Builder’s display-only fix without a flash decision.** FMB must measure at link. Preset work and Juno/Drone engine-page units already share this headroom. Remove no feature. If something must wait, land the bounded fix (option 1) first — it closes faults 1–5 without this namer — and keep chord notation as a follow-on that needs measured space.

**CPU.** Name the chord **once on a held-step or onset change**, and **cache** it. Never per column of the roll, never in the sequencer ISR, never during playback of every step. A transpose change or a chord write invalidates the cache. The matcher over at most 6 sounding notes and at most 12 roots is cheap at that rate; it is not cheap if it is called 16 times per band per blit. This is display-only: **no playback path, no engine, no stream, no pattern-layout change.**

---

### 9.6 Measurable acceptance

Host harness: `python3 chord_name.py` (23 rows, 0 failures at this writing). Firmware tests should assert the same outputs, on the sounding MIDI after `+ bp->transpose - 24`.

**Invariants, every row:** the namer does not rewrite MIDI values; the store is untouched; playback is untouched; a symbol, if any, contains no `?`; 0 wrong symbols.

| # | Input (sounding MIDI, after transpose) | `put_note` notes | Tier | Expected symbol | Expected NOTE | Pitches / playback |
|---|---|---|---|---|---|---|
| 1 | C3 E3 G3 (48,52,55) | C3 E3 G3 | 1+2 | `C` | `C` | unchanged. Recorded. |
| 2 | E3 G3 C4 (52,55,60) | E3 G3 C4 | 1+2 | `C/E` | `C/E` | unchanged. Inversion, not ambiguous. |
| 3 | C3 Eb3 G3 Bb3 (48,51,55,58) | **C3 D#3 G3 A#3** | 1+2 | `Cmin7` | `Cmin7` (FONT_S) | unchanged. Flats in a prompt are input aliases; output is sharps. |
| 4 | C3 G3 C4 E4 (48,55,60,64) | C3 G3 C4 E4 | 1+2 | `C` | `C` | unchanged. Spread / doubles, not ambiguous. |
| 5 | C3 G3 (48,55) | C3 G3 | 1+2 | `C5` | `C5` | unchanged. |
| 6 | C3 E3 G3 A3 (48,52,55,57) | C3 E3 G3 A3 | **2** | `C6` | `C6` | unchanged. Bass-root wins. |
| 7 | same | C3 E3 G3 A3 | **1 only** | `Amin7/C` | `Amin7/C` (FONT_S) | unchanged. |
| 8 | C3 C#3 D3 (48,49,50) | C3 C#3 D3 | 1+2 | *(notes)* | `C3x3` | unchanged. Never `?`. |
| 9 | C4 C5 (60,72) | C4 C5 | 1+2 | *(notes)* | `C4x2` | unchanged. |
| 10 | C3 (48) | C3 | 1+2 | *(none)* | `C3` | unchanged. Mono. |
| 11 | C3 E3 G3 B3 (48,52,55,59) | C3 E3 G3 B3 | 1+2 | `Cmaj7` | `Cmaj7` (FONT_S) | unchanged. Recorded tetrad. |
| 12 | C3 E3 G3 B3 D4 F4 (hex) | those six | 1+2 | *(notes)* | `C3x6` | unchanged. Recorded hexachord. |
| 13 | E3 G#3 C4 (52,56,60) | E3 G#3 C4 | 1+2 | `Eaug` | `Eaug` (paints `EAUG`) | unchanged. Bass is root, no slash. |
| 14 | A#2 C3 D#3 F#3 (46,48,51,54) | A#2 C3 D#3 F#3 | 1+2 | `Cmin7b5/A#` | `Cmin7b5` (drop `/A#`, FONT_S) | unchanged. |
| 15 | G#2 C3 D#3 F#3 (44,48,51,54) | G#2 C3 D#3 F#3 | 1+2 | **`G#7`** (not `Cmin7b5/G#`) | `G#7` | unchanged. |
| 16 | G#2 A#2 C#3 E3 | G#2 A#2 C#3 E3 | 1+2 | `A#min7b5/G#` | `A#min7b5` (FONT_XS) | unchanged. |
| 17 | D3 G3 C4 (50,55,60) | D3 G3 C4 | 1+2 | *(notes)* | `D3x3` | unchanged. Two non-bass matches. |
| 18 | C3 E3 (48,52) | C3 E3 | 1+2 | *(notes)* | `C3x2` | unchanged. Recorded dyad. |
| 19 | stored C3 E3 G3, **`bp->transpose = 36`** | **C4 E4 G4** | 1+2 | `C` | `C` | **store still C3 E3 G3**. Sounding is +12. Playback already does this. The namer does not write the store. |

Pass: 0 wrong symbols, every ambiguous or unmatched set shows notes (no `?`), every inversion that has a single exact reading shows the slash, `aug` / `dim7` never slash, Drone names the sounding set only, and a before/after of the store and of the engine voice list is byte-identical.

---

### 9.7 Findings for Firmware Main Builder

1. **Budget.** Treat 90–140 B as withdrawn as a planning number. Host-measured Tier 1 is **285 B** (254 code + 31 data) on the JieLi toolchain. That **alone** is over Elite’s 248 B. Do not stack it on the 80–200 B display fix without a flash decision. Measure at link. Tier 2’s **+46 B** (≈ 6.6 B/entry, code unchanged) matches the 6–10 B/entry estimate; take it only if Tier 1 has a home.
2. **NOTE cell, 54 px, use `tw()` on the packed font.** `Cmaj7` is 63 px and `Cmin7` is 58 px at FONT_M — both must fall to FONT_S. `Cmin7b5/G#` is 119/75/61: **never fits**, even at XS; drop `/Bass`. After the drop, `Cmin7b5` fits FONT_S (51). `A#min7b5` needs FONT_XS (51).
3. **FONT_M paints uppercase.** `Cmin7` → `CMIN7`, `C3x3` → `C3X3`, `Eaug` → `EAUG` (`gfx.c` 162–163). Do not add lowercase glyphs to FONT_M.
4. **`put_note` is sharps only.** `Cmin7` notes are `C3 D#3 G3 A#3`, and a slashed bass is `A#` not `Bb`. Same spelling as today’s held line.
5. **Do not reuse `ck_chord_detect` as-is** (wrong names, wrong slash, wrong ambiguity, no `C5`, table duplication). A SEQ-specific exact-match namer, cached once per held-step or onset change.
6. **`Cmin7b5/G#` is `G#7`** under these rules. The longest produced slash is `A#min7b5/G#`.
7. **Drone dim** is display-only and the cold-start steal (lowest two dropped) is **INFERRED**. Confirm on a device before treating N-15’s `G7` as observed. The rule stands either way: name what sounds, dim what is stored and silent.
8. **Coordinate** with the already-agreed NOTE compact `C3x3` (not `C3+2`) and with the FONT_B → S → XS held line. This section only adds the symbol and the fallback that drops `/Bass`.
9. Please send **real-framebuffer before/after** frames when the namer lands, as with faults 1–5. The host frames here are PROPOSAL, PIL TTF, not the packed glyphs.

10. The note-dump fallback (`C3x3`, notes only) is replaced by the **§10 add-on**. Exact §9 names do not change.

---

### 9.8 Errata (independent review of §9)

**Source:** Extra High review of this file at sha `a6ab7d1a15d091645f18bb45bed0057c3a2170e89d2107209dc88583f29e6466` (`/workspace/smk-ux/threads/seq-piano-roll-chord-reply-1.md`), 2026-11-13. These corrections **supersede** the named sentences in §§9.1–9.7. The exact-match names in §9.2 and the original 23-row harness are unchanged. Host checks: T20–T37, clamp, Drone, TIE and cache rows in `chord_name.py` (`python3 chord_name.py`: 41 §9 rows + errata helpers, 0 failures at this writing).

**F1. Drone input (HIGH).** Supersedes §9.1 “The namer uses only the sounding four” and §9.7 item 7’s “The rule stands either way: name what sounds.” Use the sounding set **only if a read-only voice-state source exists**. Otherwise **name the stored set and show no dim marks**. Do not assume the highest four survive. N-15’s `G7` and its dim `C3 E3` are **INFERRED** and stay an EXAMPLE SET until that source is verified. A cache keyed only by step and transpose is not enough (F5). Helpers: `drone_input`, `drone_dim_pitches`.

**F2. Ambiguity takes precedence over “first match wins.”** This is already the §9.2 rule; the implementation must follow this split and must **not** return the first non-bass candidate when two match:

- Bass-root match: that name, immediately.
- Exactly one non-bass match: slash.
- Two or more non-bass matches: **notes**, not the first candidate.

Example: `E3 G3 A3 C4` with Tier 2 is both `C6/E` and `Amin7/E` → notes, NOTE `E3x4` (T24). The §10 add-on shows both readings instead of that dump; §9 itself still shows notes.

**F3. Tier 1 and Tier 2 can produce different labels**, not merely more names. `C3 E3 G3 A3` is `C6` with Tier 2 and `Amin7/C` with Tier 1 only. The inversion `E3 G3 A3 C4` is **notes** with Tier 2 (T24) and **`Amin7/E`** with Tier 1 only (T25). Tests must cover both configurations.

**F4.** “Stable under inversion” for `aug` and `dim7` (§9.2) means **deterministic**, not that the symbol is identical: `C3 D#3 F#3 A3` → `Cdim7`; `D#3 F#3 A3 C4` → `D#dim7` (T30), no slash.

**F5. Cache invalidation on every input change.** Beyond held-step or onset, transpose, and chord writes (§9.5): pattern switch (including the same selected step number), part switch, selecting a TIE, editing the onset of a selected TIE, Juno/Drone (engine) switch, and — if F1’s voice-state source exists — a change in that voice set. Helper: `label_cache_key`. Never serve a stale symbol.

**F6. Clamp to 127 can merge notes.** Stored MIDI 103,107,110 (a G-major *shape*, `G7 B7 D8`) at `bp->transpose=48` clamps to 127,127,127. The pitch-class mask is a single G. It **must not** name a major triad. §9 compact `xN` counts **events in the clamped list**, so NOTE is `G9x3`, symbol none (row `F6-clamp-merge`). Three events, one sounding pitch.

**F7.** Unlinked 285 B + 46 B is still not a fit-to-flash proof. Only an exact-source **linked** build is acceptance against Elite’s 248 B. Unchanged from §9.5 / §9.7.

**F8. NOTE-cell clipping.** Dropping `/Bass` is kept; the held line keeps the full slash (T33: symbol `Cmin7b5/D#`, NOTE `Cmin7b5`, held line contains the slash). Fallback labels at 51 px have **3 px spare** (`Cmin7b5` FONT_S, `A#min7b5` FONT_XS). The harness asserts every NOTE `tw() ≤ 54`. A real-framebuffer clip check is still required.

**F9. Octal escape in the suffix example.** The published `"\0min\05\07\0maj7\0min7"` is unsafe as literal C: `\05` and `\07` are octal, not NUL + `5` + `7`. The host probe (`host-size/seq_chord_name.c`) already uses adjacent literals `"\0min\0" "5\0" "7\0" "maj7\0min7"`. The table in §9.5 is corrected to that form. The 19 B count is unchanged.

**T20–T37** (T36 is a TIE / orphan / onset-edit helper, not a single-row input) are in the harness, with the clamp, Drone, cache and clip rows. Across all 13 templates and 12 roots, the overlapping-mask families are exactly sus2/sus4, 6/min7, aug and dim7 — matching the review’s independent enumeration.

The §10 add-on follows these corrections: same input contract, same T1/T2 exact names, both readings on true amb, clamp is a single pitch, cache key as F5.

---

## 10. Looking over ambiguous and unnamed sets

**SUPERSEDED by §10r** (2026-11-13, America/Chicago). Reason: interval forms were used as names. The owner’s correction, relayed by the Orchestrator: “Interval forms like C(3) or C(b2 2) must NOT replace the actual notes. The owner wants to see the real notes, compact names (Cmin7, or C/E when the bass and note set justify it), and the actual-note fallback when a chord is truly uncertain. No forced names.” Named extensions such as Cadd9 and Cmaj11 stay whenever the set matches exactly. The Optimizer budget is unchanged. The text below is the withdrawn §10 proposal, kept for the record. The current rule is §10r.

**Date:** 2026-11-13, America/Chicago (written in Asia/Vladivostok).
**Baseline:** `94197fc839b00aff47b98ca02f3c6396e4a6eabf`.
**This is a design proposal**, not a firmware patch, not observed on a device. It is an **add-on** to §9 (as corrected by §9.8). It does not start a new document and does not change Firmware Main Builder’s exact-match path. **Pitches and playback never change.** No gesture remaps, no timed animation, no auto-cycling. Nothing is removed: `C3x3` stays as a last-resort overflow, and every §9 unique name stays the same.

The owner’s complaint is the §9 fallback: an ambiguous or unrecognised set shows **notes only**, and the NOTE cell shows **`C3x3`**. This section answers whether “ambiguous” is actually rare, designs four options that always show a useful honest name, measures them, and recommends one.

Host-side: `chord_name.py` (`readings`, `note_cell_10`, `held_line_10`, extended tests), `render_ambiguous.py`, `frames/1x/AMB-*.png` (native 240×240) and `frames/3x/AMB-*.png` (nearest-neighbour 3×), `sheet-ambiguous.png`, `host-size/seq_chord_label.c`.

The input contract is §9.8 F1: the **sounding** set only if a read-only voice-state source exists; otherwise the **stored** set after the effective transpose, and **no dim marks**. N-15’s `G7` stays INFERRED.

---

### 10.1 How rare is “ambiguous”?

Every pitch-class set of 2–6 notes, with every member as bass, was enumerated with `chord_name.py` (`enumerate_sets`). There are **1,023 bass-fixed** shapes (bass at C, the other classes chosen from the remaining 11). ×12 transpositions = **12,276** (set, bass) pairs. Kind is transposition-invariant. Locked in the harness.

| Table | Bass-root | Slash | Truly ambiguous | Unrecognised | Total bass-fixed |
|---|---:|---:|---:|---:|---:|
| Tier 1 only | 6 (0.6%) | 14 (1.4%) | **0 (0%)** | **1,003 (98.0%)** | 1,023 |
| Tier 1+2 | 13 (1.3%) | 16 (1.6%) | **3 (0.3%)** | **991 (96.9%)** | 1,023 |
| Tier 1+2+TIER3 | 25 | 61 | **3** (unchanged) | 934 | 1,023 |

**Truly ambiguous** means: no bass-root exact match, and two or more other roots match (§9.8 F2). All (set, bass) pairs: 0 ambiguous under Tier 1; **36** under Tier 1+2 (3 shapes × 12). The three bass-fixed shapes, and only these:

| Shape from bass C | The two exact readings | Musical family |
|---|---|---|
| {0,5,10} | `Fsus4/C`, `A#sus2/C` | Quartal / sus2–sus4. N-17 `D G C` is this shape. |
| {0,2,5,9} | `Dmin7/C`, `F6/C` | 6 / relative min7, 5th in the bass. |
| {0,3,5,8} | `Fmin7/C`, `G#6/C` | The minor twin. |

Adding TIER3 does **not** create new ambiguities (same three shapes). Of 2- and 3-note sets under Tier 1+2, 51 are unrecognised and only 1 is ambiguous (the quartal). Under Tier 1, 58 unrecognised and 0 ambiguous.

**Musically common unrecognised sets** (owner’s list), bass C, Tier 1+2:

| Set | Intervals from C | §9 (T1+2) | §10 recommended |
|---|---|---|---|
| add9 | {0,2,4,7} | notes | `Cadd9` |
| add2 | {0,2,4} | notes | `Cadd2` |
| min9 | {0,2,3,7,10} | notes | `Cmin9` |
| 9 | {0,2,4,7,10} | notes | `C9` |
| 11 | {0,2,4,5,7,10} | notes | `C11` |
| quartal 3 | {0,5,10} | **amb** `Fsus4/C` + `A#sus2/C` (notes) | both readings (F2 add-on) |
| 7sus4 | {0,5,7,10} | notes | `C7sus4` |
| 6/9 | {0,2,4,7,9} | notes | `C69` (not `C6/9`: a digit after `/` is not a bass, but 69 avoids any slash-form lookalike) |
| min(maj7) | {0,3,7,11} | notes | `Cminmaj7` |
| cluster | {0,1,2} | notes, `C3x3` | `C(b2 2)` |
| min6 | {0,3,7,9} | already `Amin7b5/C` (T2 slash) | unchanged §9 |
| recorded dyad C E | {0,4} | `C3x2` | `C(3)` |
| recorded hex C E G B D F | {0,2,4,5,7,11} | `C3x6` | `Cmaj11` |

**Answer:** yes, true ambiguity is rare (0 shapes in Tier 1, 3 in Tier 1+2, 0.3%). The real problem is the **unrecognised** sets — 97–98% of bass-fixed shapes, including every common extension on the owner’s list except min6 (already a T2 slash). A UI that only “looks over” the three dual readings still leaves `C3x3` on add9, 9, 11, 7sus4, clusters, dyads and the recorded hexachord.

A second, softer dual exists when a **bass-root** name also has a different-quality rotation: `C6` and `Amin7/C`, `Cmin7` and `D#6/C`, `Csus2` and `Gsus4/C`, `Csus4` and `Fsus2/C`. These are **not** F2-ambiguous (bass-root wins, §9 name unchanged). Option A and the recommendation still show the other quality in `C_DIM` on the held line — the candidate (a) example was `'C6'` with dim `'Amin7/C'`. `aug` and `dim7` rotations are the **same** quality and are not shown (symmetric, §9 no slash, §9.8 F4).

---

### 10.2 Four options, and the recommendation

Every option keeps every §9 unique exact name (bass-root or a single slash). Device widths use packed Barlow advances, not PIL. FONT_M folds `a–z` to `A–Z` (`gfx.c` 162–163) and has **0-advance** for `(` `)` and no usable lowercase `b`. Any label containing parentheses or a flat degree (`b2`, `b3`, `b7`) **starts at FONT_S**, never FONT_M (otherwise `C(b2 2)` would paint as `CB2 2`, which looks like a note). The NOTE cell is 54 px: FONT_M → S → XS, then drop `/Bass`, then a registered short (`minmaj7` → `minM7`) or a `+N` interval trim. Never a mid-glyph cut. The held line is FONT_B → S → XS, 232 px; overflow drops notes to compact, then drops the notes; a second reading is **never** dropped. No cycling, no gesture.

**TIER3** is an exact-match table in the **same** algorithm as Tier 1/2, used by B and Rec only. Twelve entries, bass-first, then a single slash. Data only if the existing loop bound is extended: **82 B** HOST MEASURED (24 B masks + 58 B suffixes, ≈ 6.8 B/entry). It does not add ambiguities.

| Suffix | Intervals | Shown as (root C) | NOTE (54 px) |
|---|---|---|---|
| `add2` | {0,2,4} | `Cadd2` | FONT_S (63 at M) |
| `add9` | {0,2,4,7} | `Cadd9` | FONT_S |
| `min+9` | {0,2,3,7} | `Cmin+9` (compact min(add9)) | FONT_S |
| `7sus4` | {0,5,7,10} | `C7sus4` | FONT_S |
| `9` | {0,2,4,7,10} | `C9` | FONT_M |
| `min9` | {0,2,3,7,10} | `Cmin9` | FONT_S |
| `maj9` | {0,2,4,7,11} | `Cmaj9` | FONT_S |
| `11` | {0,2,4,5,7,10} | `C11` | FONT_M |
| `min11` | {0,2,3,5,7,10} | `Cmin11` | FONT_S |
| `maj11` | {0,2,4,5,7,11} | `Cmaj11` | FONT_S (recorded hex) |
| `69` | {0,2,4,7,9} | `C69` | FONT_M |
| `minmaj7` | {0,3,7,11} | `Cminmaj7` | FONT_XS (51). `G#minmaj7` is 59 at XS: NOTE short `G#minM7`; held line keeps `G#minmaj7`. |

**Interval shorthand**, used when no exact name remains: bass-rooted within-octave degrees, sharps for the tritone and the aug 5th (`put_note` is sharps only): `b2 2 b3 3 4 #4 5 #5 6 b7 7`. Written `C(3)`, `C(b2 2)`, `C(2 3 5)`. Not compound 9/11 (so the hex under option C is `C(2 3 4 5 7)`, 52 px at FONT_XS, no trim). If XS still overflows, keep as many degrees as fit and end `+N` (`C(b3 3 +2)`), never clip. One pitch class at two or more **distinct** MIDI values is `Coct` (octave double `C4 C5`). One **distinct** MIDI after clamp (F6, `127,127,127`) is **mono**: NOTE `G9`, not `Goct` and not a triad.

**Nearest-tag** (optional, expensive): a unique pure extension from the bass, `C+F#`. Two equally near templates (C7+D# ties Cmin7+E) are **not** guessed — interval instead.

| | NOTE cell (54 px) | Held line | FONT_M fold | Solves | Gap |
|---|---|---|---|---|---|
| **A — Best + second** | §9 unique, or first upward on true amb (`Gsus4/D` FONT_S). Dual bass-root (`C6`) is unchanged. Unrecognised: still `C3x3`. | Primary in `C_AMB`, second exact different-quality reading in `C_DIM` (the `'C6'` + dim `'Amin7/C'` example). Both names kept on overflow. | Same as §9 for exact names. | The 3 true-amb shapes, and the soft duals (`C6`/`Amin7`). | Unrecognised still dumps. Fails “no note dump for ≤3 notes” (cluster, dyad). |
| **B — Named extensions** | TIER3 exact (`Cadd9`, `Cmaj11`, `C(3)` for a dyad, `C(b2 2)` for a cluster). True amb is **first upward only** (`Gsus4/D`), no second. | One name + notes. | `Cadd9` is FONT_S (lowercase paints). Interval starts at FONT_S. `Coct` at FONT_M paints `COCT`. | The unrecognised common chords, dyads, clusters, the recorded hex. | No second reading on amb (fails “every ambiguous set shows a second reading”). `C6` has no dim `Amin7/C`. |
| **C — Intervals** | §9 exact kept. Everything else is interval (`C(2 3 5)` for add9, `D(4 b7)` for the quartal, `C(2 3 4 5 7)` for the hex). Starts at FONT_S. | One shorthand + notes. | Never FONT_M for `(…)` or flats. | Always a name; bass-centric “overview” of an amb set. | Weaker than `Cadd9` / `Cmaj11`. No second chord reading. A 6-note list may trim. |
| **D — Marker** | Like A, plus a `:` (existing FONT_M glyph, 7 px) when a second reading exists: `C6:` (31 px FONT_M), `Gsus4/D:` (45 px FONT_XS; 55 at S, just over). | Same as A: primary + dim second. | `C6:` at FONT_M is fine (`:` exists). | Marks “another reading exists” on the knob, 3 amb shapes + duals. | Same dump gap as A. The colon at FONT_XS is smaller than R’s unmarked FONT_S `Gsus4/D`. |
| **Rec (recommended) — B’s names + A’s second** | B’s name (TIER3 / nearest / interval). On amb or a soft dual, the first upward or the bass-root, **no colon** (the second is on the held line). Clamp-merge: `G9`. | B’s name in `C_AMB`, A’s second in `C_DIM` when it exists, then the notes. | Same FONT_S rule for intervals and flats. | Meets all four acceptance tests. | Nearest-tag is optional (see budget). |

**Recommend Rec.** A and D only help the rare duals and still dump `C3x3` on the common unnamed chords — that is the owner’s problem. C always names, but `C(2 3 5)` is weaker than `Cadd9`, and it has no second reading. B names the unrecognised sets well but stays silent on a second exact reading. Rec is B’s names plus A’s second: `Cadd9` and `Cmaj11` where they are exact, `C(b2 2)` and `C(3)` where they are not, `Gsus4/D` with dim `Csus2/D` on true amb, `C6` with dim `Amin7/C` on the famous dual, `G9` after a clamp-merge. No cycling. The primary is deterministic (bass-root, else first upward). The second is visible, so it does not “imply a single answer” for an ambiguous set (§9.8 F2, add-on).

**Measurable acceptance** (style R, host harness, 0 failures):

- No compact dump (`C3xN`) for any set of 3 notes or fewer (full bass-fixed scan of k=2 and k=3, plus the test table).
- 0 wrong names on the test set. Every §9 unique symbol is identical under every style (add-on, not a fork).
- Every ambiguous set, and every soft dual, shows a second reading (A, D, R).
- Legible at 1× (native 240×240 mockups; interval and flats at FONT_S; every NOTE `tw() ≤ 54`; 3 px spare on the 51 px fallbacks, §9.8 F8).
- Pitches and the store are unchanged on every row. No `?`.
- F1: without voice state, the stored six is `Cmaj11` and dim marks are empty; with confirmed `G3 B3 D4 F4`, the label is `G7`.
- F6: clamp-merge NOTE is `G9`, not a major triad.

---

### 10.3 Mockups

Native 240×240 at 1×, plus 3× nearest-neighbour zoom, in the same visual language as N-01..N-18 (Direction B, Barlow roles, `C_AMB` held line, `C_DIM` second reading). Recorded pitches are reused where the earlier frames had them; others are tagged EXAMPLE SET. One comparison sheet, five hard sets across A, B, C, D and Rec.

| Set | MIDI | Source | §9 now | A | B | C | D | **Rec** |
|---|---|---|---|---|---|---|---|---|
| 01 dyad | 48,52 | **RECORDED** `before-dyad.png`, N-05 | `C3x2` | `C3x2` | `C(3)` | `C(3)` | `C3x2` | **`C(3)`** FONT_S |
| 02 cluster | 48,49,50 | **EXAMPLE SET**, N-10 | `C3x3` | `C3x3` | `C(b2 2)` | `C(b2 2)` | `C3x3` | **`C(b2 2)`** FONT_S |
| 03 sus-amb | 50,55,60 | **EXAMPLE SET**, N-17 | `D3x3` | `Gsus4/D` + dim `Csus2/D` | `Gsus4/D` only | `D(4 b7)` | `Gsus4/D:` + dim `Csus2/D` | **`Gsus4/D`** + dim **`Csus2/D`**, NOTE FONT_S, held FONT_XS |
| 04 add9 | 48,50,52,55 | **EXAMPLE SET** | `C3x4` | `C3x4` | `Cadd9` | `C(2 3 5)` | `C3x4` | **`Cadd9`** FONT_S |
| 05 hex | 48,52,55,59,62,65 | **RECORDED** `before-hexachord.png`, N-03 | `C3x6` | `C3x6` | `Cmaj11` | `C(2 3 4 5 7)` | `C3x6` | **`Cmaj11`** FONT_S, held FONT_XS |

Frames: `frames/1x/AMB-{A,B,C,D,R}-{01-dyad,02-cluster,03-susamb,04-add9,05-hex}.png` and the 3× copies. Sheet: `sheet-ambiguous.png`, `sheet-ambiguous-3x.png`. The dim second reading is `C_DIM` (sampled: RGB 132,146,148 on the R sus-amb held line, x≈86–125). Host frames are PROPOSAL, PIL TTF, not the packed glyphs.

At 1× the Rec NOTE labels `C(3)`, `C(b2 2)`, `Gsus4/D`, `Cadd9` and `Cmaj11` are readable. Dual held lines land at FONT_XS (195–215 of 232 px) and the dim second is quieter by design (existing dim). D’s `Gsus4/D:` at FONT_XS is tighter than Rec’s unmarked FONT_S `Gsus4/D`.

---

### 10.4 Label budget — HOST MEASURED, on top of the 285 B Tier 1

Same JieLi pi32v2 clang 4.0.1, `-Os -ffunction-sections -fdata-sections -fno-builtin -mcpu=r3`, `__attribute__((minsize, noinline))`. Source: `host-size/seq_chord_label.c`, `host-size/measure_label.sh`. Unlinked object sizes. `put_s`, `NOTE_N` and `seq_chord_symbol` are extern (already paid). LTO on a final image can move this by a few tens of bytes; it will not cut it in half. A **linked** build is still the flash acceptance (§9.8 F7).

| Piece | Flash (−Os) | −Oz | Notes |
|---|---:|---:|---|
| `seq_interval` | **200 B** (0xc8) | 196 B (0xc4) | Needed for ≤3-note unnamed sets. Degree table 27 B + `"oct"` 4 B. |
| `seq_nearest` | **398 B** (0x18e) | 378 B (0x17a) | Unique bass-root extension. Optional; see ship set. |
| `seq_put_reading` + `seq_put_second` | **72 B** (50+22) | 72 B | Second-reading format (A, D, Rec). |
| TIER3 data | **82 B** (24+58) | 82 B | 12 entries, ≈ 6.8 B/entry. **0 text** if the existing matcher loop bound grows, as with Tier 2. |
| Hook in the existing matcher, to stash a second different-quality fit and, for soft duals, not `goto format` before collecting it | **16–40 B HOST ESTIMATE** | — | Cheaper than a second walk. Not in the probe object. |
| Pool (`X0X_POOL`) | **0** | 0 | Cache is BSS: primary + second ≈ 32 B, plus the F5 key. Do not declare it in the motion pool. |

| Option | Add-on flash, HOST MEASURED (and estimate) | On top of T1 285 B (and T2 +46 B if taken) |
|---|---|---|
| A | 72 B + hook 16–40 ≈ **88–112 B** | Second format only. Unrecognised still dumps. |
| B | 200+398+82+27+4 = **711 B** | T3+interval+nearest, no second. |
| C | 200+27+4 = **231 B** | Interval only. |
| D | same as A, plus a 1-char `:` | Same dump gap. |
| Rec, **full** (with nearest) | 200+398+72+82+27+4 = **783 B** + hook |  |
| Rec, **ship** (recommended) | 200+72+82+27+4 = **385 B** + hook 16–40 ≈ **401–425 B** | T3 + interval + second. Nearest **deferred**: leftovers use interval (`C(3 #4 5)` instead of `C+F#`). Acceptance still holds. Saving 398 B is worth the slightly less tight leftover tag. |

Tier 1 at 285 B already exceeds Elite’s 248 B, before this add-on, before the 80–200 B display fix, and before T2’s +46 B. **Do not stack §10 without the same flash decision already in front of the owner.** Firmware Main Builder must measure at link. If something must wait after that decision: land the bounded display fix first, then §9 exact names (T1, then T2), then this ship set (T3 data is cheap; interval and the second-reading format are the code). Nearest is a follow-on.

**CPU.** Name the chord **once on a held-step or onset change**, and **cache** it, including the second reading. Invalidate on **every input change** (§9.8 F5): pattern, part, step, onset index, notes, transpose, engine, and voice state if present. Never per column, never in the sequencer ISR, never during playback of every step. T3 is the same loop with a longer table (at most 6 notes, at most 24 templates). Interval is a 6-note walk. That rate is cheap; 16 calls per band per blit is not. Display-only: **no playback path, no engine, no stream, no pattern-layout change.**

---

### 10.5 Extended test table

`python3 chord_name.py`: **41 §9 rows** (original 23 + T20–T35, T37, F6-clamp) + T36 / Drone / cache / clip helpers, and **60 §10 rows**, **0 failures** at this writing.

§9 exact names are unchanged. Selected §10 Rec rows that replace a dump, plus the review and F1/F6 cases:

| ID | Input (sounding, after transpose) | Tier | §9 | Rec primary | Rec second | Rec NOTE | Pitches |
|---|---|---|---|---|---|---|---|
| r-cluster | C3 C#3 D3 | 1+2 | `C3x3` | `C(b2 2)` | — | `C(b2 2)` FONT_S | unchanged |
| r-dyad | C3 E3 (recorded) | 1+2 | `C3x2` | `C(3)` | — | `C(3)` FONT_S | unchanged |
| r-hex | C3 E3 G3 B3 D4 F4 (recorded) | 1+2 | `C3x6` | `Cmaj11` | — | `Cmaj11` FONT_S | unchanged |
| r-sus-amb / T22-family | D3 G3 C4 | 1+2 | `D3x3` | `Gsus4/D` | `Csus2/D` | `Gsus4/D` FONT_S | unchanged |
| r-T24 | E3 G3 A3 C4 | **2** | `E3x4` | `Amin7/E` | `C6/E` | `Amin7/E` FONT_S | unchanged. First upward is A, not C. |
| r-T25 | same | **1 only** | `Amin7/E` | `Amin7/E` | — | `Amin7/E` | unchanged. T1 and T2 differ. |
| r-T35 | D3 C4 E4 G4 | 1+2 | `D3x4` | `Cadd9/D` | — | `Cadd9/D` FONT_S | unchanged. TIER3 slash. |
| r-add9 | C3 D3 E3 G3 | 1+2 | `C3x4` | `Cadd9` | — | `Cadd9` FONT_S | unchanged |
| r-CEGA-t2 | C3 E3 G3 A3 | 2 | `C6` | `C6` | `Amin7/C` | `C6` | unchanged. Soft dual. |
| r-F6-clamp | stored 103,107,110, trans 48 | 1+2 | `G9x3`, no symbol | (mono) | — | **`G9`** | **store still 103,107,110**. Sounding 127,127,127. Not a major. |
| r-drone-stored | six stored, **no** voice state | 1+2 | `C3x6` | `Cmaj11` | — | `Cmaj11` | F1: stored set, no dim. Not a guessed `G7`. |
| r-drone-voice | confirmed G3 B3 D4 F4 | 1+2 | `G7` | `G7` | — | `G7` | F1: only with a voice-state source. |

---

### 10.6 Findings for Firmware Main Builder

1. **Do not change `seq_chord_symbol`’s 0/1 return or any §9 exact name.** The current work is not blocked. This section is an add-on: a new `seq_chord_label` (or a wrapper) that calls the existing symbol first.
2. **Hook**, cheaper than a second walk (16–40 B ESTIMATE): stash the first two **different-quality** fits. For true amb, the existing loop already walks all roots and returns 0 when `nfit != 1` — also keep the second fit. For soft duals (`C6` + `Amin7/C`), do not `goto format` on a bass-root hit before collecting a different-quality other. `aug` / `dim7` are the same quality: no second.
3. **TIER3** is 12 more rows in the same table (+82 B data, 0 text if the loop bound is a constant). Names `add9`, `add2`, `min+9`, `7sus4`, `9`, `min9`, `maj9`, `11`, `min11`, `maj11`, `69`, `minmaj7`. Does not add ambiguities. Take it only with the flash decision.
4. **Interval formatter** is HOST MEASURED **200 B** + 27 B degrees + 4 B `"oct"`. Required for the “no dump for ≤3 notes” bar. Start at FONT_S when the string has `(` or a flat degree. Trim with `+N`, never clip. Within-octave degrees, not compound 9/11.
5. **Nearest-tag** is HOST MEASURED **398 B**. **Defer it.** Leftovers use interval. Acceptance still holds. If flash appears later, a unique bass-root pure extension becomes `C+F#`; ties stay interval.
6. **Ship set (Rec):** T3 82 + interval 231 + second 72 + hook 16–40 ≈ **401–425 B** on top of T1 285 (and T2 +46 if taken). Still over Elite’s 248, which T1 already exceeds. Measure at **link**. Pool 0.
7. **FONT_M.** Never paint parentheses or flat `b2` at FONT_M. `Cmin7` at FONT_S paints lowercase (FONT_S has `a–z`). `Coct` at FONT_M paints `COCT`. NOTE short for an overflowing `minmaj7` is `minM7` (held line keeps the official name). `69` not `6/9`. Sharps only: a second reading is `D#6/C`, not `Eb6/C`.
8. **Input and cache (§9.8).** F1: sounding set only with a verified read-only voice-state source; otherwise the stored set, no dim. F5: invalidate on pattern, part, step, onset index, notes, transpose, engine, and voice state. F6: after clamp, one distinct MIDI is the note (`G9`), not a triad. F8: `tw() ≤ 54` on every NOTE; 3 px spare on the 51 px fallbacks; full slash on the held line; still want a real-framebuffer clip check. TIE views read the onset (`onset_notes_of`); an orphan TIE invents nothing.
9. **`C3x3` is not removed.** It is the last-resort overflow, and it remains the §9 fallback. Rec just does not use it for ≤3 notes.
10. Please send **real-framebuffer before/after** frames when the add-on lands. The host frames here are PROPOSAL, PIL TTF, not the packed glyphs. Coordinate the NOTE and held-line label with this Rec spec (`Cadd9`, `C(3)`, `Gsus4/D` + dim `Csus2/D`, not `C3x3`, not `C3+2`).


## 10r. Actual notes, exact names only

**Date:** 2026-11-13, America/Chicago.
**Baseline:** `94197fc839b00aff47b98ca02f3c6396e4a6eabf`.
**This is a design proposal**, not a firmware patch, not observed on a device. It is an **add-on** to §9 (as corrected by §9.8) and **supersedes §10**. It does not change Firmware Main Builder’s exact-match path. **Pitches and playback never change.** The Optimizer budget stays as it is.

Host-side: `chord_name.py` (style `R` = §10r; `python3 chord_name.py`: 41 §9 rows + 63 §10/§10r rows, 0 failures), `render_ambiguous.py`, `frames/1x/AMB-{9,R}-*.png`, `sheet-ambiguous.png`.

The input contract is still §9.8 F1: the **sounding** set only if a read-only voice-state source exists; otherwise the **stored** set after the effective transpose (`MIDI' = stored + bp->transpose − 24`, clamped 0..127), and **no dim marks**. N-15’s `G7` stays INFERRED.

### 10r.1 The display rule

1. The **actual notes are always visible**. The held line already has `<notes>` (`put_note`, sharps only). Overflow may compact them to `C3xN`; they are not dropped to keep a second reading (there is no second reading).
2. The symbol is a **compact name only for an exact** Tier 1 or Tier 2 match, plus a Tier 3 named extension (`Cadd9`, `Cmaj11`, and the other exact TIER3 suffixes in §10’s table) **when the pitch-class set matches exactly**. TIER3 stays in the rule regardless of how often it occurs in real data; the rate is reported in §10r.2.
3. Use a **slash only when the root differs from the bass** and the set matches exactly (a single non-bass exact hit). An inversion with one exact reading is `C/E`, not notes.
4. Otherwise show **no name, just the notes**. No interval form (`C(3)`, `C(b2 2)`), no `?`, no nearest-tag (`C+F#`), no first-upward guess on a truly ambiguous set (`Gsus4/D` is not shown for `D G C`).
5. A bass-root exact name wins and is shown alone (`C6`, `Cmin7`, `Csus2`). A different-quality rotation is not added as a dim second.

True ambiguity remains the §9.8 F2 case: no bass-root exact match, and two or more other roots match. Under §10r that set is unnamed; the notes carry it.

### 10r.2 Validation of the 1,023-shape and ~97% claims

The 1,023 figure is a **theoretical denominator**: every bass-fixed pitch-class set of 2–6 classes (bass at C, the others chosen from the remaining 11), enumerated by `enumerate_sets`. ×12 transpositions = 12,276 (set, bass) pairs. Kind is transposition-invariant. The harness still locks the same counts: Tier 1 = 6 bass-root / 14 slash / **0 amb** / **1,003 unrecognised (98.0%)**; Tier 1+2 = 13 / 16 / **3 (0.3%)** / **991 (96.9%)**; Tier 1+2+TIER3 = 25 / 61 / **3** / 934. The three bass-fixed amb shapes are unchanged: `{0,5,10}`, `{0,2,5,9}`, `{0,3,5,8}`.

**That uniform mask count is not representative of played music.** It weights a rare cluster the same as a close-position major triad, ignores octave, voicing and doubling (`C3 E3 G3` and `C4 E4 G4` are one shape), and ignores that factory defaults are empty. The ~97% “unrecognised” rate is a statement about that enumeration, not about what users record.

**Real data actually found** (nothing was invented). Effective transpose `bp->transpose − 24` and the clamp to 127 were applied. Distinctness is by the sounding MIDI list (octaves and voicing), not by pitch-class set alone.

| Source | What it contains | Distinct sounding step chords (n ≥ 2) |
|---|---|---|
| Default patterns | `pattern_init` (`pattern.c` 13–43): every bass step is `G_REST`, `transpose = 24`, no `held_chord` lanes. | **0** sounding chords. Default step *note* bytes are the TB-3PO root, but the gate is rest, so they do not sound and are not named. |
| Factory banks / breaks | `assets/factory_banks/chicago.json` (81) and `miami.json` (30) are **sound parameter** presets (name + params). No note, chord, or pattern fields. `factory_preset()` returns 0 at this SHA. Factory breaks are audio samples. | **0** |
| Project / `.bin` files on the box | `tests/fixtures/v1_store.bin` and `v0102_store.bin` (26,908 B each): **0** bytes equal to `HELD_CHORD_TARGET` (`0xfc`, `held_chord.h`). `project-v3-sound10.json` / `project-v6-sound10.json` are synthetic (`contains_user_data: false`), no step chords. No owner-exported project with held-chord lanes was found under `/workspace`. | **0** live-project chords |
| FMB host frames at `94197fc8` | `/workspace/seq-chord-001/evidence/before/measurements.txt`, plus `seq_chord_baseline_extra.c` for the extra frames. Host render of real `ui.c` `draw_303` / `draw_main` / `draw_knobs`. Every listed frame used `bp->transpose = 24` (sounding = stored; no clamp). | **5** distinct chord voicings, plus 3 distinct monos |

The five recorded chord voicings, and the three recorded monos, after the effective transpose:

| Sounding MIDI | Notes | Frames (transpose 24) | T1+T2 | §10r (T1+T2+T3) |
|---|---|---|---|---|
| 48,52 | C3 E3 | `before-dyad.png`, N-05 | notes | **notes** (`C3 E3` in the NOTE cell) |
| 48,52,55 | C3 E3 G3 | `before-triad.png`, `before-selected-chord.png`, `before-playing-chord.png`, `before-sustained-triad.png`, `before-chord-and-mono.png` (the chord step), `before-slide-triad.png`, `before-write-triad.png`, `before-selected-triad-extra.png`, N-01 | `C` | `C` |
| 48,52,55,59 | C3 E3 G3 B3 | `before-tetrad.png`, N-02 | `Cmaj7` | `Cmaj7` |
| 48,52,55,59,62,65 | C3 E3 G3 B3 D4 F4 | `before-hexachord.png`, N-03 | notes | **`Cmaj11`** (exact TIER3) |
| 60,64,67 | C4 E4 G4 | `before-page2-chord.png`, `before-page2-selected-chord.png`, `before-page2-playing-chord.png` | `C` | `C` (same quality as the C3 triad; **different octave / voicing**, counted separately) |
| 48 | C3 | `before-mono-control.png`, `before-chicago-mono.png`, `before-miami-mono.png`, N-04 | mono `C3` | mono `C3` |
| 52 | E3 | `before-mono-E3.png`; also step 1 of `before-slide-triad.png` | mono `E3` | mono `E3` |
| 57 | A3 | `before-chord-and-mono.png` (the mono step), `before-page1-with-page2-chord.png` | mono `A3` | mono `A3` |

**Rates on the 5 distinct recorded step chords** (monos excluded):

| Matcher | Named | Notes fallback | Ambiguous | Exact TIER3 |
|---|---:|---:|---:|---:|
| Tier 1+2 | **3 / 5 (60%)** | **2 / 5 (40%)** — dyad, hex | **0 / 5** | 0 |
| Tier 1+2+TIER3 (§10r) | **4 / 5 (80%)** | **1 / 5 (20%)** — the dyad | **0 / 5** | **1 / 5 (20%)** — the recorded hex is `Cmaj11`. No other TIER3 suffix (`add9`, `9`, `11`, …) appears in this corpus. |

N-01..N-18: five frames are RECORDED (N-01..N-05, including the mono). The other thirteen are **EXAMPLE SET** (N-15 is INFERRED). Those examples were not folded into the rates above. Among them, only N-17 (`D3 G3 C4`) is a true amb; under §10r it shows the notes, not `Gsus4/D`.

**What this does not show.** There is no owner-exported live project and no factory pattern corpus of held chords on the box. The only recorded step chords are five FMB host-programmed voicings, four of them C-rooted close-position stacks (one of those is the same triad an octave up). That is enough to reject “~97% unrecognised / 3 of 1,023 truly ambiguous” as a description of *recorded* use, and not enough to estimate a user’s real mix. Smallest capture if more is needed: Firmware Main Builder logs `held_chord_get` onset MIDI, `bp->transpose`, pattern/part/step from the host sim; or the owner exports a project that already has held chords.

### 10r.3 NOTE-cell fallback order (54 px)

Device widths are packed Barlow advances, not PIL. Never a mid-glyph cut. Never `?`. FONT_M folds `a–z` to `A–Z` and must not paint a TIER3 name that needs lowercase (`Cadd9` starts at FONT_S).

1. Exact compact name (Tier 1, Tier 2, or exact TIER3), FONT_M → S → XS.
2. If the name has `/Bass`, drop `/Bass` and retry those fonts.
3. A registered short only when the official suffix still overflows XS (`minmaj7` → `minM7`). The held line keeps the official name.
4. Else the **actual notes** (`put_note`, low to high, spaces), FONT_M → S → XS. A recorded dyad fits FONT_M (`C3 E3`, 53 px). A cluster `C3 C#3 D3` fits FONT_XS (49 px). Not every 3- or 4-note list fits (`E3 G3 A3 C4` is 55 px at XS).
5. Else compact `C3xN` (last-resort overflow). `C3x3` is not removed; it is no longer the default unnamed NOTE.
6. One distinct MIDI after clamp (F6, `127,127,127`): NOTE is `G9`, not a triad and not `Goct`.

The held line is FONT_B → S → XS, 232 px: `STEP n  [<name>]  <notes>  V###` (+ TIE/ACC/SLIDE if present). If XS overflows, notes become compact; the notes field is not dropped.

### 10r.4 Label budget — HOST ESTIMATE, Optimizer unchanged

Unlinked object sizes already measured for the withdrawn §10 pieces (`host-size/seq_chord_label.c`, JieLi pi32v2 clang 4.0.1, `-Os`): `seq_interval` **200 B** + degree table 27 B + `"oct"` 4 B = **231 B**; second-reading format **72 B**; hook **16–40 B HOST ESTIMATE**; TIER3 data **82 B** HOST MEASURED (24 B masks + 58 B suffixes); nearest-tag **398 B** (already deferred). A linked build is still the flash acceptance (§9.8 F7).

§10 Rec ship was T3 + interval + second + hook ≈ **401–425 B** on top of Tier 1’s 285 B (and Tier 2’s +46 B if taken).

**§10r ship:** TIER3 data only, **82 B** HOST MEASURED (0 text if the existing matcher loop bound grows, as with Tier 2). Interval, nearest, and the second-reading format are **not shipped**.

**Byte delta vs withdrawn Rec ship:** **−319 to −343 B** (drop interval 231 + second 72 + hook 16–40). The add-on on top of T1 (+ T2 if taken) is **82 B**, not ~410 B. Tier 1 at 285 B still exceeds Elite’s 248 B; this add-on does not invent flash headroom. **Do not stack it without the same flash decision already in front of the owner.** Measure at **link**. Pool 0. The Optimizer budget is unchanged.

**CPU.** Name the chord once on a held-step or onset change, and cache it. Invalidate on every input change (§9.8 F5): pattern, part, step, onset index, notes, transpose, engine, and voice state if present. Never per column, never in the sequencer ISR, never during playback of every step. Display-only: no playback path, no engine, no stream, no pattern-layout change.

### 10r.5 Mockups

Native 240×240 at 1×, plus 3× nearest-neighbour zoom. Two columns: §9 (exact or `C3xN`) and §10r (exact T1–T3 or the notes). Sheet: `sheet-ambiguous.png`, `sheet-ambiguous-3x.png`. Frames: `frames/1x/AMB-{9,R}-{01-dyad,02-cluster,03-susamb,04-add9,05-hex}.png` and the 3× copies. The withdrawn A/B/C/D frames, which showed interval forms, have been removed so that none of the AMB set shows `C(3)` or `C(b2 2)`. Host frames are PROPOSAL, PIL TTF, not the packed glyphs.

| Set | MIDI | Source | §9 now | **§10r** |
|---|---|---|---|---|
| 01 dyad | 48,52 | **RECORDED** `before-dyad.png`, N-05 | `C3x2` | **`C3 E3`** FONT_M (no name, no `C(3)`) |
| 02 cluster | 48,49,50 | **EXAMPLE SET**, N-10 | `C3x3` | **`C3 C#3 D3`** FONT_XS (no `C(b2 2)`) |
| 03 sus-amb | 50,55,60 | **EXAMPLE SET**, N-17 | `D3x3` | **`D3 G3 C4`** FONT_S (no `Gsus4/D`) |
| 04 add9 | 48,50,52,55 | **EXAMPLE SET** | `C3x4` | **`Cadd9`** FONT_S (exact TIER3); notes stay on the held line |
| 05 hex | 48,52,55,59,62,65 | **RECORDED** `before-hexachord.png`, N-03 | `C3x6` | **`Cmaj11`** FONT_S (exact TIER3); held line keeps the six notes, FONT_XS |

### 10r.6 Measurable acceptance

`python3 chord_name.py`: 41 §9 rows + 63 §10/§10r rows, **0 failures**. §9 exact names are unchanged.

- **Real-data cases** (recorded MIDI, trans 24 unless noted): dyad `C3 E3` → notes; triad `C3 E3 G3` → `C`; tetrad → `Cmaj7`; hex → `Cmaj11`; page-2 `C4 E4 G4` → `C`; monos `C3` / `E3` / `A3`. Transpose +12 of the C3 triad → `C` on `C4 E4 G4` (store unchanged). F6 clamp-merge of stored 103,107,110 at `bp->transpose=48` → NOTE `G9`, not a major triad.
- **No wrong name** on any test row. No interval form, no `?`, no nearest-tag, no forced name on a true amb. TIER3 only when exact (`Cadd9`, `Cmaj11`, `Cadd9/D`).
- **Notes always present** on the held line (full names, or compact on overflow). When unnamed, the NOTE cell shows the actual notes if they fit 54 px, else compact.
- **Cache invalidation** on every input change (§9.8 F5), including pattern switch with the same step number. Helper: `label_cache_key`.
- **Fast-note timing check** (Firmware Main Builder, not measured in this design pass): no dropped or late notes across SEQ enter/exit and a label redraw, against the existing host `DRAW_TIME` / `PLAYBACK_HASH` baselines (`seq_chord_baseline_extra.c`, `measurements-extra.txt` hash `0x4c5a264a`). A linked before/after of the store and of the engine voice list stays byte-identical.

### 10r.7 Findings for Firmware Main Builder

1. **Do not change `seq_chord_symbol`’s 0/1 return or any §9 exact name.** The current work is not blocked. This is an add-on: extend the existing matcher with TIER3 and, on no unique exact hit, show the notes (NOTE fallback as in §10r.3).
2. **Do not implement** interval shorthand, nearest-tag, or a dim second reading. Those were the withdrawn §10 Rec path. True amb and leftovers are notes only.
3. **TIER3** is 12 more rows in the same table (+82 B data, 0 text if the loop bound is a constant). Names `add2`, `add9`, `min+9`, `7sus4`, `9`, `min9`, `maj9`, `11`, `min11`, `maj11`, `69`, `minmaj7`. It does not add ambiguities. Keep it even though only `Cmaj11` appears in the recorded corpus. Take it only with the flash decision.
4. **Ship set (§10r):** T3 **82 B** HOST MEASURED, on top of T1 285 (and T2 +46 if taken). **−319 to −343 B** versus the withdrawn Rec ship. Still over Elite’s 248 B, which T1 already exceeds. Measure at **link**. Pool 0. Optimizer budget unchanged.
5. **FONT_M.** TIER3 names with lowercase start at FONT_S. NOTE short for an overflowing `minmaj7` is `minM7`. `69` not `6/9`. Sharps only.
6. **Input and cache (§9.8).** F1, F5, F6, F8 still apply. TIE views read the onset (`onset_notes_of`); an orphan TIE invents nothing.
7. **`C3x3` is not removed.** It is the last-resort NOTE overflow only.
8. Please send **real-framebuffer before/after** frames when the add-on lands, and run the **fast-note timing check** (§10r.6). Coordinate the NOTE and held-line label with this §10r spec (`Cadd9` + notes, `Cmaj11` + notes, dyad `C3 E3`, amb `D3 G3 C4` — not `C(3)`, not `Gsus4/D`, not `C3x2` when the notes fit).
