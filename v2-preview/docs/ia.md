# DnBOX website v2 — information architecture

One task-led instrument site. This is a design direction, not a published site, and not a pick among the earlier A, B and C options. Those remain in `/workspace/website-ux/options/` as a record; they are not offered as a choice here.

Fetched 13 November 2026, Asia/Vladivostok (UTC+10). Feature and engine facts are taken only from the sources named below. Anything not in those sources is marked UNVERIFIED or stated as this author’s judgment.

## Sources

- Nightly gh-pages manual, 23:35 UTC+10: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/manual/ — short SMK-37 install, backup and recovery guide. It says the full playing manual is being reconciled, and that older X0X documentation may describe a different device. Local copy: `/workspace/website-ux/refs/dnbox/manual-nightly.html`.
- Nightly release status `/new/`, 23:35 UTC+10: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/new/ — updated 13 November 2026. Local copy: `refs/dnbox/new-nightly.html`.
- Nightly README, 23:35 UTC+10: https://raw.githubusercontent.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/main/README.md. Local copy: `refs/dnbox/README-nightly.md`.
- Production README, 23:35 UTC+10: https://raw.githubusercontent.com/gobbajogger/X0X37-for-Loopy-Pro/main/README.md. Local copy: `refs/dnbox/README-production.md`.
- Production gh-pages playing manual (written for the FM-1; title “X0X Manual”), 23:35 UTC+10: https://gobbajogger.github.io/X0X37-for-Loopy-Pro/manual/. Local copy: `refs/dnbox/manual-production.html` and `.txt`. The nightly manual points at documentation at the 0.0.25.1 source revision as historical reference and asks the reader to confirm each device-specific instruction.
- `SMK37.md` on production `main`, 23:35 UTC+10: https://raw.githubusercontent.com/gobbajogger/X0X37-for-Loopy-Pro/main/SMK37.md — SMK-37 control map and install notes. Local copy: `refs/dnbox/SMK37.md`.
- Emulator LCD captures in `/workspace/web-emu/logs/` (emulator’s own drawing, labelled as such wherever shown): `lcd-elite-360mhz-2003ms-3x.png` (“OLD DATA CLEARED”), `lcd-elite-360mhz-15001ms-3x.png` (909 SEQ, empty pattern, “HOLD ANY BUTTON: WHAT IT DOES”), `native-gate-1-lcd_end-3x.png` (PERF TEST DONE, “ALL FIVE”, GIT 94197FC8).
- DNPR contract, branch `lane/fmb-presets-format` of `gobbajogger/X0X37-for-Loopy-Pro`, raw-fetched 23:38 UTC+10: `docs/plans/2026-11-13-preset-file-io-research.md`. Local copy: `refs/dnbox/2026-11-13-preset-file-io-research.md`. The wire layout it points at is `docs/plans/2026-11-13-preset-format-v1.md` on the same branch (local `refs/dnbox/2026-11-13-preset-format-v1.md`). Format or transport claims in the presets section cite the research doc’s sections first. Details that exist only in the format plan are cited as such.
- Reference reasons (not looks): Dot’s `AUDIT.md` (`/workspace/handoff-dot-20261113/website/AUDIT.md`) and `/workspace/website-ux/reading.md`. Baud Girl: hierarchy, one task surface, Help in context, truthful state. Felucca: authored illustration craft and direct route naming.
- Prior SMK-37 panel geometry (official M-VAVE Panel Introduction and product photographs, 13 November 2026): `/workspace/website-ux/refs/smk37/SOURCES.txt`. Used only for the small support drawing.

## Site map

Pages are ordered by what a musician actually has to do, not by marketing sections. The home page is a task hub. There is no product-hero plate and no feature-card grid.

```
/                  Task hub
/first             Play a first pattern; then save; then get ready to play live
                   (engines are a section of this page)
/install           Install or update (stepped decision flow)
/presets           Back up, load, and validate .dnpr preset files
/recover           If the keyboard will not start (reachable before an install starts)
```

Navigation, always present: **DnBOX** (home), First pattern, Install, Presets, Recover. A quiet text mention of the published manual. No “Presets: coming” slot — presets are a real flow. No competing Studio / Simulator / Try as equal actions. An emulator mention, if any, is secondary and tagged as a proposal.

