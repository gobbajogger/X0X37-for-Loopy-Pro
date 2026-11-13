# Sample, project and engine-preset import/export — status

Read-only audit for the owner's question: "what's about sample, project, engine presets import/export".

Dated 2026-11-13, Asia/Vladivostok (UTC+10). No builds were run. Nothing was invented; every claim cites a file at a named commit, a GitHub URL, or a live page fetched this turn.

## Sources used

| Source | Head / identity | Notes |
| --- | --- | --- |
| `lane/026-combined-20261112` (0.0.26 combined source) | [`94197fc839b00aff47b98ca02f3c6396e4a6eabf`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/94197fc839b00aff47b98ca02f3c6396e4a6eabf) (GitHub committer 2026-11-13 17:27 UTC+10) | Preferred ROADMAP and firmware/docs checkout. `docs/ROADMAP.md` at this SHA still says, at line 200, "No 0.0.26 code is written yet" (status line dated 2026-11-12 12:50 UTC+10); that sentence is stale relative to this later combined head. |
| `lane/fmb-presets-format` | [`83e7f92d31112b58cffe5ae21be92dc7168e9202`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/83e7f92d31112b58cffe5ae21be92dc7168e9202) (GitHub committer 2026-11-13 22:40 UTC+10) | 8 commits ahead of `94197fc8`. No open or closed PR for this branch (`gh pr list --head lane/fmb-presets-format` returned `[]`). |
| `main` | [`035026fe70059af35e25c7c121b40f318b732fe3`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/035026fe70059af35e25c7c121b40f318b732fe3) | Public README only, for visitor-facing backup/importer links. Not used as the firmware head. |
| Production Pages | branch `gh-pages` tip [`e79801dfa`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/e79801dfa5f895eb3c8d76579cedad371e1f179e), 2026-10-25 23:55 UTC+10 | [breaks](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/breaks/), [projects](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/projects/), [install](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/install/). Older than `94197fc8` and than PRs #871/#875/#876. |
| Nightly Pages | `gobbajogger/x0x37-for-Loopy-Pro_nightly` `gh-pages` tip [`6a9953dee`](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/commit/6a9953dee4d31ac741ae90ec4bec70f2a10f9240), 2026-11-13 20:31 UTC+10 | [breaks](https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/breaks/), [projects](https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/projects/), [install](https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/). |

Firmware version: `firmware/src/app/version.h` at `94197fc8` defaults `X0X_VERSION` to `"DEV"`; `tools/build.py` sets a numeric `X.Y.Z` for release and nightly packages. `docs/FEATURE-REGISTRY.md` at the same SHA refers to USB names `DnBOX 0.0.26 37PRO/37ELITE` for an earlier published 0.0.26 source `600b3b45f8`. This audit treats `94197fc8` as the current 0.0.26 combined head, not as a packaged release identity.

## Short status

