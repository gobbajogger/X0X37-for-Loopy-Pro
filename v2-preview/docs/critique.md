# DnBOX website v2 — self-critique

This is a self-critique of the task-led direction in `/workspace/website-ux/v2/`. It is not user testing, not an aesthetic pass, and not an owner approval. Self-checks below are labelled as self-checks against the eight-criterion rubric used on options A, B and C.

## Why this is not another option in a pick, and not a generic template

Options A, B and C were three visual treatments of the same two pages: a product plate with a large (then corrected) SMK-37 drawing, and a mostly static installer. The owner still called them generic. v2 stops treating those as a choice. It is one direction, built around musician tasks.

Concrete differences from A, B and C:

1. **The first page is a task hub, not a product plate.** A, B and C opened with “DnBOX” as a large hero (after the 22:54 UTC+10 correction) and a hardware drawing as the main picture. v2 puts a modest “DnBOX” in the navigation, names the SMK-37 only as a supporting line (“Firmware for the M-VAVE SMK-37 Elite and Pro”), and leads with “What do you need to do?” and an ordered list of jobs. The drawing is small, at the bottom, as support. That is a direct response to “Why big smk37??” and to “options are still generic.”
2. **The installer is a decision flow, not a single still card.** A, B and C each had one install page and one error page. v2 steps through device, version, path, backup, a simulated write, a done state, and four failure states, with a stage rail outside the card and Help plus the next action at the card’s corners. Device identity is the printed name *and* the installer’s check; a USB name is explicitly not confirmation. 0.0.25.1 is the default; 0.0.26 at `94197fc8` is unselected and requires an acknowledgement of its limits; withdrawn `600b3b45` is not offered.
3. **First use, recovery, and presets are first-class pages.** A, B and C had no first-pattern tour, no engines section grounded in the manual, and no preset librarian. Recovery existed as a same-page section; v2 makes it a page of its own, linked from the hub and from the installer’s Help *before* a write, and refuses to present the FM-1 rescue script as an SMK procedure.
4. **The instrument is shown as it actually appears, with a label.** A, B and C used only an authored SVG. v2 still uses that SVG, small. Where a screen helps, it uses real emulator LCD captures (`lcd-elite-360mhz-15001ms-3x.png`, `2003ms`, `native-gate-1-lcd_end-3x.png`), each captioned “emulator capture, not hardware.” The empty 909 SEQ is the first-start fact from the playing manual, not a decorative crop.
5. **Presets are a real flow, not a “coming, owner TBD” slot.** The page follows `docs/plans/2026-11-13-preset-file-io-research.md`: `.dnpr` files, validation words (`BAD FILE`, `NEWER DATA`, `WRONG ENGINE`, `MEMORY FULL`), backup-before-overwrite, factory export OFF, and an honest **device send is blocked** banner (research introduction, R3, §5). It does not invent Elite/Pro interchange, and it contains no Web MIDI code.
6. **Engine names are not smoothed over.** A, B and C did not teach the engines. v2 lists 909, 808 and Break from the published manual and the 909 LCD; it says Chicago is the 303-style engine in the preset format while the published manual still says 303A; it never calls Miami a 303; it does not invent a Juno or Drone tour.

It is still a static HTML prototype. It does not become “not generic” only by being darker or more carefully typeset. If it fails, it will fail because the task writing is dry, because a quiet grotesk list can still read as a manual, or because the owner wanted more of the instrument in the first view than a supporting drawing and one LCD. That risk is real, and it is the opposite risk of A, B and C (which put the instrument first and the jobs second).

## Baud Girl and Felucca — reasons, not looks

Taken from Dot’s `AUDIT.md` and `reading.md`, and kept in this site’s language:

- **Hierarchy.** One question, then an ordered list of jobs. One ice-coloured primary action at a time. The brand is a wordmark, not a billboard. (`AUDIT.md`: “a small body paragraph and a pair of modest actions follow, not five equally important feature cards.”)
- **Focused task surface.** The installer and the preset librarian are each one card. A stage rail sits outside it. Samples, hashes, and release-incident history are not the hero. (`AUDIT.md`, Baud Girl installer.)
- **Help in context.** Every install state has Help in the card, including before a write. Recovery is also its own page. Error cards say what was *not* changed. (`reading.md`, Baud Girl: Help “including before anything fails.”)
- **Truthful state.** 0.0.25.1 is the known-good fallback and is not a hardware sign-off. 0.0.26’s limits sit next to the offer. The iPhone path is supported and tagged UNVERIFIED. The prototype says, on every install and preset page, that it does not connect and does not write. Device send for presets is blocked, and the page says so. (`AUDIT.md`: “Positive capability result is phrased in user terms with precise limitations.”)