The SMK-37 is supported hardware, named in supporting copy (“for the M-VAVE SMK-37 Elite and Pro”), never as a giant hardware label. The brand, spelled exactly **DnBOX**, sits in the navigation as a wordmark, not as the page’s main headline.

---

### 1. Home — “What do you need to do?”

**Musician task.** Choose the next useful job: play a first pattern, install or update, back up or load a preset, recover a keyboard that will not start.

**Why this helps.** A musician opening this site already has, or is about to have, a keyboard. They are not looking for a product story. A task hub puts the job first and keeps every other route one step away. The first view of options A, B and C led with a brand plate and a hardware drawing; the owner called those generic, and earlier asked “Why big smk37??”.

**Reference.** Baud Girl’s product page, as recorded in `AUDIT.md` (“What is actually on screen”): “A small body paragraph and a pair of modest actions follow, not five equally important feature cards.” The task list is this author’s judgment extending that reason — hierarchy by job, not by feature — rather than a copy of Baud Girl’s name-as-graphic hero, which would repeat the “big name” problem.

**On the page.** A short identity line (DnBOX; firmware for the SMK-37 Elite and Pro). Then an ordered list of tasks, each a verb, one sentence of consequence, and one link. A small emulator LCD of the empty 909 SEQ (`lcd-elite-360mhz-15001ms-3x.png`) sits with “Play a first pattern,” labelled “emulator capture, not hardware.” The SMK-37 drawing is small, in support, at the edge. A quiet, truthful package line: public package 0.0.25.1 is the known-good fallback; it is software-checked and not signed off as a physical device test (production README, “Hardware test status”; nightly `/new/` and `/manual/`).

---

### 2. Play a first pattern (`/first`)

**Musician task.** After a first start, make a sound within a few minutes, then keep it, then know what matters for a live set.

**Why this helps.** The published playing manual already contains a five-minute tour. Putting that tour on the site, with SMK-37 button names and an honest limit about the FM-1 origin of the text, is more useful than a feature list. Baud Girl’s “copy names the user's purpose” (`AUDIT.md`) applies: the page is “put a kick on the 909 and press PLAY,” not “explore the sequencer architecture.”

**Reference.** Production manual, section 2, “Getting started”: “A five-minute tour, from switching on to saving your first beat. When X0X starts, the screen shows the 909 and every part is empty.” The empty 909 SEQ is also what the emulator LCD shows at 15 s (`lcd-elite-360mhz-15001ms-3x.png`). The first-start banner “OLD DATA CLEARED” is on `lcd-elite-360mhz-2003ms-3x.png`. SMK-37 control names come from `SMK37.md` (SEQ PLAY is PLAY, BT is SAVE, PATCH is HOME, K3 is ALGORITHM, and so on). The nightly manual warns that older X0X documentation may describe a different device or control layout; each FM-1-specific step is labelled, and any SMK-37 key-to-drum assignment not stated in `SMK37.md` is **UNVERIFIED**.

**Sections, in order.**

1. **What you should see after a first start.** The 909, an empty pattern, and (on a first install) “OLD DATA CLEARED.” LCD captures, labelled. `SMK37.md` Safety: “The first start clears the stock firmware’s settings, and any other firmware’s data, in the shared data area.”
2. **Five-minute tour** (production manual §2), with SMK-37 names from `SMK37.md`: pick the bass drum, write kicks, press SEQ PLAY; add a clap and open hats; write a bass line on the 303-style part (see engines, below); shape the filter; add the break; mute; record a sweep; stop and SAVE (the BT button).
3. **Saving.** Production manual §15: SAVE stores sounds, all 32 patterns, the song, knob motion, tempo and settings. AUTOSAVE (on by default) saves after you stop and leave the controls for four seconds, and says AUTOSAVED. It never saves while the pattern plays: SAVE then says SAVES WHEN STOPPED. A failure says SAVE FAILED. This is what “saving” means on the instrument; it is not a website account.
4. **The engines** (`#engines`). See the engine note below. Only what the sources support. No invented playing instructions for Juno or Drone.
5. **Get ready to play live** (`#live`). USB MIDI and USB audio with no driver (production README). GLO settings for clock, notes out, USB AUDIO FOLLOWS KNOB or FULL (manual §15–§16). PERFORMANCE and GUARD when the hardware is close to its limit (manual §15; emulator PERF captures at `native-gate-1-lcd_end-3x.png`, GIT 94197FC8). Bluetooth MIDI on the SMK-37 is in the production README as experimental and not hardware-verified, and needs a matching controller image that cannot be shipped with public packages — mentioned only with those limits. Clip mode as an APC40 mkII for Loopy Pro is the same: experimental, not hardware-verified.

