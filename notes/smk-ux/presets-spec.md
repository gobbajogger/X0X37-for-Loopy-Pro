# DnBOX SMK-37 — preset IMPLEMENTATION SPEC

## Changelog

- 2026-11-13 05:45 America/Chicago: §3A marked BLOCKED pending owner inputs, per Firmware Main Builder's plugin-identity report (Chicago and Miami are KORG Gadget AU 3.1.6; proprietary formats; 32 B cell).

- **2026-11-13, America/Chicago — this revision.** Requirement update (Dot relay of an authorized task update, not an owner quote). (1) New §3A **Plugin cross-work**: Chicago and Miami import from and export to the actual source plugins; format-agnostic, with a placeholder for the format Main Builder identifies (KORG Gadget is a lead only). UX: pick the file, then a per-preset mapping report on the 240×240 screen before any commit; each parameter is `EXACT`, `APPROXIMATE`, or `UNSUPPORTED` (dropped or defaulted), plus a preset-level summary. The words never say or suggest an identical sound. Cancel leaves nothing changed. Imported records keep provenance (source plugin, version if known, mapping summary). Export shows the same honesty report in reverse. Chicago incoming velocity-to-cutoff is `UNSUPPORTED`. Miami is not a 303. Acceptance and BugBot tests include round-trip drift checks that need render evidence. (2) **JUN-60** (Juno-60-style) factory role intents expanded for live play; each role original or rights-verified with a provenance record; APPEND-ONLY after existing IDs 0–3, which keep their meaning; no padded counts — the final number is decided after flash and render evidence; parameter intent only, each pending render evidence. (3) Project vs device bank, imported-section placement, rename in v1, and role counts moved out of owner options into §9A **Deferred until flash + render evidence**. Previous revision SHA-256: `d318d7003d0021af7fd86e2994cc7b35f2ba5e92ee3bd12e6068c0106ce73574`.

**For:** Firmware Main Builder (the only source writer).
**QA:** BugBot, independently, against §8 and the tests in each section.
**Background:** `/workspace/smk-ux/presets-considerations.md` (Oct 25 apply/supersede, design reasoning). This file is the build brief. If the two disagree, this file wins on what to implement. Open display decisions remain in §9. Project vs device bank, imported placement, rename in v1, and role counts are in §9A (deferred until flash + render evidence), not owner options now.

**Date:** 2026-11-13, America/Chicago.
**Target SHA (behaviour baseline):** `94197fc839b00aff47b98ca02f3c6396e4a6eabf`, as already mapped. This spec does not re-investigate source.

**Authority.** Owner-authorized execution, relayed by Dot, **not** an owner quote. Inventory at `94197fc8`, **as reported by Dot** (cite it that way; do not re-derive it): 81 Chicago, 30 Miami, 4 Juno and 4 Drone factory presets. There is no working user bank. Import returns `-1`.

**Citations this spec may use, and must not redo:**

- `/workspace/smk-ux/source-map-94197fc8.md`
- `/workspace/smk-ux/plan-94197fc8.md`, including the decision record at 2026-11-13 05:20 America/Chicago
- Archive `54bb3c496:docs/plans/2026-10-25-native-preset-workflow.md` (full SHA `54bb3c49696e1c5d4f9a8f95c0423a5a08fea0ce`), as reviewed in the considerations file
- Visual directions in `/workspace/smk-ux/directions/` — Direction **selected / playing / queued**, Direction **sound vs step**, Direction **save chip**, Direction **PARA SOUND header**. At the time this was written, that directory was **not** on the box. Where a direction is not final, **reference it by name**. Do not fork a separate look.

**Out of scope for this document’s author, and already true for Main Builder’s inputs:** no source investigation in this pass; no code in this pass; no builds or renders in this pass. Main Builder writes the code. BugBot does not write the product source.

**Standing product rules** (map / plan; not reopened): preserve `ui_live_key_owner` and `live_boundary` exactly; preserve the full shortcut list (map §8.6, plan preserve-list); remove nothing without the owner’s words; flash does not erase during play; chain is live-only and is not saved, on purpose; streams 13 / 14 / 15 are not casually changed; names PATCH/HOME, BT/SAVE, PARA/EDIT, NOTE REPEAT/MIX, 909/808/BREAK/Chicago/Miami; Miami is never a 303 or 303B; Chicago is the 303 engine and has no velocity-to-cutoff; K4 remains pattern cue (not a preset browser); p-locks remain for per-step param variation.

Guesses are marked **INFERRED**.

---

## 0. Inventory and current gaps (Dot, `94197fc8`)

Cite as **Dot’s inventory**, not as a re-derived fact and not as an owner quote:

| Engine | Factory presets at `94197fc8` | Notes |
|---|---|---|
| Chicago | 81 | Indices 0–80. Do not reorder, retune, or rename in this work. |
| Miami | 30 | Indices 0–29. Same rule. Miami is not a 303. |
| Juno | 4 | Indices 0–3. APPEND-ONLY after these. |
| Drone | 4 | Indices 0–3. APPEND-ONLY after these. |
| User bank | none working | This work makes it work. Capacity is blocked on a flash-reserve measurement (map §11: unmeasured). |
| Import | returns `-1` | This work makes a valid file succeed, and defines visible errors for the failure cases. |

Main Builder lists the existing factory **names** from that inventory when implementing. This spec does not invent those names.

**Flash reserve is a blocker for counts.** Before shipping a number of new factory slots or user slots, Main Builder measures the reserve at the implementation SHA and reports it. Do not reuse Oct 25 / PR188 figures. Do not shrink the reserve without review. Do not silently evict. Zero new factory slots is acceptable if the measurement does not permit them, or if the existing four already cover the needed Juno/Drone roles (after evidence, §5).

---

## 1. Provenance

This section leads from **Dot’s inventory**. That inventory is **not proof** and **not an owner quote**.

Historical PR #276, as described in that inventory, described the 81 Chicago / 30 Miami sounds as coming from owner-paid KORG Gadget packs, private / dev-only, with an intent to strip them before public redistribution. **That does not prove the identity of the banks at `94197fc8`, and it does not prove the current rights.** Main Builder traces factory-asset identity and rights at the implementation SHA. Until that trace is done, export and sharing of factory assets is **UNRESOLVED**. This spec does **not** design factory-sound export as cleared, and does **not** assume it is restricted either.

### 1.1 Factory-asset trace (Main Builder)

Main Builder produces a short written trace (a note is enough) covering, at least:

- Whether the 81 Chicago and 30 Miami factory sounds at the implementation SHA are the same material PR #276 described, or not, and on what evidence.
- The provenance of the existing 4 Juno and 4 Drone factory sounds (indices 0–3). This spec does not assume they are clean or encumbered.
- What “export” and “share” would emit (parameters, names, identifiable pack material, or not).
- A recommendation to turn the factory-export switch ON or OFF for public builds, with the rights finding stated.

BugBot does not perform the legal finding. BugBot checks that the trace exists before a public build ships with factory export compiled on, and checks the switch and tests below.

### 1.2 Export gate — one build-time switch

- The **export flow is implemented and works for user presets**, and for imported presets the user brought, regardless of the switch. See §3.5 and §3.6.
- **Factory export** is compiled behind **a single build-time switch**. There is exactly one such switch (one symbol). **INFERRED** name: `DNBOX_FACTORY_PRESET_EXPORT`. Main Builder may pick an equivalent single symbol; there must not be a second flag that also gates this.
- **Default while the trace is pending:** recorded here as **PENDING THE TRACE**. An implementation cannot ship an unspecified `#ifdef`, so the temporary **engineering default is OFF** (factory export compiled out). This is **not** a determination that factory export is restricted, and **not** a determination that it is cleared. It exists so a build cannot emit factory assets before the trace. After the trace: if sharing is cleared, the default may become ON for the intended distribution; if it is restricted, the default stays OFF and the trace records the finding. A public build must not ship with the switch ON before the trace is complete.

**What the switch gates.** Export of a preset whose origin is `factory`. **INFERRED**, temporary, until the trace says otherwise: also gate presets that are factory-derived — a copy-on-write user capture whose source is a factory snapshot, and a re-import of exported factory material. User-authored-from-scratch captures, and imported files the user brought that are not factory-derived, export with the switch OFF. The trace may narrow or widen this set; until then, this is a safety posture, not a rights finding.

**Behaviour when factory export is requested and the switch is OFF.** Refuse. Write no file (and no partial file). Show a short explicit toast. **INFERRED** copy: `FACTORY EXPORT OFF`. Do not pretend the export succeeded. Do not fall back to silently exporting a stripped or relabelled factory sound.

**Behaviour when the switch is ON.** The factory-export path works (a versioned file is produced). That success is a functional fact. It is **not** a rights clearance.

### 1.3 New Juno and Drone factory sounds — original or rights-verified

Every **new** factory preset (index 4 and up, §5) carries a **provenance record** and is either **original** or **rights-verified**:

