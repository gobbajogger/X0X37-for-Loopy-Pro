# DnBOX manual, glossary and naming plan

**Kind:** investigation and planning only. No documentation edits, commits, pull requests, GitHub writes, builds or tests were made in this pass.

**Base source:** `/workspace/smk-ux-src` at `94197fc839b00aff47b98ca02f3c6396e4a6eabf` (verified with `git rev-parse HEAD`; message: “Merge reviewed SMK host fixture identity corrections”; `git describe`: `v0.0.25.1-87-g94197fc83`).

**Prior map:** `/workspace/smk-ux/source-map-94197fc8.md` (SMK Firmware UX), read first. Line numbers below were re-read in this tree at this SHA.

**Spell:** the product name in user-facing text is exactly `DnBOX`.

**Goal:** a reliable, intuitive instrument for live play. A first-time player should hear a sound, write a pattern, and save it, without losing expert muscle memory.

**Evidence grades:** **SOURCE** = read in this tree at this SHA; **DOC** = repository manual or behaviour text; **INFERRED** = reasoning, not claimed as device behaviour. This plan does not claim hardware behaviour. Anything not re-read is marked unverified.

---

## 0. Coordination with Dot (do not duplicate)

Another bot, Dot, may hold the docs-owner role. Its status is **unverified**. Edits that follow this plan must be coordinated with the docs owner and must not copy or publish Dot’s work as current firmware.

Checked on this box (not a broad workspace grep):

| Path | What it is | Use in this plan |
|---|---|---|
| `/workspace/handoff-dot-20261113/` | Dot firmware UI study and a website audit, dated 2026-11-13 | Read as a proposal, not as current firmware copy |
| `/workspace/smk-ux/` | Source map plus mockups | The source map is evidence for this SHA; mockups were not treated as shipped UI |
| `/workspace/smk-ux-src` | The firmware tree | Sole source of truth for current behaviour |
| `/workspace/steers/dot-rename-20261113.txt` | Repo-rename steer (gobbajogger/DnBOX) | Context only; not a manual draft |

Dot’s firmware README is labelled **PROPOSAL, NOT IMPLEMENTED**. The six-screen journey and the right-hand “after” HOME (title “DnBOX”, “STOP”/“PLAY” words, “PATTERN 01”, always-on “CHAIN 01”, “EDITING: SOUND”, “PROJECT SAVED”, and so on) are **not** in source at this SHA. The source map already checked those claims (§9.2). This plan describes **current** behaviour for a version-aligned manual. It does not adopt proposed strings as documentation of what the unit does today.

Website `AUDIT.md` in that handoff is a site-design audit, not a firmware control map.

---

## 1. Summary

The end-user manual in this tree is still the older FM-1 “X0X” book (`docs/MANUAL.md`, 1000 lines, and the generated `docs/manual.html`). It does not name the product **DnBOX**, it uses Felucca printed names (EDIT, HOME, SAVE, ALGORITHM) instead of the SMK printed legends (PARA, PATCH, BT, K3), and it is wrong about the bass engines, the part order, the getting-started tour, and what PARA’s white keys do.

The three known findings are **verified in source**. This pass also found further manual errors (HOME mute key, leftover schwung-303 sound pages, USB and Bluetooth names, missing Juno/Drone and Loopy clip mode) and a naming split: what the player sees (USB product, About, boot) is not the same as the identifiers that installers, M-UPGRADE and Loopy must keep unchanged.

A new manual should be written for the SMK-37 Elite and Pro at a stated firmware version, with one glossary, a first-sound then pattern then save path, and a separate expert-shortcuts chapter. Package identity, USB VID/PID, BLE-MIDI UUIDs, OTA SysEx and APC identity must not be “fixed” to match the product spelling. Tooling that still picks a device by its USB display name is a brick risk and must be called out, not used as a model picker.

---

## 2. Existing user docs (inventory)

These are the manuals and user-facing texts in the tree. Agent operating guides and engineering audits are listed only so they are not mistaken for the end-user manual.

| Path | Role at this SHA | Product spelling | Aligned to 94197fc8? |
|---|---|---|---|
| `docs/MANUAL.md` (1000 lines) | Full keyboard manual. Title “# X0X”. FM-1 drawings (`img/fm1-panel.svg`, `img/fm1-keys.svg`). Last changed at `79280e763` (tag `v0.0.25.1`), **unchanged** on the 87 commits to this SHA. | X0X throughout; one “DnBOX” in the USB-audio paragraph (line 829) | **No.** This is the main gap list below. |
| `docs/manual.html` (327 lines) | Generated HTML of the same book. Title “X0X Manual”. | X0X | Same errors as `MANUAL.md`. |
| `web/MANUAL.md` (51 lines) | A **different** short “DnBOX manual”: install, samples and projects in the browser. Points to `legacy.html` for “the project’s full existing manual” and calls those engine descriptions historical. | DnBOX in the title; still “only Chicago and Miami” | Partial for the web workspace; stale on Juno/Drone; not a control manual. |
| `docs/NEW.md` (220 lines) | “What’s new”, starting at **1.0.6**, still written as X0X / 303s. | X0X | Not aligned (version scheme and engines). |
| `web/NEW.md` (17 lines) | Web-workspace “what’s new”. “Revision 13 uses Chicago and Miami.” | DnBOX | Stale on Juno/Drone. |
| `SMK37.md` (98 lines) | SMK physical-control map. Title “X0X on the M-Vave SMK-37”. Says “The X0X manual still applies.” | X0X | Control map useful; fader order, outer keys and velocity are **wrong**. |
| `README.md` | Project README: “X0X for the M-VAVE FM-1”, “two 303s”. Installer URL still `gobbajogger.github.io/X0X37-for-Loopy-Pro`. | X0X, X0X37 | Stale product and engine names. |
| `BEHAVIOR.md` (620 lines) | Agent behaviour contract, not an end-user manual, but designers and docs often trust it. | Mixed; GEN-27 maps SMK legends correctly | Several GEN/BAS/CLP rows are stale (see findings). |
| `PAD_MAP.md` | Pad and LED index path for implementers. | X0X37 in the title | Specialist; not a player manual. |
| `docs/FIRMWARE_VERSIONING.md` | How versions are allocated and shown. Title “X0X37 firmware versioning”. | X0X37 | Process doc; example labels still say X0X37. |
| `docs/handoff/OPERATING-GUIDE.md` | Agent/orchestrator operating rules, **not** a player guide. | — | Out of scope. |
| `APC40_IMPLEMENTATION.md` | Loopy Pro / APC40 mkII clip mode. | — | Expert reference; almost no path from `MANUAL.md`. |
| `web/x0x_installer.html`, `web/x0x_projects.html` | Install and project pages. Titles “DnBOX · …”. | Mostly DnBOX; nav `aria-label="X0X"` | Web UX, not the firmware manual. |
| `web/offline-sequencer/index.html` | Offline editor. Brand **`DNBOX`** (wrong spelling). | DNBOX | User-visible spelling error. |

There is **no** version-stamped SMK end-user manual that matches this SHA. The “legacy” book and the short web book disagree on product name, and neither is a complete, correct control manual for 94197fc8.

---

## 3. Firmware version the manual must state

**SOURCE.** The string the unit itself shows is `X0X_VERSION`, set at build time by `tools/build.py` (`-DX0X_VERSION="X.Y.Z"`). If the flag is absent, `firmware/src/app/version.h` 6–8 defaults to `"DEV"`. The same macro is used in:

