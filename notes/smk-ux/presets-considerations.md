# DnBOX SMK-37 — preset design considerations (background)

**Date:** 2026-11-13, America/Chicago.
**Status:** BACKGROUND for the implementation spec at `/workspace/smk-ux/presets-spec.md`. This file is not the build brief. Main Builder implements the spec. BugBot checks the spec. This document records design reasoning, the Oct 25 apply/supersede review, and open owner options.

**Authority and citations.** Owner request to design factory presets for the 0.0.26 engines (String Machine drone and Juno-60-style with chorus), saving user presets, and importing presets for all engines — relayed by Dot, not an owner quote. Inventory at `94197fc8`, **as reported by Dot** (cite that way; not re-derived here): 81 Chicago, 30 Miami, 4 Juno and 4 Drone factory presets; no working user bank; import returns `-1`. Historical PR #276 remarks in that inventory are **not proof** and **not an owner quote** — see the spec’s Provenance section.

- Archive (still applies / superseded, item by item below): `git show 54bb3c496:docs/plans/2026-10-25-native-preset-workflow.md` (full SHA `54bb3c49696e1c5d4f9a8f95c0423a5a08fea0ce`), 467 lines, LX-REQ-033. Extracted read-only; no checkout into a working tree.
- Current interaction evidence: `/workspace/smk-ux/source-map-94197fc8.md` (SHA `94197fc839b00aff47b98ca02f3c6396e4a6eabf`).
- Current UX plan and decision record (2026-11-13 05:20 America/Chicago): `/workspace/smk-ux/plan-94197fc8.md`.
- Visual directions being drawn in `/workspace/smk-ux/directions/` (selected/playing/queued, sound vs step). At the time this was written that directory was not on the box. Do not fork a look; reference those directions by name.

**This pass did not** investigate firmware source, read flash layouts, write code, build, render audio, flash a device, commit, or push. Capacities and flash-reserve figures are not invented. Miami is never called a 303 (or 303B). Chicago is the 303 engine and has no velocity-to-cutoff. Printed legends stay PATCH/HOME, BT/SAVE, PARA/EDIT, NOTE REPEAT/MIX, 909/808/BREAK/Chicago/Miami. Remove nothing without the owner’s words.

Guesses are marked **INFERRED**.

---

## 1. Oct 25 2026 design — still applies vs superseded

Cited as the archive at `54bb3c496`. Compared with the source map and plan at `94197fc8`, and with Dot’s inventory (not re-derived).

### 1.1 Still applies (as principles; implement per the spec, not per Oct 25 capacities)