Felucca is used for authored-geometry craft (the small SMK-37 drawing) and for direct route naming (Install, Recover, Presets — not “Studio” and “Simulator” as equal actions). Its wood, gold line-art, “first custom firmware” claim, and competing CTAs are not used.

The comparison boards (`compare-home.png`, `compare-install.png`) are for that review, not as proof of likeness. v2 does not use Baud Girl’s cream italic, magenta, or browser-smile tiles.

## Self-check against the eight-criterion rubric

These are self-checks, not tests.

1. **Model / version confidence — pass (self-check).** Elite and Pro, 0.0.25.1 as the known-good fallback, on-device label 0.0.25, and source `79280e76` are in the installer’s first version step and in the home truth strip. 0.0.26 at `94197fc8` is described as a hardware-test candidate, unselected.
2. **Device match — pass (self-check).** The first install step is the printed name; a USB or port name is not confirmation; the real installer also checks the connected device and must stop on a mismatch; Starrykey 37 Play uses the Pro package; an Elite may report a Pro-like USB name. The mismatch error is its own state, and says the write did not start.
3. **Clear next action — pass (self-check) on a 1280 × 800 first view** for the hub (the task list itself), the install device step (“This is the printed name — continue” is in the first view), and presets (“Load a shared file”). **Partial on 390:** on the install device step, the primary button sits a short scroll below the two model choices and the match callout. That is better than burying it, and it is still a gap against the rubric’s “in the first view” wording.
4. **Honest readiness — pass (self-check).** No “Ready,” “Stable,” or “Latest” without a basis. 0.0.25.1 is software-checked, not signed off as a physical device test. 0.0.26 requires a visible acknowledgement of its limits. A simulated finish is labelled as not a confirmed restart. Preset “CRC ok” is labelled as a simulation.
5. **Recovery before it is needed — pass (self-check).** “If the device does not start” is in the install card’s lower-left on the first step. Recovery is a page of its own, linked from the hub, and is the first node of the install flow. The FM-1 rescue script is explicitly not offered as an SMK procedure.
6. **Understandable errors — pass (self-check).** Each failure is a heading in plain language (USB lost, write stopped, device mismatch, screen stayed dark; BAD FILE, NEWER DATA, WRONG ENGINE, MEMORY FULL, factory refused, sample missing). Each says what was not changed, and gives one next step. The stopped-write cards say this page cannot tell whether the keyboard will start on its own.
7. **Identity — pass (self-check) as a dark, correctly spelled DnBOX site with a small authored instrument and labelled LCD captures.** This is not an aesthetic pass by the owner. The risk, noted above, is that a task list in a quiet grotesk can still feel like a manual. The ice accent is used for actions and focus, not for 909, status, and brand at once (amber appears on the real LCD captures, and as a warning tag with a word, not as the brand colour).
8. **Restraint and accessibility — pass (self-check).** No feature-card grid, no popularity or analytics, no third equal “Try” button. Body `#E6E2D6` on `#121416` is 14.3:1; muted `#B8B3A6` is 8.8:1; faint `#9A9588` is 6.2:1; accent `#6EC4D4` is 9.2:1; danger and warning tags also meet 4.5:1. `:focus-visible` is a 3px ice outline with an offset. Status is a word plus a tag, never colour alone. Keyboard: native links, buttons, radios, and hash-routed steps. This prototype was not run through a third-party audit.

## What is still weak

- On a 390-wide install device step, the primary action needs a short scroll. Tightening the match callout would help.
- The rendered flow PNGs are designed pictures of the mermaid in `flow.md` (the mermaid-cli layout truncated labels). A few long return edges on the install picture still travel farther than they should.
- The five-minute tour has to carry UNVERIFIED tags on SMK-37 key-to-drum and mute assignments, because the published tour is FM-1. That is honest, and it is also a bump for a musician who just wants to put a kick on the grid.
- The prototype cannot write a real `.dnpr` or `.x0xslots` file, and says so. A visitor clicking “Simulate a download” is correctly told that nothing was saved — which is truthful and a little disappointing.
- Engine naming (303A vs Chicago) is left visible. Until the playing manual is reconciled, that tension stays.
- This author did not run a screen reader, and did not measure mobile focus order beyond reading the markup.

## Open questions

1. **The iPhone app.** The path is supported, with the tag **UNVERIFIED: route pending Installer QA**. No app is named. The exact steps are unknown. This is unchanged, and it is still the most important unconfirmed install fact.
2. **Preset ownership.** The research document says website implementation is Dot’s area, and that factory-asset provenance for Chicago and Miami is UNRESOLVED (format plan §8). Device wiring of the codec is blocked on flash headroom. This prototype designs the host librarian and the later device path; it does not claim that ownership is settled, and it does not implement a live send.
3. **Default note range.** The official SMK-37 hardware manuals give a three-octave C-to-C keybed. The exact default MIDI numbers at power-on (for example C2–C5) remain **UNVERIFIED**, as in the earlier drawing-fix work.