---

### 3. Install or update (`/install`)

**Musician task.** Put a known package on this exact keyboard, without losing sample slots, and know what to do if the write stops.

**Why this helps.** Installing firmware is the riskiest task on the site. A stepped decision — which device, which version, which path, backup, write, confirm — keeps each choice small and stops a mismatch before a write. Recovery is visible before the first action, so a musician is not hunting for it only after a failure.

**Reference.** Baud Girl’s installer, `AUDIT.md`: one task surface; Help and Next at the card’s corners; a stage rail *outside* the card; purpose-worded copy (“Chrome Can Install this Firmware”), not “Web MIDI is available”; readiness that changes the same card. The visual language is this site’s, not Baud Girl’s magenta italic. Felucca, same audit: “direct route naming,” not its “first custom firmware” claim. Device-match wording follows the live-installer problem recorded in `reading.md` (a USB or port name is not confirmation; the installer must also check the connected device).

**Facts used (cited).**

- Choose the model printed on the keyboard; the installer also checks the connected device (nightly `/manual/`). “A connection name is not proof of the board model” (same page). Elite and Pro have separate packages; Starrykey 37 Play uses the Pro package (production README, FAQ).
- Back up sample slots before installing (nightly and production READMEs; nightly `/manual/` “Install and keep a backup”). Do not unplug USB while writing (both READMEs).
- Default package: **0.0.25.1**, the known-good fallback reported by the owner (nightly `/new/`). On-device label is 0.0.25 (nightly `/manual/` and `/new/`). Base `79280e76` is the published 0.0.25.1 source revision (nightly `/manual/` documentation link). Software-checked, not signed off as a physical device test (production README, “Hardware test status”; nightly README).
- **0.0.26, original `600b3b45`:** withdrawn after reports of updates stuck at Restarting and very long or failed startup. Do not reinstall those saved packages (nightly `/new/`; nightly README).
- **0.0.26, replacement candidate `94197fc8`:** available for owner-operated testing. Physical startup, restart and recovery are not yet verified. Not a stable-release recommendation (nightly `/new/`). The owner reports having tested both 0.0.25.1 and a 0.0.26 candidate on the Elite, not on the Pro, and not as full qualification — that report is not a hardware sign-off.
- Computer path: Chrome or Edge on a computer (nightly and production READMEs).
- iPhone path: supported, equally, with the visible tag **UNVERIFIED: route pending Installer QA**. Neutral copy: “Install from your iPhone with a compatible app.” No app is named. Never “cannot” or “unsupported” for iPhone or Safari. Owner rule of 31 October 2026, as recorded in `reading.md`.
- Package 0.0.25.1 displays 0.0.25 on the device; the package revision is not a different keyboard model (nightly `/manual/`).
- Restore to M-VAVE stock from the installer or M-UPGRADE (both READMEs; nightly `/manual/` says do not use another device’s stock firmware or recovery instructions; do not treat the FM-1 recovery procedure as an SMK procedure).

The stepped states and failure branches are in `flow.md`.

---

### 4. Recover (`/recover`)

**Musician task.** If the screen is dark, a write stopped, or the keyboard will not start, know the next safe step — including *before* starting an install.

**Why this helps.** Recovery that appears only after a failure is too late to read calmly. Baud Girl keeps Help on the card “including before anything fails” (`reading.md`, “Recovery and errors”). The nightly manual already tells a musician to retain the activity log, selected package and exact model, and to report them; it forbids using another device’s stock firmware.