| Oct 25 item | Why it still applies |
|---|---|
| Do not reassign PRESETS (K4) from pattern cue | Source map §1.2, §2.2: K4 cues a pattern and ends a live chain. Decision record keeps existing controls. No remaps. |
| Parameter locks stay for per-step param variation; presets must not replace them | Map §4.4, plan preserve-list: hold-step + K5–K8, `P-LOCK STEP n`. |
| No silent save; existing dirty / autosave / explicit BT policy | Map §4.5; plan §2. Words stay `SAVES WHEN STOPPED`, `SAVE AT STOP`, `SAVED`. |
| Deferred flash; no flash erase during play; no allocation in the audio path | Map §4.5, §7; plan Constraints and CPU section. A flash erase would stall audio (`ui.c` 2222–2223, cited in the plan). |
| Overwrite needs an explicit confirm or an undo; never a silent replace | Already used for `CLEAR … IN THIS PATTERN?` and `LOAD SAVED PROJECT?` / `DISCARD UNSAVED EDITS` (map §4.5, §4.6). Digitakt “accidental loss” pattern in the plan’s user-reports section **APPLIES**. |
| ASSIGN while playing queues; show pending; PATCH can cancel | Map §8.1: `cancel_sound_request` (3939–3957). Oct 25 PENDING / WAITING FOR NOTE. Prefer existing pending strings already in the product over new copy. |
| Held notes keep the old snapshot until the next eligible attack; ties wait rather than breaking glide | Aligns with `live_boundary` / held-note continuity (map §8.6). Preserve `ui_live_key_owner` exactly. |
| Preview / assign / list navigation must not write steps or motion | Oct 25; consistent with PARA whites auditioning, not writing (map §4.4). |
| Unknown engine or schema rejects an import before any live mutation; explicit error; no silent relabel or fallback | Oct 25. Current import returns `-1` (Dot) — the principle stands; the spec defines the visible errors. |
| Old firmware rejects unknown newer streams (`NEWER DATA: NOT SAVED`, `NEWER DATA: LOAD BLOCKED`) | Map / plan; streams 13 / 14 / 15 are not to be casually changed (plan Constraints). |
| Malformed import preserves the live project, samples, and undo | Oct 25. |
| One assignment, capture, delete, or copy = one undo gesture; PATCH+SEQ REC / PLAY stay the undo / redo | Map §8.1, plan preserve-list. |
| No All Notes Off of other parts on a preset change | Oct 25. Panic paths (FX+SEQ, clip NOTE REPEAT) already do not clear the project (plan user reports). |
| CPU: measure PERF TEST (`FACTORY LOOP`, `ALL FIVE`, `WORST CASE`); no regression; test restores the project and does not write it | Plan CPU section. |
| Flash reserve is not shrunk without review; capacities are agreed before storage work; do not silently evict | Oct 25 principle. PR188 numbers from that document are **not** a 94197fc8 measurement (see superseded). Map §11: flash reserve unmeasured. This remains a **blocker for counts**. |
| Factory IDs / indices stay stable; embedded parameters in a project are authoritative; a later factory-bank update must not change old projects | Oct 25. Spec requires APPEND-ONLY after the existing Juno and Drone four. |
| Copy-on-write: never mutate a shared factory snapshot in place; an edit becomes a user copy | Oct 25. |
| KORG / “stock-equivalent” as a distribution path is legally and technically **unproven** | Oct 25. Treat import as **user files** with per-engine validation, not as a claim of stock equivalence. Strengthened by Dot’s inventory lead on historical PR #276 (not proof, not an owner quote) — details in the spec’s Provenance section. |
| Navigation itself must not write sound; SCALE moves a list, PATCH backs out / NO, SCALE confirms | Oct 25, aligned with map §2.2 and §8.1 (`LIST: BACK   QUESTION: NO`). |
| Chain is live-only and is not saved, on purpose | Not an Oct 25 heading, but map §4.5 and plan Constraints. Still applies. |
| Remove nothing without the owner’s words | Plan decision record; standing constraint. |
| De-click / no-glitch on assign is a testable requirement | Oct 25 proposed a 64-frame de-click. The **requirement** (no audible glitch) still applies. The 64-frame figure is **INFERRED / a proposal**, not a measured proof at this SHA. |

### 1.2 Superseded, or must be updated, at 94197fc8

| Oct 25 item | Why it is superseded or must not be copied blindly |
|---|---|
| “303-only first”; “alternate engine independent / not implemented” | Juno and Drone exist at this SHA (map §4.3, §6, §7.1, `BENGINE_N`). Miami is a real engine and is not a 303. The present request is factory roles for Juno and Drone, a working user bank, and import for **all** engines. |
| “No native named bank” at the Oct 25 inspected SHA (`662ce39e…`) | At `94197fc8`, Dot reports 81 Chicago, 30 Miami, 4 Juno, 4 Drone factory presets. Pattern bass-preset handles and a PARA SOUND path already exist (plan / map). There is **no working user bank**; import returns `-1` (Dot). |
| Proposed capacities: 8 shared snapshots, 16 step overrides, 264-byte sidecar, 24-byte records, “four original sounds” | Do **not** invent or reuse those counts as 94197fc8 budgets. Flash reserve is unmeasured (map §11). Agree capacities only after a measurement. |
| Proposed new `stream4` | Plan Constraints: streams already in use are `PROJ_STREAM` 13, `PROJ_STREAM_14` 14 (including per-pattern bass preset handles in `pattern_t.rsv[0..1]`), `PROJ_STREAM_CHORD` 15. Do not blindly add stream 4. The handle / inherit idea is partly realized in stream 14; any new object must version so old projects load unchanged and unknown newer data still reports `NEWER DATA`. |
| “Custom 01” and deferred user naming as out of scope | Saving user presets is now in scope. Naming and a working bank are design work; **capacity** is still blocked on flash measurement. |
| Storyboard names “303A · P03 SOUND”, “Round Saw”, engine 1 = native303, 16-pot schema | On-screen names are CHICAGO / MIAMI / JUNO / DRONE (map §2.1, plan Naming for Docs Keeper). Each 0.0.26 engine has its own PARA pages (map §7.1). Do not describe Miami as 303A on the screen. |
| PRESETS as a possible future entry, and a new remapped control for SOUND | K4 stays pattern cue. SOUND is entered by the **existing** PARA / SCALE-list / SOUND-page path that Dot inventories. No remaps. |
| PR188 resource numbers (~132 bytes after an 8192 reserve, XIP estimates) | Stale estimates from another SHA. Not a fit claim at `94197fc8`. Do not reuse as measured budgets. |
| Per-step sound overrides as the second increment | Still optional and an owner matter. Must not collide with existing p-locks. Not required to ship the user bank or Juno/Drone append. |
| KORG pack conversion as a content path | Still not established. New Juno and Drone factory sounds must be **independently created** (spec Provenance). Do not treat the 81/30 as a proven-cleared factory identity at this SHA. |