- **Original** (independently created): not copied, retuned-from, or resampled from the 81 Chicago or 30 Miami factory sounds, from KORG Gadget pack material as described in historical PR #276, or from any other source the author cannot name in the parameter-source field. This is the expected path for new **JUN-60** (Juno-60-style) and **Drone** appends.
- **Rights-verified**: the parameter-source field names the source, and the licence statement documents the verification. Main Builder’s factory-asset trace (§1.1) is what verifies; this spec does not. A record that names a third-party pack without a verification statement is not rights-verified, and the preset does not ship.

Each new factory preset has a **provenance record** stored with the factory bank (in firmware or in the factory-data object Main Builder already uses — do not invent a second unofficial store). Required fields, all non-empty:

| Field | Meaning |
|---|---|
| Author / agent | Who made it (a named person or a named agent). |
| Date | Authorship date, reported in America/Chicago. |
| Method | How it was made (for example, parameter-authored against the role intent, then render-evidenced). Record the method that was actually used. |
| Parameter source | Where the values came from, in words. For an **original** preset: must be consistent with independent creation, and must not cite the KORG Gadget packs or the Chicago/Miami factory banks as the source. For a **rights-verified** preset: must name the source. |
| Licence statement | A non-empty rights statement. This spec does not invent the licence text. Main Builder (and the owner, if needed) fills it. BugBot checks that the field is present and non-empty. An **original** preset’s statement must not claim a third-party pack licence as the source. A **rights-verified** preset’s statement must document the verification. |

Existing indices 0–3 are **not** retuned to satisfy this. Their records may be `unknown` until the trace fills them. They still load. Their meaning is unchanged (§5.1).

### 1.4 Provenance field on user and imported presets

Every user preset and every imported preset carries a provenance / source field in this format:

- **origin:** `factory` / `user` / `imported` (and `unknown` when the field is absent — see below). A copy-on-write capture from a factory snapshot is origin `user` with the factory preset identified in **source name**, so the export gate in §1.2 can see the derivation.
- **source name:** a short name of the source (factory preset name or index, file name on import, or `CAPTURE` / the user name on a capture).
- **date:** when it was captured or imported (America/Chicago on the device clock is enough).
- **original engine:** the engine the sound was made for (909, 808, BREAK, Chicago, Miami, Juno, Drone). This is the engine used in import validation (§3.5).
- **For a plugin-cross import or export (§3A), also kept:** **source plugin** (the plugin Main Builder identifies, or `unknown` until identified); **version** of that plugin or of the file, if known, else `unknown`; **mapping summary** (preset-level counts of `EXACT` / `APPROXIMATE` / `UNSUPPORTED`, and enough to recover the report’s honesty — not a claim of an identical sound). These three are absent on a native-only user capture, and that is fine. Old files without them load without failing (§1.4 last paragraph; origin may already be `unknown`).

**How it is shown, and how it is kept:**

- The field is **kept** in the preset record and in any exported file that this work writes.
- On the 240×240 **list**, the origin is shown as the origin tag in §2 (`FACTORY` / `USER` / `IMP` / `UNKNOWN`). That is the at-a-glance distinction.
- The full line (source name, date, original engine) is **kept** always, and **shown** where it fits without a new view: **INFERRED** — a selected-row footer in XS 12 if it fits, otherwise hold-help on the selected row (existing hold-help, no new button). Do not invent a provenance page. Align any selected-row treatment with Direction **selected / playing / queued**, and keep this on the SOUND side of Direction **sound vs step**.
- Factory new-append records (§1.3) are kept with the factory bank. They need not all appear on the 240×240 list; BugBot still reads them from the bank / build artefact.

**Old files and old projects without this field load as origin `unknown`.** They do **not** fail. They remain playable and assignable. The list tag is `UNKNOWN`. Export of a user or imported preset that is `unknown` still follows §1.2 and §3.6 (user/imported export works; factory-derived / factory origin still gated). Saving or re-exporting may fill a date and write `unknown` into origin rather than inventing a false origin. Do not rewrite an old project on load just to add the field (load is non-mutating; a later explicit save may add it).

### 1.5 Tests (BugBot) — provenance

**P1. Trace and switch default.** A public build with factory export compiled ON fails this test if the factory-asset trace is missing. The recorded default while pending is OFF. BugBot records the compiled value of the single switch.

**P2. Single switch.** Exactly one build-time symbol gates factory export. Grep / build-flag inspection is enough.

**P3. User export, switch OFF.** Export a user-authored preset (origin `user`, not factory-derived). A valid versioned file is written. The file contains the provenance field (origin, source name, date, original engine).

**P4. User export, switch ON.** Same as P3; still succeeds.

**P5. Imported export, switch OFF and ON.** Import a valid non-factory user file, then export it. Succeeds in both switch values. Provenance origin is `imported`. Original engine matches the file.

**P6. Factory export, switch OFF.** Select a factory preset and request export. No file is written (no partial file). An explicit refusal toast is shown (not `SAVED`, not a success). The live sound and the bank are unchanged.

**P7. Factory export, switch ON.** The same request writes a valid versioned file. This test is functional. It is not a rights clearance. Skip-or-fail policy: if the switch is OFF in the build under test, BugBot reports P7 as **not exercised**, not as a product pass on rights.

**P8. Factory-derived capture, switch OFF (temporary rule in §1.2).** Capture a user preset from a factory snapshot (copy-on-write). Origin is `user`, source name identifies the factory preset. Export is refused while the temporary rule stands, same as P6. If the later trace narrows the rule, this test is updated to match the trace; until then, P8 fails if the file is written with the switch OFF.

**P9. New Juno/Drone provenance completeness.** Every shipped factory Juno (JUN-60) or Drone preset at index ≥ 4 has all five §1.3 fields, all non-empty, and is marked original or rights-verified. A missing field fails the test. Indices 0–3 may be `unknown` and must still load (P12).

