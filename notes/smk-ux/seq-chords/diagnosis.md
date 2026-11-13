# Chord display diagnosis at `94197fc8`

**Date:** 2026-11-13, America/Chicago.
**Baseline:** `94197fc839b00aff47b98ca02f3c6396e4a6eabf`, read-only worktree `/workspace/smk-ux-src`.
**PR:** [gobbajogger/X0X37-for-Loopy-Pro#866](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/866) “Record held-key chords in sequencer steps,” head `lane/026-held-key-chords`. The PR is **CLOSED** and was not merged as itself (`mergeCommit` is null). Its implementation commit **`be48fcaed4e75f4ca9d6baf46cbfac1343e949bd`** (`feat: record held-key chords for polyphonic synth slots`, 2026-11-12) **is an ancestor of `94197fc8`** and is the held-chord work as it exists at this SHA. Other PR commits are docs and fixture tests (`ef5d1e0bc`, `456fd914d`, `80a16fb21`, `f34321c41`, `11bb95923`). This diagnosis inspects that work **at `94197fc8`**, not a separate checkout of the PR branch.

**Method.** Read-only source inspection. Frames in `frames/1x` and `frames/2x` are **SOURCE RECONSTRUCTION**: they reconstruct what the draw code produces from source. They are **never claimed as observed output** (no host-UI run, no device, no build). Reuses the held-chord facts already in `/workspace/smk-ux/source-map-94197fc8.md` §7.1 (`held_chord_write`, toast `CHORD NOT SAVED`; `draw_303` 5572–5666 described as a piano roll with a held-step line).

**Scope.** Display of recorded held-key chords on the SEQ view. Recording and playback are traced only as far as they feed stored data and the draw path. No firmware writes.

Guesses are marked **INFERRED**.

---

## 1. What is stored (needed to judge the draw)

Held chords (PR #866 / `be48fcaed`, at this SHA):

- Sparse extra notes live in the existing **motion lane pool** (`held_chord.h` 4–6, 8–13). Target `HELD_CHORD_TARGET = 0xfc`. **No pattern or song layout change.**
- The **primary** pitch, gate, accent, slide, velocity, and endpoint stay in `bstep_t` (`pattern.h` 23–33): `uint8_t note` (MIDI), `uint8_t flags` (`gate | BS_ACCENT | BS_SLIDE | endpoint << 4`, `BS_END_SHIFT` 4), `uint8_t velocity`. **Three bytes per step.** Velocity 0 means default 100 (`BASS_VELOCITY_DEFAULT`). Gate: `G_REST`, `G_NOTE`, `G_TIE`. Endpoint nibble: `0` = legacy gate; `1..15` = captured release fraction of **this playback slot** (`pattern.h` 24).
- Extra chord pitches (up to **`HELD_CHORD_NOTES - 1` = 5**) sit in one lane per extra index `i`, `val[step]` = MIDI note or `MOT_NONE` (`0xFF`). `held_chord_get` returns the root plus extras, **strictly increasing**; a decrease, duplicate, or `MOT_NONE` **stops** the list (`held_chord.h` 21–32, 29). Maximum stored notes: **6**.
- **Duration is per step, not per note.** One gate and one endpoint nibble on `bstep_t` cover every pitch of that step’s chord. There is no per-note gate, endpoint, or velocity in the chord lanes (lanes store pitch only). Ties (`G_TIE`) continue the sounding onset (`sequencer.c` 305, 386; `note_start_of` / `held_note_of`, `ui.c` 5556–5570).
- On **REC while playing**, a held chord that crosses a step boundary writes **TIE steps with the root only**: `bp->step[p].note = r->note`, `flags = G_TIE | 0xF0u`, and the sound lock on that step is cleared (`sequencer.c` 278–287). **Chord lanes are not written on those TIE steps.** Extras exist only on the **onset** step (`seq_bass_chord_set` at the onset, `sequencer.c` 1379, 1408; stopped path `bass_write_held_chord`, `ui.c` 3366–3367). A draw that called `held_chord_get` at a TIE’s own index would see no extras. The correct lookup for a tied step is `held_chord_get(..., note_start_of(bp, s), root)`.
- **Polyphony (engines), not assumed:** `seq_bass_poly` is true only when `original_bass[b].kind` is `>= 2` and `<= 3` (`engine.c` 1085–1088): **Drone = 2**, **Juno = 3** (`original_sound.h` 9–12). Chicago (0) and Miami (1) stay mono. **Juno** has **`JUNO6_VOICES 6`** (`dsp/juno6.h` 7). **Drone** has **`DRONE_STRING_VOICES 4`** (`dsp/drone_string.h` 32). The **store** can hold 6 notes on either engine. A five- or six-note Drone chord can be saved; the engine has only four voices. Recording is offered when `engine_bass_sound_bank(...) >= 2` (`ui.c` 3393).
- Refusal: more than 6 distinct held keys, or a full lane pool, toasts **`CHORD NOT SAVED`** (`ui.c` 3355, 3364; playing path `rec_refused == 3`, 4450). Nothing is committed.

`lane_t` is `NLANE` 160 × (`8` header bytes + `NSTEPS` 64 values) ≈ **72 bytes** each (`motion.h` 25–36). That pool is shared with motion and note-lock lanes.

---

## 2. The draw path

Juno and Drone occupy a 303 slot. SEQ uses the **same** `draw_303` as Chicago and Miami (`ui.c` 6888–6889: `else if (is_303()) draw_303(band, ui.view == V_GEN)`). There is **no** Juno/Drone branch and **no** `held_chord_get` in the draw function.

`draw_303` (`ui.c` 5572–5668), in order:

1. **Pitch window** (5581–5596): for each step, `note_start_of` then **`bp->step[start].note` only**. Extra lane pitches do not expand `lo`/`hi`. Floor of 14 semitones (`hi = lo + 14`).
2. **Octave rules** (5597–5603): horizontal `C_LINE` at GX, width 213, and an `FONT_XS` `C_DIM` name via `put_note`, only on pitches where `(n - root) % 12 == 0`.
3. **Sixteen columns** (5604–5641): `col_x(c) = GX + c * 13 + (c >> 2)` (5508–5509). Playhead: a tall `dim(part_col(), 2)` bar (5612–5613). For a non-rest with a real onset: **one** `night_mark` at `y = note_y(held_note_of(bp, s), lo, hi)`, height **5**, width `end ? (end * 12 + 14) / 15 : 12` (5615–5619). Colour: white if this is `ui.held_step`, else part colour if accent, else `dim(col, 9)`. TIE connectors and slide lines use that **same single y**, and the slide destination is `bp->step[nx].note` (5622–5638). P-lock notch on that one bar (5620–5621).
4. **`held_note_of`** (5566–5570) returns `bp->step[note_start_of(...)].note` — the **primary** stored pitch. It never reads chord lanes.
5. **AC / SL rows** (5640–5644): per-step flags, not per note. Correct for the data.
6. **Held or WRITE label** (5645–5659): `STEP n` / `WRITE n`, then **`put_note(..., held_note_of(...) + transpose)`** — **one** name — then TIE / ACC / SLIDE and `V` + one velocity, `FONT_B` at y 122. If not held: scale and seed (5660–5666). Scale names are the short TB-3PO table (`MIN`, `PHRY`, `HARM`, `MPEN`, `DOR`, `MAJ` in `tb3po.c` 16), so a C major setting is **`C MAJ`**, not “C MAJOR”. Seed is `SEED` plus four hex digits (`put_hex(..., seed & 0xFFFF, 4)`). Frames use example seed `00A3`.
7. **Held-step knobs** (`draw_knobs`, 8378–8381): the NOTE value is again **`put_note(held_note_of(...))` only**. GATE / ACCENT / SLIDE are per-step (matches the data).

`part_col()` is the **slot** colour (`PART_COL[ui.part]`, 4869), not an engine colour. Juno or Drone on 303B (`PART_303B`) is cyan `RGB(70,205,255)`; on 303A, green `RGB(130,240,90)`. With `X0X_BASS_PRESET_PREVIEW` **1** (`engine.h` 47), the SEQ/PART header chip is **`JUNO B`** / **`JUNO A`** by slot (`ui.c` 5429), not the shorter `part_name`. `part_name` is `JUNO` when the other 303 slot is not Juno (`bass_display_name`, `ui.c` 117) — that shorter form appears in the held-step **knob tag**, which is `part_name` + ` STEP ` + n (`ui.c` 8287–8288): **`JUNO STEP 1`**. The unheld SEQUENCE page tag is **`P1 SEQUENCE`** (`GR_PAT`, 8303–8305). SEQ 303 has three pages (SEQUENCE, PRESET, KEYS), so three page dots sit at the right of the tag (8356–8369). Unheld knobs are **LENGTH, RATE, DIRECTION, TRANSP** (`SEQN`, 954–958), not DIR / TRANSPOSE.

PR #866’s `ui.c` hunk (`be48fcaed`) added the include, `bass_write_held_chord`, clear-on-write/tap, `held_note_by` transposition of extras, generate/mutate clears, and the `CHORD NOT SAVED` toast. **It did not change `draw_303` or the NOTE knob.** At `94197fc8` that is still true.

---

## 3. Patterns

Five realistic patterns. All on **Juno** in the **303B** slot unless noted, pattern length 16, rate 1/16, transpose 24 (none), root C. MIDI: C3=48, E3=52, G3=55, B3=59, D4=62, F4=65, A3=57. Frames: `frames/1x/SR-0N-….png` and `2x/`. Each is labelled SOURCE RECONSTRUCTION.

### P1 — Sustained triad (`SR-01-sustained-triad`)

**Musical intent.** A C major triad held across four steps (PR #866 acceptance: “Held chords sustain across step boundaries”).

**Stored.** Step 0: `bstep_t` `{note=C3, flags=G_NOTE, velocity=100}`, lanes `i=0,1` at step 0 hold E3 and G3. Steps 1–3: `{note=C3, flags=G_TIE|0xF0, velocity=100}` (REC-while-playing form, `sequencer.c` 280–284). **No chord-lane values on steps 1–3.** Playing; playhead on step 2 (the third step).

**Draw.** Window from C3 only: `lo=46`, `hi=60`. One 5 px bar at C3 on step 0, width 12 (legacy endpoint 0). TIE connectors at **that same y** through step 3 (`ui.c` 5615–5629). Octave rule at each C. Scale/seed line: **`C MAJ`** and **`SEED 00A3`** (example seed). Footer `PRESS: ON / TAP: OFF / HOLD + KNOBS: EDIT` (`ui.c` 6796–6798).

**Faults.**

- **Hidden notes.** E3 and G3 are in the store and would sit inside this 14-st window, but `draw_303` never calls `held_chord_get`. **`ui.c` 5614–5619** (the only `night_mark` for pitch); **5572–5668** as a whole; contrast **`held_chord.h` 21–32**.
- **Wrong length for extras.** Playback holds the triad across the TIE run (`sequencer.c` 305, 386, 356–376: extras fired on the onset, gate follows the step). The roll shows a four-step length only for C3. Extras have **no** bar and **no** tie. Looking up lanes at the TIE index would also fail (they were never written there): **`sequencer.c` 278–287** plus **`ui.c` 5566–5570**.

### P2 — Overlapping chords (`SR-02-overlapping-chords`)

**Musical intent.** A C triad held two steps, then an E-minor triad (E3–G3–B3) that shares E and G — overlapping pitch, new onset.

**Stored.** Steps 0–1: C3–E3–G3 as in P1 (NOTE + TIE). Steps 2–3: onset `{note=E3, G_NOTE, 100}` + lanes G3, B3; step 3 TIE on E3 only.

**Draw.** Two single-pitch runs: C3 for two columns, then E3 for two. Looks like a two-note melody. Shared voices and the inner/upper notes are invisible.

**Faults.**

- **Hidden notes** in both chords, same site as P1: **`ui.c` 5614–5619**.
- **Overlap is not visible.** Successive onsets are drawn as monophonic pitch changes (`held_note_of` jumps from C3 to E3). A player cannot see that G3 and E3 continue, or that B3 entered. **`ui.c` 5566–5570, 5615**.
- **Slide, if used, would aim at the next root only** (`ui.c` 5635–5637: `ny = note_y(bp->step[nx].note, ...)`). Not used in this pattern’s flags, but the path is monophonic.

### P3 — Chord plus a single note, step held (`SR-03-chord-and-single`)

**Musical intent.** A C triad on step 1, a single A3 on step 8. The player holds step 1 to inspect it.

**Stored.** Step 0: C3–E3–G3 (`G_NOTE`). Step 7: `{note=A3, G_NOTE, 100}`, no extra lanes. `ui.held_step = 0`.

**Draw.** Two equal-looking 5 px marks, at C3 and A3. Held bar is **white** (`ui.c` 5616). Label: **`STEP 1  C3 V100`** (`5648–5658`) — E3 and G3 are omitted. Knob tag: **`JUNO STEP 1`** (`8287–8288`). NOTE knob in the strip: **`C3` only** (`8378–8381`).

**Faults.**

- **Hidden notes** on the triad column: **`ui.c` 5614–5619**. The single A3 is drawn correctly (it is the primary).
- **Wrong / truncated label.** The held-step line can name only one pitch: **`ui.c` 5645–5659**, especially **5651** (`put_note(..., held_note_of(...))`). The NOTE knob repeats that: **`ui.c` 8381**. A six-name string would also not fit `FONT_B` on 240 px — any bounded fix still has a width problem here (**INFERRED** layout limit).
- **Indistinguishable kinds.** A chord and a single note use the same mark language. Nothing on the roll says “three keys.”

### P4 — At the store and Juno voice limit (`SR-04-voice-limit`)

**Musical intent.** Six keys at once on Juno: C3–E3–G3–B3–D4–F4. Store limit **and** `JUNO6_VOICES` are 6, so the record is accepted (`held_chord.h` 39, `ui.c` 3355 does not fire).

**Stored.** Step 0: root C3, five lanes E3, G3, B3, D4, F4. `count = 6`.

**Draw.** Window from C3 only: `lo=46`, `hi=60` (5581–5596). F4 (MIDI 65) is **above** that window. Only a C3 bar is drawn.

**Faults.**

- **Hidden notes** (all five extras): **`ui.c` 5614–5619**.
- **Range ignores extras** — **`ui.c` 5581–5589**. Even a naive add of `night_mark` for extras would **clip D4/F4** (and maybe B3 at 59, which is just inside 60). This is a latent clip in the current window rule, already wrong as soon as extras are drawn. File:line of the window: **5581–5596**.
- **Drone, same store, four voices** (**INFERRED** as a sibling case, same draw path): a five-note Drone chord **saves** (store allows 6; `DRONE_STRING_VOICES` is 4). Display still shows one note. That is a display fault plus a store-vs-engine mismatch; this diagnosis does not change voice stealing.

### P5 — Beyond the store (`SR-05-beyond-limit`)

**Musical intent.** A seventh key on a Juno already holding the P4 hexachord.

**Stored.** Unchanged (still the six notes of P4). `bass_write_held_chord` hits `n==HELD_CHORD_NOTES` and **returns before write** (`ui.c` 3355–3356), or the playing path sets `rec_refused=3` (`sequencer.c` 1348–1349) and `say("CHORD NOT SAVED")` (`ui.c` 4450). Lane-pool exhaustion uses the same toast (`ui.c` 3364).

**Draw.** Full-header toast: part-colour bar, black centred `CHORD NOT SAVED`, 1300 ms, **no** chip, pattern, or clock (`ui.c` 5345–5369, early return). The roll under it is still P4 — one C3 bar.

**Faults.**

- The toast itself is the designed refusal; it is not a draw bug. The **underlying roll still hides** the six notes that *were* saved: **`ui.c` 5614–5619** again.
- **Wrong labels in the usual header** are replaced by the toast for 1.3 s, then the monophonic roll returns with no explanation of how many notes are stored.

---

## 4. Top faults (file:line)

| # | What is wrong | Site at `94197fc8` |
|---|---|---|
| 1 | **Hidden notes.** `draw_303` never reads chord lanes. Every extra pitch is omitted, on Juno and Drone (same function). | `ui.c` **5614–5619** (single `night_mark` at `held_note_of`); **5572–5668** (no `held_chord_get`). Data: `held_chord.h` **21–32**. |
| 2 | **Pitch window from primaries only.** Extras do not expand `lo`/`hi`. A wide chord’s upper notes would clip if drawn. | `ui.c` **5581–5596**. |
| 3 | **Wrong / truncated labels.** Held-step and WRITE lines, and the NOTE knob, name only the root. | `ui.c` **5645–5659** (esp. **5651**); **8378–8381**. |
| 4 | **Wrong length for extras on a sustained chord.** Ties and endpoints are per-step and drawn only at the root’s y. Extras are stored on the onset only; TIE steps do not carry lanes (`sequencer.c` **278–287**). | Draw: `ui.c` **5617–5629**, **5566–5570**. Store: `sequencer.c` **280–284**. |
| 5 | **Overlap and “chord vs single” are indistinguishable.** Successive chords read as a monophonic line. Slide preview uses the next **root** only. | `ui.c` **5615**, **5635–5637**, **5566–5570**. |

AC/SL rows are not a fault given per-step flags. Legacy width `12` when endpoint is 0 (`5617–5618`) is existing 303 convention, not chord-specific.

---

## 5. Frames

All **SOURCE RECONSTRUCTION**, not observed output. 240×240 at 1×; nearest-neighbour 2×. Sheets annotate the faults outside the panel.

| Pattern | 1× | 2× |
|---|---|---|
| P1 Sustained triad | `frames/1x/SR-01-sustained-triad.png` | `frames/2x/SR-01-sustained-triad.png` |
| P2 Overlapping chords | `frames/1x/SR-02-overlapping-chords.png` | `frames/2x/SR-02-overlapping-chords.png` |
| P3 Chord + single, held | `frames/1x/SR-03-chord-and-single.png` | `frames/2x/SR-03-chord-and-single.png` |
| P4 Six-note limit | `frames/1x/SR-04-voice-limit.png` | `frames/2x/SR-04-voice-limit.png` |
| P5 Seventh key refused | `frames/1x/SR-05-beyond-limit.png` | `frames/2x/SR-05-beyond-limit.png` |
| Annotated sheet | `sheet-diagnosis.png`, `sheet-diagnosis-2x.png` | |

Fonts: Barlow Semi Condensed TTF (T10 / XS12 / S15 / B15 / M23), same approach as `/workspace/smk-ux/render_mockups.py`. Not the packed Felucca bitmaps; advances may differ by a few pixels. Colours RGB565-safe, night + AMBER. 20+69+69+82 strips.

---

## 6. Bounded fix — functions a display-only patch would occupy

Firmware Main Builder is the **sole writer** of this display-only patch, on `lane/fmb-seq-chord-display` (off `94197fc8`, existing SEQ visuals only). Second Builder is cancelled. A `git fetch origin lane/fmb-seq-chord-display` on 2026-11-13 (America/Chicago) still returned `couldn't find remote ref`, so the function and line list remains **INFERRED**. The earlier name `lane/sb-seq-chord-display` is also absent.

The Juno/Drone appendix (`/workspace/smk-ux/directions/engines-juno-drone.md`, Writer ownership) points at the same SEQ chord areas: `draw_303` “5572–5918”, `held_chord_*` 3348–3493, `bass_write_held_chord` 3348, and piano-roll comments at 5557 and 5918. Cross-check at `94197fc8`:

- **`draw_303` is `ui.c` 5572–5668**, not through 5918. The appendix span overruns `draw_param_widget` (5671), `draw_part_sound` (5740) and `draw_break` (5836) — PARA and Break, not the SEQ roll. Those are not chord-display territory.
- The comment at **5556–5558** (appendix “5557”) sits on `note_start_of`: a TIE must not invent a pitch on the piano roll. The comment at **5918–5919** belongs to **`bass_content_bits16`** (5920–5938), “shared with the piano roll.” Its callers are HOME-lane glyphs (5957, 6043). It uses `note_start_of` and gates only, not extra pitches, so a stored chord does not change the bits. A display-only roll patch **leaves it alone**.
- **`held_chord_*` in `ui.c` 3348–3493** is the **write/edit** path, not the draw: `bass_write_held_chord` **3348–3370**, `bass_kbd` (3393 calls the writer), `bass_step_key` clear-on-create/tap (3430, 3441), and **`held_note_by` 3472–3503** (the function runs past the appendix’s 3493, through the preview at 3499–3502). `held_note_by` already calls `held_chord_get` at `ui.held_step` (**3484**), which misses extras if that index is a TIE. These sites are **not** in the display-only bound.

**Likely touch list (INFERRED):**

- **`draw_303`** (`ui.c` **5572–5668**) — the whole function: expand `lo`/`hi` with `held_chord_get` at `note_start_of` (5581–5596); draw each stored pitch (5614–5638), including TIE connectors at every pitch of the onset chord, not only `held_note_of`; extend the held-step / WRITE string (5645–5659) without inventing a new banner.
- A small helper beside **`held_note_of` / `note_start_of`** (`ui.c` **5556–5570**) that returns the onset chord (`held_chord_get` at the start step). Avoids looking up TIE indices, which have no lanes (`sequencer.c` 278–287).
- **`draw_knobs`** held-step NOTE value (`ui.c` **8378–8381**) — same helper, still the existing NOTE/GATE/ACCENT/SLIDE cells.
- **Not** (for a display-only bound): `bass_write_held_chord` / `held_chord_write` (3348–3370), `held_note_by` (3472–3503), `bass_content_bits16` (5920–5938), `seq_rec_bass`, streams 13/14/15, engine voice counts, Chicago/Miami monophonic paths, or the PARA/Break functions inside the appendix’s 5572–5918 span.

Overhaul options (Direction B chord/note view, A and C variants) should **build on** that bounded `draw_303` data path — same `held_chord_get` at onset, same per-step duration, same 16-column overview — and change mark language, not duplicate a second reader. Detail in `piano-roll.md`.

---

## 7. INFERRED log

- `lane/fmb-seq-chord-display` function and line list (branch not on the remote yet; fetch 2026-11-13 returned `couldn't find remote ref`).
- Held-step label width: a full chord name may not fit `FONT_B` at y 122.
- Drone five-note save vs four-voice engine is a store/engine fact; voice stealing was not simulated here.
- Exact Felucca pixel advances vs Barlow TTF in the reconstruction frames.