---

## 2. (a) UX on 240×240 and existing controls

### 2.1 How factory, user, and imported are told apart

The 240×240 ST7789, 20+69+69+82 strips, Barlow faces (T10 / XS12 / S15 / B15 / M23), night theme and RGB565-safe inks are in map §3.1. The preset list lives in the **SOUND** context, not the STEP context.

**Origin tag on the list (required).** Each row shows a short origin word or abbreviation, distinct at a glance, in addition to the preset name:

- Factory: `FACTORY` (or `FAC` if the full word will not fit before the name is unreadable). Immutable bank. Stable index order.
- User: `USER`. Writable bank. Copy-on-write from a factory snapshot lands here, never on top of the factory row.
- Imported: `IMP` (or `IMPORT`). A file the user brought in.
- Missing provenance on an old file or old project: `UNKNOWN`. It still loads (spec Provenance).

**Ordering.** Factory first, in stable index order (never sorted by name, never reindexed). Then user. Then imported — **or** imported live in the user section with the `IMP` tag. **INFERRED** recommended: three sections with sticky section labels (`FACTORY` / `USER` / `IMPORTED`) so a short tag and a section agree. Do not interleave factory rows with user rows.

**Naming.** Factory names are the names already in the bank at `94197fc8` (Dot / Main Builder list them; this document does not invent them). New appends are named for their role (spec). User captures get an automatic `USER 01`… name in the first increment if there is no existing text-entry gesture in the source map (there is not a documented QWERTY namer — **INFERRED**). Rename is an owner option, and if present must use existing SCALE / K2 / K3 / `< >` list gestures only.

**Selected, playing, queued — do not fork a look.** Map these three list states onto Direction **selected / playing / queued** (`/workspace/smk-ux/directions/`; plan §5; map §3.2, §8.5):

- **Selected:** the list highlight, same language as a selected part or pattern (highlighted row, part colour).
- **Playing (assigned):** the preset that is the current part sound — the “playing” language of that direction (not a new glyph).
- **Queued (pending assign):** a load/assign that is waiting for an eligible attack, or a deferred write — the “queued” language of that direction, consistent with the pattern-cue blink and with `SAVE AT STOP` as a queued-save state. Prefer existing pending words (`WAIT FOR NOTES` and kin, as Dot inventories) over a new essay.

**Sound vs step.** The preset list is a SOUND surface. Follow Direction **sound vs step** and the PARA SOUND-header work (plan §4, mockup sheets `03-edit-sound.png`, `04-edit-one-step.png`, `03-04-how-you-tell.png`; owner decision 3 still open). Do not put `STEP n` / `P-LOCK` language on the preset list. Do not introduce a third header treatment.

Until those directions are final, **reference them by name**. Do not ship a one-off selected-row or pending look.

### 2.2 Gestures — existing controls only, no remaps

From the source map only. K4 is not a preset control.