**Reference.** Baud Girl Help-in-context, as above. Nightly `/manual/`, “Connection and recovery.” Production README, “Restore and rescue.” The FM-1 “Recovering an FM-1 that won't start” script in the production manual §17 is **not** presented as an SMK-37 procedure (nightly `/manual/` explicit warning).

**On the page.** Reachable from the hub, from a persistent Help control on every install state, and from every failure card. Covers: dark display during a write can be normal — do not switch the device off; unplug mid-write is unsafe; replug and finish the same package; device mismatch — stop, do not write; original 0.0.26 (`600b3b45`) already on a device — restore stock, do not reinstall that package; if the screen stays black after a write, retain the log, package and model, and use the installer’s restore to M-VAVE stock or M-UPGRADE. This page cannot tell, from a still screen, whether the keyboard will start on its own after a stopped write.

---

### 5. Presets (`/presets`)

**Musician tasks.**

- **Back up my sounds** — download a user preset as a `.dnpr` file, and keep that file if a later load fails.
- **Load a shared preset** — pick a `.dnpr` (or a zip of them), validate it, and only then consider a slot write.
- **Move a preset between an Elite and a Pro** — only if the contract supports it. It does not say that it does. See the gap below.

**Why this helps.** A musician’s sounds are the work. A file they can keep, with a check that runs *before* anything is overwritten, is the difference between a librarian and a dangerous upload box. Confirm-before-overwrite and a backup of the occupied slot come first because loss is the thing they will not forgive.

**Reference.** The contract is `docs/plans/2026-11-13-preset-file-io-research.md` on `lane/fmb-presets-format`. Musician-facing structure is this author’s judgment; every format and transport claim is cited to that document (and, where the research points at it, the v1 format plan). The “no accidental loss” stance is in research §2.1 (“failed sends leave the user’s file alone”), §2.3 (“back up before a mass replace; do not overwrite presets already on the destination”), and R2 (confirm + one undo; validate before commit). Baud Girl’s focused task surface applies: one librarian card, Help in the corner, not a second product page.

**What the contract says, and what the page does.**

- **Format** (research §4, and R1, R3): the file is the raw DNPR v1 byte record. Magic `DNPR`. Maximum `PRESET_MAX_BYTES` 548. One preset per `.dnpr` file. CRC as in the plan. The page does not re-encode parameters and must not strip provenance.
- **Filename** (research R1, §4): `<sanitized-display-name>.<engine>.dnpr`, for example `acid-bass.chicago.dnpr`. Engine tokens, lower case: `chicago`, `miami`, `juno`, `drone`, `909`, `808`, `break`. The authoritative name is the 32-byte display-name field inside the record; renaming the file on disk must not break import.
- **Upload** (research §4): accept `.dnpr` and a `.zip` of `.dnpr`. The zip is unpacked on the host (R3, R4). Validate magic, CRC, version, and engine **before** any device send. Map failures to the visible words `BAD FILE`, `NEWER DATA`, `WRONG ENGINE`. A bulk send that would overflow is stopped up front with `MEMORY FULL` language; nothing is evicted; the user’s file is never deleted on failure (R3, R4, §4).
- **Versioning** (research R2): newer format or schema → `NEWER DATA`, no partial load. Older unknown → `BAD FILE`. Parse and CRC-check before any slot write, and outside the audio path.
- **Occupied slot** (research R2): ask, then one undo. Factory rows are never a write target.
- **Factory export** (research R2, §4, §5): factory and factory-derived records export only when `DNBOX_FACTORY_PRESET_EXPORT` is ON. The temporary default is OFF. The page must not offer those downloads, and must not send them, while the switch is OFF.
- **Transport v1** (research R3, §4): **host file download and upload**. Optional later: a Web MIDI SysEx wrapper of the same bytes, only when the device is stopped and quiet, HTTPS, explicit SysEx permission — not required to specify the page, and not built here. **Not in v1:** Web Serial, USB mass-storage, or a boot-time disk mode (R3, §2.4).
- **Device path is blocked** (research introduction, R3, §5): “Device wiring of the codec is **BLOCKED** on Optimizer flash savings (+2,164 B on JieLi; floor 8,192 B).” “It must not claim a live device send until the firmware link exists and the flash floor holds.” The page can already create, open, CRC-check, and rename-on-disk `.dnpr` files produced by a host build of the codec. The prototype is an inert simulation of that host librarian, plus the later device-send flow drawn as a flow, clearly marked as not yet available.
- **Display** (research §4): name, engine, origin, date, CRC ok/fail, sample-ref warnings. No “identical to the plugin” language. No KORG or other plugin converter in v1 (R3, §5; plugin-identity).
- **909 / 808 / BREAK samples** (research R5): if a sample CRC or slot does not match, show a clear “sample missing / changed” warning; do not pretend the kit is complete. Sample data is a reference, not embedded PCM.
- **Live-safe write, when a device path exists** (research R2): if playing, recording, or busy — no flash erase; `SAVES WHEN STOPPED`. Never applied mid-note.