- the About line: `"DnBOX " X0X_VERSION "  MEM …"` (`ui.c` 2682, compact form 2693);
- the USB product: `"DnBOX " X0X_VERSION " SMK-37 Elite"` or `"… SMK-37 Pro"` (`usb_product_name.h` 16–18);
- the editor-protocol INFO reply: `"X0X " X0X_VERSION` (`editor.c` 875) — this is a **protocol** string, not on-screen branding (see the naming table).

A display-only label `X0X_RELEASE_LABEL` (for example `0.1.3_openbeta`) may also be passed; `docs/FIRMWARE_VERSIONING.md` says the `_openbeta` suffix is **not** written into the package identity.

**What this SHA is:** `git describe` reports `v0.0.25.1-87-g94197fc83`. The published tag compared in the source map is `v0.0.25.1`. A parent of this merge is “0.0.26 bootguard…”. The open-beta ledger (`releases/openbeta/ledger.json`) says **“No number has been allocated yet.”** So this tree is **after** 0.0.25.1 and is work toward a later number, but **0.0.26 is not an allocated firmware version in the ledger**. A manual for this exact SHA should say it describes source `94197fc8` (short), and that a published copy must print the same `X0X_VERSION` the package was built with — the same words as About and the USB product — not a guessed number.

**How to keep the manual aligned:**

1. The first page states `DnBOX` and the exact `X0X_VERSION` (and, if useful, the short Git id from `X0X_GIT_SHA`, which About already appends as `g…` when present).
2. A docs check compares the printed version to the About format `DnBOX ` + `X0X_VERSION` and fails if they differ.
3. `docs/MANUAL.md` was unchanged between `v0.0.25.1` and this SHA while `ui.c` gained Juno, Drone and held-chord recording. Alignment is not automatic today. Treat a manual change as part of the same release as the firmware it describes.
4. Elite and Pro share one number and keep separate package hashes (`FIRMWARE_VERSIONING.md`). One manual can cover both models, with a short “what differs” note (USB and Bluetooth names, package identity — not the controls, which are the same: source map §0).

---

## 4. Verified known findings

These three were given as known. All three were re-read in source and in `docs/MANUAL.md`.

### K1. The manual says “303A / 303B”. Miami is never a 303. Chicago is the 303 engine.

**DOC:** `docs/MANUAL.md` title of §6 is “303A and 303B” (line 274). The tour (92–100), the control table (123), the screen colours (201), knob tags (213), mix (616) and MIDI table (777–778) all use those names. Line 276: “There are two identical 303s.”

**SOURCE:** `ui.c` 94–99:

- Comment: “Bass slots are not named 303: Chicago is the 303 engine, Miami is not.”
- Display names follow the engine (`part_name` / `bass_display_name`, 108–149): `CHICAGO`, `MIAMI`, and at this SHA also `JUNO` and `DRONE`, with A/B only when both slots share a kind.
- Internal slot IDs stay `PART_303A` (default Miami) and `PART_303B` (default Chicago). `engine.c` 784: `v == 0 ? ORIGINAL_SOUND_MIAMI : ORIGINAL_SOUND_CHICAGO`.
- Presentation order is not engine-ID order: `PART_ORDER = {PART_909, PART_808, PART_BRK, PART_303B, PART_303A}` (`ui.c` 99) — on screen: **909, 808, BREAK, Chicago, Miami**.

**Verdict:** verified. The manual’s “303A/303B” language is wrong for players. Slot IDs in saved data and MIDI channel tables must stay (see N-MIDI below). Miami must never be called a 303.

### K2. Wrong fader order. Source order is 909, 808, BREAK, Chicago, Miami.

**DOC:** `docs/MANUAL.md` does **not** itself list SMK fader banks (it is an FM-1 manual). The wrong order lives in:

- `BEHAVIOR.md` GEN-31 (line 57): bank A = 909, 808, 303A, 303B;
- GEN-32 (line 58): bank B = BREAK, reverb return, delay return, master FILTER;
- `CLP-28` (line 410): the same stale bank A/B claim, including in clip mode;
- `SMK37.md` 49–50: the same.

**SOURCE:** `main_fm1.c` 883–935 and the comment at 883–886. Bank A sets levels of the first four **displayed** parts (`part_at(i)`), so **909, 808, BREAK, Chicago**. Bank B’s first fader is the fifth displayed part (**Miami**, default), then reverb return, delay return, and master FILTER. A bank change toasts `FADERS: …` with the short labels (`main_fm1.c` 909–915).

**Verdict:** verified against source. GEN-31/32, CLP-28 and `SMK37.md` are stale. BREAK is on bank A, not bank B.

### K3. The manual says PARA (EDIT) drum white keys write steps. Source: they only audition.

**DOC:** `docs/MANUAL.md` 130: “EDIT | The selected part’s sound. The keys still write its steps.”

**SOURCE:** `ui.c` 3801–3803: on a drum part, a white key in `V_PART` (PARA) calls `engine_drum(...)` “audition selected sound, never write a hidden step”. Step writes happen only in `V_SEQ` (`drum_step`, 3798–3799). The PARA footer is `WHITE: SOUND` (source map; `ui.c` 6776–6779).

**Verdict:** verified. This is both a documentation error and a live-play trap: a player who opens PARA to “fix the pattern” will hear the drum and think a step was written. The step-versus-sound split must stay in the firmware (`ui.c` 2755–2759, `RELEASE STEP FIRST`) and must be explained clearly in the manual.

---

## 5. New findings (by manual section)

Counts at the end of this section. Each item is **SOURCE** vs **DOC** unless marked **INFERRED** or **UNVERIFIED**.

### 5.1 Whole book: product, hardware and control language

**N1. The manual is still an FM-1 / “X0X” book.** Title `# X0X` (line 1); opening sentence “X0X turns the M-VAVE FM-1 into a groovebox: a 909, an 808, two 303s and a breakbeat player” (3–5). Install URLs point at `charlesvestal.github.io/fm1-x0x/` (7, 39, 65, 403, 873) and a GitHub releases page under that name (65). `README.md` and `SMK37.md` still say X0X; `README.md` 8 still says “two 303s”. The shipped USB and About identity is `DnBOX`. **DOC vs SOURCE.**

**N2. The control table uses Felucca printed names, not SMK legends.** MANUAL §3: ALGORITHM, SELECT, PRESETS, KNOB 1–4, SEL, ENV, LFO, EDIT, GLO, HOME, SAVE, OCT-, OCT+, PLAY, REC (118–139). On the SMK those are printed K3, K2, K4, K5–K8, SCALE, CHORD, NOTE REPEAT, PARA, GLOBE, PATCH, BT, OCTAVE−/+, SEQ PLAY, SEQ REC (`panel.c` 28–33; `ui.c` 39–54). `SMK37.md` 43–44 has the map, but the main manual never uses the words on the plastic. A first-time SMK player cannot follow the tour by looking down at the keys. **DOC.**

**N3. Panel figures are FM-1 drawings.** `img/fm1-panel.svg`, `img/fm1-keys.svg` (MANUAL 9, 145). There is no SMK panel figure in the manual. **DOC.**

**N4. `web/MANUAL.md` and `docs/MANUAL.md` disagree**, and the web book tells readers that the full manual is “historical” (`web/MANUAL.md`, last section). That hides the only detailed control text and does not replace it. **DOC.**

### 5.2 §2 Getting started (the five-minute tour)

**N5. K3 / ALGORITHM part order and click counts are wrong.** MANUAL 92–97: “two clicks clockwise to 303A”, then “two more clicks to BREAK”. **SOURCE** `PART_ORDER` and `part_neighbor` (`ui.c` 99, 108–111): from 909, one click is 808, **two clicks are BREAK**, three are Chicago (slot B), four are Miami (slot A). The tour never lands on the intended bass, and it tells the player to skip past the break after they have already been on it. Same error in the control table (123).