| Act | Control | Map citation |
|---|---|---|
| Enter SOUND / preset context | PARA (`B_EDIT`) and the existing SOUND page / SCALE page list that Dot inventories | §1.2, §2.2, §4.4 |
| Walk list rows / pages | K2 | §2.2 |
| Walk list values / parts | K3 | §2.2 |
| Confirm, ask YES | SCALE (`B_SEL`) | §2.2; Oct 25 |
| Back, ask NO, cancel a pending sound | PATCH (`HOME`) | §8.1, `cancel_sound_request` 3939–3957; help “LIST: BACK   QUESTION: NO” |
| Project save (flash of project, including a dirty user bank if it lives with the project) | BT tap (`B_SAVE`) | §4.5. Not Bluetooth. |
| Undo / redo | PATCH+SEQ REC / PATCH+SEQ PLAY | §8.1 |
| Cue pattern; end chain | K4 | §1.2, §2.2 — **do not remap** |
| Confirmation | Overlay `O_ASK` | §2.1 |
| Short feedback | `say()` toast, 1300 ms | §3.2 |

Do not use NOTE REPEAT, KNOB BANK, FX, CHORD, ARP, or OCTAVE as preset-browser keys. Live-note ownership and the full shortcut list in map §8.6 and the plan preserve-list stay exactly as they are.

### 2.3 Save, rename, load, import, export — design rules

Detail and error copy live in the spec. The considerations are:

**Save a user preset (capture).** A list action (SCALE on a “SAVE PRESET” / “CAPTURE” row, then `O_ASK` if needed). **Not** a remap of BT. BT remains project SAVE. **INFERRED** recommended first increment: the user bank is persisted with the project, so a capture dirties the project and BT (or autosave, stopped and quiet) does the flash under the existing deferred policy. A device-global user bank is a later increment only after flash is measured, and is an owner option.

**Save feedback** uses the existing words only: `SAVES WHEN STOPPED` (toast when a write is requested during play), `SAVE AT STOP` (persistent queued state), `SAVED` (only after `rc==0`). Also keep the existing failure words `SAVE FAILED`, `NEWER DATA: NOT SAVED`, `MEMORY FULL: NOT SAVED`. Do not adopt “PROJECT SAVED”, “SAVE QUEUED”, or “Playback continues.”

The **save-chip vs full-header toast** treatment is **open owner decision 4** in the plan (sheets `05-save-while-playing.png`, `06-saved.png`). Present both; do not pick:

- **Option A (proposed chip):** same pickup-style left chip as plan §2, full words, pattern / tempo / play icon stay visible. Prefer full words over a squeezed `Q`.
- **Option B (current toast):** full-header toast for 1300 ms, then the badge (`SAVE AT STOP`, or `Q` / `*` on a busy header).

Either option is display only. Semantics do not change. Preset-related flash follows the same words and the same deferred rule. Device Bluetooth remains `BLUETOOTH SAVED`, never merged.

**Overwrite.** Never a silent replace. **Confirm before** the write, with `O_ASK` (SCALE = yes, PATCH = no), in the same family as `CLEAR … IN THIS PATTERN?` and `LOAD SAVED PROJECT?`. After a confirmed write, the change is **one undo** (PATCH+SEQ REC). Factory rows cannot be the target of a save. A full bank says a memory-full word and writes nothing.

**Load / assign.** SCALE on a row assigns. Stopped: assign without a flash write (parameters into the live sound). Playing: queue until the next eligible attack; show the queued/pending state; PATCH cancels (`cancel_sound_request`). Held notes and running patterns: see §2.4.

**Rename.** First increment may ship automatic `USER 01` names. A rename flow, if the owner wants it, is a list editor on existing gestures only. **INFERRED:** no existing QWERTY name entry in the source map.

**Import (all engines: 909, 808, BREAK, Chicago, Miami, Juno, Drone).** Today it returns `-1` (Dot). A valid per-engine file must succeed. Validate engine and schema **before** any live mutation. Visible errors, no silent fallback, nothing replaced unless confirmed:

- Wrong engine (including a file for a different engine than the current part): a short toast such as `WRONG ENGINE`.
- Bad or malformed file: `BAD FILE`.
- Bank full: `MEMORY FULL` (align with `MEMORY FULL: NOT SAVED` / Oct 25 `SOUND MEMORY FULL`).
- Newer-than-known schema: `NEWER DATA` language, consistent with `NEWER DATA: LOAD BLOCKED`.

