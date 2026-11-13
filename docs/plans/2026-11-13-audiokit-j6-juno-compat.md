# AudioKit Synth One J6 ↔ DnBOX Juno compatibility (2026-11-13)

This document extends the existing ROADMAP **26.8** plugin-compat scope on `lane/fmb-presets-format`. It is a sibling of `docs/plans/2026-11-13-chicago-miami-plugin-compat-gaps.md`. Native DNPR Juno presets stay **independent**: a user Juno `.dnpr` does not require AudioKit, and this converter is not a firmware change.

**26.8b (engine fidelity).** A sibling engineering goal — that the DnBOX Juno **engine** should reproduce J6 sounds faithfully, without claiming identity — is in `docs/plans/2026-11-13-j6-juno-engine-fidelity.md`. That work is host-only prototypes in `/workspace/j6-fidelity`. It does not change this converter contract, and it does not make the J6 schema verified. CPU-adding engine items are **NOT ALLOWED as-is** until an offsetting saving exists. Engine edits wait until after 26.8b and the 26.9 loudness study.

**Cite, do not redo.** Website UX finished the AudioKit product-identity and format research (relayed to this lane on 2026-11-13, America/Chicago). This document **does not redo** that format research. Their findings, used as given:

- The product is **most likely AudioKit Synth One J6**, **not** the older open-source AudioKit Synth One.
- The old Synth One **JSON is not evidence of the J6 format**. Do **not** use it as the J6 schema.
- The J6 preset **schema and version are still UNVERIFIED**.
- **Known mismatch:** J6 has **separate amp and filter ADSRs** and **three routable LFOs**. DnBOX Juno has **one shared envelope** and a **PWM-only LFO**.
- Never equate the two engines, and never promise every parameter or full sound.

**Merge hook.** At write time, a published path and SHA for that Website UX research were not present under `/workspace/website-ux`. When Website UX files it (path and SHA), cite that file here and merge any extra format facts; do not start a second format investigation. Website UX’s site work already consumes the DNPR host contract (`/workspace/website-ux/v2/ia.md`, `v2/flow.md`) and leaves the v1 page **without** a plugin converter. This J6 host/web converter is a separate tool that can sit beside that page once the schema is verified. Website UX and Installer QA use the acceptance contract in §5 once it is verified.

This step is docs and research only. No firmware, no JieLi build, no implementation of the converter.

## 1. Product identity (from Website UX research, plus public registry facts)