**N6. Tour step 8 mutes the wrong part.** MANUAL 98–99: “Press HOME, then black key 5 to mute the break.” **SOURCE** `ui.c` 3784–3787: on HOME, a black key calls `mute_part(part_at(bl))`, so black keys 1–5 follow `PART_ORDER`. Black key **3** mutes BREAK. Black key **5** mutes the fifth displayed part (default **Miami**). After the accidental PATCH that the tour itself uses to reach HOME, this also teaches a dangerous habit (N7).

**N7. PATCH (HOME) from SEQ is an accidental mode change.** The tour says “Press HOME” to mute (98). **SOURCE** `ui.c` 4042–4051: a PATCH tap from SEQ goes to HOME; a second tap already on HOME opens **SONG**, not back to the 909 grid. HOME keys do not play sound (3733–3787). The source map’s first-sound dead-end (inferred) is confirmed in source. The boot state is 909 SEQ (`ui_init` 8604–8606), which the tour’s first four steps need — and which PATCH leaves.

**N8. “Shape the 303: KNOB 1 opens the filter” is the old engine.** MANUAL 94–95. **SOURCE** Chicago PARA pages start at OSCILLATOR / FILTER 1 / … (`ui_original_controls.h` 28–29), Miami at CARRIER / XMOD / FILTER / WOBBLE (`31–32`). There is no single “TONE: CUTOFF, RESO, ENVMOD, DECAY” page at this SHA (that is MANUAL §6, 317–320, leftover schwung-303). What K5 does on first PARA open depends on the engine.

### 5.3 §3 Controls and §4 The screen

**N9. White-key and black-key summaries are incomplete and partly wrong.** MANUAL 138–139: whites are “Steps. On HOME, patterns. On MASTER and FX they do nothing.” They also: audition on PARA drums (3803); play 303 notes when the keyboard is on; paste when BT is held; chain when two are held on HOME. Blacks on HOME mute **in display order** (N6), not a fixed “part 5 = break”.

**N10. Header copy is stale.** MANUAL 201–207: part names “303A green, 303B blue”; pattern cue written as `P3 >5`; unsaved as “a small grey dot”. **SOURCE:** display names CHICAGO/MIAMI (and JUNO/DRONE); 303A (Miami default) is green `RGB(130,240,90)`, 303B (Chicago default) is cyan `RGB(70,205,255)`, BREAK is violet, and HOME lane order is Chicago then Miami (`ui.c` 89–99). Cue is a NEXT glyph and a blinking number (`draw_header` 5470–5475). A live chain shows a small amber **“CHN”**, not “CHAIN 01” (5476–5477). Unsaved can be `UNSAVED`, `SAVE AT STOP` / `SAVE WHEN QUIET` / `REC OFF TO SAVE`, compact `*` / `Q` / `!`, plus a 2-pixel dot (5450–5460, 5501–5502). About is **ABOUT DnBOX** (`ui.c` 461, 6525), not “ABOUT X0X” (MANUAL 680).

**N11. `panel.c` still says “KNOB BANK stays free.”** `panel.c` 32. **SOURCE** `main_fm1.c` 2098–2121: one press toggles Loopy clip mode (`LOOPY CLIPS` / `LP OFF`). Not in `MANUAL.md` at all, except a save-timing sentence in §15 (702).

### 5.4 §5–§8 Engines (909, 808, “303”, TB-3PO, Break)

**N12. §6 and §18 still describe the removed schwung-303.** MANUAL 317–320 and 936–949: TONE (CUTOFF, RESO, ENVMOD, DECAY), VOICE (ACCENT, WAVE saw/square, DRIVE, SLIDE), FILTER ADSR, FENV, Accent decay 30–3000 ms. Credits 993–995 still attribute “the 303” to schwung-303 / Open303. **SOURCE** at this SHA: Chicago, Miami, Juno and Drone page tables (`ui.c` 1509–1524, `ui_original_controls.h` 25–48, `BENGINE_N` = `"CHICAGO", "MIAMI", "DRONE", "JUNO"` at `ui.c` 426). `BEHAVIOR.md` BAS-07 (line 115) already said bass303 was removed, and only named Chicago and Miami — that second sentence is itself stale here (N13).

**N13. Juno and Drone are missing from every end-user manual.** **SOURCE** `ui.c` 426, 1512–1518; this is a 0.0.25.1 → 94197fc8 change (source map §7.1). `web/MANUAL.md` still says “current revision 13 uses only Chicago and Miami.” TB-3PO does not run on Juno or Drone (`engine.c` 118: “no arp”). ARP on a non-Chicago/Miami part says `TB-3PO: PICK CHICAGO OR MIAMI` (`ui.c` 4070–4072). The manual must say that, and must not call Juno or Drone a 303.

**N14. “Two identical 303s” is wrong in several ways.** They are two **bass slots**. Defaults are different engines (Miami on A, Chicago on B). Miami **ignores** accent and slide (`BEHAVIOR.md` BAS-13/14; source map §4.3). MANUAL §7 still teaches TB-3PO ACCENT and SLIDE as if both slots behave the same. **DOC vs SOURCE.**

**N15. “All 27 keys become a keyboard” is incomplete on the SMK.** MANUAL 287. **SOURCE** the live path is 37 physical keys when `ui_live_key_owner()` is true (`ui.c` 4279–4297, `live_edge` 4318–4344). Outer keys also page BREAK slices (`ui_brk_page_keys` 3596–3625). `BEHAVIOR.md` GEN-21 and `SMK37.md` 37 still say the ten outer keys “do nothing” — **stale**, and already in the source map.

**N16. Velocity: docs disagree with source and with the manual’s own FREE PLAY paragraph.** `BEHAVIOR.md` GEN-38 and `SMK37.md` 59: “keys and pads play without velocity.” MANUAL 312–314 correctly describes SMK strike velocity in FREE PLAY. **SOURCE** `drum_key` uses `plat_key_velocity`; HAL has velocity curves (`smk_input.h` 26, 234–263). GEN-38 is stale.

### 5.5 §9–§15 Patterns, save, settings

**N17. Loopy clip mode is almost absent.** One paragraph on save timing (MANUAL 702). No mention that **KNOB BANK** enters it, that SCALE/CHORD/ARP/NOTE REPEAT then belong to the clip keybed (`main_fm1.c` 1641, 1655, 1669), that NOTE REPEAT alone is **panic**, that NOTE REPEAT+ARP is LATCH, or that PATCH+SEQ REC / SEQ PLAY become Loopy history (`ui.c` 4497–4500). For a product whose public URL still contains “Loopy-Pro”, this is a large hole. **DOC vs SOURCE.**

**N18. Printed BT is Save; Bluetooth is inside GLOBE.** MANUAL’s SAVE row (133) is functionally right if the player knows SAVE is the BT button. MANUAL 684 says the Bluetooth **NAME** is “X0X- and four characters.” **SOURCE** on SMK: advertised/GATT name is `SMK_MODEL "-" +` four hex digits, for example `SMK-37 Pro-A1B2` or `SMK-37 Elite-A1B2` (`ble_glue.h` 94–123). The `X0X-` prefix is only the non-SMK path (`ble_glue.h` 102). `plat.h` 142 still comments `"X0X-XXXX"`. GLOBE still has a read-only NAME row (`ui.c` 598–601) that shows `plat_ble_name()`.