**Export.** The flow must work for **user** presets (and for imported files the user brought). **Export and sharing of factory assets is UNRESOLVED** — see the spec’s Provenance section. Do not design factory export as cleared, and do not assume it is restricted. Factory export is gated by a single build-time switch whose default is pending the Main Builder trace.

### 2.4 Live play

- Flash writes are deferred: never erase flash during play, record, sounding notes, or clip activity (map §4.5, §7; 2222–2294, 8783–8789).
- No audio glitch on assign; no CPU regression on PERF TEST (`FACTORY LOOP`, `ALL FIVE`, `WORST CASE`).
- Held notes keep the previous snapshot until the next eligible attack. Running patterns keep playing. New attacks use the newly assigned preset. P-locks still apply. Ties / slides are not broken to force the new sound (`WAITING FOR NOTE` / existing pending reason).
- `ui_live_key_owner` and `live_boundary` are unchanged. A preset-list visit on PARA must not steal live keys (map: a settings list or HELP on SEQ/PARA does not steal keys, 4282–4283).
- Chain is not saved, on purpose. Assigning or saving a preset does not write the chain, and does not start or end a chain (K4 still ends a chain — do not involve K4).
- No All Notes Off of other parts. Navigation does not write steps or motion.

### 2.5 Expert muscle memory

No remaps. No removed shortcuts. PATCH HOME↔SONG toggle stays (decision record C, 05:20 America/Chicago). NOTE REPEAT stays MIX (C). KNOB BANK stays a single press (A). BT timings stay 250 / 400 (A). SEQ header word stays (A). First-sound footer, PARA SOUND header, and save chip remain open owner decisions (2, 3, 4); preset work follows those directions when they land, and does not depend on closing 2.

---

## 3. (b) Sound design — factory roles (parameter intent only)

No invented counts, sizes, or parameter values. Sound-character **claims** need rendered audio evidence. This pass cannot render (no builds or execution). Every role below is **intent only, pending render evidence**. The evidence protocol and acceptance live in the spec.

### 3.1 Juno and Drone — APPEND-ONLY after the existing four

Dot’s inventory at `94197fc8`: 4 Juno and 4 Drone factory presets. Those occupy indices 0–3 on each engine. They are not renamed, reordered, retuned, or replaced. New curated roles are **appended** at index 4 and up so existing indices and projects keep working.

Before appending, Main Builder documents what the existing four already cover (same evidence protocol). If an existing index already fulfils a proposed role, **do not append a duplicate**. How many new slots ship is blocked on a flash-reserve measurement and is an owner count decision. The list below is the curated **role intent**, in recommended append order, not a mandated head-count.

**Juno** (Juno-60-style with chorus):

| Role id | Role | Parameter intent (not values) |
|---|---|---|
| J-PAD | Chorus pad | Sustained pad, moderate attack, chorus on, mid filter, long enough release to hold a chord. |
| J-BRASS | Brass | Faster attack, brighter than the pad, chorus on, filter-envelope snap. |
| J-LEAD | Poly lead | Mid register, clearer than the pad, shorter release, chorus on. |
| J-BRASS-BASS / J-BASS | Bass | Low register, tighter envelope than the pad, chorus present but not washing out the low end. |
| J-STAB | Stab | Short gate, bright attack, short release; usable for chops. |
| J-STR | Strings | Slower attack than a stab, chorus on, ensemble-like sustain, mid-to-upper mid. |

**Drone** (String Machine):

| Role id | Role | Parameter intent (not values) |
|---|---|---|
| D-DRONE | Sustained drone | Very slow attack, long hold, ensemble / string-machine character, little sharp transient. |
| D-ENS | Ensemble strings | Bowed-ish attack, mid register, ensemble, playable as a chord bed. |
| D-AIR | High shimmer | Upper register, brighter, airy; still a string-machine, not a lead synth. |
| D-WASH | Dark wash | Low, slow, muted, long; a bed under other parts. |

New Juno and Drone factory sounds must be **independently created**, each with a provenance record (spec Provenance). They are not derived from the Chicago or Miami factory banks, and not from KORG Gadget material described in historical PR #276 (that description is not proof of identity at this SHA; it is a reason to create new work independently).

### 3.2 Existing engines — brief intent, not a shipping mandate to append