| Area | In `94197fc8` / 0.0.26 combined | Branch / PR | ROADMAP | Missing |
| --- | --- | --- | --- | --- |
| (a) Sample import/export (BREAK user slots, `.x0xslots`) | **Shipped** in firmware + web companion + CLI. Installer also backs up/restores slots around an update. | Later integrity/rate fixes already merged into this head: [#875](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/875), [#876](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/876). | Product goal 3, and Done steps 1/3/4; third slot marked DONE. | No USB mass-storage disk. |
| (b) Project import/export or backup (`.x0xproject`, on-keyboard SAVE/LOAD/RESET) | **Shipped** in firmware SysEx + Projects companion + on-device GLO actions. Format-2 backup fix [#871](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/871) is an ancestor of `94197fc8`. | — | Not an open 0.0.26 item; portable files are documented as current behavior, not as leftover work. | No USB mass-storage disk. Sample writes during import are not power-failure atomic. |
| (c) Engine/sound preset import/export | On-device **factory** preset recall is shipped (Chicago 81 / Miami 30). A writable user-bank import API exists but **returns -1** (stub). No user-preset file or SysEx I/O in the default image. | Codec + host tests + measurement on `lane/fmb-presets-format` @ `83e7f92d`. **No PR.** Default (stage 0) image does not link the codec. Device STAGE=4 cost **+2,164 B**, which breaks the 8,192 B flash-free floor; waits on Optimizer flash savings. | No ROADMAP row for user-preset file import/export (0.0.26 list is 26.1–26.7). | UI, live bank wiring, web companion, plugin parsing, and a factory-rights trace are explicitly deferred. |

Transfer on the keyboard is USB MIDI SysEx (`F0 7D 46 4C … F7`), not a USB disk. A `git grep` at `94197fc8` over `firmware/` for mass-storage / USB MSC / `usb_msc` returned no matches.

---

## (a) Sample import/export

**Status: shipped in `94197fc8`.** End users import their own BREAK loops into three user sample slots, back them up as `.x0xslots`, and restore them. Transport is the existing sample SysEx commands `SMP_BEGIN`…`SMP_INFO` / `SMP_READ` (`ED_SMP_BEGIN` = 11 through `ED_SMP_INFO` / `ED_SMP_READ`) in `firmware/src/app/editor.c` at `94197fc8` lines 6–8, 19, 913–992.

### ROADMAP

From `docs/ROADMAP.md` at `94197fc8`:

- Line 11, product goal 3: "End users can import their own loops for BREAK mode, and the loops stay safe across updates."
- Line 23, Done Step 1: "The installer backs up the loop slots and restores them automatically".
- Line 25, Done Step 3: "Factory breaks: 2 bars at 44.1 kHz. The import tools use 44.1 kHz".
- Line 26, Done Step 4: "New flash layout: one 64 KiB Bluetooth block, 3 loop slots (about 11 s of import space at 44.1 kHz)".
- Line 178, 0.0.25 real-build row "Third imported-loop sample slot": "DONE. Owner 2026-11-12 03:23 UTC+10: restore it; … slot 3 is 0xB3000-0xBAFFF, 32 KiB … Slots 1 and 2 do not move. Installers keep slots 1 and 2 and back up slot 3 on a downgrade."

0.0.26 rows 26.1–26.7 (`docs/ROADMAP.md` lines 198–210) do not add a new sample-import task.

### Firmware and behavior at `94197fc8`

`BEHAVIOR.md`:

- Lines 158–169, BRK-20–BRK-29: imported samples play only in BREAK mode; user loops appear as BR1, BR2, …; default 44.1 kHz IMA ADPCM; 22.05 kHz is an opt-in; the slot header carries the rate.
- Line 515, FLS-02: SMK-37 has 3 user slots — slot 1 80 KiB at 0xC2000, slot 2 120 KiB at 0xD6000, slot 3 32 KiB at 0xB3000.
- Line 516, FLS-03: "Keep the imported-sample flash of the slots. A space saving must not shrink slot 1, slot 2 or slot 3."
- Lines 523–525, FLS-10–FLS-11: `tools/x0x_slots.py backup` and the break page's "Back up all slots" are read-only (`SMP_INFO` 15, `SMP_READ` 16) into `.x0xslots`; restore writes with `SMP_BEGIN` / `SMP_WRITE` / `SMP_END` or `SMP_ERASE`, then reads back.
- Lines 532–537, FLS-16: if an install would move, shrink or drop a slot, the web installer and `tools/fm1_install.py` back up every slot to `.x0xslots` and restore afterwards.

`d6d35c12d` ("fix(samples): preserve import data across retries and rejected commits") is in this head via merge [#875](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/875) (`d4088e606`, also [#876](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/876) host rate match), merged 2026-11-13 16:51 UTC+10 into `lane/026-combined-20261112`.

### Web installer / nightly site

The site generator at `tools/make_pages.py` lines 6–9 and 207–216 (commit `94197fc8`) emits `breaks/index.html` from `web/x0x_breaks.html` ("sample importer for break loops") and copies the Projects page beside it.

Live production [breaks page](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/breaks/) (fetched 2026-11-13): title "X0X break loops", heading "Import samples"; the HTML mentions Back up, Restore, Upload, Download, slots, WAV and `.x0xslots`. The production [install](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/install/) page also mentions backup, `.x0xslots`, Restore, sample slots and Projects.

Nightly [breaks](https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/breaks/) (fetched the same turn): title "DnBOX break loops", heading "Import samples". Nightly [install](https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/) likewise mentions backup, `.x0xslots`, Restore, sample slots, Projects and "Import samples".

`web/MANUAL.md` at `94197fc8` (sections "Back up and install", "Manage samples and backups") describes the installer snapshot of every slot, the requirement to re-select the saved `.x0xslots` file before firmware writing, post-OTA restore and readback, and "Back up all slots" / Restore in the Samples workspace.

`docs/MANUAL.md` at `94197fc8` lines 401–430 ("Your own breaks") documents the importer, 44.1 kHz default, SMK-37 slot sizes, "Back up all slots", Restore, and `tools/upload_breaks.py` / `tools/x0x_slots.py`. That section still links the older URL `https://charlesvestal.github.io/fm1-x0x/breaks/`. The `94197fc8` README line 19 and `main` README point at `https://gobbajogger.github.io/X0X37-for-Loopy-Pro/breaks/`.

### CLI

`README.md` at `94197fc8` lines 66–70 and `docs/MANUAL.md` lines 421–430: `tools/upload_breaks.py` and `tools/x0x_slots.py backup` / `restore`.

### Blockers / caveats

- Slot flash must not be shrunk (FLS-03; flash/pool floors stay 8,192 B).
- Sample flash replacement is not power-failure atomic (`BEHAVIOR.md` SET-12; `docs/PROJECT_FILES.md`). A mid-write unplug can need the recovery `.x0xslots` / portable project backup.
- Production Pages last moved 2026-10-25, before this 0.0.26 head and before #875/#876. The firmware protocol is in `94197fc8`; the production page is an older snapshot. Nightly Pages were updated 2026-11-13.
- No USB mass storage (see Sources / Short status).
- Recorded user breaks "belong to whoever made them and aren't covered by this project's licence" (`README.md` at `94197fc8` line 73; same sentence on `main`).

---

## (b) Project import/export or backup

**Status: shipped in `94197fc8`.** Two layers exist:

1. **On the keyboard:** GLO SAVE / AUTOSAVE / LOAD SAVED / RESET PROJECT store or replace the live project in device flash and leave sample slots in place (`BEHAVIOR.md` SET-02, SET-04, SET-07, SET-10 at lines 234–244).
2. **Portable files:** the Projects companion exports and imports `.x0xproject` over the same USB MIDI SysEx editor, command `ED_PROJECT` = 9, with operations 64–74 (`docs/PROJECT_FILES.md`; `firmware/src/app/editor.c` lines 19, 170–328, 871). A portable file can include a nested `.x0xslots` archive of every populated user slot.

[#871](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/871) "Fix format-2 project backup and restore compatibility" merged into `lane/026-combined-20261112` at `0af6d8d59` (2026-11-12 23:43 UTC+10) and is an ancestor of `94197fc8`.

### ROADMAP

There is no open 0.0.26 ROADMAP row for project-file import/export. The closest shipped notes are goal 3 / Step 1 (loop-slot safety across updates, quoted above) and the 0.0.25 rows that already talk about project stream formats (for example line 163, per-pattern preset recall: "a project without it saves as 13 … a rev-13 project loads with these bytes 0"; line 172, format 2 for RATE_8 / RATE_4). Portable `.x0xproject` I/O is specified as current behavior in `BEHAVIOR.md` and `docs/PROJECT_FILES.md`, not as leftover work under "Left to do" (lines 72–89) or "Not planned" (lines 97–102).

### Firmware and docs at `94197fc8`

- `BEHAVIOR.md` lines 245–246, SET-11 and SET-12: the Projects companion exports/imports `.x0xproject`; portable files keep each populated slot's 512-byte header and encoded data; project-only import requires matching sample identities and does not mutate slots; occupied-slot replacement needs a saved, verified recovery backup; a transfer holds runtime input and audio; failed transfers attempt rollback; power loss can still require that backup.
- `docs/PROJECT_FILES.md` (232 lines): page at `projects/` via `tools/make_pages.py`; Web MIDI + SysEx; export defaults to project + original samples; optional project-only file; prepare → review slot table → download recovery backup → import; recovery mode after an interrupted sample write; container magic `X0XPROJ`, version 1; inner project magic `X0XP`; protocol `F0 7D 46 4C 09 op args F7`, ops 64 query, 65–66 export, 67–69 / 74 import, 70 abort, 71 save, 72 load, 73 reset.
- `docs/MANUAL.md` lines 720–726, "Portable project files": same companion page, default export with original samples, project-only file, pointer to `PROJECT_FILES.md`.
- `web/MANUAL.md` section "Projects": export a portable project with original samples "for a complete project backup"; project-only export; recovery backup before replacing occupied slots; "The current Chicago/Miami build imports revision 13 only."
- `firmware/src/app/editor.c` lines 4–8: Felucca parameter / preset / track commands "are not answered." Project and sample-slot commands are the supported subset.

`BEHAVIOR.md` SET-15 (line 249) and the ROADMAP pattern-format notes: a save stays stream 13 unless PAT SOUND or NOTE LOCK needs 14; 0.0.24.x refuses 14. That is on-device save compatibility, not a substitute for the portable-file companion.

### Web installer / nightly site

Live production [projects page](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/projects/) (fetched 2026-11-13): title "X0X songs & projects"; headings "Inspect or import a project", "Export a project", "On the keyboard"; buttons include Prepare import, Download recovery backup, Import project, Export project + samples, Save on keyboard, Load saved project, Reset project · keep samples. The HTML mentions `.x0xproject`, `.x0xslots` and "Include original samples". Production and nightly install pages mention Projects (see (a)). Nightly [projects](https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/projects/) was live with the same headings (HTTP 200, fetched this turn).

The production landing page links "Projects — Save, load and reset on the keyboard, or export a project with its original samples" to `projects/`.

Caveat: production `gh-pages` is dated 2026-10-25. Use the firmware at `94197fc8` (including #871) as the protocol source of truth; the production page may predate format-2 and later sample-integrity fixes. Nightly Pages were updated 2026-11-13.

### Blockers / caveats

- Import/load/reset require stopped, quiet input (`BEHAVIOR.md` SET-10).
- Sample mutation during a project import is not power-failure atomic (SET-12; `docs/PROJECT_FILES.md`).
- No USB mass storage.
- `web/MANUAL.md` states the current Chicago/Miami workspace "imports revision 13 only" and does not migrate earlier engine projects. `BEHAVIOR.md` SET-15 describes stream 13/14 on the device itself. Those are different layers; do not collapse them.

---

## (c) Engine / sound preset import/export

Two different things are easy to confuse here: **factory preset recall already on the device**, and **a portable user-preset record** that can be imported or exported as a file.

### What is already in `94197fc8` (not file I/O)

- Factory banks: 81 Chicago + 30 Miami names/counts, loaded through the original-sound path (`firmware/src/app/factory_banks.h` lines 12–14, 44–47; `factory_banks.c` `factory_preset()` returns NULL and names come from `original_sound_name`).
- On-device recall: SEQ > PRESET > PAT SOUND per bass part, OFF or a factory preset (`BEHAVIOR.md` BAS-16, line 125; ROADMAP line 163 "Per-pattern preset recall | DONE").
- Sounds are included in an ordinary project SAVE (`BEHAVIOR.md` SET-02; `docs/MANUAL.md` "Saving"). A portable `.x0xproject` therefore already carries the current sound state of the project, not a separate user-preset library file.
- `user_bank_t` / `user_bank_import()` is declared as a writable bank that "must not overwrite factory ROM" (`factory_banks.h` lines 5, 44–56). The implementation at `factory_banks.c` lines 42–47 is a stub: it returns `-1` and does not store a record ("This API accepts the retired rounded-byte record, not an OS patch cell."). Nothing else in `firmware/` at `94197fc8` calls `user_bank_import` except `tests/host/factory_banks_test.c`.
- Felucca editor "user presets" (commands 16–26 in `web/EDITOR_PROTOCOL.md`) are explicitly **not answered** (`firmware/src/app/editor.c` lines 4–8).

There is no shipped web companion, SysEx opcode, or CLI for a standalone engine-preset file in this head.

### ROADMAP

`docs/ROADMAP.md` at `94197fc8` has no item for user-preset or engine-preset file import/export. 0.0.26 is 26.1–26.7 (drone, chords, Juno, chorus, emulator, declined extra engines). Factory recall is the already-DONE 0.0.25 "Per-pattern preset recall" row (line 163).

A 2026-10-25 design document exists at `docs/plans/2026-10-25-native-preset-workflow.md` (KORG section at lines 385–431). It is a plan, not a ROADMAP row, and it does not itself authorize redistribution (quoted under Blockers).

### Branch work: `lane/fmb-presets-format` @ `83e7f92d`

Compare vs `94197fc8`: 8 commits ahead, 0 behind. Added `docs/plans/2026-11-13-preset-format-v1.md`, `firmware/src/app/preset_record.c`, `preset_record.h`, host tests, plus gated measurement hooks in `firmware/src/x0x.c` and `firmware/src/app/main_fm1.c`. Branch URL: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/tree/lane/fmb-presets-format

The plan (byte-identical at `83e7f92d` and in the box copy `/workspace/presets-001/docs/plans/2026-11-13-preset-format-v1.md`, sha256 `e797983017334712e1cceace3ee7db7701bc512d9e4e69e4e98073198fd0db76`) says:

- Lines 3–5: this is "the versioned user-preset record for on-flash and on-file storage" in `preset_record.h` / `.c`. "It is not wired into the UI, the audio path, or the device build in this step."
- Lines 13–14: "The module does not write flash and does not run in an audio callback."
- Wire format: little-endian record, 96-byte header, magic `DNPR` (`PRESET_MAGIC 0x52504E44` in `preset_record.h` lines 1–9, 27–28). Engines include Chicago, Miami, Juno, Drone, 909, 808, BREAK. Origins: unknown / factory / user / imported.
- Section 3, lines 74–82: build-time switch `DNBOX_FACTORY_PRESET_EXPORT` defaults to `0` (OFF). "This is not a rights finding. The factory-asset trace is not done in this step." Non-factory-derived user and imported records always export; factory and factory-derived records export only when the switch is `1`, otherwise `PRESET_ERR_EXPORT_FACTORY` and no bytes.
- Lines 193–207, "Not covered in this step": factory-asset trace / rights research; UI and 240×240 list; "wiring of import into the live bank"; live play and deferred flash on the device; plugin parsing (`§3A`).
- Lines 205–209: a later device link would include only the codec; "UI words come later."

**Size, and why it waits on the Optimizer.** Section 14 (lines 455–478), JieLi pi32v2 Elite+Pro STAGE=4 measurement vs the `94197fc8` base, `X0X_DEV_PADCAL=0`, measurement images **not for install**:

- Whole-image `.text` / app flash used grew **+2,164 B** on both models (2,049 B named codec symbols + 108 B hook + 4 B `fm1_cstart` call + 3 B alignment).
- Elite app 544,232 → 546,396 B; flash free 8,440 → **6,276 B** (1,916 B below the 8,192 B floor).
- Pro app 544,224 → 546,388 B; flash free 8,448 → **6,284 B** (1,908 B below the floor).
- RAM `.data`+`.bss` and pool used are unchanged; pool free stays 8,340 B (holds the 8,192 B pool floor).
- "A default stage-0 link (the shipped default) does not add this cost." Both gated sites compile out when `DNBOX_PRESET_MEASURE_STAGE` is 0 or unset (`preset_record.h` lines 13–17; plan lines 478, 518–521).
- Commit `cd9a19480` message: "Flash free is below the 8192 B floor; pool free holds. The images are measurement only and are not for install. The storage map and floors are not changed."
- Plan line 306: "`lane/opt-flash-savings-026` is not touched." The codec is waiting on flash savings before a default device link can stay above the floor.

Section 13 plans a user-preset bank in the existing unused 8 KiB spare at `0xAD000`–`0xAEFFF` without changing the storage map (lines 111–118, 309–311). 64 A/B slots do not fit in that spare (line 118). That placement is a plan, not wiring.

No GitHub PR has been opened for this branch.

### Blockers

1. **Flash size.** Linking the codec at STAGE=4 costs +2,164 B and goes 1,916 B / 1,908 B under the 8,192 B flash-free floor (plan lines 455–475). It waits on Optimizer savings. Floors and the storage map must not be lowered or moved without the owner.
2. **Not wired.** No UI, no live bank, no SysEx, no web page, no default-image link (plan lines 3–5, 193–207).
3. **KORG / factory-asset rights: UNRESOLVED.** The plan, section 8, lines 137–141:

   > The factory Chicago (81) and Miami (30) sounds have unresolved provenance and rights. Historical PR #276 is a lead only. It said they came from owner-paid KORG Gadget packs, were private/dev-only, and were meant to be stripped before public redistribution. That claim is not proven for source `94197fc8`. This work does not change the factory data and does not research the rights further.

   In-tree leads for that historical claim, all at `94197fc8`:

   - `assets/factory_banks/README.md` lines 11–12 and 18–19: Chicago 81 from KORG Gadget `ChicagoLibrary.bundle` (`Factory2.prog` + NKS); Miami 30 from `MiamiLibrary.bundle` (NKS). "Owner-paid personal-use packs. Owner strips these before public redistribution. No strip automation in-tree unless asked." The same `license_note` is in `assets/factory_banks/manifest.json` line 4, `chicago.json` and `miami.json`.
   - `firmware/src/app/factory_banks.h` lines 4–5: "Owner-paid personal-use packs; owner strips before public redistribution."
   - Merged PR [#276](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/276) "Import Chicago/Miami faithful-303 factory banks as built-in defaults".
   - `docs/plans/2026-10-25-native-preset-workflow.md` lines 391–421: installation of the plugins "does not establish permission to extract, adapt or distribute preset content"; "Assess the applicable KORG EULA/content terms … consult KORG for explicit written permission if unclear; public inclusion needs separate permission …"

   The factory-export switch defaulting to OFF is explicitly "not a rights finding" (plan line 76; `preset_record.h` lines 20–24). A later public strip of factory tables is called out as an open question (plan line 143). This audit does not treat the rights question as settled.

4. Plugin file parsing is not in this step. `PRESET_PLUGIN_FORMAT` (`1`) "is a placeholder. KORG Gadget is a lead only." (plan lines 84–88).

---

## What this audit did not do

- No device MIDI session, no flash, no package rebuild, no claim that production Pages bit-match `94197fc8`.
- GitHub issue search for these exact phrases was thin (the general search mostly missed in-tree work that lives in code and merged PRs). Status above is from the named commits, the two Pages sites, and the PRs cited.
- No owner-chat quotation about KORG rights was opened in this pass; the rights section cites in-tree documents and PR #276 only.