**N19. “ABOUT X0X” and “Bluetooth MIDI is new and not yet tested” and “SysEx is not sent or taken over Bluetooth.”** MANUAL 680, 816, 813–814. About is **ABOUT DnBOX** and shows `DnBOX ` + version (N10). Bluetooth exists and has a full path (source map §8.2). **SOURCE** APC Device Inquiry **is** answered over Bluetooth (`ble.c` 107–168, same 0x7E identity as `usb.c` 374–385). The “no SysEx” sentence is at least too strong; whether other SysEx is dropped is counted in `ble.c` 108–146. “Not yet tested on the keyboard itself” is **UNVERIFIED** at this SHA (may be stale).

**N20. Held-chord recording is undocumented.** **SOURCE** at this SHA: polyphonic slots can record held-key chords; toast `CHORD NOT SAVED` (source map §7.1). Not in `MANUAL.md`.

### 5.6 §16 MIDI and USB audio; §17 Troubleshooting

**N21. USB MIDI and USB audio names are wrong.** MANUAL 768, 820–821: the device “shows up as a MIDI device called `X0X FM-1`” and the same for audio. **SOURCE** SMK product string is `DnBOX <X0X_VERSION> SMK-37 Elite` or `… SMK-37 Pro` (`usb_product_name.h` 16–18). Manufacturer string is **`X0X`**, not DnBOX (`usb.c` 494). There is no separate MIDI jack string (descriptor jack names are index 0, `usb.c` 454–455), so hosts typically show the product (and maybe the manufacturer). Exact OS wording is **UNVERIFIED** (no device run).

**N22. “MIDI from the FM-1’s TRS MIDI IN” does not apply to the SMK.** MANUAL 770. **SOURCE** `x0x.c` 63–64: the SMK TRS jack is MIDI OUT and is unused; there is no TRS MIDI IN. `SMK37.md` 61 already says this correctly. The SMK manual must not tell players to use a MIDI IN jack.

**N23. MIDI channel numbers are still right; the names are not.** **SOURCE** `sequencer.c` 33–34: `SEQ_CH_DEFAULT = {9, 10, 1, 2, 3}` (0-based), i.e. 909 = ch 10, 808 = ch 11, slot A = ch 2, slot B = ch 3, BREAK = ch 4. On-screen labels are `BASS A MIDI CH` / `BASS B MIDI CH` (`ui.c` 469–470), not “303A MIDI CH”. Slot A defaults to **Miami**, slot B to **Chicago**. These channel numbers and slot IDs **must stay** (comment `ui.c` 95–97: “Project lanes, MIDI channels, engine buses and all saved arrays keep their IDs”). The manual should say “Bass A (default Miami), channel 2” — not “303A”.

**N24. Crash and safe-mode titles are DnBOX.** MANUAL 843, 847 and the recovery story still say “X0X CRASH”. **SOURCE** `main_fm1.c` 2236 `DnBOX CRASH`, 2291 `DnBOX SAFE MODE`. Boot factory lines are `DnBOX` and `909 808 CHICAGO MIAMI` (`boot_screen.h` 61–62) — BREAK is a part and is not on the factory subtitle; Juno/Drone are also absent. Whether that subtitle should change is an open question, not a manual “error” until the owner decides.

### 5.7 Behaviour file and SMK37 map (trusted by docs)

**N25. Stale behaviour rows that will mislead a new manual if trusted:**

| Claim | Where | Source at this SHA |
|---|---|---|
| Outer keys do nothing | GEN-21; `SMK37.md` 37; stale comment `main_fm1.c` 1991–1992 | Live 37-key path; BREAK paging (`ui.c` 3596–3625, 4279–4344) |
| Bank A = 909, 808, 303A, 303B; bank B starts with BREAK | GEN-31, GEN-32, CLP-28, `SMK37.md` 49–50 | `PART_ORDER` faders (`main_fm1.c` 883–935) |
| Keys and pads play without velocity | GEN-38, `SMK37.md` 59 | Velocity curves; MANUAL itself describes FREE PLAY velocity |
| Bluetooth MIDI is not available; BT only in clip mode | GEN-08, GEN-09 | Full BLE + device-BT-save path; GEN-10 (BT is SAVE) is still right |
| 303 parts use only Chicago and Miami | BAS-07 line 115 | Juno and Drone (`ui.c` 426) |
| GEN-21 citation `main_fm1.c:766-767` | GEN-21 | That line is APC layout at this SHA (source map §11) |

### 5.8 Finding counts

- **Verified known findings:** 3 (K1–K3).
- **New findings in this pass:** 25 (N1–N25), several of which group more than one stale sentence.
- **Combined actionable doc errors / gaps** used for work items: **28** (the 3 known, plus 25 new). Stale GEN rows inside N25 are not double-counted.
- **Agreeing items** (manual already matches source, keep): hold-help ~1 s (MANUAL 163–164, `HELP_HOLD` 900 ms); deferred save `SAVES WHEN STOPPED` / `SAVE AT STOP` / autosave 4 s (686–701, `ui.c` 2222–2294, 8771–8804); chain is live only (458–462); boot shows 909, empty (83–84, `ui_init`); KEY SOUND default silent while playing (89–90, `drum_key` 3205–3208); undo PATCH+SEQ REC / redo PATCH+SEQ PLAY; SMK legend table in `SMK37.md` 43–44 matches `panel.c` 28–31 (names only; faders and outer keys in the same file do not).

---

## 6. Glossary proposal

One term per physical control, used everywhere: manuals, help cards, web, checklists. **Printed SMK legend** is what the player sees on the plastic. **DnBOX function name** is the one word the manual uses for what it does (and what the help card already titles with the printed legend, `ui.c` 1905–1938). **Do not use** the Felucca synonyms in new player text, except once in a “if you used the FM-1 build” box. Code identifiers (`B_EDIT`, `PART_303A`) stay in source and in agent docs, never in the owner manual.

### 6.1 Physical controls