**Elite and Pro.** The research document does not define a device-model field, and it does not say that a preset from an Elite can or cannot load on a Pro, or the other way. The v1 header it points at (format plan §1) has magic, versions, engine id, flags, length, CRC, origin, original engine, factory index, date, plugin room, display name and provenance — not Elite or Pro. **Whether a `.dnpr` can move between Elite and Pro is UNVERIFIED.** The site does not offer that move as a supported action, and does not invent a “device match” check the contract does not define. Engine match (`WRONG ENGINE`) is the check the contract does define (research §4; format plan §2).

**On the page.** Browse host files (and, greyed, “device slots — not available while the codec is unwired”). Download / back up. Upload with a validation result. Overwrite confirm, with a backup of the occupied slot first. Every error word above, as its own state. Inert: no Web MIDI code, no real device send.

---

### Engine names — what is verified

The brief named 909, 808, Chicago (the 303 engine), Miami (not a 303), Juno and Drone. These are listed only where a cited source supports them.

- **909.** Production manual §2 and §5; production README “What’s inside”; emulator LCD `lcd-elite-360mhz-15001ms-3x.png` (header “909 SEQ”). Confirmed.
- **808.** Production manual §5; production README. Confirmed in the published playing manual and README. Not the subject of the LCD captures used here.
- **Break.** Production manual §8; production README. Confirmed there. (DNPR token `break`, research R1.)
- **303A and 303B.** These are the bass-engine names in the published playing manual (§2, §6) and in the production README (“303 A and B”). `SMK37.md` still labels fader bank A as “the 909, the 808, 303 A, 303 B.”
- **Chicago and Miami.** They do **not** appear in the nightly `/manual/`, the production playing manual, or the two READMEs. They **do** appear in the DNPR contract: research R1 lists engine tokens `chicago` and `miami`; the format plan the research points at opens with “Miami is not a 303 and must never be called 303 or 303B. Chicago is the 303-style engine,” and §1 gives engine ids `1` Chicago, `2` Miami. The presets page uses those tokens. The first-use tour, which is grounded in the playing manual, says “the 303-style part (Chicago in the current preset format; the published manual still says 303A)” and **never** calls Miami a 303. The emulator LCDs in `/workspace/web-emu/logs` used here show 909 and PERF, not a Chicago or Miami screen.
- **Juno and Drone.** Same situation: absent from the published playing manual and READMEs; present as tokens and ids in research R1 and format plan §1 (`3` Juno, `4` Drone). Format plan §7 inventories “81 Chicago, 30 Miami, 4 Juno, 4 Drone” factory entries at source `94197fc8`. There is no published playing tour for Juno or Drone. The site lists them as DNPR engine identities on the presets page, and does **not** invent how to play them. Playing instructions are **UNVERIFIED**.

The nightly `/manual/` states that the full playing manual is being reconciled with the current SMK build. Until that is published, this discrepancy is left visible, not smoothed over.

---

### What is not a page

- No preset web implementation that talks to a device (codec unwired; this is a static prototype).
- No plugin converter, no factory-pack download, no SysEx engineer panel.
- No FM-1 rescue script presented as an SMK procedure.
- No popularity, analytics, or “hardware verified” claim.
- No light theme.
