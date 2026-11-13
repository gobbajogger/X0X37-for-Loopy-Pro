# Owner request status (2026-11-13, reconciled against audit items 1–6 only)

Short list of what is done, what is in work, what is waiting on the owner, and what has no owner yet. Matches `docs/ROADMAP.md` on `docs/roadmap-owner-sync-20261113` (PR #887). This is not a claim that every request is captured. Times below that carry a UTC offset are converted in prose; the box is America/Chicago (UTC-6).

## DONE

- Project export and import (#50): shipped in `94197fc8` via the [projects page](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/projects/). On-device Save/Load are #47 and #48. Sample backup/restore and the third loop slot are #51. Do not add a second project-export row.
- 0.0.25.1 product features: PANIC, part reset, drum fill and shared drum FX, Chicago accent at +6 dB and global Tune, per-pattern presets (#7), note lock (#8), part on/off, PLAY-answers-question, SMC peek, QUEUE MUTES, ECHO page, BREAK 1/8 and 1/4 rates, pattern chain = longest playing part, autosave toggle, 0.0.25 pushed to main with `DN Box 0.0.N` names, 360 MHz emu gate (26.6). Nightly: [DN Box 0.0.25.1](https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/).
- #1–#122 items marked SHIPPED or DONE in ROADMAP (Chicago/Miami engines, factory preset recall on the device, BREAK tools, mixer basics, and the rest of the 2026-11-01 list that was already in 0.0.24 and carried forward).
- 26.7 extra AMB-1 / other engines: declined ("Nah let's stick to the previous plan than").
- Box entrance **asset** (U2): host render `21261737` is ASSET ONLY; last frame matches the current home screen. Firmware startup integration is U3, PENDING (Main Builder; budget being measured). The asset is not delivered startup.
- Owner-reported Elite use of 0.0.25.1 and 0.0.26 is recorded (scoped, hash not confirmed, Pro untested).
- RN1: v0.0.25.1 release-note body is live as v3, sha `32e7ef26`. Elite hardware wording is already corrected; do not redo it.
- I1 / #880: MERGED `cd345c94`. Installer QA live check PASSED: live `install/index.html` matches `7beaf50f`; main installer offers DnBOX 0.0.25.1 (Elite `5e24e0d4`, Pro `6f135a3d`, BT `a334208b`); package SHAs match. DONE (live), not hardware-verified. No rerun of unchanged tests.
- 94197fc8 flash: exact #869 receipts Elite 8,440 B free, Pro 8,448 B free. The 6,848 B Elite XIP overflow was the *initial* combined checkpoint (`docs/engineering-delivery/026-combined/CHECKPOINT.md`: lanes #865/#866/#867/#868 on `78f81bad`, Elite SDK V1.2.14 / PADCAL=0), resolved by lossless font packing before 94197fc8. Do not attribute that overflow to 94197fc8.

## IN PROGRESS

- 26.1–26.5 Drone, held-key chords, Juno and chorus: in source on [`94197fc8`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/94197fc83), draft [#869](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/869). Exact receipts 8,440 / 8,448 B free. **Optimizer** is measuring further savings for later additions (device presets, entrance, chord names), not to clear a 94197fc8 overflow. Not a qualified package.
- 26.9 Juno/Drone **gain staging**: owner report 2026-11-13 ("new engines sound not loud enough"). **Main Builder** owns the diagnosis. IN PROGRESS. Diagnosis only; no unmeasured gain change.
- REC1: REC records only FM-1-layout keys; full SMK-37 37-key keybed must record. Full 37-key REC applies to all keybed REC paths, including drum/BREAK, keeping the actual drum mapping. Owner via authenticated ChatGPT (Dot), 2026-11-13 07:55 CST. Owner: FMB (sole ui.c/record-path writer). IN PROGRESS: reproduce/trace first; FM-1 range is the symptom, not the cause.
- DROP1: Audio dropout on real hardware: Chicago arp + Juno triad + BREAK + 909 BD four-on-the-floor, no 808 (tempo, patches, FX and installed SHA unknown). Owner via Dot, 2026-11-13 08:10 CST. FMB diagnosis/fix (sole writer); Runner one emulator repro + audit of why the earlier emulator missed it; scene added to pipeline regression. IN PROGRESS.
- LAG1: Chicago params page lags on entry and in use; audio keeps playing. Owner via Dot. FMB trace first (Optimizer read-only review). IN PROGRESS.
- MW1: Show the mod wheel on screen like knob feedback, with the real target parameter and value. Owner via Dot. FMB (trace now), Firmware UX design; after chord and REC ui.c work. IN PROGRESS.
- I4: Bring the live iOS in-page install confirmation and SysEx continuity into nightly source before any regeneration/publication (integrated head 85e56bdf). Merge Integrator; Installer QA. IN PROGRESS.
- 26.8a Device preset save/load/import/export transport, storage and UI (F1): **Main Builder**. BLOCKED on Optimizer flash savings.
- 26.8b READY now, independent of flash: host librarian and DNPR codec tests (**Main Builder**, acceptance A1–A8 at `lane/fmb-presets-format` [`e0c42135`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/e0c42135)); web file transfer (W3, **Website UX**, OPEN until a draft PR; **Installer QA** holds A1–A8 now and accepts later); Chicago/Miami user-preset plugin compatibility (F2, **Main Builder**; gap doc `docs/plans/2026-11-13-chicago-miami-plugin-compat-gaps.md`; waits on owner gap-doc inputs U1–U3: AU capability log, user-saved `.aupreset` files, plugin version). Factory KORG redistribution stays off and does not block this. Extension, same item: **AudioKit Juno** preset compatibility with DnBOX Juno (owner request via Dot, 2026-11-13 07:44 CST); owner FMB, Dot's evidence feeds in; acceptance is real plugin identity, version and schema, round-trip and mapping tests, explicit limits, no full-fidelity claim. No Dot implementation dependency. Juno engine J6 fidelity: owner (authenticated, via Dot) 2026-11-13 07:54 CST: adapt the DnBOX Juno engine to faithfully reproduce AudioKit J6 sounds; FMB, same lane, sequenced with 26.9; IN PROGRESS (architecture comparison + bounded experiments); no fidelity claim without evidence.
- Box-entrance firmware startup integration (U3): **Main Builder**. PENDING. Budget being measured. Asset `21261737` only.
- Chord names (U4 / U1): exact-match compact forms (Cmin7, inversions C/E when justified); otherwise the actual notes, always visible; no interval forms. UX withdrew 1,023-shape and 97% figures (theoretical only). Real data: 0 recorded chords besides FMB's 5 host frames. A real rate needs a held-chord capture (FMB host sim or an owner project). **Main Builder + Firmware UX**.
- Firmware UX overhaul (U1): **Firmware UX**. Next: user-sentiment research (causal why, negative and positive, SMK firmware first), then 2–3 complete 240×240 directions. Glyphs from real source, plus new Juno and Drone glyphs. #881 is left untouched (no adoption request).
- Task-first manual (D1): **Docs Keeper**. Draft [#884](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/884) head `9516a218`. Nothing merged.
- Product READMEs (D2, R10): **Docs Keeper**. Main draft [#885](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/885). Nightly draft [nightly #129](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/129) head `403bf9f2` (correct repo, default README, Docs Keeper sole writer). Wrong-repo [#886](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/886) is SUPERSEDED and is not reused. Nothing merged.
- Website craft (W1) and accurate keyboard (W2): **Website UX**. Also [#874](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/874). Final design publication still needs an approved mockup. Website UX may prepare a branch/draft.
- Web preset file transfer (W3): **OPEN**, routed to **Website UX** (web) + **Installer QA** (acceptance). No draft PR yet. No Dot dependency. FMB DNPR A1–A8 are at `e0c42135`.
- Nightly exact-source integration (I2): **Merge Integrator** is the writer. **Installer QA** is read-only QA of the exact integrated head only. Nightly-repo drafts [#119](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/119), [#121](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/121), [#126](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/126). Coordinate with Dot before taking files; no duplicate PRs; do not rerun unchanged checks.
- Nightly manual notice (I3): **Release Packager**, draft [nightly #128](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/128). Under the owner-authored website publish rule; waits on the owner's own yes; does not merge without one; do not invent a new owner question; honor an actual tool gate if present. No firmware release promotion.
- DnBOX naming and valid install links (N1): **Release Packager**. I1 is live. `lane/fmb-naming-026` at `f3bfee74` is proposal-only. Keep old links; do not redo RN1 Elite wording.
- Admin/public install verification, catalog and promotion (RP2 / #883): **Release Packager**. IN PROGRESS (catalog generator, static status page, admin audit CLI dry-run, promotion evaluator report-only).
- Combined 0.0.26 integration: **Merge Integrator** / #869, remaining gates (not a 94197fc8 flash overflow). **Runner** is on emulator Step 6. **BugBot** reviews remaining diffs as they land.
- #73 m-bootloader investigation: **BugBot** read-only assessment exists (`docs/mbootloader-usrtrim-install-plan.md`). Not unowned. Firmware still missing. No device writes.
- This ROADMAP sync (P3): this branch, [#887](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/887). Reconciled against audit items 1–6 only.
- Automatic idle-browser cleanup (R8): agreed and being set to run on its own.
- Workflow Auditor (WA1): OPEN. Workflow Auditor supplies the text; **Docs Keeper** is the writer (`lane/dk-rules-handoff-fields` @ `49281685`, base nightly `66073c68`, file `skills/x0x37-workflow/references/handoff.md`). The draft PR waits on the owner's own yes. Not yet amended.

## WAITING ON OWNER

Only real open decisions. Do not add new questions for things already answered. RN1 is DONE; do not re-ask.

- F2 / 26.8b Chicago/Miami plugin work waits on owner gap-doc inputs U1–U3: an AU capability log from his Mac, his own saved `.aupreset` files, and the plugin version.
- I3: [nightly #128](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/128) waits on the owner's own yes. **Release Packager** does not merge without one.
- WA1: the handoff guide draft PR waits on the owner's own yes. **Docs Keeper** is the writer; Workflow Auditor supplies the text. Not yet amended.
- Website UX will bring him the corrected keyboard / direction boards when they are ready; there is no question to answer until then. Final design publication needs an approved mockup.
- Three old open choices, only if work is blocked: spare flash vs loop slots; Bluetooth pairing; installer `ctl.img` with or without `--ctl`. Do not re-ask them now.

## NOT STARTED / OPEN

These stay on ROADMAP so they are not forgotten. They are not a second tracker.

- EM1: a real try-before-install emulator page (**Runner**; heavy audio not ready).
- J1: richer curated Juno-60 and new-engine presets (**Main Builder + Firmware UX**). Do not stop this for factory-rights or plugin-evidence waits.
- ELF1: renamed ELF provenance and publication decision. Existing owner/context was not found in current ROADMAP or the 2026-11-13 steers (source unverified). Do not publish or rename live artifacts blindly.
- WA1: Docs Keeper writes the `handoff.md` amendment; Workflow Auditor supplies the text. Draft PR waits on the owner's own yes. Not yet amended (see above and WAITING ON OWNER).
- #15 Cutoff-lock persistence while navigating (still missing at the last proof).
- #29 Three owner-chosen stock breaks (needs lawful samples).
- #52 Atomic operations across several clients.
- #55 Restore all historical former samples (needs authorised source; not the same as #51).
- #88 Several visual site variants and a quality 3D accent (Website UX is on W1/W2 first).
- Off-device editor gaps: #97 iPhone interface, #98 group operations, #101 scale/snap/ghost/touch keyboard, #104 two-way editor protocol, #105 usable TouchOSC.
- #117 Shorten the path from a request to a usable feature (process; this ROADMAP sync is a start).
- #119 Community song library (deferred on purpose).
- #120 Automatic suggestion inbox and the owner's vote.
- #122 Cursor/review contribution audit (no current owner).
- Factory Chicago/Miami sounds as shareable files: redistribution stays off until KORG provenance is known (not 26.8). That hold does not block F2, AudioKit Juno, or J1 original sound design.
- #73 firmware implementation is still missing; the investigation is not unowned (see IN PROGRESS).