**P10. Original or rights-verified (record check).** For each index ≥ 4: if **original**, the parameter-source field is consistent with independent creation (it does not name the KORG Gadget packs, PR #276 material, or the Chicago/Miami factory banks as the source). If **rights-verified**, the parameter-source field names the source and the licence statement documents the verification. This is a record check, not a sonic fingerprint, and not a legal proof. Sonic role evidence is §5, and is separate.

**P11. Licence field.** For each index ≥ 4, the licence statement is present and non-empty. An original preset’s statement does not claim a third-party pack licence as the source. A rights-verified preset’s statement documents the verification.

**P12. Old file without provenance.** A preset file or project from before this field exists loads. Origin is `unknown`. No error toast that claims a failure. The sound is assignable. The list tag is `UNKNOWN`. The project is not dirtied solely by the load filling a missing field.

**P13. Display.** On the 240×240 list, origin tags distinguish factory, user, imported, and unknown (§2). A selected user or imported row keeps the full field; the full field is present in an exported file (P3–P5).

**P14. Original-engine mismatch is an import error, not a silent relabel.** See §3.5 and I2. Provenance original-engine is not rewritten to the current part’s engine.

---

## 2. How factory, user, and imported are told apart

### 2.1 On the 240×240 screen

The preset list is a **SOUND** surface (Direction **sound vs step**; plan §4; sheets `03-edit-sound.png`, `04-edit-one-step.png`, `03-04-how-you-tell.png`). It uses the existing 20+69+69+82 strips, Barlow faces, and night / RGB565-safe inks (map §3.1). The PARA SOUND header follows owner decision 3 and Direction **PARA SOUND header** when that direction is final; until then, reference it by name, and do not introduce a third header.

Each list row:

- **Name** on the left (factory name as already in the bank; user automatic `USER 01`… as the working assumption until §9A is resolved).
- **Origin tag** on the right or as a section label: `FACTORY` (or `FAC` if the full word makes the name unreadable), `USER`, `IMP` (or `IMPORT`), `UNKNOWN`.
- **Index** for factory rows is the stable factory index, shown if it fits without crowding (helpful for “append after 4”; **INFERRED** as `F04` or `#4` in XS). Do not renumber.

**Ordering.** Factory section first, **stable index order** (0…N−1), never sorted by name, never reindexed. Then the user section. Then imported as a third section. **Working assumption** until §9A is resolved: **three sections** with a sticky section word (`FACTORY` / `USER` / `IMPORTED`). Factory and user rows are never interleaved. Imported-inside-USER with an `IMP` tag is the other deferred option; do not ship a third scheme.

**Selected / playing / queued** map onto Direction **selected / playing / queued** (plan §5; map §3.2, §8.5). Do not fork a look:

| List state | Meaning | Visual |
|---|---|---|
| Selected | The row the encoder is on | The direction’s **selected** language (highlighted row, part colour), same family as a selected pattern or part. |
| Playing (assigned) | This preset is the part’s current sound | The direction’s **playing** language. Not a new glyph family. |
| Queued | Assign is pending an eligible attack, or a write is deferred | The direction’s **queued** language, in the same family as the pattern-cue blink and as `SAVE AT STOP`. Pending copy prefers existing product words (`WAIT FOR NOTES` and kin, as Dot inventories), not a new essay. |

If the same row is selected and assigned, both facts are visible (selected highlight **and** the playing mark), as the direction specifies. If the direction is not yet final, implement the states as named above and follow the drawings when they land.

### 2.2 In naming and in ordering

- Factory names at indices already in the bank do not change.
- New factory appends are named for their role (§5). Names must fit the 240×240 list (prefer short role names; Barlow XS/S).
- User captures: `USER 01`, `USER 02`, … as the working assumption until §9A (rename) is resolved. The number is a user-bank slot, **not** a factory index. Do not start user numbers at a factory index.
- Imported: keep the file’s name if it fits and is safe to display; otherwise `IMP 01`…. Provenance **source name** keeps the original file name even if the display name is shortened.
- Edited factory: copy-on-write into a user slot. The factory row is unchanged. The user row’s provenance source name points at the factory preset. The origin tag is `USER`, not `FACTORY`.

### 2.3 Tests (BugBot) — distinction

**D1.** A factory, a user, and an imported preset on the same engine are visually distinct on a 240×240 host snapshot (origin tag and/or section). A reviewer can tell them apart without opening a file.

**D2.** Factory order is index order. Loading a project that points at Juno index 2 still points at the same index after new appends exist (see also C2).

**D3.** A copy-on-write edit of a factory preset creates a user row, leaves the factory row and its sound unchanged, and tags the new row `USER`.

**D4.** An old preset without provenance shows `UNKNOWN` and is still in the list (P12).

**D5.** The list is on a SOUND surface: no `STEP n` / `P-LOCK` copy on that list. Header treatment matches Direction **sound vs step** / **PARA SOUND header** when those are final, or is recorded as “direction not final, referenced by name.”

---

## 3. Save, rename, load, import, and export flows

Built only on **existing** controls and gestures from the source map. **No remaps.** K4 is not used. Gestures:

| Act | Gesture | Map |
|---|---|---|
| Enter SOUND / preset context | PARA (`B_EDIT`), existing SOUND page / SCALE page list that Dot inventories | §1.2, §2.2, §4.4 |
| Walk rows / pages | K2 | §2.2 |
| Walk values | K3 | §2.2 |
| Confirm, ask YES | SCALE (`B_SEL`) | §2.2 |
| Back, ask NO, cancel pending sound | PATCH (`HOME`) | §8.1, `cancel_sound_request` 3939–3957 |
| Project save | BT tap (`B_SAVE`) | §4.5. Not Bluetooth. |
| Undo / redo | PATCH+SEQ REC / PATCH+SEQ PLAY | §8.1 |
| Cue pattern (unchanged) | K4 | §1.2, §2.2 — do not remap |
| Ask | `O_ASK` | §2.1 |
| Toast | `say()`, 1300 ms | §3.2 |

### 3.1 Save (capture to the user bank)

1. From the preset list, a **list action** (a `SAVE PRESET` / `CAPTURE` row or an equivalent SCALE confirm on “save here”). Not a BT remap. Not a new button.
2. If the target slot is empty: write the live sound into that user slot. Set provenance: origin `user`; source name `CAPTURE` or the factory source if this is copy-on-write from factory; date now; original engine = current engine.
3. If the target is occupied: **confirm before write** (`O_ASK`). **INFERRED** ask copy, in the existing ask family: `REPLACE USER PRESET?` with SCALE = yes, PATCH = no. PATCH does nothing to the slot. SCALE overwrites, and the write is **one undo** (PATCH+SEQ REC restores the previous user-slot contents). Never a silent replace.
4. Factory rows are not valid save targets. A save aimed at a factory row is refused or redirected to a new user slot (copy-on-write). It never mutates the factory snapshot.
5. If the user bank is full: write nothing; toast `MEMORY FULL` (align with `MEMORY FULL: NOT SAVED`). No eviction.
6. **Persistence.** **INFERRED** working assumption until §9A (project vs device bank) is resolved: the user bank is project-scoped and flushed with the existing project save. The capture dirties the project. The flash write is the existing BT / autosave path, **deferred**:
   - Requested while playing, recording, or otherwise busy: toast `SAVES WHEN STOPPED`; header (or chip) `SAVE AT STOP` (or the existing quiet / rec variants `SAVES WHEN QUIET`, `REC OFF TO SAVE`, `SAVE WHEN QUIET`). No flash until stopped and quiet (map §4.5, 2222–2294, 8783–8789).
   - On success (`rc==0` only): `SAVED`. Dirty clears only then. Failures stay `SAVE FAILED`, `NEWER DATA: NOT SAVED`, `MEMORY FULL: NOT SAVED`.
7. **Visual for those words.** Follow Direction **save chip** (plan owner decision 4, sheets `05-save-while-playing.png`, `06-saved.png`). Until the owner picks:
   - **Option A:** pickup-style left chip, full words, pattern / tempo / play icon stay.
   - **Option B:** current full-header toast, then the badge, which may squeeze to `Q` on a busy header.
   Either is display only. Main Builder must use the **existing words**, must not use “PROJECT SAVED” / “SAVE QUEUED” / “Playback continues,” and must not merge `BLUETOOTH SAVED` into `SAVED`. If the capture itself is RAM-only until BT, the words appear on the BT / autosave write, not as a false `SAVED` at the moment of the RAM capture. A RAM capture may show a short non-flash confirmation that is **not** the word `SAVED` (**INFERRED** acceptable: the slot appears in the list tagged `USER`; unsaved state remains the existing unsaved badge). `SAVED` is only for a successful flash write.

### 3.2 Rename

Working assumption until §9A (rename in v1) is resolved: automatic `USER 01`… names. A list-based name editor, if later selected, uses K2 / K3 / SCALE / PATCH / existing `< >` only. No new gesture, no QWERTY page unless the source map already has one (it is not documented). Factory names are not renamed. Rename of a user slot is one undo. Rename does not change origin or original engine. Source name in provenance is not destroyed (keep the original source; the display name can change).

### 3.3 Load / assign

1. SCALE on a row assigns that preset to the current part, only if `original engine` (or, for old `unknown` files, the file’s engine id) matches the part’s engine. A mismatch is `WRONG ENGINE` and a no-op (same as import).
2. **Stopped:** assign into the live sound immediately. No flash. Does not write steps or motion. Does not dirty the project solely because the live sound now differs from the last saved project — **INFERRED:** follow the existing “live sound is part of project dirty” rule already in the product (Dot / Main Builder); do not invent a new dirty policy. If the product already marks sound edits dirty, this is a sound edit.
3. **Playing:** queue the assign until the next eligible attack. Show the **queued** state (Direction **selected / playing / queued**). Prefer existing pending words. PATCH cancels (`cancel_sound_request`, map §8.1). Held notes and ties: §4.
4. Navigation of the list without SCALE does not assign and does not write.

### 3.4 Overwrite policy (all write paths)

Never a silent replace. Confirm (`O_ASK`) before overwriting an occupied user or imported slot (save, import-into-slot, or a replace-export on the same name if that exists). After a confirmed write, one PATCH+SEQ REC undo restores the previous slot. Load-project and CLEAR PART already ask; do not weaken those. Factory bank is never the destination of a user write.

### 3.5 Import (all engines) — native DnBOX files

Engines: 909, 808, BREAK, Chicago, Miami, Juno, Drone. Today’s `-1` becomes a real implementation. **Plugin-format** import for Chicago and Miami is §3A, not this subsection. A native DnBOX file still follows this subsection.

1. Choose the file through the **existing** file / transfer path that Dot inventories. Do not invent a new USB UI or remap a control.
2. **Validate before any live mutation:** engine id, schema version, and well-formedness. If the file names an original engine, it must match the current part’s engine (or the user must be importing into a bank for that engine — the file does not get relabelled).
3. On success: create a new **imported** slot (origin `imported`, source name = file name, date now, original engine from the file). If the only way to store it would overwrite an occupied slot, **ask** first (§3.4). Then the live assign follows §3.3 (queue if playing). Persist with the deferred project-save path (§3.1.6).
4. **Error states** — visible, no mutation, no silent fallback, no relabel:

| Condition | Visible feedback (**INFERRED** short copy; Main Builder may adjust to the 240×240 width, but the meaning stays) | Mutation |
|---|---|---|
| File’s engine ≠ current / target engine | `WRONG ENGINE` | None |
| Malformed, unreadable, or truncated file | `BAD FILE` | None |
| User / import bank is full | `MEMORY FULL` | None; no eviction |
| Schema newer than this firmware knows | `NEWER DATA` language, consistent with `NEWER DATA: LOAD BLOCKED` | None |
| Import path still unable to run (today’s `-1` must not remain the user-visible result for a valid file) | A real error word above, never a silent `-1` | None |

5. Import does not write factory indices. A file that claims to be a factory identity does not overwrite the factory bank. If it is valid for the engine, it lands as **imported**.

### 3.6 Export — native DnBOX files

1. List action on the selected preset, existing file / transfer path. No new control.
2. **User and imported:** always implemented. Writes a versioned file (§6) that includes parameters, name, engine, schema version, and the provenance field (§1.4). Failure: `EXPORT FAILED` (**INFERRED** copy), no partial file if that can be avoided; the bank is unchanged.
3. **Factory:** gated by the single switch in §1.2. Switch OFF: P6. Switch ON: a versioned file, same schema family, provenance origin `factory`. This is UNRESOLVED as a rights matter until the trace.
4. Export does not dirty the project. Export does not flash-write the project. If export itself needs flash, it follows the deferred rule and the same save words; prefer not to need a project flash for a file export.
5. **Plugin-format** export for Chicago and Miami is §3A. Native export of a plugin-imported record still writes a DnBOX file with the plugin provenance fields kept (§1.4).

### 3.7 Tests (BugBot) — flows

**F1. Gestures.** Every flow in §3 is driven only by the table at the top of §3. K4 still cues a pattern and still ends a chain; it does not open or confirm a preset. A remap fails this test.

**F2. Capture empty slot.** Live sound lands in a new user slot, tagged `USER`, with provenance origin `user`. Factory bank unchanged.

**F3. Capture occupied slot, confirm.** `O_ASK` appears. PATCH leaves the old slot. SCALE replaces it. PATCH+SEQ REC restores the old slot. No path writes without the ask.

**F4. Capture occupied slot, no silent replace.** A test that omits the confirm and expects a write must fail. BugBot looks for a path that writes on a single SCALE without `O_ASK` and flags it.

**F5. Bank full.** Capture when full: `MEMORY FULL` (or `MEMORY FULL: NOT SAVED` if the failure is on the flash write); no slot changed; no eviction.

**F6. Save words, stopped.** A flash write that succeeds says `SAVED` only if `rc==0`. Dirty clears only then. Pattern / tempo / play-icon treatment follows the chosen (or still-open) Direction **save chip**.

**F7. Save words, playing.** A requested project / bank flash while playing says `SAVES WHEN STOPPED`, shows `SAVE AT STOP` (full words preferred over `Q`, if the chip option is in), does not erase flash until stopped and quiet, then `SAVED` on success. Audio continues through the request.

**F8. Load stopped / playing.** Stopped: immediate assign, no flash. Playing: queued, pending words, PATCH cancels, no flash. List motion without SCALE does not assign.

**F9. Import success.** A valid Chicago file onto a Chicago part (and the same for Miami, Juno, Drone, 909, 808, BREAK, one case each) creates an `IMP` row, provenance origin `imported`, original engine correct, and is assignable. Today’s `-1` is gone for these valid files.

**F10. Import `WRONG ENGINE`.** A Juno file onto a Chicago part (and at least one other mismatch): toast `WRONG ENGINE`, no new slot, no live-sound change, no relabel.

**F11. Import `BAD FILE`.** Truncated or garbage input: `BAD FILE`, no mutation.

**F12. Import `MEMORY FULL`.** Bank filled first; import refused; existing slots unchanged.

**F13. Import newer schema.** `NEWER DATA` language; no partial load.

**F14. Export user.** P3–P5 apply. `EXPORT FAILED` leaves no partial file and no bank change.

**F15. Rename.** Until §9A is resolved, new captures are `USER 01`… and factory names are unchanged. If a list-based rename later lands, it uses only existing gestures, is one undo, and still does not rename factory rows.

**F16. Undo granularity.** One capture, one overwrite, one rename, one successful import = one PATCH+SEQ REC undo each.

---

## 3A. Plugin cross-work (Chicago and Miami)

> **STATUS: BLOCKED pending owner inputs (2026-11-13 05:44 America/Chicago).** Source: Firmware Main Builder, `/workspace/presets-research/plugin-identity.md` (SHA-256 `71361571107929cdeb69db9bc0da3084448318733ab830f174038b594a4242ab`). Main Builder reports the plugins as proven: KORG Gadget Chicago and Miami, AU 3.1.6. Real two-way import/export is not justified yet, for three reasons: the native formats are proprietary or undocumented, the extraction was one-way and partly heuristic, and the 32 B DnBOX cell can't hold a full plugin program. Inputs needed from the owner: (1) a capability log of his installed AU parameter tree, (2) the sample files he uses locally, and (3) a tested round trip with an audio comparison. Until all three exist, nothing in 3A is to be implemented. `PLUGIN_FORMAT` stays a placeholder. The honesty rules below (EXACT/APPROXIMATE/UNSUPPORTED report, no identical-sound claim, cancel changes nothing) still apply to any future version. Factory rights are UNRESOLVED (owner decision), and factory export stays off.


Chicago and Miami presets must **import from** and **export to** the **actual source plugins**. Main Builder is researching which plugins and which formats those are. **KORG Gadget is a lead only** — not a confirmed format at `94197fc8`, and not a rights clearance (see §1). This section stays **format-agnostic**.

**Format placeholder.** Call the identified format (or formats) **`PLUGIN_FORMAT`** in this spec. Main Builder replaces that placeholder with the real plugin name(s) and file-format name(s) once identified. Chicago and Miami may not share a format. Until the placeholder is filled, the UX, honesty language, cancel rule, provenance, and tests in this section still stand; the parser is completed against the identified format. BugBot records the filled-in name. A ship that still says only “KORG Gadget” without Main Builder’s identification is using a lead, not a format.

This path is **in addition to** native DnBOX import/export (§3.5, §3.6). It applies to **Chicago and Miami** only. 909, 808, BREAK, Juno, and Drone are not required to have a plugin-format path in this work. Miami is **not** a 303 and is never treated as one. Chicago is the 303 engine and has **no velocity-to-cutoff**.

Factory-asset export to a plugin format is still **UNRESOLVED** and uses the same single switch as §1.2. User and imported (non-factory-derived) plugin export is implemented regardless of the switch, as with native user export.

### 3A.1 UX — existing gestures only, report before commit

No remaps. The gesture table at the top of §3 applies. K4 is not used. Do not invent a new button or a new view family. The report is a **SOUND** surface (Direction **sound vs step**). Align selected-row / scroll treatment with Direction **selected / playing / queued** by name; do not fork a look. Until those directions are final, reference them by name.

**Import from the plugin:**

1. **Pick the file** through the existing file / transfer path that Dot inventories (same as §3.5).
2. Validate and map **in memory**. **Nothing is committed** yet: no new slot, no live-sound change, no dirty bit, no flash, no factory write.
3. Show a **per-preset mapping report** on the 240×240 screen (§3A.2).
4. The user **cancels** with PATCH (`HOME`, ask NO): the report closes, **nothing has changed**.
5. The user **commits** with SCALE (ask YES). If commit would overwrite an occupied slot, `O_ASK` first (§3.4). On commit, the sound lands as an **imported** record (§3A.3) and live assign follows §3.3 (queue if playing). Persist with the deferred path (§3.1.6). Flash still does not erase during play.

**Export to the plugin** shows the **same honesty report in reverse** (DnBOX parameters → plugin parameters) **before** any file is written. PATCH writes no file (and no partial file). SCALE writes the plugin-format file. Factory or factory-derived sources follow §1.2. Failure: `EXPORT FAILED` (or the same meaning), bank unchanged.

Wrong engine, bad file, full bank, and newer-than-known schema still use §3.5 error words and mutate nothing. A Chicago plugin file is not imported onto a Miami part, and a Miami plugin file is not imported onto a Chicago part: `WRONG ENGINE`. A 303-labelled plugin file is not silently accepted as Miami.

Viewing the report during play must not glitch, steal live keys, or start a flash write (§4).

### 3A.2 Mapping report (240×240) — honesty

Every parameter that is considered in the mapping is labelled with exactly one of:

| Label | Meaning |
|---|---|
| `EXACT` | The parameter value maps exactly (same meaning, same stored value after the documented unit conversion, if any). This is a **parameter** label. It is **not** a claim about the sound. |
| `APPROXIMATE` | The parameter is mapped with a documented approximation (range, curve, or missing finer field). The report shows the incoming value and the value that would be stored (or, on export, written out). |
| `UNSUPPORTED` | The parameter cannot be represented. Its value is **dropped** or **defaulted**, and the report says which. The dropped or default value is visible. |

Plus a **preset-level summary**: counts of `EXACT`, `APPROXIMATE`, and `UNSUPPORTED`, always visible (sticky header on the 240×240, Barlow XS/S, RGB565-safe). **INFERRED** layout: a scrollable list (K2) of parameter rows (name, label, short fate such as the stored value, `DROP`, or `DEF`); footer `SCALE: COMMIT  PATCH: CANCEL` in the existing ask family. If the direction drawings specify a report or ask treatment, follow that rather than a new chrome family.

**The words must never say or suggest an identical sound.** This holds even when every parameter is `EXACT`, and even when a round-trip later measures no audible drift. Engines differ; Chicago has no velocity-to-cutoff; a mapped preset is a mapped preset.

- Required standing caveat on the report, import and export. **INFERRED** copy: `MAPPED, NOT IDENTICAL`. Main Builder may shorten for width; the meaning must stay. A report with no caveat and only `EXACT` labels is not acceptable — it would suggest identity.
- Forbidden on this report, in toasts, in hold-help, and in shipping manual copy for this path: “identical,” “identical sound,” “exact sound,” “same sound,” “sounds the same,” “perfect,” “1:1 sound,” “equivalent sound,” “faithful copy,” “stock-equivalent,” and any other wording that suggests the plugin and the DnBOX part are the same sound. The parameter label `EXACT` is allowed; “exact sound” is not.
- The title is a mapping title (for example `PARAM MAP`), never `SAME SOUND` or similar.

**Chicago velocity-to-cutoff.** Incoming velocity-to-cutoff (if the plugin file has it) is always **`UNSUPPORTED`**. Never `EXACT`, never `APPROXIMATE`. The value is dropped or defaulted as the report states. After commit, the Chicago sound still has no velocity-to-cutoff. On **export** from Chicago, if the plugin format has that parameter, it is `UNSUPPORTED` in the reverse report: Chicago does not invent a velocity-to-cutoff value to write out. It is dropped or defaulted on the plugin side, and the report says which.

**Miami is not a 303.** The Miami report uses Miami parameter names and Miami engine identity. It does not say 303, 303B, or Chicago. Miami ignored accent and slide (map §4.3, DOC BAS-13/14) remain Miami rules; a plugin field that assumes 303 accent/slide behaviour is mapped honestly (`APPROXIMATE` or `UNSUPPORTED`, not silently relabelled as 303).

### 3A.3 Provenance on plugin import

A committed plugin import keeps, in addition to §1.4 (origin `imported`, source name, date, original engine):

- **source plugin** — the actual plugin as Main Builder identified it (the filled-in `PLUGIN_FORMAT` identity), not a generic “gadget” guess;
- **version** if known, else `unknown`;
- **mapping summary** — the preset-level counts and enough of the per-parameter labels to recover what was `EXACT`, `APPROXIMATE`, or `UNSUPPORTED` (and dropped vs defaulted). This is a record of the map, not a sameness certificate.

These are **kept** in the preset record and in a later native DnBOX export of that record. They are **shown** as in §1.4 (origin tag `IMP` on the list; fuller line on footer or hold-help). Old files without them load as `unknown` for the missing fields and do not fail.

### 3A.4 Acceptance (plugin cross-work)

**A-PX1.** A valid Chicago plugin-format file (the format Main Builder identified) can be picked, reported, cancelled with nothing changed, or committed to an imported Chicago slot. The same for Miami, on a Miami part.

**A-PX2.** The 240×240 report appears **before** commit, labels every considered parameter `EXACT` / `APPROXIMATE` / `UNSUPPORTED`, shows dropped vs defaulted for `UNSUPPORTED`, and shows a preset-level summary and the standing “not identical” caveat. No copy says or suggests an identical sound.

**A-PX3.** PATCH cancels with zero mutation. SCALE commits only after the report (and after `O_ASK` if overwrite).

**A-PX4.** Provenance after commit includes source plugin, version if known, and mapping summary, and survives project save/reload.

**A-PX5.** Export to the plugin shows the reverse report before write; cancel writes no file; honesty rules are the same. Chicago velocity-to-cutoff is `UNSUPPORTED` in both directions. Miami is never treated as a 303.

**A-PX6.** Factory plugin-export follows §1.2. User / non-factory-derived plugin export works with the switch OFF.

**A-PX7.** Round-trip **parameter** drift meets X13. Round-trip **render** drift is measured per X14 and is **pending render evidence** until Main Builder or BugBot supplies the package. The UI and the manual still must not claim an identical sound after a clean measurement.

### 3A.5 Tests (BugBot) — plugin cross-work

Run against the format Main Builder identifies (the filled-in `PLUGIN_FORMAT`). If the format is not yet identified, BugBot reports these as **not exercisable** for parse/round-trip, and still checks that the UX copy, cancel rule, honesty language, and Chicago / Miami rules are specified and, where stubbed, do not claim identity. A ship of a parser against a lead (KORG Gadget) that Main Builder has not identified as the actual format fails X10.

**X1. Gestures.** The path uses only the §3 gesture table. K4 still cues a pattern and ends a chain. No new button.

**X2. Cancel import.** Pick a valid file, show the report, PATCH: no new slot, no live-sound change, no dirty, no flash, factory unchanged.

**X3. Commit import.** After the report, SCALE (and `O_ASK` if needed) creates an `IMP` row on the correct engine, assignable. Origin is `imported`. Live assign follows §3.3.

**X4. Labels complete.** Every considered parameter has exactly one of `EXACT`, `APPROXIMATE`, `UNSUPPORTED`. `UNSUPPORTED` states dropped or defaulted. The preset-level summary’s counts match the rows. No parameter is applied but omitted from the report.

**X5. Chicago velocity-to-cutoff in.** A plugin file that contains velocity-to-cutoff, imported onto Chicago: that parameter is `UNSUPPORTED` on the report, never `EXACT` or `APPROXIMATE`. After commit, the Chicago sound has no velocity-to-cutoff. The rest of a valid file can still commit.

**X6. Chicago velocity-to-cutoff out.** Export from Chicago to the plugin format, when that format has a velocity-to-cutoff parameter: it is `UNSUPPORTED` on the reverse report. The written file does not invent a Chicago velocity-to-cutoff value.

**X7. Miami is not a 303.** A Chicago / 303 plugin file onto a Miami part is `WRONG ENGINE`, no slot, no report-as-success. A Miami file onto Chicago is the same. The Miami report’s visible words do not say 303, 303B, or Chicago as the engine identity.

**X8. Honesty language.** A 240×240 snapshot of a report (including an all-`EXACT` map) contains the standing caveat and does **not** contain forbidden sameness wording (§3A.2). `EXACT` as a parameter label is allowed. BugBot uses a forbidden-word check and a human look at the snapshot.

**X9. All-`EXACT` still not identity.** A map with zero `APPROXIMATE` and zero `UNSUPPORTED` still shows the standing caveat. Commit does not toast a sameness claim.

**X10. Format identification.** BugBot records the filled-in plugin and format names. KORG Gadget noted only as a lead, without identification, is not a pass. Chicago and Miami formats are recorded separately if they differ.

**X11. Bad or wrong file.** Garbage or truncated plugin input: `BAD FILE` (or §3.5 equivalent), no mutation, no success report. Wrong engine: X7.

**X12. Provenance kept.** After commit, the record has source plugin, version if known (or `unknown`), and mapping summary. After `SAVED` and reload, those fields are still there (with C7). A native DnBOX export of the record includes them.

**X13. Round-trip parameter drift** (does not itself require audio). Take a Chicago user preset and a Miami user preset. Export to `PLUGIN_FORMAT` (commit the reverse report), import back (commit the import report). For each parameter labelled `EXACT` on both reports: values match (no drift beyond a documented unit conversion that was already `EXACT`). For `APPROXIMATE`: drift is within a tolerance **Main Builder publishes** with the mapping; BugBot checks against that document. For `UNSUPPORTED`: the value stays dropped or defaulted as reported, and is not silently filled with a guessed curve. Repeat in the other direction (plugin file → DnBOX → plugin file) for `EXACT` fields (X15).

**X14. Round-trip render evidence — pending render evidence.** This pass cannot render. Main Builder or BugBot supplies, on the implementation SHA, lossless WAV (or equivalent) for: (A) the original DnBOX user preset, (B) the same preset after export-to-plugin and import-back. Protocol: mid-register note, mid velocity (no velocity-to-cutoff assumption), **4 beats held at 120 BPM** and **one eighth-note** stab, for Chicago **and** Miami; **INFERRED** pitch C2 or C3 for these bass engines, recorded in the package; project FX off, part level unity, no motion (same as §5.3). A comparison sheet measures **drift**, and must not claim an identical sound. Acceptance: if both reports were all-`EXACT`, audible drift is none or below a noise floor Main Builder states and BugBot checks. If any `APPROXIMATE` or `UNSUPPORTED` was in the path, drift may be audible and **must be documented** on the sheet; shipping copy still must not say or suggest identity. BugBot checks independently of Main Builder. A ship whose UI or manual claims “faithful,” “identical,” or equivalent, with or without this package, fails. Plugin cross-work may ship with honest labels and X13 green while X14 is still pending, **only** if no sameness language is used.

**X15. Plugin-file round-trip.** Plugin file → import (commit) → export (commit): `EXACT` fields in the plugin file match within the same rule as X13. Honesty report is shown on both legs.

**X16. Factory gate.** Plugin-format export of a factory (or, while the temporary §1.2 rule stands, factory-derived) preset is refused when the switch is OFF (no file, explicit refusal). User-authored, not factory-derived, plugin export succeeds with the switch OFF.

**X17. Live play.** Opening and scrolling the report during play does not glitch, does not flash, and does not steal keys (§4, L1, L8, L9). Commit of an import while playing queues assign and defers flash.

**X18. Overwrite.** Commit into an occupied slot still asks (§3.4). Cancel at that ask writes nothing.

---

## 4. Live-play rules

1. **Deferred flash writes.** Flash does not erase during play, record, sounding notes, clip activity, or `ui_boot_flash_busy` (map §4.5, §7; plan CPU). The existing `save_wait` continuation (8783–8789) is the model. Preset work does not add a second flash path that writes during play.
2. **No audio glitch.** Assign, queue, cancel, import-validate, and list navigation must not click, drop, or stall the audio stream. Oct 25’s 64-frame de-click is a **proposal**, not a measured requirement (**INFERRED**). Acceptance is audible and measurable: no glitch on assign at a note onset, mid-note, or on a tie/slide. If a de-click is implemented, Main Builder records the actual method.
3. **No CPU regression.** PERF TEST `FACTORY LOOP`, `ALL FIVE`, and `WORST CASE` (plan CPU; `ui.c` ~2302–2303) do not regress against the pre-change baseline on the same hardware. The test restores the project and does not write it. No allocation in the audio path. Validation and file parse run outside the audio path.
4. **Held notes.** A note already sounding keeps the **previous** snapshot until it is released. The new preset takes effect on the next eligible **attack**. Do not choke the held note to apply the new sound. Do not send All Notes Off to other parts.
5. **Running patterns.** The pattern continues. Steps already sounding keep the previous snapshot. New attacks use the new preset. **P-locks still apply** on top of the assigned preset. Preset assign does not write, clear, or bypass p-locks.
6. **Ties / slides / waiting.** Do not break a Chicago tie or slide to force the new sound. Wait for an eligible attack. Show pending with existing words (`WAIT FOR NOTES` / kin). PATCH cancels the pending assign.
7. **Chain is not saved.** Assign, capture, import, and export do not write the chain, and do not start or end a chain. K4 still ends a chain and is not used here. `CHN` still appears only when a chain is actually set (plan §5).
8. **Live-note ownership.** `ui_live_key_owner` (4279–4297) and `live_boundary` (4358–4376) are unchanged. Opening the preset list on PARA must not steal keys (a settings list or HELP on SEQ/PARA already does not, 4282–4283). A context change that already cancels live notes still does; a mere list move must not.
9. **Preview / navigation** does not write steps, motion, or the chain. Wheels still bend / open without writing saved pots (map §1.4), unless the existing REC-motion path applies — do not extend that to “browsing presets.”

### 4.1 Tests (BugBot) — live play

**L1. No flash during play.** Request save / import-persist / any new flush while playing. Flash erase does not start until stopped and quiet. Logic analyser, existing save-wait flags, or the host equivalent Main Builder documents — BugBot records the method. Audio continues.

**L2. Held note.** Hold a Juno (and a Drone, and a Chicago) note; assign a different preset of the same engine; the held pitch and timbre stay on the old snapshot until release; the next attack is the new preset.

**L3. Running pattern.** A playing pattern is not interrupted. The playhead continues. Mid-note steps finish on the old snapshot. Subsequent attacks use the new preset. P-locks on a step still change the locked parameters.

**L4. Tie / slide (Chicago).** A pending assign during a tied or sliding run does not break the tie/slide. Pending is visible. PATCH cancels.

**L5. Other parts.** Assigning on Juno does not send All Notes Off to 909 / 808 / BREAK / Chicago / Miami (and the same for a Drone assign).

**L6. Chain.** Set a chain; assign and capture; stop and save; reload: the chain is not in the save (existing product rule, still true). K4 still ends the live chain.

**L7. CPU.** PERF TEST `FACTORY LOOP`, `ALL FIVE`, `WORST CASE` are no-worse than the baseline at this work’s parent SHA, on the same hardware. Project is restored and not written.

**L8. Glitch.** Host or device recording of assign-at-onset, assign-mid-note, and cancel-pending: no audible click or dropout that is new versus the baseline. BugBot files the recordings with the report.

**L9. Live keys.** On PARA with the preset list open, 303 / Juno / Drone live keys still play when `ui_live_key_owner` says they should. Shortcuts in the plan preserve-list still fire. `RELEASE STEP FIRST` still holds.

---

## 5. Curated JUN-60 (Juno) and Drone factory roles (APPEND-ONLY)

### 5.1 Index rule

The existing **4 Juno** and **4 Drone** factory presets (Dot’s inventory at `94197fc8`) occupy indices **0–3**. Those IDs **keep their meaning**. This work is **APPEND-ONLY** after those four:

- Do not rename, reorder, retune, replace, or delete indices 0–3.
- New roles, if any ship, start at index **4** and increase.
- Old projects that point at 0–3 (or that embed parameters from those sounds) load unchanged (§6).
- **No padded counts.** The curated list below is **role intent**, in recommended append order, not a mandated head-count, and not a target to fill for its own sake. The **final number is decided after the flash-reserve measurement and after render evidence** (§9A). Zero new slots is allowed. A subset is allowed. If an existing 0–3 index already fulfils a role (evidence in §5.3), **do not append a duplicate**. Do not add filler presets that do not earn a distinct live-play role.

### 5.2 Roles — intent only, pending render evidence

**Every role in this section is intent only, pending render evidence.** This pass cannot render (no builds or execution). Each **new** role that ships is **original or rights-verified** and carries a complete provenance record (§1.3). No parameter values are specified here. No role for any engine may assume Chicago velocity-to-cutoff (Chicago does not have it); JUN-60 and Drone roles below do not rely on it either. On-screen engine name remains **JUNO** (plan Naming for Docs Keeper). **JUN-60** in this section means Juno-60-style with chorus.

**JUN-60** — varied and musically useful for live play. Recommended append order after index 3. This is a menu of distinct intents to choose from after flash and evidence, **not** a promise to ship every row:

| Role id | Role | Parameter intent |
|---|---|---|
| J-PAD | Chorus pad | Sustained pad, moderate attack, chorus **on**, mid filter, release long enough to hold a chord. A live bed you can play with both hands. |
| J-SOFT | Slow swell pad | Clearly slower attack than J-PAD, long rise into a held chord, chorus on; a pad that arrives, not a pluck. |
| J-STR | Strings | Slower attack than a stab, chorus on, ensemble-like sustain, mid-to-upper mid; bowed-ish, not brass. |
| J-BRASS | Brass | Faster attack than the pads, brighter, chorus on, filter-envelope snap. |
| J-STAB | Poly stab | Short gate, bright attack, short release; polyphonic chops and hits, not a held pad. |
| J-BASS | Bass | Low register, tighter envelope than the pads, chorus present but not washing out the low end. |
| J-LEAD | Poly lead | Mid register, clearer than the pads, shorter release than J-PAD, chorus on; a line you can play. |
| J-PLUCK | Arp-friendly pluck | Fast attack, short decay and release, tight and articulate so a running arpeggio or a fast right-hand pattern stays distinct; chorus as seasoning, not a smear. |
| J-CHO | Chorus-defined lush texture | Chorus is the **identity** of the sound (a held chord without that chorus character would fail the role); moderate-to-slow attack, lush mid, for live held textures. Distinct from J-PAD by how clearly chorus, not just filter, defines it. |
| J-CHO-HI | Chorus-defined bright texture | Chorus is again the identity, but upper mid / higher than J-CHO, brighter, still a texture (not a lead line and not a pluck). |
| J-ORG | Organ-like | Even, mid, sustained, less filter-envelope snap than J-BRASS; chorus on; useful for held live chords with a clearer keyboard-organ speak than a string pad. |

**Drone** (String Machine) — recommended append order after index 3:

| Role id | Role | Parameter intent |
|---|---|---|
| D-DRONE | Sustained drone | Very slow attack, long hold, ensemble / string-machine character, little sharp transient. |
| D-ENS | Ensemble strings | Bowed-ish attack, mid register, ensemble, playable as a chord bed. |
| D-AIR | High shimmer | Upper register, brighter, airy; still a string-machine, not a lead synth. |
| D-WASH | Dark wash | Low, slow, muted, long; a bed under other parts. |

Existing engines (Chicago 81, Miami 30, 909 / 808 / BREAK) are **not** appended in this work. Brief intent for them is in the considerations file §3.2 only.

### 5.3 Render evidence Main Builder or BugBot must supply

This document’s author cannot render. For **each shipped new role** (each index ≥ 4 that is claimed to fulfil a role above), and for **each existing index 0–3** (documentation only — **no retune**), an evidence package is required before a sound-character claim is accepted.

**Who supplies, who checks.** Main Builder supplies the package (or a designated render on the implementation SHA). BugBot independently checks it against the role intent and against the acceptance rows below. A package that only the author has heard is not evidence.

**Package contents:**

1. **Identity:** engine, factory index, display name, role id (or “unmapped” for an existing 0–3 that does not match a curated role), firmware SHA, provenance record id (§1.3) for index ≥ 4.
2. **Renders,** lossless WAV (or equivalent), host or device, same SHA:
   - **Held:** one mid-register note, held **4 beats at 120 BPM**. Pitch: Juno **C4**; Drone **C3** (Drone / String Machine sits lower; if C3 is out of range, the nearest in-range C, and record which). Velocity mid (do not choose a velocity that assumes cutoff tracking).
   - **Stab:** the same pitch, a short gate (**one eighth-note** at 120 BPM), same velocity.
   - **Register check** for J-BASS, D-WASH (an octave below the mid note) and for J-LEAD, J-CHO-HI, D-AIR (an octave above), same two lengths, if the role cares about register.
   - **JUN-60 chorus:** for every JUN-60 role that says chorus on, the held render must make chorus audible as part of the character. Do not claim chorus from a parameter dump alone. For **chorus-defined** roles (J-CHO, J-CHO-HI), chorus must be the defining character, not a mild extra — BugBot’s listen treats that as a pass/fail bullet.
   - **Arp-friendly (J-PLUCK) extra:** a short run of **eighth-notes** at 120 BPM (at least one bar), same mid pitch class, mid velocity. Notes must stay articulate (not smear into a pad).
3. **Comparison sheet,** one line per intent bullet in §5.2, **pass / fail** against the recording (attack character, sustain / release, register, chorus or ensemble, brightness, “usable as a stab / bed / pad” as the role states).
4. **Method note:** host or device, and any limiter / FX that was in the path. The factory sound is heard **without** a project FX chain that would disguise it (**INFERRED:** delay/reverb off, part level unity, no motion).

**Acceptance for each role** (BugBot):

| Role | Passes when the recordings show, at least |
|---|---|
| J-PAD | Sustained chord-length hold, moderate (not plucked, not a slow swell) attack, chorus audible, mid (not bass-only, not ice-lead) filter character. |
| J-SOFT | Clearly slower attack than the J-PAD evidence, a swell into a held chord, chorus on; the eighth-note need not speak as a stab. |
| J-STR | Slower attack than J-STAB, chorus on, sustained, ensemble-like; not a brass snap. |
| J-BRASS | Faster attack than J-PAD evidence, brighter, chorus on, a snap of filter envelope on the stab render. |
| J-STAB | Eighth-note is short and bright; polyphonic-stab useful; long-hold is not required to be a pad. |
| J-BASS | Low register (octave-down render is the usable one), tighter than J-PAD, chorus not erasing the low end. |
| J-LEAD | Mid register, shorter release than J-PAD, clearer, chorus on; the eighth-note ends sooner than the pad’s; playable as a line. |
| J-PLUCK | Fast attack, short tail; the eighth-note run stays articulate, not smeared; chorus does not wash the notes together. |
| J-CHO | Chorus is the defining character on the held render (not merely audible); lush mid texture, not a pluck and not a brass snap. |
| J-CHO-HI | Chorus is the defining character; brighter / higher than J-CHO evidence; still a texture, not J-LEAD and not J-PLUCK. |
| J-ORG | Even mid sustain, less snap than J-BRASS, chorus on; speaks more like an organ bed than strings. |
| D-DRONE | Very slow attack, long hold, ensemble / string-machine, little sharp transient on the stab (the stab may simply be a truncated swell). |
| D-ENS | Bowed-ish (faster than D-DRONE) attack, mid register, ensemble, holds as a chord bed. |
| D-AIR | Upper register (octave-up render), brighter, still a string-machine. |
| D-WASH | Low, slow, muted, long; a bed, not a lead. |

**Fail / consequences.** If any required bullet fails, the preset is **not** accepted as that role. Main Builder may retune **only** an unshipped append (an index not yet in a released bank). **Never retune indices 0–3** to chase a role. An existing 0–3 that fails a curated role is documented as whatever it actually is, and a new append may take the role (if flash allows). A role without a complete evidence package remains **intent only, pending render evidence**, and must not be claimed as shipping character in the manual or in release notes.

**Existing 0–3.** The same package is required as **documentation**, so appends do not duplicate them. Those packages do not authorise retune.

### 5.4 Tests (BugBot) — roles and evidence

**R1. Append-only.** After the change, Juno and Drone indices 0–3 are bitwise-identical (or parameter-identical) to `94197fc8`. A project saved at `94197fc8` that selects Juno 0–3 or Drone 0–3 loads and sounds the same (C1–C2). New presets, if any, are at 4+.

**R2. Evidence complete.** Every shipped index ≥ 4 that is named as a §5.2 role has a complete §5.3 package. Missing WAV, missing SHA, or missing comparison sheet fails.

**R3. Evidence independently checked.** BugBot’s pass/fail on the comparison sheet is recorded separately from Main Builder’s. A Main-Builder-only “pass” is not enough.

**R4. Role acceptance.** Each shipped role meets the acceptance row in §5.3. A fail drops the claim, per §5.3 consequences.

**R5. No duplicate.** If an existing 0–3 package already passes a curated role, no new append is shipped for that same role on that engine.

**R6. Provenance on appends.** R2 includes P9–P11. An append without a complete original or rights-verified record does not ship.

**R7. Chicago velocity.** No Juno, Drone, or (if touched) Chicago preset in this work is documented as depending on velocity-to-cutoff.

**R8. Zero appends is a valid ship** if flash forbids them or if 0–3 already cover the needed roles. In that case R2–R6 apply only to documentation of 0–3, and the release does not claim new JUN-60/Drone factory roles. Shipping a **subset** of §5.2 is valid. Shipping every row is not required. Padding the bank to a round number, or shipping two appends that pass the same role, fails.

**R9. JUN-60 live variety (if more than one new role ships).** Shipped new JUN-60 roles are distinct under §5.3 (BugBot can tell the evidence packages apart on attack, register, chorus role, or length). Two nearly identical pads do not count as two roles.

---

## 6. Compatibility and migration

1. **Old projects load unchanged.** A project saved at `94197fc8` (and earlier projects that already load on that SHA) opens on the new firmware with the same pattern data, the same embedded sound parameters, the same factory indices, and the same audible result for those parts. No silent migration that retunes a sound. The project is not dirtied on load solely to add a missing provenance field (§1.4).
2. **Old preset files load unchanged.** A file that this firmware already understood still loads. If it has no provenance field, origin is `unknown` (P12). It does not fail for that reason.
3. **Factory indices are stable.** Chicago 0–80, Miami 0–29, Juno 0–3, Drone 0–3 stay those sounds. Appends only at Juno/Drone 4+. A later factory-bank update does not rewrite embedded parameters in an old project (Oct 25: pots / embedded parameters are authoritative).
4. **Versioning.**
   - Exported preset files carry a schema version and an engine id.
   - This firmware reads older known versions.
   - A **newer-than-known** file or stream is refused with `NEWER DATA` language (`NEWER DATA: LOAD BLOCKED` / `NEWER DATA: NOT SAVED` as appropriate). No partial load, no silent drop of fields, no silent relabel of engine.
   - Streams **13 / 14 / 15** are not casually changed (plan Constraints). If a working user bank needs storage, Main Builder versions it so: (a) old projects without a user bank still load (empty user section, factory and embedded sounds as before; Oct 25 “handle 0 / inherit” is the idea already partly in stream 14 — do **not** blindly add a `stream4`); (b) older firmware that does not know a new object reports `NEWER DATA` rather than corrupting or partially applying it; (c) a measured flash reserve exists before a new object is sized.
5. **Import `-1`.** Replaced by §3.5. Old **invalid** files still fail, visibly (`BAD FILE` / `WRONG ENGINE` / `NEWER DATA`), without mutating the live project.
6. **Miami** remains not a 303. Display names remain CHICAGO / MIAMI / JUNO / DRONE.
7. **Factory-asset rights** do not change load compatibility. UNRESOLVED export does not block loading old projects.

### 6.1 Tests (BugBot) — compatibility

**C1. Old project load.** A fixture project from `94197fc8` (at least: Chicago using a mid-bank factory index, Miami, Juno 0–3, Drone 0–3, a pattern with p-locks, a dirty-then-saved project) loads. Embedded parameters match. Audible result matches a baseline render or a parameter dump. Dirty is not set solely by the load.

**C2. Index stability.** After appends, Juno 2 is the same sound as Juno 2 at `94197fc8`. The same for Drone 2, Chicago 0 and 80, Miami 0 and 29.

**C3. Old preset file.** A pre-provenance file loads as `unknown`, assignable, no failure toast.

**C4. Newer file.** A file with a future schema version is refused (`NEWER DATA`); live project unchanged.

**C5. Streams.** Projects at streams 13 / 14 / 15 still load. A newer-than-known stream still reports `NEWER DATA` and does not overwrite. BugBot records whether a new object was added and how old firmware is protected.

**C6. P-locks and pattern handles.** Pattern bass-preset handles already in stream 14 (plan) still resolve. P-locks still apply after a preset assign (L3).

**C7. Round-trip.** Capture a user preset, save the project (`SAVED`), reload (`SAVED PROJECT LOADED`): the user preset, its provenance, and the assignment are back. Chain is still not in the file.

**C8. Earlier firmware.** If a new stream or object exists, an older firmware image that does not know it does not corrupt the project; `NEWER DATA` (or the existing refusal) is the behaviour. If this cannot be run, BugBot marks it UNVERIFIABLE and states why, but Main Builder must still design for it.

---

## 7. Implementation order (Main Builder)

Recommended increments. Each is shippable only with its tests green. Flash measurement is first because it blocks counts.

0. Measure flash reserve at the implementation SHA; report the number. Do not reuse Oct 25 / PR188 figures.
1. Write the factory-asset **trace** (§1.1). Keep the factory-export switch at the temporary default OFF until the trace is done.
2. Document existing factory names and indices 0–3 (Juno, Drone) and the 81 / 30 (Chicago, Miami) from Dot’s inventory. No reorder, no retune. Supply §5.3 documentation renders for Juno/Drone 0–3.
3. **Import:** replace `-1` with §3.5, all engines, error states, provenance on success, P12 for old files.
4. **User bank:** capture, confirm-before-overwrite, undo, deferred persist with the existing save words, origin tags, provenance field.
5. Tell-apart UI (§2), aligned with the directions by name.
6. **Export** for user and imported (§3.6, §1.2). Factory path present but compiled behind the single switch.
7. **Append** original or rights-verified JUN-60 / Drone roles only after: flash budget, provenance records (§1.3), and render evidence (§5.3) accepted by BugBot. Final **count** waits on §9A — no padded counts.
8. **Plugin cross-work (§3A).** After Main Builder identifies the actual source plugin(s) and format(s) (KORG Gadget is a lead only), implement pick-file → mapping report → cancel or commit, reverse report on export, provenance, and X1–X18. X14 remains pending render evidence. Do not ship sameness language.

Do not ship a public build with the factory-export switch ON before step 1 is done. Do not claim new factory character in the manual before step 7’s evidence is accepted. Do not claim plugin-path identity of sound at any step.

---

## 8. Acceptance criteria (summary for BugBot)

The work is accepted when all of the following are true. Individual tests are in §1.5 (P), §2.3 (D), §3.7 (F), §3A.5 (X), §4.1 (L), §5.4 (R), §6.1 (C).

**A1. Distinction.** Factory, user, imported, and unknown are told apart on the 240×240 list and in naming / ordering (§2). D1–D5 pass. Visual states use Direction **selected / playing / queued** and Direction **sound vs step** by name, not a forked look.

**A2. Flows.** Save, rename (per the §9A working assumption until that list is resolved), load, import, and export use only the existing gesture table. Save feedback uses `SAVES WHEN STOPPED` / `SAVE AT STOP` / `SAVED` (and the existing failure words). Overwrite always confirms or is undoable — this spec requires **confirm before**, **undo after**. Never a silent replace. Import errors `WRONG ENGINE`, `BAD FILE`, `MEMORY FULL`, and `NEWER DATA` are visible and mutation-free. F1–F16 pass. Import `-1` is gone for valid native files.

**A3. Live play.** Deferred flash; no glitch; no PERF TEST regression; held notes keep the old snapshot; running patterns continue; p-locks remain; chain is not saved; live-note and shortcuts are preserved. L1–L9 pass.

**A4. Roles.** JUN-60 / Drone append-only after the existing four, which keep their meaning. Each shipped new role is original or rights-verified, has a complete provenance record, and is **intent only** until §5.3 evidence is accepted. No padded counts; final number is after flash and evidence (§9A). R1–R9 pass. No Chicago velocity-to-cutoff dependence.

**A5. Compatibility.** Old projects and old preset files load unchanged. Versioning and `NEWER DATA` hold. Streams 13 / 14 / 15 are not casually broken. C1–C8 pass.

**A6. Provenance.** P1–P14 pass. Factory export is UNRESOLVED until the trace. User and imported export work. The switch default while pending is the temporary engineering OFF, recorded as pending the trace, not as a rights finding. Old files without the field load as `unknown` and do not fail.

**A7. Muscle memory.** No remaps. K4 is still pattern cue. PATCH / BT / PARA / NOTE REPEAT / KNOB BANK meanings from the decision record stand. Nothing removed without the owner’s words.

**A8. Open owner display decisions** (plan 3 and 4: SOUND header, save chip) do not block the semantics above. When those directions land, Main Builder follows them. Until then, reference them by name. Project vs device bank, imported placement, rename in v1, and role counts are **not** owner options now; they are §9A.

**A9. Plugin cross-work.** A-PX1–A-PX7 and X1–X18. Chicago and Miami import from and export to the actual source plugins (format Main Builder identifies; KORG Gadget is a lead only). Mapping report before commit; `EXACT` / `APPROXIMATE` / `UNSUPPORTED`; standing “not identical” caveat; cancel mutates nothing; provenance kept; reverse report on export; Chicago velocity-to-cutoff `UNSUPPORTED`; Miami is not a 303. Round-trip render drift (X14) is pending render evidence and must not be replaced by a sameness claim.

---

## 9. OWNER DECISIONS still open

Closed already (plan decision record, 2026-11-13 05:20 America/Chicago; Dot via Orchestrator; **not** owner quotes): PATCH HOME↔SONG kept; NOTE REPEAT unchanged; KNOB BANK single press; BT 250/400; SEQ word kept; remove nothing; Docs Keeper updates the manual against `94197fc8`.

Still open as **owner display / rights** items (not moved):

1. Save-chip display (plan 4) — Options A / B in §3.1.7. Semantics already fixed.
2. PARA SOUND header (plan 3) — preset list follows this direction; header implementation stays on hold per that record.
3. Factory-export switch **after** the Main Builder trace — not asked of the owner until the trace exists. Recorded in §1.2 as pending.

First-sound footer (plan 2) does not block this work.

Project vs device bank, imported-section placement, rename in v1, and role counts are **not** owner options now. They are deferred in §9A.

## 9A. Deferred until flash + render evidence

These are decided **after** Main Builder measures flash reserve at the implementation SHA and after the relevant render-evidence packages exist (§5.3, and X14 where plugin drift is in question). They are not on the owner-options list in §9. Until then, Main Builder implements the **working assumptions** below so earlier increments can proceed. Do not treat a working assumption as a final count or a final product pick.

| Item | Options | Working assumption until then |
|---|---|---|
| User-bank scope | Project-scoped, flushed with the existing BT / deferred project save; or a device-global user bank | Project-scoped (first increment). A device-global bank, if chosen later, still needs the flash measurement and does not shrink the reserve without review. |
| Imported-section placement | A third list section after user; or imported rows inside USER with an `IMP` tag | Three sections (`FACTORY` / `USER` / `IMPORTED`). Origin tags are required either way (§2). |
| Rename in v1 | Automatic `USER 01`… names only; or a list-based rename on existing gestures | Automatic `USER 01`…. Factory names are never renamed. |
| Role counts | How many JUN-60 and Drone roles to append, including zero and including any subset of §5.2 | **No padded counts.** No number is committed in this spec. Ship only roles that have flash budget, a complete provenance record, and accepted §5.3 evidence, without duplicating indices 0–3. |

Flash measurement remains a blocker for any new slot count (§0). Render evidence remains a blocker for any shipping character claim (§5.3) and for plugin round-trip audible-drift claims (X14).

---

## 10. INFERRED log

Items marked **INFERRED** in the body, collected:

- Temporary engineering default OFF for `DNBOX_FACTORY_PRESET_EXPORT` (name of the symbol too), not a rights finding.
- Factory-derived copy-on-write captures and re-imports of factory material are gated with factory export until the trace narrows or widens that set.
- Refusal toast `FACTORY EXPORT OFF`; import/export short copy `WRONG ENGINE`, `BAD FILE`, `MEMORY FULL`, `EXPORT FAILED`, `REPLACE USER PRESET?`.
- §9A working assumptions: project-scoped user bank; three list sections; automatic `USER 01` names. RAM capture is not the word `SAVED`.
- `FAC` / `IMP` abbreviations; factory index visible as `F04` if it fits.
- Full provenance line on a selected-row footer or existing hold-help, not a new page. Imported display-name fallback `IMP 01`.
- Oct 25 64-frame de-click is a proposal, not a measured figure.
- Mid-note pitches C4 (JUN-60) and C3 (Drone) for role evidence; C2 or C3 for Chicago/Miami plugin-drift renders; FX off during those renders.
- Dirty-on-assign follows the product’s existing sound-edit dirty rule, not a new policy.
- Plugin format placeholder name `PLUGIN_FORMAT`; mapping-report layout and standing copy `MAPPED, NOT IDENTICAL`; footer `SCALE: COMMIT  PATCH: CANCEL`.
- Main Builder–published tolerances and noise floors for X13 / X14.

---

## 11. What this spec does not do

It does not grant factory-export rights. It does not prove the identity of the 81 / 30 at `94197fc8`. It does not identify the actual source plugins or formats (Main Builder does; KORG Gadget is a lead only). It does not invent flash capacities, parameter values, or a padded role count. It does not remap controls. It does not ask Main Builder to retune Juno/Drone 0–3. It does not append Chicago or Miami factory slots. It does not change chain-not-saved, live-note ownership, or streams 13 / 14 / 15 as a casual side effect. It does not render audio — role character and plugin round-trip audible drift stay **pending render evidence** until Main Builder or BugBot supplies §5.3 and X14. It does not claim that a mapped plugin preset is an identical sound.