Do **not** append, reorder, or retune Chicago’s 81 or Miami’s 30 in this work unless the owner asks. The lines below are language for understanding and for any later curation.

- **Chicago** (the 303 engine): acid bass, short pluck, sustained line, driven / overdriven line. **No role may rely on velocity-to-cutoff** — Chicago does not have it. Accent and slide are Chicago-style (map §4.3).
- **Miami:** not a 303 and never named 303 or 303B. Wobble / Miami character (map: wheel can open Miami wobble depth). Roles are Miami roles (bass, wobble bed), not “303B” roles. Miami ignores accent and slide (**DOC** BAS-13/14, map §4.3).
- **909, 808, BREAK:** kit / slice engines. Dot’s factory-preset counts above are for Chicago, Miami, Juno, Drone only. Import still applies to all engines, in whatever preset object Dot inventories for drums. Do not invent a drum factory-bank size.

---

## 4. (c) Constraints

- **Compatibility.** Old projects and old preset files load unchanged. Existing factory indices stay. Embedded project parameters stay authoritative. Versioning: unknown newer data reports `NEWER DATA` and does not partially load or silently drop fields. Streams 13 / 14 / 15 are not casually changed. Details in the spec.
- **Flash reserve.** Not measured at this SHA (map §11). **Blocker for any new count** (user slots, appended factory slots). Measure before agreeing a number. Do not shrink the reserve without review. Do not silently evict.
- **Live performance and CPU.** Deferred writes; no glitch; no PERF TEST regression; no audio-path allocation.
- **Chain** is not saved, on purpose.
- **Nothing is removed** without the owner’s words (decision record; standing).
- **Provenance and factory-asset rights** are a shipping constraint. See the spec. This considerations document does not clear or forbid factory export.

---

## 5. OWNER DECISIONS

Short options. Already closed in the plan decision record (2026-11-13 05:20 America/Chicago, Dot via Orchestrator, **not** owner quotes) and **not** reopened: PATCH keep HOME↔SONG (C); NOTE REPEAT no change (C); KNOB BANK single press (A); BT times 250/400 (A); keep SEQ header word (A); Docs Keeper updates the manual against `94197fc8`; remove nothing.

Still open, and used by preset work:

1. **Save-chip display (plan decision 4).** A: pickup-style chip, full words `SAVES WHEN STOPPED` / `SAVE AT STOP` / `SAVED`, clock stays. B: current full-header toast, then badge / `Q`. Display only. The spec implements both visually by following Direction **save chip** when it lands; semantics are the same either way.
2. **PARA SOUND header (plan decision 3).** A vs B/C on the existing sheets. The preset list is a SOUND surface and follows this, plus Direction **sound vs step**. Implementation of the header itself stays on hold until the owner picks, as the decision record already says.
3. **User-bank scope.** A: project-scoped user presets, flushed with the existing BT / deferred project save (recommended first increment). B: a device-global user bank (only after flash measurement).
4. **Imported-section placement.** A: a third list section after user. B: imported rows live in the user section with an `IMP` tag.
5. **Rename in the first increment.** A: automatic `USER 01` names only. B: a list-based rename on existing gestures.
6. **How many Juno / Drone roles to append.** Choose a subset (or all) of the curated roles **after** flash is measured and after render evidence. Zero new slots is allowed if flash does not permit or if the existing four already cover the needed roles. Do not pick a number in this document.
7. **Factory-export switch after the Main Builder trace.** Not an owner pick until the trace exists. Recorded in the spec as pending. This list does not ask the owner to clear rights without that trace.

First-sound footer (plan decision 2) does not block preset work.

---

## 6. INFERRED log

- Exact factory names and the precise PARA / SOUND widget already bound to preset select: Dot’s inventory and Main Builder, not this document.
- Recommended project-scoped user bank as the first increment.
- Three list sections rather than one flat list.
- Automatic `USER 01` names in the first increment, because a text-entry gesture is not in the source map.
- Temporary engineering default OFF for the factory-export switch (spec), which is **not** a rights finding.
- Treating copy-on-write captures whose source is a factory snapshot as factory-derived for the export gate until the trace says otherwise (spec).
- Oct 25’s 64-frame de-click figure is a proposal, not a measured value here.
- Footer / hold-help as the place for the full provenance line on 240×240.