| Field | Value | Status | Source |
| --- | --- | --- | --- |
| Product name | AudioKit Synth One J6 + AUv3 | Likely target (Website UX). Not the replaced “AudioKit Synth One”. | Website UX research; https://audiokitpro.com/synthonej6/ ; the older app’s page says J6 **replaces** it: https://audiokitpro.com/synth/ |
| Seller | AudioKit Pro, LLC | Verified (App Store lookup) | iTunes lookup `id=6741535277`, 2026-11-13 |
| App Store | https://apps.apple.com/us/app/audiokit-synth-one-j6-auv3/id6741535277 | Verified | Same |
| App bundle id | `com.audiokitpro.superj6` | Verified as the **app** bundle | iTunes lookup, field `bundleId` |
| AUv3 component / Audio Unit four-char id | — | **UNVERIFIED** (may differ from the app bundle) | Needs the owner’s install or a capability log |
| Current store version | **0.999.8**, current-version date 2026-11-06 12:27 America/Chicago (from `2026-11-06T18:27:42Z`) | Store current as of this write. The owner’s installed version may differ and **must** be recorded with any export. | iTunes lookup `version`, `currentVersionReleaseDate` |
| First store release | 2025-06-17 | Verified | iTunes `releaseDate`; announcement https://audiokitpro.com/releasedsynthonej6/ |
| Preset schema, keys, ranges, units, file extension, version field | — | **UNVERIFIED** | No real J6 export was found (see §2). Public pages describe iCloud Preset Sync, not a file schema. |
| Source code location | **No public repository found.** A GitHub search of `org:AudioKit` and of `synthonej6` / `superj6` / “Synth One J6” returned **no** J6 repo (2026-11-13). The MIT repo https://github.com/AudioKit/AudioKitSynthOne is the **older** Synth One. | J6 source **UNVERIFIED** | GitHub API org listing and search |
| Code license | **UNVERIFIED** for J6. The older Synth One is MIT (https://github.com/AudioKit/AudioKitSynthOne/blob/master/LICENSE). The J6 marketing line “AudioKit is an open-source project” refers to the AudioKit project at large, not a cited J6 LICENSE. | Do not treat MIT as the J6 app license. | Vendor legal notice on https://audiokitpro.com/synthonej6/ ; MIT file on the old repo |
| Factory preset content rights | **UNRESOLVED.** The app ships “400+ hand crafted presets.” Redistribution stays **OFF** unless a clear permission is cited. | §4(c) | https://audiokitpro.com/synthonej6/ |

**Candidates considered, and what picks J6.** (1) **AudioKit Synth One J6** — current AudioKit “Juno-inspired” AUv3 (“Inspired by Juno-6 & Jupiter series”; App Store legal notice). Website UX picks this. (2) **AudioKit Synth One** (2018, MIT, GitHub) — replaced by J6; different engine (hybrid analog/FM, two LFOs); its JSON is **not** the J6 schema. This document does not map that JSON. If the owner later says they meant the old Synth One, that is a different workstream.

The engines are not the same instrument. J6 uses machine-learning oscillators trained on recorded Juno-106 and Jupiter-8 tones (vendor page). DnBOX Juno is a compact 6-voice subtractive engine with original equations informed by the Plaits analog oscillator/filter **design** (no Plaits source copied; `firmware/src/dsp/juno6.c`). Never claim bit-identical or “full fidelity” sound.

## 2. Search for a real J6 export

Authorized sources checked on 2026-11-13 (America/Chicago). **No real J6 preset or bank export was found.** Schema and version therefore stay **UNVERIFIED**. No sha256 is recorded.

| Source | Result |
| --- | --- |
| `/workspace` and `/home/box` (`find` / `rg` for `.aupreset`, `.nksf`, `fullState`, `*.j6`, `*superj6*`, `*synthonej6*`, `*audiokit*`, `*synth-one-j6*`) | No J6 export. No AudioKit preset file. |
| Lane repo history and docs | No J6 fixture. |
| Owner attachments at `/cursor/stores/store-6e77e905-88f8-4f31-921a-cef7003f3e5e/grok-bot/agents/8e2a9333-8400-4d21-ac99-423d3cd1e065/attachments/` | Path **does not exist**. The agent folder has only `assets/` with one unrelated PNG. |
| Transcripts / memory | No owner-supplied J6 file. |
| Vendor public docs and sample files (https://audiokitpro.com/synthonej6/ , App Store, AudioKit GitHub org) | Feature lists and iCloud Preset Sync only. **No** downloadable J6 preset/bank sample and **no** schema document. |

**Chicago / Miami U1–U3 recheck (same pass):** still **MISSING**. No `.aupreset`, `.nksf`, `.component`, `Factory2.prog`, `fullState` blob, or capability log on the box, in those attachments, or in `/workspace/presets-research`. `CHICAGO-PARAMETER-TRACE.json` (names and native IDs only) is unchanged. There is still no Miami TRACE. U1–U3 in the sibling gaps doc are not filled by this search.

## 3. DnBOX Juno engine (verified at this lane)

Read from `firmware/src/dsp/juno6.c`, `juno6.h`, `firmware/src/app/original_sound.c` / `.h`, `original_runtime.c`, `ui_original_controls.h`, and `preset_record.h`, on this lane (engine also at source base `94197fc8`). Engine id **3**. Schema v1: **18** params, **0** samples, wire length **118** B. The last two macros are **performance-only** (not patch edits). Display bytes are 0–127 (`q/127`, except pitch bend is centred: 64 = 0.5). `juno6_set` admits only `id < 18` and `x` in `[0, 1]`.

Six voices. One shared 16-bit chorus delay (512 B, 256 samples, depth clamped 1.7–5.4 ms). Two-pole low-pass (not four-pole). High-pass. Saw/pulse blend plus a square sub at half frequency. One ADSR, used for **both** amplitude and (via EnvAmt) filter cutoff. One triangle LFO used **only** for pulse-width modulation. No second envelope, no general-purpose LFO routing, no arp, no sequencer, no delay, no reverb, no noise osc, no second DCO, no microtonality inside the engine.

| ID | Name | `juno6_set` / render (x in 0..1) | Notes |
| ---: | --- | --- | --- |
| 0 | Wave | `waveform = (uint8_t)(x * 2.99)` (three bins, **unused in render**). Render: `saw*(1-x)+pulse*x`. | Continuous saw–pulse blend. |
| 1 | Sub | `sub_level = x * 0.5` | Square, −1 octave. |
| 2 | Cutoff | `cutoff = x`. Render base Hz: `35 + cutoff² * 12000`. | Quadratic. |
| 3 | Res | `resonance = x`, applied as `resonance * 0.8`. | |
| 4 | EnvAmt | Render only: `7000 * macro[4]` Hz added from the **shared** envelope. | No `juno6_set` side effect. |
| 5 | Attack | `0.002 + x² * 3` seconds | Shared envelope. |
| 6 | Decay | `0.01 + x² * 3` seconds | Shared. |
| 7 | Sustain | `sustain = x` | Shared. |
| 8 | Release | `0.01 + x² * 4` seconds | Shared. |
| 9 | HPF | `hpf = x`. Pole: `15 + 2000 * hpf²` Hz. | |
| 10 | LFORate | LFO freq = `0.1 + 8 * x²` Hz (triangle). | PWM only. |
| 11 | LFODep | Pulse width += `LFO * x * 0.2`. | PWM only. |
| 12 | Width | `pulse_width = 0.1 + x * 0.8`. | |
| 13 | ChoRate | Runtime: chorus rate = `0.1 + 4.9 * x` Hz (0.1–5). | Chorus LFO, not the PWM LFO. Depth is **fixed at 5.4 ms** in `original_runtime_param`. |
| 14 | Chorus | Runtime: mix = `0.5 * x` (0–0.5). | |
| 15 | Level | Render: `mix *= x / 6`. | Per-voice scale. |
| 16 | PBend | `bend = (x - 0.5) * 4` semitones. | **Performance only.** |
| 17 | Damper | On when `x >= 0.5`. | **Performance only.** |

Factory table (four DnBOX names: JUNO POLY, JUNO PAD, JUNO PLUCK, JUNO BRASS) stores **16** patch bytes, not AudioKit content. Those four are subject to the global factory-export switch. They are not J6 fixtures.

## 4. Three sections: map, unsupported, feasibility and licensing

Because the J6 **schema is UNVERIFIED**, no row is **EXACT**. No numeric formula that names a J6 key, unit, or range is claimed. The tables are a **skeleton**: DnBOX side verified; J6 side is a feature counterpart from the vendor page (amp and filter ADSRs; three LFOs; chorus; HPF; sub; waveforms) plus the Website UX mismatch. Formulas become writable only after a real user-saved J6 export and a versioned schema (missing items in §6). Honesty classes may move after measurement. **Never** claim bit-identical or full-fidelity sound. **Counts today:** **0 EXACT**, **16 APPROXIMATE-pending** (the patch macros, each with a stated policy), **2 performance-only** (out of the patch map), plus the UNSUPPORTED J6 features in (b).

### (a) Parameters that can map (skeleton: APPROXIMATE-pending)

| DnBOX (id, name, range) | Intended J6 counterpart (feature only) | Formula | Class | Policy |
| --- | --- | --- | --- | --- |
| 0 Wave, 0..1 (7-bit) | A primary oscillator shape toward saw vs pulse | **TBD** after schema | APPROXIMATE-pending | One-oscillator blend only. A second DCO, detune, super-saw, sine/triangle-only shapes: (b). |
| 1 Sub, 0..1 → 0..0.5 level | Sub-oscillator level, if the schema has one | **TBD** | APPROXIMATE-pending | Extra sub octave/shape bits: (b). |
| 2 Cutoff, 0..1 → 35..12035 Hz quadratic | Filter cutoff | **TBD** (range and curve unknown; 2-pole vs J6 filter is already a sonic mismatch) | APPROXIMATE-pending | |
| 3 Res, 0..1 | Filter resonance | **TBD** | APPROXIMATE-pending | |
| 4 EnvAmt, 0..1 → 0..7000 Hz | Filter-envelope **amount**, if present | **TBD** | APPROXIMATE-pending | This is amount, not the Filter ADSR times. |
| 5–8 Attack, Decay, Sustain, Release | **Amp** ADSR only. See fold policy below. | **TBD** (DnBOX times are quadratic in x; J6 units unknown) | APPROXIMATE-pending | Do **not** average Amp and Filter ADSRs. |
| 9 HPF, 0..1 | High-pass amount, if present | **TBD** | APPROXIMATE-pending | |
| 10 LFORate, 11 LFODep | At most **one** J6 LFO, and only if it is routed to pulse width / PWM. See LFO policy. | **TBD** (DnBOX: 0.1–8.1 Hz triangle, depth `* 0.2`) | APPROXIMATE-pending | Otherwise both stay at DnBOX default / zero depth. |
| 12 Width, 0..1 → 0.1..0.9 | Pulse width, if present | **TBD** | APPROXIMATE-pending | |
| 13 ChoRate, 0..1 → 0.1–5 Hz | Chorus rate, if J6 chorus exposes one | **TBD** | APPROXIMATE-pending | DnBOX chorus **depth is fixed** at 5.4 ms. A second J6 chorus: (b). |
| 14 Chorus, 0..1 → mix 0..0.5 | Chorus mix | **TBD** | APPROXIMATE-pending | |
| 15 Level, 0..1 | Output / master | **TBD** | APPROXIMATE-pending | DnBOX also divides by 6 voices. |

**Amp / Filter ADSR fold (required policy).** J6: two envelopes. DnBOX: one shared envelope for amp and (via EnvAmt) filter.

- **J6 → DnBOX:** map **Amp** ADSR A/D/S/R onto DnBOX Attack/Decay/Sustain/Release. Class **APPROXIMATE** (the shared envelope cannot also equal a different Filter ADSR). Filter ADSR times are **UNSUPPORTED**: keep them in the sidecar; **do not** fold them into the shared times; **do not** average the two envelopes. Filter-envelope **amount**, if a key exists, maps to EnvAmt (APPROXIMATE-pending).
- **DnBOX → J6, sidecar present:** restore Amp and Filter ADSRs from the sidecar (file-level restore of unmapped fields). If the user **edited** the DnBOX envelope after import (DNPR bytes differ from the values the import wrote), write the new shared ADSR to J6 **Amp** ADSR only; leave Filter ADSR from the sidecar; report APPROXIMATE and that the filter envelope was not taken from the edit.
- **DnBOX → J6, no sidecar:** write the shared ADSR to J6 Amp ADSR only. Leave Filter ADSR at the J6 default (not a copy of Amp, unless a later measured test shows that is safer — today the default is “Amp only, Filter default”). Report that Filter ADSR was **not** restored.

**Three-LFO policy (required).** J6: three routable LFOs, 60+ destinations, LFO-to-LFO (vendor page). DnBOX: one PWM LFO, plus a separate chorus rate.

- **J6 → DnBOX:** if one or more LFOs route to pulse width / PWM, map the **lowest-index** such LFO’s rate and depth to LFORate and LFODep (APPROXIMATE-pending). Every other LFO, and every non-PWM routing (filter, pitch, amp, LFO-to-LFO, sequencer, tempo-sync), is **UNSUPPORTED** and stays in the sidecar. If several LFOs route to PWM, record the conflict in the mapping report. If none route to PWM, do not invent a mapping; LFODep stays empty.
- **DnBOX → J6, sidecar present:** restore all three LFOs from the sidecar. If the user edited LFORate/LFODep, apply that edit only to the LFO that was mapped (the PWM one).
- **DnBOX → J6, no sidecar:** write one LFO (PWM destination) from LFORate/LFODep; other LFOs default/off; report UNSUPPORTED for the rest.

Pitch bend and damper stay performance-only on DnBOX. If they appear in a J6 preset, they go to the sidecar, not to the 16 patch bytes.

### (b) Unsupported features, and exactly what is lost

These J6 features (vendor page: https://audiokitpro.com/synthonej6/ and the App Store listing) have **no** DnBOX Juno counterpart. On import they are dropped from the sounding DNPR patch and, when a sidecar exists, kept only for later J6 restore. On a DnBOX render they are **lost**.

| J6 feature | What is lost on DnBOX Juno |
| --- | --- |
| Filter ADSR (separate A/D/S/R) | Independent filter shape. Only Amp times + EnvAmt remain. |
| LFO 2, LFO 3, and all non-PWM routings; LFO-to-LFO; tempo-sync LFO | All of that motion. |
| Second DCO, detune, super-saw, noise oscillator, FM (if present) | Those sources. DnBOX is one saw–pulse blend plus sub. |
| Second (stacked) chorus; J6 chorus depth (DnBOX depth is fixed) | The second unit and any depth control. |
| Reverb, tape delay, crush, stereo widen, auto-pan, dedicated VCA LFO (tremolo) | All of those effects. Project FX is a different path and is off for A/B tests. |
| Dual arpeggiators, step sequencer, dual touch pads | All performance sequences and pads. |
| Microtonality / world scales / Scala | Tuning other than 12-TET at A=440 in this engine. |
| Play modes: mono, legato, pitch-glide | DnBOX Juno is 6-voice poly with the allocator in `juno6_note_on`; no glide param. |
| ML oscillator “analog” drift and J6 filter family (vendor: Juno-106 / Jupiter-trained osc; third-party write-ups mention a J106 filter) | The character itself. DnBOX is a 2-pole low-pass with a BLEP osc. This alone forbids a full-sound claim. |
| Factory 400+ bank | Not imported, not shipped. See (c). |

DnBOX HPF, chorus, and PWM LFO **exist**, but only as the APPROXIMATE-pending rows in (a). They are not “full” J6 equivalents.

### (c) Import/export feasibility and licensing

**Feasibility (host/web converter, not firmware).** A host or Website UX tool can, **after** a verified schema:

1. Identify the file as Synth One J6 (bundle / AU id / a documented magic or version field). Refuse anything else, including **old Synth One JSON**, with **no write**.
2. Map the (a) rows into a DNPR v1 Juno record (engine id 3, 18 param bytes, 118 B). Fill a **mapping report** (EXACT / APPROXIMATE / UNSUPPORTED counts and the LFO/ADSR decisions).
3. Keep **source metadata and every untouched J6 field** in a **sidecar / opaque block**: a byte-for-byte copy of the user’s J6 export, plus the mapping report and hashes. Do **not** invent a J6 JSON schema for this block. The sidecar lives on the **host** (a zip of `.dnpr` + opaque source + report). It is not stuffed into the 118 B DNPR record (and must not depend on the 548 B max for a whole J6 bank). The device does not need the sidecar to play the mapped 16 patch params. A DnBOX → J6 round-trip that **restores** unmapped fields **requires** the sidecar. Without it, export can only write mapped fields and documented defaults, and must say so.
4. Run the A/B method in §5. No identical-sound language.

Until a real export exists, the converter must not ship a parser that guesses the format.

**Code license vs content rights.**

- **J6 app/source license:** UNVERIFIED. No public J6 repo or LICENSE was found. The MIT license of the **old** Synth One does **not** apply to J6 by implication. A user-preset **converter** that runs on the host and talks to the installed AUv3 (or reads a user-saved export) does not itself require redistributing J6 source. Redistributing J6 source or DSP would need a cited license.
- **Factory J6 preset content:** UNRESOLVED. The 400+ included bank stays **OFF** for redistribution (plugin format or DNPR). User-owned J6 programs are in scope, the same split as Chicago/Miami. Permission to ship factory J6 content must be **cited**; none is cited here.
- **DnBOX Juno factory** (four names above) follows `DNBOX_FACTORY_PRESET_EXPORT` (default 0). That is a separate switch.
- Native DNPR Juno user presets stay independent of this path.

## 5. Host/web converter design and acceptance

**Safe rejection and version handling** (no slot write, no partial file):

| Case | Word | Mutation |
| --- | --- | --- |
| Wrong product (old Synth One, other AU, wrong bundle) | `WRONG PLUGIN` (or `BAD FILE` if the file is unreadable as J6). Do **not** parse old Synth One JSON as J6. | None |
| Corrupt, truncated, or unreadable | `BAD FILE` | None |
| Schema or format version **newer** than the pinned J6 version (once known) | `NEWER DATA` | None; no partial map |
| Older **unknown** version | `BAD FILE` | None |
| Engine target is not Juno | `WRONG ENGINE` | None |
| Factory J6 bank, or a factory-derived program, offered for redistribution | Refuse; factory redistribution OFF | No file written for distribution |

Until the schema is pinned, any file whose identity cannot be verified as J6 is refused.

**A/B audio method** (user programs only; same spirit as UX-spec X14):

- Lossless WAV. Mid-register, mid velocity. 4 beats held at 120 BPM and one eighth-note stab. Pitch C2 or C3, recorded in the package. Project FX off. Unity level. No motion.
- (A) J6 plugin render of the **user** preset, named version on the report.
- (B) DnBOX Juno render of the imported DNPR.
- (C) J6 render of the re-exported preset **with** sidecar restore, and a second C' **without** sidecar, so restore is measurable.
- A comparison sheet (peak, RMS, audible and spectral drift). **No** identical-sound language. Parameter survival is not acoustic equivalence.

**Parameter-mapping tests:** dump J6 fields (once schema exists), the 18 DNPR bytes, and the re-export. Grade each row EXACT / APPROXIMATE / UNSUPPORTED against published tolerances (Main Builder publishes; none exist yet). LFO pick and ADSR fold must appear in the report.

**Acceptance criteria** — Website UX and Installer QA use this **same** contract once it is verified:

1. Source plugin identity, version, and schema are documented (identity: J6, store 0.999.8 as of this write; **schema still UNVERIFIED** — this criterion is not PASS today).
2. Real sample round-trip tests (J6 → DNPR → J6, with and without sidecar) and parameter-mapping tests, on **user** programs, have been run on a named SHA.
3. Compatibility limits in §4(a)–(b) are published with the report (counts, fold policy, LFO policy).
4. Safe rejection and version handling as in the table; Installer QA A-tests show **no write** on refuse.
5. Native Juno `.dnpr` files still load without this converter.
6. No factory J6 redistribution; no full-fidelity sound claim.

## 6. Still missing (only these), and who supplies them

After the search in §2, only the following are still required. Do not ask the owner for items already on the box — none of these are.

| # | What | Who |
| --- | --- | --- |
| J1 | **One user-saved** (non-factory) Synth One J6 preset **export**, plus the **exact app / AU version** from the same install (bundle string, version, and if possible the AUv3 component id). Hashes may enter the repo; factory bank values must not. | **Owner** |
| J2 | A verified J6 **schema and version** (file shape, keys, ranges, units, version field, destination enum for the three LFOs) derived from J1 and/or a capability log. | FMB and Website UX, after J1. Website UX’s format research is merged here, not redone. |
| J3 | Owner confirmation that the installed product is J6 (not the old Synth One), if that is not already obvious from J1’s version string. | **Owner** (only if J1 does not already name the product) |
| J4 | Round-trip and A/B packages on user programs (§5), and published X13-style tolerances. | Installer QA and FMB, after a converter exists |
| J5 | A cited J6 **code license** or a cited **permission** to redistribute factory J6 banks, **only if** someone proposes to ship J6 source or factory content. Not required to start the **user** converter. Until then, factory redistribution stays OFF. | Vendor / legal, if and when asked. Not an owner blocking item for J1–J4. |

## 7. Sources

- Website UX AudioKit identity research, relayed 2026-11-13 (America/Chicago); merge the filed path when it exists. Website UX DNPR page contract: `/workspace/website-ux/v2/ia.md`, `v2/flow.md`.
- https://audiokitpro.com/synthonej6/ (features, legal notice, 400+ presets, amp & filter ADSRs, 3 LFOs).
- https://audiokitpro.com/synth/ (old Synth One replaced by J6; MIT source link for the **old** app only).
- https://audiokitpro.com/releasedsynthonej6/
- App Store / iTunes lookup `id=6741535277`: name, `com.audiokitpro.superj6`, version 0.999.8.
- https://github.com/AudioKit/AudioKitSynthOne and `LICENSE` (old Synth One, MIT) — **not** the J6 schema.
- DnBOX Juno: `firmware/src/dsp/juno6.{c,h}`, `firmware/src/app/original_sound.{c,h}`, `original_runtime.c`, `preset_record.h`.
- `docs/plans/2026-11-13-preset-format-v1.md`, `docs/plans/2026-11-13-preset-file-io-research.md`, `docs/plans/2026-11-13-chicago-miami-plugin-compat-gaps.md`.