| Printed on the SMK | DnBOX function name (use this) | What it does (short) | Synonyms now used (retire from player text) | Where the synonyms appear |
|---|---|---|---|---|
| FX | FX | Reverb and delay. With SEQ: panic (all notes off). | — | Consistent. |
| SCALE | SCALE (list / yes) | Opens this screen as a list; confirms a question. In **clip mode**, it really is scale. | SEL, SCL | MANUAL §3; GEN-27; Felucca caption `UI_CAP_SEL` |
| CHORD | CHORD (accent) | Hold + key: accent. In clip mode: chord. | ENV | MANUAL 128, 253; GEN-27 |
| NOTE REPEAT | NOTE REPEAT — opens the **mixer** (when clip mode is off) | Mixer and master. Hold + key: slide on Chicago. In clip mode: panic; with ARP: LATCH. | LFO, MIX, MASTER | MANUAL 129, 608+; help “THE MIXER AND THE MASTER” (`ui.c` 1939–1940). Lamp means MIX, not note-repeat. |
| PARA | PARA (sound) | Sound of the selected part. Drum white keys **audition only**. | EDIT | MANUAL 130 and throughout; GEN-27 |
| GLOBE | GLOBE (settings) | Settings. Press again to close. Bluetooth lives here. | GLO | MANUAL 131; GEN-27 |
| PATCH | PATCH — **HOME / SONG / back / no** | From most views: HOME. Already on HOME: SONG. In a list: back. In a question: no. Hold as a modifier (pattern, mute, undo, tempo). | HOME | MANUAL 132; header word is still `HOME` (`VIEW_N`, `ui.c` 88). Help: “THE PATTERN: ALL FIVE PARTS” / “AGAIN: SONG”. |
| BT | BT — **Save** (not Bluetooth) | Tap: save (or queue until stop). Hold ~250 ms: copy. Hold ~400 ms: help. Bluetooth is GLOBE → BLUETOOTH. | SAVE | MANUAL 133; GEN-10 (correct). Lamp = unsaved / queued / failed, not wireless. |
| ARP | ARP (TB-3PO) | Line generator, Chicago or Miami only. | — | MANUAL §7. |
| SEQ | SEQ (steps) | Step grid / piano roll. Boot lands here (909). | — | Consistent. |
| SEQ PLAY | SEQ PLAY | Start/stop, on **press**. | PLAY | MANUAL 136. |
| SEQ REC | SEQ REC | Record on/off, on **press**. | REC | MANUAL 137. |
| OCTAVE− / OCTAVE+ | OCTAVE− / OCTAVE+ | Meaning depends on screen (see 6.3). | OCT- / OCT+ | MANUAL 124. |
| `<` `>` | `<` `>` (page / list value) | No list: one SELECT step, no hold-repeat. In a list: value, hold-repeat 400 then 80 ms. | SELECT arrows | `SMK37.md` 47; GEN-29, GEN-56. |
| KNOB BANK | KNOB BANK — **Loopy clips** | One press leaves the X0X instrument UI (`LOOPY CLIPS` / `LP OFF`). | “stays free” | `panel.c` 32 (comment only). |
| FADER BANK | FADER BANK | Bank A / B. LED on for bank B (GEN-30, still right). | — | `SMK37.md`. |
| K1 | MASTER | Volume. Never a taught role. | — | Consistent. |
| K2 | K2 (page / chance / tempo) | Page or list row; held-step chance; PATCH+K2 = tempo. | SELECT | MANUAL 121. |
| K3 | K3 (part) | Walks 909 → 808 → BREAK → Chicago → Miami. In a list: value. | ALGORITHM | MANUAL 123. |
| K4 | K4 (pattern) | Cue pattern (all five on HOME; this part on a part screen). Ends a chain. | PRESETS | MANUAL 122. |
| K5–K8 | K5–K8 | The four values at the bottom. | KNOB 1–4 | MANUAL 125. |
| 16 pads | Pads (same as the 16 white keys) | Step grid, or patterns on HOME/SONG. | — | `PAD_MAP.md`; GEN-22. |
| Faders 1–4 | Faders, bank A or B | A: 909, 808, BREAK, Chicago levels. B: Miami level, reverb return, delay return, master FILTER. Soft pickup. | “303A/303B/BREAK” bank claims | GEN-31/32, `SMK37.md` 49–50. |
| Pitch / mod wheels | Pitch wheel / mod wheel | On a 303-style slot: bend / cutoff (Miami: wobble depth), **without writing saved pots**. | TUNE / CUTOFF only | GEN-35/36; Miami path in `engine.c` 590. |
| 37-key keybed | Keys | Middle 27 match the old FM-1 bits; outer ten are live and also page BREAK slices. | “27 keys”; “outer keys do nothing” | MANUAL 287; GEN-21. |

### 6.2 Parts and engines (one name each)

| Use this | Do not use for players | Notes |
|---|---|---|
| 909, 808, BREAK | — | Unchanged. |
| Chicago | 303, 303B (as a player name), “the 303” for both slots | Chicago **is** the 303 engine. Default on bass **slot B**, third from the left on HOME after BREAK, MIDI channel **3**. |
| Miami | 303, 303A, “the other 303” | Miami is **never** a 303. Default on bass **slot A**, rightmost on HOME, MIDI channel **2**. Ignores accent and slide. |
| Juno, Drone | 303 | Selectable engines at this SHA. No TB-3PO. |
| Bass slot A / Bass slot B | 303A / 303B in player text | Keep as the MIDI and settings labels (`BASS A MIDI CH`), and when explaining that IDs do not move. |

### 6.3 Same gesture, many meanings (document in one place, keep the behaviour)

The manual should have a single “same key, different job” table, not scatter synonyms:

- **White keys:** steps (SEQ) · patterns (HOME) · song-bar patterns (SONG) · sound audition (PARA drums) · 303 notes (keyboard) · paste targets (BT held) · chain ends (two keys on HOME).
- **Black keys:** drum tracks · BREAK slices · part mutes on HOME **in display order** · mixer channel · mute when PATCH is held.
- **OCTAVE±:** step pages · pattern banks 1–16 / 17–32 · 303 keyboard octave · held-step octave · TB-3PO new/mutate · copy-bank when BT held.
- **PATCH:** back / no / HOME / SONG / modifier. One unused release changes the view.
- **NOTE REPEAT:** mixer vs clip-mode panic.
- **BT:** save vs copy vs help, on tight timings (tap / 250 ms / 400 ms).
- **Transport is on press; almost every other `B_*` button is on release.**

Expert shortcuts to preserve and to document (do not “simplify” in firmware — source map §8.6): PATCH+SEQ REC undo, PATCH+SEQ PLAY redo, PATCH+K2 tempo, PATCH+white pattern, PATCH+black mute, BT+white paste, BT+knob forget motion / p-lock, hold step + K5–K8 p-lock, hold step + K2 chance, hold 303 step + K3 velocity, CHORD/NOTE REPEAT as accent/slide, NOTE REPEAT+OCTAVE tie, CHORD+OCTAVE rest, FX+SEQ panic, two white keys chain, KNOB BANK clip toggle, clip PATCH+transport history, fader pickup, wheel bend that does not write pots, `RELEASE STEP FIRST`, live-note cancel on context change.

---

## 7. Proposed manual structure

Write **one** SMK-37 Elite and Pro end-user manual, titled **DnBOX**, stating the firmware version (§3). Keep `web/MANUAL.md` as the install/samples/projects companion, with a clear link to the control manual — not a claim that the control book is only “historical.” Use SMK printed names from the first sentence. Each section says **how** and **why**. Felucca names appear once, in an appendix. Expert shortcuts are not deleted; they live after the first-path chapters so a novice is not blocked by them.

Suggested outline (goal: first sound, then a pattern, then save, obvious):

1. **What this is, and which version.** `DnBOX` + `X0X_VERSION`. Elite and Pro: same playing surface; names on USB and Bluetooth differ (§8). Spell DnBOX exactly. Goal: a reliable live instrument.
2. **Look at the panel.** Photograph or drawing of the **SMK** (not the FM-1). Glossary table from §6.1 in short form. “Hold any button about a second: a card says what it does; let go and nothing happens.” Why: the printed words are not always the job (BT is Save, NOTE REPEAT is the mixer, KNOB BANK is Loopy).
3. **First sound (from a cold start).** Why the unit boots on the **909 step grid** (already in SEQ, empty). Stopped: black key 1 selects the kick and you hear it; white keys 1, 5, 9, 13 place steps. SEQ PLAY starts the loop. Why white keys on this screen write steps, and why you should **not** press PATCH or KNOB BANK yet (HOME is silent; clip mode leaves this instrument). If you did: PATCH from HOME is SONG — press PATCH again for HOME, then SEQ to return to the grid; KNOB BANK again for `LP OFF`.
4. **First pattern.** Add clap and hats (same as today’s tour steps 3–4, which match source **if** you are still on SEQ). Why KEY SOUND is silent while playing (no stray hit). How long/rate sit on SEQ’s SEQUENCE page (K5–K6).
5. **First bass line — Chicago, then Miami.** Why K3’s order is 909, 808, BREAK, **Chicago**, **Miami** (three clicks to Chicago, four to Miami — never “two clicks to 303A”). Why Chicago is the 303 engine and Miami is not, and why they are not identical (accent/slide). ARP / TB-3PO only on those two. PARA to shape the sound (engine pages, not the old TONE/VOICE table). Juno and Drone: short pointer, no TB-3PO.
6. **First break.** K3 two clicks from 909 (not four). White keys are the bar; why that is different from a drum track.
7. **Save, and why it waits.** The button is printed **BT**. Why a save during play says `SAVES WHEN STOPPED` and writes only when stopped and quiet (a write would hitch the audio). Autosave. How to tell unsaved (`UNSAVED` / `Q` / the amber dot). How to load. Why a **chain is not saved**. Why Bluetooth is not this button.
8. **Step versus sound (keep this split).** SEQ writes steps; PARA changes the sound; PARA drum whites **audition only**. Why `RELEASE STEP FIRST` can feel like “stuck.” How p-locks work (hold a step). This is the place to be exact, because the old manual is wrong here.
9. **The rest of the instrument**, in today’s depth but with current names and engines: mixer (NOTE REPEAT), FX, GLOBE, song, motion, probability, MIDI channels (by slot and default engine), USB audio (name `DnBOX … SMK-37 …`), Bluetooth (name `SMK-37 Pro-XXXX` / `Elite-XXXX`, UUIDs unchanged), clip mode / Loopy (KNOB BANK, and that SCALE is then really scale), SMC if connected, troubleshooting (DnBOX CRASH, SAFE MODE, update mode, **check the model printed on the keyboard**, not the USB name — §8).
10. **Expert shortcuts** (muscle memory). The full list in §6.3. Why transport is on press and other buttons on release. Why these must not be removed “for simplicity.”
11. **Parameters** — Chicago, Miami, Juno, Drone, 909, 808, BREAK, FX, master — from `ui_original_controls.h` and the drum tables **at this SHA**, not the schwung-303 list.
12. **Credits and licences**, current engines and fonts. Appendix: FM-1 / Felucca name map for people who used the old build; protocol identifiers that must not change (§8).

