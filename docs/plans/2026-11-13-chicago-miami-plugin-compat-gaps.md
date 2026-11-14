# Chicago / Miami plugin compatibility gaps (2026-11-13)

This document does **not** redo `/workspace/presets-research/plugin-identity.md` (SHA-256 `71361571107929cdeb69db9bc0da3084448318733ab830f174038b594a4242ab` as cited in the UX spec) or `/workspace/smk-ux/presets-spec.md` §3A (SHA-256 `1aaeea74569dbe1aedb49511db37c7956bb417d87e9db1514ff867b774383584`). It lists the **exact items still MISSING** to build and verify format compatibility with the **real** KORG Gadget Chicago and Miami Audio Units.

**Rights split.** Format compatibility and two-way round-trips of **user-owned** presets with those plugins are **in scope**. They are **not** blocked by the factory-sound rights issue. Only **redistributing factory KORG sounds** stays **OFF**. Spec §3A’s “not justified yet” and “nothing in 3A is to be implemented until the inputs exist” is an **evidence** gap, not a rights block. Gathering the inputs below, and implementing the host AU-parameter path for **user** presets, is allowed. “No two-way claim” applies only to **unproven fidelity claims** (do not say parameters or sound are identical until measured). It does **not** forbid doing the work.

Chicago is the 303 engine. Miami is never 303 or 303B.

**26.8 scope, sibling.** AudioKit Synth One J6 ↔ DnBOX Juno is in the same 26.8 plugin-compat scope: `docs/plans/2026-11-13-audiokit-j6-juno-compat.md`. That work is not a Chicago/Miami rights issue. U1–U3 were rechecked on 2026-11-13 (box, repo, attachments): still MISSING; no `.aupreset`, `.nksf`, capability log, or plugin binary was found.

## 1. User-preset two-way round-trip with the real plugins (IN SCOPE)

**Goal.** A user program saved from the real Chicago or Miami plugin can become a DNPR v1 record (and a DnBOX user slot), and a DNPR / DnBOX user program can be written back in a form the same plugin accepts, with every public macro reported honestly as EXACT, APPROXIMATE, or UNSUPPORTED (spec §3A.2, X13, X15). A/B renders (X14) measure acoustic drift. No identical-sound language until those measurements exist.

**Feasible path** (already researched, not implemented): a host that reads and writes the plugin’s exposed AU `parameterTree` / `.aupreset` / `fullState`, as described in `docs/plans/2026-10-25-native-preset-workflow.md` (`54bb3c496`, “next permitted evidence”) and in plugin-identity.md. The in-tree one-way factory-bank reader (`tools/import_chicago_miami_factory_banks.py`) does **not** fulfil this: it is a reverse-engineered reader of `Factory2.prog` and a heuristic NKSF `PCHK` reader, with **no writer**. There is no official KORG file-write specification in the tree.