## Acceptance criteria Installer QA could test

These are criteria the *eventual* presets page and installer could be tested against, once they are real. They are not claims that this prototype already runs them. Each format point cites the research document.

**Installer**

- The default selected package is 0.0.25.1, described as the known-good fallback, with on-device label 0.0.25. A 0.0.26 candidate is not selected, and cannot continue without an acknowledgement of “not hardware-verified.” Withdrawn `600b3b45` is not offered; copy tells a visitor who already has it to restore stock and not reinstall it.
- Choosing a package requires a printed-name choice. A USB name is not treated as confirmation. A simulated (and, in the real installer, an actual) mismatch stops the write.
- Computer Chrome/Edge and iPhone-with-a-compatible-app are both presented as supported. The iPhone path shows **UNVERIFIED: route pending Installer QA** and names no app. No copy says iPhone or Safari “cannot” or is “unsupported.”
- Backup is a required step before a write. Recovery is reachable from the first step.
- Stopped-write and lost-USB states tell the visitor to finish the *same* package, and say the page cannot tell whether the keyboard will start.
- Inert in this prototype: no Web MIDI, no write, no device check actually run.

**Presets (from the DNPR research contract)**

- A download is the raw DNPR v1 record, suggested name `<sanitized-display-name>.<engine>.dnpr` (research R1, §4).
- Upload accepts `.dnpr` and a `.zip` of `.dnpr`; the zip is unpacked on the host; each record is validated *before* any send (research §4, R3, R4).
- Validation covers magic `DNPR`, size at most 548 bytes, CRC as in the plan, format and schema version, and engine. Failures map to `BAD FILE`, `NEWER DATA`, `WRONG ENGINE`. Overflow is `MEMORY FULL`, with no eviction. The user’s file is never deleted on failure (research §4, R2, R3).
- Factory and factory-derived records are not offered for download and not sent while the export switch is OFF (research §4, R2, §5). Factory rows are never a write target (R2).
- An occupied user slot requires a backup of the current slot, then a confirm. One undo is specified after a confirmed overwrite (R2).
- The page does **not** claim a live device send while the codec is unwired and the flash floor does not hold (introduction, R3, §5). When a later path exists: send only when the device is stopped and quiet (`SAVES WHEN STOPPED`, never mid-note); one record at a time; read-back CRC; a failed import leaves the slot clean; 909/808/BREAK sample mismatches show a “sample missing or changed” warning (R2, R3, R5).
- No plugin converter, no “identical to the plugin” language, no Web Serial, no Web MIDI in v1 of this page (R3, §4, §5).
- Elite/Pro interchange is not advertised. The contract does not define a device-model field; that move is UNVERIFIED.

## Document gaps (presets and elsewhere)

- **Elite vs Pro preset move.** The research document is silent. The v1 header it points at (format plan §1) has no model field. UNVERIFIED; not offered as a supported action.
- **CRC algorithm on the page.** Research §4 says “CRC as in the plan.” The plan (format plan §1) specifies zlib CRC-32 of the whole record with the CRC field set to 0. This prototype does not compute it.
- **Device-slot capacity.** A later bank is blocked on a flash-reserve decision (research R4; format plan §6, §13). 64 A/B 909 slots do not fit in the 8 KiB spare. The page does not invent a slot count.
- **SysEx command and manufacturer IDs.** Research R3: “a later product decision; do not invent a final ID here.” Not on this page.
- **Factory-asset rights.** Format plan §8: UNRESOLVED. The page follows the export switch (default OFF) and does not try to settle the rights.
- **Rename, bank scope, imported-section placement.** Deferred in the format plan §9 (working assumptions: automatic `USER 01` names, project-scoped bank). Not designed further here.
- **Playing manual vs current engines.** Nightly `/manual/` says the full playing manual is being reconciled. Chicago, Miami, Juno and Drone are in the DNPR contract, not in the published playing manual. Juno and Drone playing instructions are UNVERIFIED.
- **SMK-37 key-to-drum map** in the five-minute tour: UNVERIFIED (FM-1 manual, not restated in `SMK37.md`).
- **Live `/manual/` vs `/new/` on 0.0.26.** As already recorded in `reading.md`: the nightly manual still has a “0.0.26 is withdrawn” notice, while `/new/` describes a replacement candidate at `94197fc8`. v2 follows `/new/` for the candidate and the withdrawn original, and cites both.

## What this is not

It is not a published site, not a pick among A, B and C, not a live installer, not a hardware test, and not an implementation of the DNPR codec or of a device send. No repo was edited. No firmware-side change was made.