Dot’s proposed screens (banners “EDITING: SOUND”, “PROJECT SAVED”) may inform a **future** UI, but the manual of record must describe the strings that exist (`BD  DECAY` readout, `SAVED` toast for 1300 ms). Coordinate any screenshot set with Dot so two contradictory “current” pictures are not published.

---

## 8. Naming audit (USB, MIDI, Bluetooth, protocol)

User-visible names may be spelled `DnBOX`. Identifiers used by installers, M-UPGRADE, Loopy and the editor protocol **must stay the same**. Elite and Pro **share playing code**; only display and package strings differ (source map §0; `tools/build.py` 169–180: “one hardware, three names”).

### 8.1 Table

| Name | Where defined (file:line) | Elite value | Pro value | User-visible? | Must stay the same? | Issue |
|---|---|---|---|---|---|---|
| USB product (app) | `firmware/src/usb_product_name.h` 15–18, included from `usb.c` 501–502 | `DnBOX <X0X_VERSION> SMK-37 Elite` | `DnBOX <X0X_VERSION> SMK-37 Pro` | **Yes** (OS MIDI and USB-audio device list) | Display may be worded for players, but see 8.2–8.3: **do not drop the `SMK-37` token**; **do not use this string to choose a package** | MANUAL still says `X0X FM-1`. Owner’s historical “rename Pro to `X0X 37PRO`” is **not** what source does. |
| USB manufacturer (app) | `usb.c` 494 `STR1` | `X0X` | `X0X` | **Yes** (some hosts show manufacturer) | Changing it can change the OS port name and break name-matching tools | Spelling is X0X, not DnBOX. Flag for a later owner decision; not a silent change. |
| USB product (update loader) | `usb_product_name.h` 10–14, `usb.c` 496–497 | `SMK-37 Elite Update` | `SMK-37 Pro Update` | **Yes** while in update mode | **Prefer stay.** M-Flasher and humans look for a model token; comment in `loader.c` 5–7 still says “Felucca Update” for the non-SMK path | Manufacturer in loader is “Hügelton Instruments” (`usb.c` 491–492). |
| USB VID / PID (app) | `usb.c` 8, 431–435 | VID `0x1209`, PID `0x0001` | Same | Indirect (OS device id) | **Yes. Never change.** Comment: “pid.codes test id.” `usb_product_name.h` 6: “OTA model admission and USB VID/PID stay unchanged.” | Shared by Elite and Pro. Not a model discriminator. |
| USB VID / PID (loader) | `loader.c` 15, `usb.c` with `FELUCCA_USB_PID 0x0002` | `0x1209` / `0x0002` | Same | Indirect | **Yes.** | |
| MIDI port / jack string | `usb.c` 450–455 (jack descriptor string index 0) | (none; hosts use the product) | Same | **Yes**, as the OS port | Do not add a second conflicting name without a host check | MANUAL’s `X0X FM-1` is wrong. Exact macOS/iPad/Windows wording **UNVERIFIED**. |
| Package / OTA identity (`FELUCCA_ID`) | `x0x.c` 53–58; `tools/build.py` 167, 475, 724–727; `ota.c` 150–152 | `SMK-37 Elite_900` (release: `SMK-37 Elite_9` + XXYYZZ) | `SMK-37 Pro_900` (release: `SMK-37 Pro_9` + XXYYZZ) | Shown in the installer after handshake, not as the everyday USB name | **Yes. This is what admits a package.** A wrong-model write can make the keyboard unusable (installer warning, `web/x0x_installer.html` 421, 436). | Older Elite units can **report Pro**. Always check the **printed** model. |
| Loader OTA identity | `loader.c` 17–18 | `ota-SMK-37 Elite_900` | `ota-SMK-37 Pro_900` | To updaters | **Yes.** Hosts look for `ota-` (`loader.c` 7). | |
| Stock update-record parm id | `ota.c` 33 `OTA_PARM_ID` | `ota-SMK-37 Elite_016` (20 bytes) | `ota-SMK-37 Pro_016` (18 bytes) | No (flash record) | **Yes.** So M-UPGRADE will still accept that unit’s factory package (`SMK37.md` 88–89). | This is the “brick if the wrong model was written” identity. |
| Handshake SysEx | `tools/fm1_install.py` 90 `HS_QUERY` = `F0 00 32 45 …`; `ota.c` cmd `0x11` | Identity reply = `FELUCCA_ID` | Same mechanism | To installers | **Yes.** `fm1_install.py` 98–99 and `web/fm1ota.js` 180: **port names never gate discovery**. | Keep it that way. |
| M-UPGRADE upgrade SysEx | `usb.c` 7, 938; `ota.c` 7 | `F0 22 24 35 7F F7` | Same | No | **Yes.** | UBOOT soft key `F0 22 24 35 7D F7`. |
| BLE advertised / GATT name | `ble_glue.h` 94–123; shown on NAME (`ui.c` 598–601, `plat_ble_name` `ble.c` 648) | `SMK-37 Elite-` + 4 hex (from a stable address suffix) | `SMK-37 Pro-` + 4 hex | **Yes** (Bluetooth MIDI device list, GLOBE NAME) | **Prefix may be discussed**, but the **four-hex suffix and address derivation must stay** (“Keep the factory-derived address and four-hex-digit suffix stable,” `ble_glue.h` 94–95). | MANUAL says `X0X-` + four characters — that is the **non-SMK** prefix only (`ble_glue.h` 102). Not `DnBOX`. |
| BLE-MIDI service UUID | `ble_midi_uuid.h` 11–13 | `03B80E5A-EDE8-4B33-A751-6CE34EC4C700` | Same | To BLE-MIDI hosts | **Yes.** Standard BLE-MIDI UUID. | |
| BLE-MIDI characteristic UUID | `ble_midi_uuid.h` 14–15 | `7772E5DB-3868-4112-A1A9-F2669D106BF3` | Same | To BLE-MIDI hosts | **Yes.** Standard BLE-MIDI UUID. | |
| APC / Loopy identity SysEx | `usb.c` 374–385; same on BLE `ble.c` 157–164 | Universal inquiry reply: manufacturer `0x47` (Akai), family `0x29` (APC40 mkII), firmware 1.2, device ID `0x7F` | Same | To Loopy Pro and APC hosts | **Yes. Loopy mappings depend on this, not on the USB product string.** | Introduction `F0 47 7F 29 61 …`. |
| Editor INFO version string | `editor.c` 875 | `"X0X " X0X_VERSION` (24-char field) | Same | To the web editor / sample tools, not on the LCD | **Treat as protocol until tools are updated.** Changing to `DnBOX` without a tool change can break parsers. | Storage magic `"X0X0"` (`storage.c` 19) and data family `"X0X"` (`storage.c` 66) must stay. |
| About / crash / boot / settings | `ui.c` 2682, 461, 6525; `main_fm1.c` 2236, 2291; `boot_screen.h` 61 | `DnBOX <ver> …`, `ABOUT DnBOX`, `DnBOX CRASH`, `DnBOX SAFE MODE`, boot `DnBOX` | Same | **Yes** | Display; already spelled DnBOX | MANUAL still says ABOUT X0X / X0X CRASH. |
| On-screen part names | `ui.c` 108–149, 426 | CHICAGO / MIAMI / JUNO / DRONE, 909, 808, BREAK | Same | **Yes** | Display follows the engine; slot IDs must stay | Manual 303A/303B. |