**Already known (do not redo):** Chicago = Gadget `Chicago.component` **3.1.6**; Miami = `Miami.component` **3.1.6** (PR #276). Firmware stores 7-bit maps; FxType is reduced to 12 of 30 original IDs; pitch bend and damper are performance-only, not patch data; hidden native IDs above 120 are Chicago CMP4 no-ops. The cell cannot fully reconstruct a plugin program from those facts alone. `experiments/original_synth/CHICAGO-PARAMETER-TRACE.json` has Chicago control names and native IDs only — not ranges, units, or enums. There is **no** Miami TRACE file. PR #276: AU `factoryPresets` was unsupported on the owner’s 3.1.6. Factory `Factory2.prog` / factory `.nksf` packs are **section 2** material; they are not required to start this user path.

### 1.1 Exact items still MISSING

Each item is still missing from the repo and from the research trees on this box (`/workspace/presets-research`, `/workspace/smk-ux`, and the 94197fc8 source tree cited by plugin-identity.md). No `.aupreset`, `.nksf`, `.prog`, or plugin-binary fixtures were found there.

| # | What is MISSING | Why it is needed | Where it comes from |
| ---: | --- | --- | --- |
| U1 | **Capability log** of the exact installed `Chicago.component` and `Miami.component`. Record: component and version strings (3.1.6 was current at 94197fc8; confirm or record a newer build); whether `factoryPresets` is still unsupported; the **full** host-visible AU parameter tree (native IDs, names, ranges, units, enum / discrete values); whether parameter read-back is complete (every set value can be read back). Completeness is recorded locally. Do **not** publish factory program values. | This is the “next permitted evidence” in the 2026-10-25 plan. The host AU-parameter path cannot be designed without the tree. | The owner’s Mac, from the same Gadget 3 / Gadget 3 Plugins install (3.1.6 or a logged newer build). A host or `auval`-class dump of `parameterTree` is enough. |
| U2 | **User-saved (non-factory) plugin preset files** from that same version: at least one user-created or user-edited **Chicago** program and one **Miami** program, as `.aupreset` and/or an AU `fullState` / `fullStateForDocument` blob. If NKS is a target, also one **user-saved** (not factory-pack) Chicago `.nksf` and one Miami `.nksf`. Hashes and structural notes may enter the repo. Raw factory values must not. | No in-tree writer exists, and no user-plugin fixture exists. These files show the write path the plugin itself accepts, and they are the round-trip fixtures for U5–U6. Factory `Factory2.prog` and factory-pack `.nksf` are **not** substitutes. | The owner’s DAW or AU host session, saved from the version in U1. Keep any file that embeds factory material out of public source. User-only programs may be test fixtures. |
| U3 | **Pinned plugin version builds.** A fresh confirmation of the exact AU / bundle version used for U1 and U2. If the owner now has a newer Gadget build than 3.1.6, that version’s parameter tree and a short note of differences versus 3.1.6. | A map against 3.1.6 can be wrong on a later build (NKS arrived in 3.1.0; later changes are unknown). The map and the fixtures must name the same version. | Owner install: AU component string and bundle version, recorded next to U1. |
| U4 | **Complete parameter map**, AU tree ↔ DNPR v1 bytes, for **user** macros: Chicago’s 22 and Miami’s 18 public parameters. For each: AU ID, name, range, unit, enum values (waveforms, arp modes, FX types, Miami wobble shapes), and the conversion to v1 param bytes. Also a written list of: FX type reduction (firmware 12 of 30 original IDs), Chicago velocity-to-cutoff (**always UNSUPPORTED**), pitch bend and damper (performance, not patch data), and hidden native IDs above 120 (no-ops). The existing Chicago TRACE is names and IDs only. A Miami TRACE is missing. | Required for an honest EXACT / APPROXIMATE / UNSUPPORTED report (spec §3A.2) and for X13 drift tolerances. Building this map is in scope once U1 exists. Publishing it is not a fidelity claim. | Built by Main Builder from U1 plus the in-tree `original_chicago_mappings` / `original_miami_mappings` (private headers and `original_sound.h`). The map is a work product of this path, not a pre-existing complete document. |
| U5 | **A writer the real plugin accepts**, for the chosen `PLUGIN_FORMAT`. Options: `.aupreset`, AU `fullState`, or (if NKS is a target and a write path is demonstrated) `.nksf`. Official write specifications for NKSF `PCHK` and `Factory2.prog` are UNKNOWN; do not invent a chunk writer without a plugin-accepted demonstration. | The export leg of two-way cannot exist without this. The current in-tree code is read-only and factory-oriented. | Implemented as a host tool after U1–U4. Proven only when the **same** plugin version in U3 **loads** the written file and the parameter dump matches the map (U6). |
| U6 | **Tested two-way trips on user-created programs**, every public macro, plus discrete enums, FX types, unknown fields, and unmapped native IDs, listed honestly (spec X13, X15): (a) plugin → DNPR → plugin; (b) DNPR → plugin → DNPR. Artifacts: mapping-report snapshots (EXACT / APPROXIMATE / UNSUPPORTED counts), parameter dumps, hashes of input and output files. | Parameter fidelity of the **user** path is unproven. Plugin-identity.md records that no plugin → DnBOX → plugin trip has been run. Running these trips is in scope. Claiming they already pass is not. | Installer QA / FMB test package on the implementation SHA, using the user-saved files from U2 and the writer from U5. User programs only. |
| U7 | **A/B render package** per spec X14, **user** presets, both engines. Lossless WAV: (A) the original DnBOX or plugin user program; (B) after the two-way trip. Protocol: mid-register, mid velocity, 4 beats held at 120 BPM and one eighth-note stab; Chicago and Miami; pitch C2 or C3 recorded in the package; project FX off; part level unity; no motion. A comparison sheet of measured drift. **No identical-sound language.** | Parameter survival is not acoustic equivalence (2026-10-25 plan; spec A-PX7). X14 is explicitly pending. | Main Builder or BugBot on the implementation SHA, from U6 programs. |
| U8 | **Published X13 tolerances and noise floors** (spec: Main Builder publishes; currently INFERRED). | Required to grade U6 dumps as PASS / APPROXIMATE / FAIL. | Main Builder, with U1 and U4 in hand, before U6 is scored. |

U1–U4 are the inputs needed **to build**. U5 is the implementation. U6–U8 are the inputs needed **to verify**. None of U1–U8 is blocked by section 2.

### 1.2 What “no two-way claim” means here

Until U6 and U7 pass and are published against a named plugin version and a named implementation SHA:

- Do **not** claim that a DnBOX user preset is parameter-identical or sound-identical to a Chicago or Miami plugin program.
- Do **not** treat the one-way factory-bank importer, the Chicago TRACE, or the heuristic NKSF reader as a completed two-way path.

Those limits are about **honesty of claims**. They are not a stop on gathering U1–U4, writing U5, or running U6–U8 on user-owned programs.

## 2. Factory content redistribution (OFF)

This section is **independent** of section 1. Rights for factory KORG sounds are **UNRESOLVED**. The 30 October header-and-log-push approval does **not** clear factory export. A factory strip was never done at `94197fc8`.

**Stays OFF:**

- Export or publication of factory Chicago or factory Miami programs as downloadable presets (plugin format or DNPR).
- `DNBOX_FACTORY_PRESET_EXPORT` remains **0**. Factory-origin records, and records with a nonzero factory index (factory-derived captures and re-imports), are refused on export.
- Shipping or publishing factory values from `Factory2.prog`, factory NKS packs, factory JSON, or private factory program headers.
- Any plugin-format export of a factory or factory-derived program (spec X16, §1.2, A-PX6). User plugin export (section 1) works with this switch **OFF**.

**Not required to start section 1:** local-only copies of `Factory2.prog` or factory `.nksf` packs (plugin-identity.md “inputs still needed,” item 2) may still help historical analysis of the one-way reader. Hashes and structural notes only in the public repo. They do not authorise redistribution, and they are not fixtures for U2 or U6.

Do not wait on a rights clearance to gather U1–U4 or to implement the user path in section 1.

## 3. Sources (existing; not redone)

- `/workspace/presets-research/plugin-identity.md` and the source tree it cites (`/workspace/dnbox-94197-rebuild/source/gobbajogger-X0X37-for-Loopy-Pro-94197fc/`).
- `/workspace/smk-ux/presets-spec.md` §3A, and §3.5–§3.6, §4, §6, §9A as they bound file I/O.
- `docs/plans/2026-10-25-native-preset-workflow.md` (also blob `54bb3c496`), lines on the feasible host AU-parameter workflow and “next permitted evidence.”
- `docs/proposals/bass-real-presets-20261026/README.md` (mixed Factory2 / heuristic; no sound-equivalence claim).
- `experiments/original_synth/CHICAGO-PARAMETER-TRACE.json` (Chicago names and native IDs only).
- `tools/import_chicago_miami_factory_banks.py` (one-way factory reader; no writer).
- `docs/plans/2026-11-13-preset-format-v1.md` (DNPR v1; factory-export switch).
- `docs/plans/2026-11-13-preset-file-io-research.md` (host file I/O; no plugin converter on the v1 page).
- `docs/plans/2026-11-13-audiokit-j6-juno-compat.md` (sibling: AudioKit J6 ↔ DnBOX Juno).