MKE-P37 (same hardware, third name): USB `DnBOX <ver> MKE-P37`, identity `MKE-P37_900`, BLE `MKE-P37-` + four hex. Starrykey 37 Play uses the Pro package (`SMK37.md` 24).

### 8.2 Owner note: “X0X 37PRO”, and a USB-name mix-up that bricked an Elite

**SOURCE, current tree:** the Pro USB product is `DnBOX <version> SMK-37 Pro`, not `X0X 37PRO` and not `DnBOX <version> 37PRO`. The test `tests/host/usb_product_names_test.py` 52–81 is explicit:

- M-Flasher v0.11 discovers units by a **token in the OS port name** (`fm-1`, `smk-37`, `felucca`, …), **not** by SysEx.
- It **asserts** that `DnBOX 0.0.26 37PRO` and `… 37ELITE` are **not** recognised.
- It **asserts** that the real strings contain `SMK-37` and therefore **are** recognised.

Comments in `tools/fm1_install.py` 98–99 and `tools/fm1_sample_upload.py` 53 give `DnBOX <ver> 37PRO` only as an example of a name that **changes** and must **not** gate discovery. That example is not the current descriptor.

The installer (`web/x0x_installer.html` 421, 436) already warns, in owner words: check the model **printed on the keyboard**, not just its USB name; an older Elite may report Pro; a wrong-model write can make the keyboard unusable.

**INFERRED recommendation:** do not implement `X0X 37PRO` as the USB product. It uses the wrong product spelling, it omits `SMK-37` (M-Flasher would not see the unit), and it looks too much like a model identity. If the owner still wants a shorter name, any change must be tested against M-Flasher and must **never** be used to choose Elite vs Pro packages. Package identity (`SMK-37 Elite_900` / `SMK-37 Pro_900`) and `OTA_PARM_ID` stay.

### 8.3 Tools that identify a device by USB / port name

These need a docs or tool warning. Discovery that uses the handshake identity is the safe pattern.

| Tool | How it finds the unit | Risk |
|---|---|---|
| `tools/fm1_install.py`, `web/fm1ota.js` | Every port is a candidate; **handshake identity** decides (`fm1_install.py` 98–99, 408–421; `fm1ota.js` 180) | **Low** for model mix-up, if the player confirms the printed model. This is the right pattern. |
| `web/x0x_installer.html` | Handshake + explicit printed-model warning | Warns about Elite reporting Pro. Keep this wording in the player manual. |
| `web/offline-sequencer/project-adapter.js` 11–12 | Refuses a USB port name as a package model: “Choose the verified SMK-37 Elite or Pro package model, not a USB port name.” | **Good.** |
| **M-Flasher** (external, `jvitkauskas/m-flasher` v0.11, documented in `usb_product_names_test.py`) | Tokens in the **USB / OS port name** | **High** if the product string loses `SMK-37`. Does not prove Elite vs Pro. |
| `tools/ble_autotest.py` 37, 56–57 | `PORT_KEYS = ("X0X", "SMK", "FM-1", "Felucca")` in the port name | Current `DnBOX … SMK-37 Pro` matches `SMK`. A name with only `DnBOX` and `37PRO` would **miss**. Not a package writer, but it is name-gated. |
| `tools/fm1_debug.py` 29 | First output port whose name contains `"FM-1"` or `"X0X"` | **Does not look for `DnBOX` or `SMK-37`.** If the host shows only the product string `DnBOX … SMK-37 Pro`, this tool **will not find the unit**, unless the manufacturer `X0X` is part of the port name (**UNVERIFIED** per OS). |
| `tools/manual_html.py` 244 | Generated meta: “X0X … FM-1” | Docs only, not a flasher. |

No tool in this tree should be documented as “pick Elite or Pro by reading the USB product.” That is the brick path the owner already hit.

### 8.4 Spelling flags (`DnBOX` only)

User-visible text that is **not** exactly `DnBOX`:

| Spelling | Where (examples) | User-visible? |
|---|---|---|
| **DNBOX** | `web/offline-sequencer/index.html` 4: `<strong class="brand">DNBOX</strong>`; `web/offline-sequencer/touchosc/pack.py` 96: `"DNBOX / MIDI PERFORMANCE"` | **Yes** — fix in a docs/web pass (coordinate with the web owner). Other `DNBOX_*` hits are environment-variable names in tests (`DNBOX_CHROMIUM_EXECUTABLE`) — not player-facing. |
| **X0X** | `docs/MANUAL.md` title and body (about 100 uses); `docs/manual.html`; `README.md`; `SMK37.md`; USB manufacturer `usb.c` 494; editor INFO `editor.c` 875; nav `aria-label="X0X"` in `web/x0x_projects.html` 53 | Mix: manuals **yes**; manufacturer **yes**; INFO **protocol**. |
| **X0X37** | `PAD_MAP.md` title; `docs/FIRMWARE_VERSIONING.md`; README / historical URLs `X0X37-for-Loopy-Pro` | Docs and URLs. The live install URL still uses this path (`README.md` 20). Repo-rename is a separate owner steer (`steers/dot-rename-20261113.txt`) and is **not** done in this pass. |
| **X0X FM-1** | MANUAL 768, 820 | **Yes**, and **wrong** for SMK. |
| **X0X-** (BLE) | MANUAL 684; `plat.h` 142 comment | Manual is **wrong** for SMK. |
| **37PRO / 37ELITE** | Only as compile flags `X0X_USB_37PRO` / `37ELITE` and as rejected test strings | Not the current user-visible USB name. |
| **DnBox**, **DNBox** | None found in firmware or primary docs | — |
| **dnbox-** file names | `dnbox-….x0xproject` downloads in `web/x0x_projects.html` | File names; optional to capitalise. |

Firmware on-screen strings already use `DnBOX` (About, crash, safe mode, boot, USB product). The gap is the manuals, the manufacturer string, the BLE prefix, the editor INFO prefix, and the offline-sequencer brand.

---

## 9. Ordered work items

Each item is planning only. **Owner** is the role that should do the work when the orchestrator dispatches it. Docs edits must be coordinated with Dot / the docs owner (rule: do not duplicate). No firmware behaviour change is required for a correct manual; protocol IDs must not be changed to “clean up” names.

| # | What | Why | Evidence | Owner | Risk |
|---|---|---|---|---|---|
| 1 | Agree the glossary (§6) with the owner and Dot, especially: BT = Save, NOTE REPEAT = mixer, PATCH = HOME/SONG, K3 order, Chicago vs Miami, “never call Miami a 303.” | Without one term list, the next manual rewrite will mix EDIT/PARA again. | K1–K3, N2, N18, GEN-27 | Orchestrator + owner; Docs Keeper drafts | Low if we only document. High if someone remaps buttons “to match the manual.” Do not remap. |
| 2 | Write a **new** SMK end-user manual at the structure in §7, titled DnBOX, stamped with `X0X_VERSION` / `94197fc8` until a number is allocated. Use SMK printed names. Fix every K and N item. STE-plain owner sentences. At most the facts that are SOURCE at this SHA. | The current book is the FM-1 X0X manual and is wrong on the tour, engines, PARA, names and USB. | K1–K3, N1–N24, §2 inventory | Docs Keeper, coordinated with Dot | Publishing screenshots that match Dot’s **proposal** as “current” would mis-teach. Describe current strings. |
| 3 | Replace the getting-started tour: 909 SEQ first sound → pattern → K3 to Chicago (3 clicks) or BREAK (2) → save with **BT**, with a “if you pressed PATCH / KNOB BANK” recovery. Mute the break with **black key 3** on HOME, and warn that PATCH leaves SEQ. | Today’s tour lands on the wrong part and mutes Miami instead of BREAK. | N5, N6, N7, K1 | Docs Keeper | Low. |
| 4 | Rewrite §6 / §18 parameters from `ui_original_controls.h` and drum tables; add Juno/Drone; say Miami ignores accent/slide; drop schwung-303 credits as the current engine. | Players cannot shape Chicago/Miami from the old TONE/VOICE pages. | N12–N14, N8 | Docs Keeper | Medium: parameter tables must be copied from source, not from memory. |
| 5 | Add a Loopy / clip-mode chapter (KNOB BANK, SCALE truly scale, NOTE REPEAT panic, LATCH, Loopy undo) and a Bluetooth chapter with the **real** name `SMK-37 Pro-XXXX` / `Elite-XXXX`. | Almost missing, despite the product’s Loopy use. | N17, N18, `APC40_IMPLEMENTATION.md` | Docs Keeper | Must not tell players that NOTE REPEAT is still the mixer in clip mode. |
| 6 | Add the USB / install safety page: product string vs package identity; **check the printed model**; Elite may report Pro; wrong package can brick; VID/PID and OTA ids do not change. | Owner already had an Elite bricked by a name mix-up. | §8, installer 421/436 | Docs Keeper | **High** if this page is vague. Quote the installer’s own warning. |
| 7 | Align or clearly split `web/MANUAL.md`, `docs/manual.html`, `docs/NEW.md`, `README.md`, `SMK37.md`: same glossary, same part order, same fader banks, `DnBOX`, current installer URL. Fix `DNBOX` in the offline sequencer and TouchOSC label. | Two manuals and a stale SMK37 map will keep teaching 303A and the old faders. | N1, N4, N25, §8.4 | Docs Keeper + web, with Dot | Site “legacy.html” must not remain the only full manual without a warning that it is wrong. |
| 8 | Correct `BEHAVIOR.md` GEN-21, GEN-31, GEN-32, GEN-08, GEN-09, GEN-38, BAS-07 (Juno/Drone), CLP-28, and the `panel.c` 32 comment “KNOB BANK stays free.” These are agent contracts, not the player manual, but docs will be written from them. | Stale contracts caused the known fader and outer-key errors. | N25, K2, source map §6 | Docs / rules lane per project rules (12A: rule edits on a lane, Orchestrator review) — **not** a silent docs-only edit if they are rule files | Process risk: do not edit rules and player docs in parallel on the same branch without the 12A flow. |
| 9 | Add a **manual alignment check**: About format `DnBOX ` + `X0X_VERSION`; USB product fixtures already in `usb_product_names_test.py`; optional STE and “303A” / “X0X FM-1” / “two identical 303s” bans in player markdown. | The manual froze at 0.0.25.1 while firmware moved. | §3, `git log` on `docs/MANUAL.md` | Build and Test Runner + Docs Keeper | Low. Do not gate on proposal copy. |
| 10 | Tooling follow-up (not a player-manual edit): `fm1_debug.py` port filter; confirm `ble_autotest.py` keys; never add USB-name model admission. Leave protocol strings (`FELUCCA_ID`, APC 0x47/0x29, BLE UUIDs, `ED_INFO` “X0X ”) unchanged until a deliberate, tested migration. | Name-gated tools plus a USB rename are how a unit gets the wrong package or becomes undiscoverable. | §8.3, `fm1_debug.py` 29 | Firmware / tools owners, after Orchestrator | **High** if INFO or identity strings are “rebranded” in the same pass as the manual. |

---

## 10. Open questions for the owner

1. **What version number should a published manual print?** This SHA is `v0.0.25.1-87-g94197fc83`. The ledger has **no** allocated number yet. Should the manual wait for an allocated `X0X_VERSION`, or ship as “source 94197fc8 (development)”?
2. **Should the USB manufacturer stay `X0X`?** Changing it changes some hosts’ port names and can break `fm1_debug.py` and similar filters. It is user-visible and is not `DnBOX`.
3. **The historical “rename Pro USB to `X0X 37PRO`” request:** current source uses `DnBOX <ver> SMK-37 Pro` on purpose so M-Flasher still sees `SMK-37`. Do you want that short name anyway, a `DnBOX` short name, or to keep the current string?
4. **Bluetooth advertised name** is `SMK-37 Pro-XXXX`, not `DnBOX-XXXX`. Should that change (suffix and address must stay), or only be documented?
5. **Editor INFO still says `X0X <version>`.** May tools depend on the `X0X ` prefix? (**UNVERIFIED** beyond the protocol doc.) Change only with a tool pass?
6. **Juno and Drone:** document as shipped at this SHA, or are they still experimental for the player manual?
7. **Dot’s proposed screens** (HOME titled “DnBOX”, “EDITING: SOUND”, “PROJECT SAVED”): documentation of current firmware only, or is a UI change in scope later? This plan assumes **current** firmware until you say otherwise.
8. **Boot subtitle** `909 808 CHICAGO MIAMI` omits BREAK (and Juno/Drone). Leave it, or treat as a future firmware copy issue?
9. **One manual for Elite and Pro** (recommended: yes, with a names appendix) or two books?
10. **Repo and Pages URLs** still say `X0X37-for-Loopy-Pro`. The rename steer exists. Should the manual already use `gobbajogger.github.io/DnBOX/`, or only after the redirect is live?

---

## 11. Limits (unverified)

- No device and no host-UI run in this pass. Pickup feel, exact OS port wording, LED brightness and LCD latency are not measured.
- Dot’s full render harness (`render-before.c`, extra `before/` frames) is not in the handoff drop on this box (source map §9, §11).
- Whether “Bluetooth MIDI is new and not yet tested on the keyboard” is still true is unverified.
- Whether `fm1_debug.py` matches in practice depends on whether the host puts manufacturer `X0X` into the port name — unverified per OS.
- Elite vs Pro plastics and silkscreen are outside this tree; interaction code is the same.
- MKE-P37 and Starrykey were mapped only at the identity-string level.
- This plan did not re-audit every sentence of `APC40_IMPLEMENTATION.md`, `SMC_MIXER.md`, or the 220-line `docs/NEW.md` history against source; those should be checked when work item 7 runs.
- Ledger: no 0.0.26 allocation found; do not treat 0.0.26 as a published version.

---

*End of plan. Source worktree `/workspace/smk-ux-src` at `94197fc839b00aff47b98ca02f3c6396e4a6eabf` was not modified. Only this file is the deliverable.*
