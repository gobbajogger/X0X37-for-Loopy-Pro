# Release status and admin workflow (design)

This document is a design only. It does not implement the workflow. It does not push code. It does not write a pull request comment.

It does not write to a device. It does not publish a package. It does not change a credential or a permission.

The survey date is 13 November 2026, America/Chicago. The workstream owns this design file. The workstream does not edit the files in the survey list.

## 1. Purpose

The workflow holds one release record for each exact build. The record names the source SHA and the package SHA-256. The record also names the model (Elite or Pro) and the image SHA-256 for that model.

Install reports stay separate from owner hardware approval. A successful web install is not proof of boot, restart, audio or hardware qualification. A promotion occurs only when the required evidence is present. An inferred field does not satisfy `install-verified` or `owner-hardware-approval`.

## 2. Existing assets (survey)

This section lists what the survey found. The survey did not edit these files.

### 2.1 Nightly catalog and artifacts

The nightly site repo is `gobbajogger/x0x37-for-Loopy-Pro_nightly`. The Pages branch is `gh-pages`. The live Pages URL is `https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/`.

The survey found these catalog files on nightly `gh-pages`:

| File | Role |
|---|---|
| `versions.json` (89,223 B) | Live catalog. Schema field `schema`. Latest id `nightly-94197fc8-dev`. 37 version rows. |
| `package-version-assignments.json` (35,952 B) | Numeric version map. Row for `0.0.26` maps to `nightly-94197fc8-dev`. |
| `build-status.json` (275,938 B) | Build ledger. No `94197fc8` or `0.0.26` hit in the live copy. |
| `qualified-nightly-candidate.md` | Candidate rules. Uses `dev-checked` and `hardware_tested: false`. |
| `nightly-candidate-continuity.md` | Continuity rules. |
| `firmware-nightly-migration-policy.md` | Retired proposal. Historical only. |
| `preview-catalog.json` | Small preview file (127 B on `gh-pages`). |

The survey did not find a file named `releases.json` or `manifest.json` on either `gh-pages` branch.

Immutable packages live under `artifacts/nightly-dev/<sha256>/`. The 0.0.26 Elite and Pro packages use that path. Section 9 lists the exact hashes.

The live `versions.json` row for `nightly-94197fc8-dev` has `status: dev-checked` and `hardware_tested: false`. The label is `HARDWARE TEST — BOOT/RESTART NOT YET VERIFIED`. That catalog text is older than the relayed owner report in section 9. This workflow records the report in its own record. It does not edit the nightly catalog.

The same row cites an emu360 result on unchanged `d408` inputs: Elite 11,406 µs and Pro 11,410 µs, 0 late. The text says that result is not a measurement of the new image hashes. The exact-image gate in section 9 uses different numbers. Do not mix the two results.

### 2.2 Main repo `gh-pages`

The main firmware repo is `gobbajogger/X0X37-for-Loopy-Pro`. Its `gh-pages` branch holds the MAIN installer. Top-level files include `versions.json` (7,235 B), `install/`, `firmware/`, `index.html` and `compact-ui-publication.json`.

The live MAIN `versions.json` latest id is `dev-20261022-1638-e894804`. That catalog is older than PR #880. PR #880 is the current MAIN installer work. See section 2.3.

### 2.3 PR #880 (Installer QA, base `gh-pages`)

PR #880 is in `gobbajogger/X0X37-for-Loopy-Pro`. The state is OPEN and draft. The base is `gh-pages`. The head is `fix/main-installer-0251-20261113`. The author is `gobbajogger`. The URL is `https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/880`.

The PR files are `install/index.html`, `versions.json`, and three tests under `tests/main-251-*.test.*`. Installer QA owns this PR. This workstream does not edit those files.

The PR admits source `79280e763341d03ba43266d3e25dad66f12de860` as 0.0.25.1. The PR body says `0.0.26 stays withdrawn` on the MAIN installer. The PR #880 `versions.json` latest id is `nightly-79280e76-dev`. The description says `0.0.26 is withdrawn`. That statement is for the MAIN installer. It is not a change to the nightly 94197fc8 row.

### 2.4 Nightly PRs #126 and #127 (Dot)

PRs #126 and #127 that Dot works on are in `gobbajogger/x0x37-for-Loopy-Pro_nightly`. They are not the same as PRs #126 and #127 in the firmware repo.

Firmware repo PR #126 has state MERGED. It is a licensing docs change from 24 October 2026. Firmware repo PR #127 has state MERGED. It is a project hash-collision fix from 24 October 2026. This workstream does not treat those old PRs as Dot's current work.

Nightly PR #126 has state OPEN and is a draft. The base is `main`. The head is `fix/026-numbered-candidate-label`. The title is `fix: bind DnBOX 0.0.26 naming and prevent unnumbered candidate publication`.

The URL is `https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/126`. The files include `docs/install/index.html`, `docs/qualified-nightly-candidate.md`, `package-version-assignments.json`, tests and `web/x0x_installer.html`. This workstream does not edit those files.

Nightly PR #127 has state MERGED. The head was `fix/dnbox-sample-copy-20261113`. The merge commit is `535565cfee28dcdee0f4f997d749ab48c76200f9`. The files are `tests/sample-branding.test.mjs` and `web/x0x_breaks.html`. The body says Pages publication is still pending. This workstream does not edit those files.

### 2.5 `/workspace/x0x37-results/index.md`

This file is the old gate result index. The last rows are from 22 October 2026, commit `f752007`. The file has no row for `94197fc8` or 0.0.26. The exact-image emu360 gate for 0.0.26 lives in `/workspace/kept-026-94197fc8/GATE-RESULT.txt`. The new record points at that evidence. This workstream does not edit the index.

`INSTALLED.tsv` lists Mac installs through `c6bf44a` on 17 October 2026. `QUEUE.md` is the old install queue. Merge Integrator owns that queue. This workstream does not edit those files.

### 2.6 `/workspace/release/TEMPLATE/`

This is the box-local package template from 30 October 2026. It holds `README.md` (eight sections), `make_package.sh`, nine hardware checklists in `hw-checklists/`, and two source snapshots. The script fills CRC32 and SHA-256. It does not overwrite an existing package. It does not install. This workstream does not edit the template.

### 2.7 Docs Keeper tree `/workspace/x0x37-docs-keeper`

This tree holds Docs Keeper drafts and rule copies. Paths include `rules-v3/rules/` (`install.md`, `docs.md`, `core.md` and more), `ste/` (including an older `ROADMAP.md`), `checklists/`, `pointers/` and `pr-docs-onebuild/`. Docs Keeper owns the live docs in the firmware repo. This workstream does not edit this tree.

Rule 12 in `docs.md` says Docs Keeper owns docs and logs. It also says a device PASS is only for the items and the build that the owner names.

### 2.8 `ROADMAP.md`

The live firmware `ROADMAP.md` is on default branch `main` (33,299 B, 208 lines at survey time). It has a 0.0.25 continuation section and a 0.0.26 section. The 0.0.26 section still says "No 0.0.26 code is written yet" with a 12 November 2026 status. That text is stale against the 94197fc8 packages. Docs Keeper owns this file. This workstream does not edit it.

The box copy at `/workspace/X0X37-for-Loopy-Pro/docs/ROADMAP.md` is on branch `docs/roadmap-tracker` and is older. The Docs Keeper copy at `/workspace/x0x37-docs-keeper/ste/ROADMAP.md` is older still (16 October 2026). Use the GitHub `main` file as the live reference. Do not edit any copy.

### 2.9 Kept 0.0.26 evidence (box-local, not published)

`/workspace/kept-026-94197fc8/` holds packages, build logs, gate output, and ELF files. The ELF files have no public publication. Do not copy the ELF files. Do not publish the ELF files. Section 8 uses hashes only.

## 3. Release record schema

Each record is one JSON file. The validator checks the file against `release-record.schema.json`.

### 3.1 Identity fields

The record must hold:

| Field | Rule |
|---|---|
| `version` | Display version string. Example: `0.0.26`. |
| `catalog_id` | Catalog id. Example: `nightly-94197fc8-dev`. |
| `source_sha` | Full source commit. Exactly 40 lowercase hex characters. |
| `source_tree` | Git tree SHA, 40 hex characters, when known. |
| `models.elite` and `models.pro` | One object per model. A model can be absent only when that model has no image. |
| `models.<m>.image_sha256` | SHA-256 of the model `.fwsc` image. 64 lowercase hex characters. |
| `models.<m>.image_size` | Size in bytes. |
| `models.<m>.package_sha256` | SHA-256 of the installable package for that model. |
| `models.<m>.controller_sha256` | SHA-256 of the Bluetooth controller bundle. |
| `models.<m>.controller_size` | Controller size in bytes. |
| `models.<m>.controller_strategy` | Must be `bundled` for a new record. |
| `models.<m>.controller_check` | Result of `package_nightly --check`. Must be `PASS` or `FAIL` or `NOT_RUN`. |
| `combined_package_sha256` | Optional. Use only when one archive holds both models. Use `null` when no such archive exists. |
| `elf_provenance` | Optional object. See section 8. Hashes only. No binary. |
| `gate_evidence` | List of gate refs. See section 3.2. |
| `states` | Object of state flags. See section 4. |
| `identity_confidence` | Confidence for model and build fields. See section 5. |

For current 0.0.26 packages, `package_sha256` equals `image_sha256`. Each model has its own `.fwsc` file. The survey found no combined archive SHA for 0.0.26. The seed record sets `combined_package_sha256` to `null`.

### 3.2 Gate evidence refs

Each `gate_evidence` item must hold:

| Field | Rule |
|---|---|
| `kind` | For example `emu360-strict`. |
| `result` | `PASS` or `FAIL` or `NOT_RUN`. |
| `elite_worst_us` | Integer microseconds, or `null`. |
| `pro_worst_us` | Integer microseconds, or `null`. |
| `elite_late` | Integer. Must be 0 for a strict PASS. |
| `pro_late` | Integer. Must be 0 for a strict PASS. |
| `threshold_us` | For emu360 strict, 11609. |
| `ref` | Path or URL to the result file. |
| `applies_to_image` | `exact-image` or `other-inputs`. The validator requires `exact-image` for `gate-passed`. |

A Bluetooth controller bundle check is a separate evidence item. The kind is `bt-controller-bundle`. A PASS requires a bundled controller, a matching SHA-256, and `package_nightly --check` PASS for each model in the record.

## 4. Status states

States are flags on one record. They are not a single exclusive label. The validator can set a flag only when the evidence for that flag exists. `withdrawn` can occur with any other flag. A withdrawn record cannot gain a new promotion flag.

The flags are:

| Flag | Meaning | Required evidence |
|---|---|---|
| `built` | Images exist for the named source SHA. | Per-model `image_sha256`, size, and a build or package receipt. |
| `gate-passed` | The strict emu360 gate passed on the exact images. | `emu360-strict` item with `result=PASS`, 0 late, worst time under the threshold, and `applies_to_image=exact-image`. |
| `published-nightly` | The nightly catalog published this exact package. | Catalog id, Pages URL, and the published SHA-256. |
| `installation-reported` | The owner or a user reports an install. The model and the build can lack a verified identity. | A report object with source, time, and text. Confidence can be `reported`. |
| `install-verified` | Hardware or backup fields name the model and the exact build. | See section 5. Inferred fields fail this gate. A USB name is not enough. |
| `boot-restart-reported` | A report says boot or restart occurred. | A report object. If the record does not verify identity, the flag stays `reported` and lacks verification. |
| `owner-hardware-approval` | The owner approves the hardware result in the owner's own words. | Quoted words, timestamp, and a direct source link. See section 5. |
| `withdrawn` | Do not offer this exact build for install. | Withdrawal ref and reason. |

A successful web install is not proof of boot, restart, audio or hardware qualification. The validator must reject a change that treats web install success as `boot-restart-reported` proof, or as `install-verified`, or as `owner-hardware-approval`.

`installation-reported` is not `install-verified`. `boot-restart-reported` is not `owner-hardware-approval`. `gate-passed` is an emulator result. It is not a hardware result.

## 5. Inferred fields and confidence

Every identity field that a gate uses must carry a confidence value. The values are `verified`, `inferred`, `reported`, and `absent`.

The validator uses these rules:

1. A field with confidence `inferred` does not satisfy `install-verified`.
2. A field with confidence `reported` does not satisfy `install-verified`.
3. A field with confidence `absent` does not satisfy `install-verified`.
4. `install-verified` requires `model` confidence `verified` and exact-build confidence `verified`.
5. Exact-build proof must be the installed image SHA-256, or a firmware GIT string readback, or a backup field that names the image hash. A USB port name is not proof. A "current nightly" guess is not proof.
6. `owner-hardware-approval` requires the owner's own words, quoted, with a timestamp and a direct source link. A relay with no quotation does not satisfy this gate.
7. A relayed report can set `installation-reported` and `boot-restart-reported`. The record must mark those reports with confidence `reported` when identity has confidence `inferred`.

The 0.0.26 seed uses these rules. Section 9 shows the result. The seed does not claim `install-verified`. The seed does not claim `owner-hardware-approval`.

The seed is not a full functional suite result. The seed is not hardware qualification on both models. The seed is not a promotion.

## 6. Authenticated admin role and audit trail

An admin change is a state change or a record edit. The workflow records every admin change in an append-only audit file. The file is JSONL. One line is one event. A later line can retract an earlier line. No line is rewritten.

### 6.1 Options that need no new credential

Option A uses the GitHub identity `gobbajogger`. Each change is a signed or verified GitHub commit on the lane branch. A second factor is a GitHub pull request approval by that same identity. The audit identity is the commit author and the approval actor. This option needs no new credential.

Option B uses the append-only JSONL file in the repo. Each event holds `time`, `actor`, `action`, `record_id`, `from_states`, `to_states`, `evidence_refs`, and `commit_sha`. The Git commit binds the line to the GitHub identity. This option needs no new credential.

Option C combines A and B. This design recommends option C. The owner still chooses.

### 6.2 Options that need new access (open questions)

These options need new access or a permission change. They are questions for the owner. This workstream does not enable them in the design phase.

- A GitHub App or a new bot token.
- A new private repository for records.
- A change to branch protection or required reviewers.
- A new `CODEOWNERS` rule that grants or blocks write access.
- A new GPG key if the owner does not already sign commits.
- A web admin page with a new login.

## 7. Promotion rules and validator

The validator is a script. The planned path is `release-status/tools/validate_release_status.py`. The script reads the schema, the records, and the audit file. The script exits 0 only when every record and every proposed state change is valid.

### 7.1 Promotion rules

The script applies these rules:

1. Add a state flag only when the evidence for that flag exists.
2. Refuse `gate-passed` when the emu360 result is not for the exact image hashes. Refuse it also when late is not 0, or when the worst time is not under the threshold.
3. Refuse `install-verified` when model or build confidence is not `verified`.
4. Refuse `install-verified` when the only model clue is a USB name.
5. Refuse `owner-hardware-approval` when the words are not a quotation of the owner, or the source is only a relay, or `install-verified` is false.
6. Refuse any promotion of a `withdrawn` record, except a documented correction that keeps `withdrawn`.
7. Refuse a claim of full suite PASS when a fail, a skip or a not-run item remains. The owner can exclude such an item only in a quoted order.
8. Refuse to treat a web install success as boot, audio or hardware proof.
9. Require the Bluetooth controller bundle check for `built` when the package schema uses `bundled`.
10. Append one audit line for every accepted state change. Refuse a rewrite of an old audit line.

### 7.2 Validator script spec

The command is:

```
python3 release-status/tools/validate_release_status.py --records release-status/records --schema release-status/schema/release-record.schema.json --audit release-status/audit/audit.jsonl
```

An optional `--propose CHANGE.json` mode checks one state change and does not write the record. The exit codes are:

| Code | Meaning |
|---|---|
| 0 | All records and the proposal are valid. |
| 1 | A schema or evidence error. The script prints the rule and the record id. |
| 2 | A usage or file error. |

The script does not push. The script does not publish. The script does not write to a device.

## 8. ELF provenance (hashes only)

The box path `/workspace/kept-026-94197fc8/named-elf/` holds named ELF copies and `PROVENANCE.md`. The ELF files have no public publication. This design does not copy them. This design does not publish them. Public publication of ELF bytes needs an owner decision. The ELF holds symbols and links the JieLi SDK.

A release record can attach an `elf_provenance` object. The object holds hashes and build facts. It does not hold and does not embed a binary.

### 8.1 ELF provenance fields

| Field | Rule |
|---|---|
| `published` | Must be `false` until the owner allows publication of the bytes. |
| `source_sha` | The build source SHA. Must equal the record `source_sha`. |
| `sdk` | SDK name and version. For 0.0.26 this is `V1.2.14`. |
| `padcal` | Integer. For 0.0.26 this is `0`. |
| `toolchain` | Compiler identity when known. Use `null` when the kept logs do not name it. |
| `input_zip_sha256` | SHA-256 of the owner-approved input zip, when one exists. |
| `models.<m>.elf_sha256` | SHA-256 of the ELF. 64 hex characters. |
| `models.<m>.elf_size` | Size in bytes. |
| `models.<m>.image_sha256` | Must equal the record image SHA-256 for that model. |
| `models.<m>.mapping` | How the ELF maps to the image. See below. |
| `models.<m>.mapping_result` | `MATCH` or `FAIL` or `NOT_RUN`. |

The mapping statement for 0.0.26 is: every flash `PT_LOAD` segment equals the decrypted package app area. `VERIFY.txt` records `MATCH` for both models.

### 8.2 Seed ELF hashes (reference only)

These hashes come from `named-elf/PROVENANCE.md` and `inputs-provenance.json`. They are references. They are not a publication of the files.

```
source_sha        94197fc839b00aff47b98ca02f3c6396e4a6eabf
sdk               V1.2.14
padcal            0
toolchain         null (kept build logs do not name the compiler)
input_zip_sha256  f344c213a51497a71381c24364d4db17d892b48ca2528b116e549f133b220018
elite elf_sha256  720f851c83a62fbef71fe52ad84e669409a69fde1d1fb313427d7942ed3ca1ea
elite elf_size    628872
elite image       95b3195f4767ef5df33b365197b01203873e5eec3d7fa497c73e5e509484a4a6
pro elf_sha256    7e67c3b33fe2e690e5f913b5d048727a715c70daca06ddd326a10e37eacb9535
pro elf_size      628864
pro image         ce67d560eebd03cb8b9087f7723b1e0b71fb7cca256184cdfdd55a8c27c708b8
published         false
```

The implementation must not add ELF binaries to git. The implementation must not copy ELF bytes into a Pages branch.

## 9. Seed records

### 9.1 0.0.26 (`nightly-94197fc8-dev`)

The survey found the full hashes in the nightly catalog, in the assignment file, and in the kept provenance files. The hashes are complete. They are not partial.

```
version              0.0.26
catalog_id           nightly-94197fc8-dev
source_sha           94197fc839b00aff47b98ca02f3c6396e4a6eabf
source_tree          72d28fbc741244d18fa3d18495da16ab0fcb1d24
source_branch        lane/026-combined-20261112
source_pr            https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/869

elite image_sha256   95b3195f4767ef5df33b365197b01203873e5eec3d7fa497c73e5e509484a4a6
elite image_size     651732
elite package_sha256 (same as image)
elite crc32          a5953c62
elite product        SMK-37 Elite_900
elite pkg URL        artifacts/nightly-dev/<elite image_sha256>/DnBOX-0.0.26-SMK37-Elite-94197fc8-hwtest.fwsc

pro image_sha256     ce67d560eebd03cb8b9087f7723b1e0b71fb7cca256184cdfdd55a8c27c708b8
pro image_size       651730
pro package_sha256   (same as image)
pro crc32            d8957148
pro product          SMK-37 Pro_900
pro pkg URL          artifacts/nightly-dev/<pro image_sha256>/DnBOX-0.0.26-SMK37-Pro-94197fc8-hwtest.fwsc

controller_sha256    a334208b51f62bc7231ebb76a6de0dfa260033d7f7316e5b92053c540bc1d984
controller_size      61552
controller_crc32     564a8f55
controller_strategy  bundled
controller_check     PASS for both models (package_nightly --check)
combined_package     null
```

Gate evidence (exact image, `gate-20261113-202119`):

```
kind              emu360-strict
board             smk37
mhz               360
FM1_STRICT        1
result            PASS both models
elite_worst_us    11561
pro_worst_us      11525
elite_late        0
pro_late          0
threshold_us      11609
ref               /workspace/kept-026-94197fc8/GATE-RESULT.txt
applies_to_image  exact-image
note              Elite is 13 us slower than 0.0.25.1 (11548). Pro is 39 us faster than 0.0.25.1 (11564). Both have 0 late.
```

State flags for this seed:

| Flag | Value | Reason |
|---|---|---|
| `built` | true | Packages and package checks exist. |
| `gate-passed` | true | Exact-image emu360 strict PASS, 0 late. This is an emulator result, not a hardware result. |
| `published-nightly` | true | Live nightly catalog latest id is `nightly-94197fc8-dev`. |
| `installation-reported` | true | Owner-reported install on an SMK. See the report below. |
| `install-verified` | false | Model confidence: `inferred`. Build confidence: `inferred`. Installed hash: `absent`. |
| `boot-restart-reported` | true | Owner-reported automatic restart and boot at normal speed. Identity lacks verification. |
| `owner-hardware-approval` | false | The source does not quote the owner. The source is a relay. This is not a full suite. This is not qualification on both models. This is not a promotion. |
| `withdrawn` | false | This SHA replaces withdrawn 600b. The MAIN installer still keeps 0.0.26 off MAIN (PR #880). That MAIN fact is not this nightly record. |

Owner report (does not satisfy `install-verified` or `owner-hardware-approval`):

```
source             relayed by Dot via Orchestrator, owner words not quoted
events             install, automatic restart, boot at normal speed
device             owner's SMK
model              Elite, confidence=inferred, not re-checked
build              94197fc8, confidence=inferred from current nightly
installed_hash     absent (NOT verified)
scope              NOT full functional suite
                   NOT hardware qualification on both models
                   NOT promotion
```

### 9.2 Withdrawn 600b (`nightly-600b3b45-dev`)

`600b` is the short source SHA `600b3b45`. The full source SHA is `600b3b45f8d59c3432d958238b05753eb6f2fcb5`. The source tree is `bcf12a1e20b6f74104f31241a609462926f7c970`. The catalog id was `nightly-600b3b45-dev`. The display version was also `0.0.26`.

PR #116 in the nightly repo published this build to `gh-pages`. The merge time is 13 November 2026, 01:34 UTC (12 November 2026, 19:34 America/Chicago). The publish branch is `publish/nightly-600b3b45-0.0.26`.

PR #120 withdrew the installer selection. The merge time is 13 November 2026, 05:44 UTC (12 November 2026, 23:44 America/Chicago). The title is `Withdraw 0.0.26 installer selection pending boot fix`.

The body says the owner requested temporary removal after boot and restart failures. The PR removed the `nightly-600b3b45-dev` catalog row. It restored 0.0.25.1 as latest. It kept the immutable package files.

The 94197fc8 description says it replaces the withdrawn 600b build and includes the retained-RAM boot-guard correction.

600b hashes from the publish-branch catalog:

```
elite image_sha256  25b78701e4184f78e3d8b0a02bf25d53a95482803a0239c065284446a40ac68a
elite image_size    651721
pro image_sha256    528dac38557312bd001529c95383be0232ebc1e751df3e1eb1a8eeafda5f5655
pro image_size      651721
controller_sha256   a334208b51f62bc7231ebb76a6de0dfa260033d7f7316e5b92053c540bc1d984
status              withdrawn
withdraw_ref        https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/120
withdraw_reason     boot and restart failures, pending boot fix
replaced_by         nightly-94197fc8-dev
```

The 600b seed record sets `withdrawn` to true. The validator refuses a promotion of this record.

## 10. File ownership (no edit conflict)

One writer owns each scope. This workstream does not edit another writer's files.

| Scope | Writer | Paths | This workstream |
|---|---|---|---|
| This design | Release status workstream | `/workspace/release/design/release-status-workflow.md` | Writes this file only in the design phase. |
| New records and tools | Release status workstream | `release-status/` on branch `lane/rp-release-status-workflow-20261113` | Sole writer after the owner approves implementation. |
| Firmware docs | Docs Keeper | `docs/ROADMAP.md`, `BEHAVIOR.md`, `USER_PREFERENCES.md`, `docs/rules/`, agent rules, checklists | Do not edit. Ask Docs Keeper for a pointer later. |
| MAIN installer | Installer QA | PR #880: `install/index.html`, `versions.json`, `tests/main-251-*`, base `gh-pages` | Do not edit. |
| Nightly naming and installer pages | Dot | Nightly PRs #126 and #127, and those file paths | Do not edit. |
| Nightly `gh-pages` publish | Designated nightly publisher | nightly `gh-pages`, `versions.json`, artifacts | Do not publish. Do not edit the live catalog. |
| Gate index | Gate script / Merge Integrator | `/workspace/x0x37-results/index.md`, `QUEUE.md` | Do not edit. Reference only. |
| Package template | Release Packager (this role, box-local) | `/workspace/release/TEMPLATE/` | Do not edit during this design. |
| ELF bytes | Owner decision | `/workspace/kept-026-94197fc8/named-elf/` | Read hashes only. Do not copy. Do not publish. |

## 11. Implementation plan

This section is a plan. It is not an implementation. No push occurs in the design phase.

### 11.1 Branch

The branch name is `lane/rp-release-status-workflow-20261113`. The branch is in `gobbajogger/X0X37-for-Loopy-Pro`.

Do not base this branch on `main`. Do not base it on `nightly`. Do not base it on `wip/one-build`. Use an orphan branch that holds only `release-status/`. An orphan branch avoids a merge conflict with those live refs and with PR #880. If the owner names a different live parent, use that parent and still add only `release-status/`.

### 11.2 Exact files to add

Add only these paths, all under `release-status/`:

```
release-status/README.md
release-status/schema/release-record.schema.json
release-status/schema/audit-event.schema.json
release-status/records/0.0.26-94197fc8.json
release-status/records/0.0.26-600b3b45.json
release-status/audit/audit.jsonl
release-status/tools/validate_release_status.py
release-status/tests/test_schema.py
release-status/tests/test_validator.py
release-status/tests/test_promotion_gates.py
```

Do not add ELF binaries. Do not add `.fwsc` bytes. Do not change `docs/`, `install/`, `.github/`, `versions.json`, or any nightly file.

### 11.3 Tests

`test_schema.py` checks the JSON schema. A valid seed record must pass. A missing required field must fail. A source SHA that is not 40 hex characters must fail. A SHA-256 that is not 64 hex characters must fail.

`test_validator.py` checks the script against the seed records. The 94197fc8 seed must pass as written. The 600b seed must pass as withdrawn. A rewrite of an audit line must fail.

`test_promotion_gates.py` holds negative tests. These cases must fail:

- Inferred model must not pass `install-verified`.
- Inferred build `94197fc8` from "current nightly" must not pass `install-verified`.
- Absent installed hash must not pass `install-verified`.
- USB name as the only model proof must not pass `install-verified`.
- Web install success alone must not pass `boot-restart-reported` as verified proof, and must not pass `owner-hardware-approval`.
- Relayed owner words with no quotation must not pass `owner-hardware-approval`.
- An emu360 result on other inputs (`d408`) must not pass `gate-passed` for the 94197fc8 images.
- A withdrawn 600b record must not gain a promotion flag.
- A controller check `FAIL` or `NOT_RUN` must not pass `built` for a bundled package.

A positive test can set `installation-reported` and `boot-restart-reported` from the relayed report, with inferred flags kept, and with `install-verified` and `owner-hardware-approval` still false.

### 11.4 Rollout steps

1. Keep this design on the box. Wait for the owner and Orchestrator to review it.
2. After approval to implement, write the files on the box under a work tree for the lane branch. Do not push yet.
3. Run the unit tests and the validator on the seed records. All tests must pass. All negative tests must fail the illegal change.
4. Ask the owner before the first push. Push only the lane branch. Open a draft PR. Do not target `main`, `nightly`, or `wip/one-build` as a merge that writes those branches.
5. Do not merge without owner approval. Do not publish. Do not write to a device. Do not change credentials.
6. Ask Docs Keeper to add a pointer in `ROADMAP.md` if the owner wants that pointer. Docs Keeper writes that pointer.
7. Do not wire the records into the nightly catalog or the MAIN installer unless the owner orders that step. Installer QA and Dot keep their files.

### 11.5 One writer per scope

The release status workstream is the only writer of `release-status/` and of this design file. Docs Keeper is the only writer of firmware docs. Installer QA is the only writer of PR #880 files. Dot is the only writer of nightly PRs #126 and #127 files. The designated publisher is the only writer of live nightly `gh-pages`. The gate script is the only writer of the PASS rows in the results index.

If two scopes need a shared sentence, the owners agree first. Then only the owner of that file writes the change.

### 11.6 Milestones

| Id | Work | Output | Push? |
|---|---|---|---|
| M0 | Design (this document) | `/workspace/release/design/release-status-workflow.md` | No |
| M1 | Schema, validator, and tests on the box | Files in section 11.2, tests pass | No, until the owner approves |
| M2 | Seed records and first audit lines | 94197fc8 and 600b records validate | Lane branch only, after approval |
| M3 | Owner chooses admin option A, B or C | Audit identity stays as chosen | No new credential in A, B or C |
| M4 | Docs Keeper pointer (optional, Docs Keeper writes) | One link in docs | Docs Keeper's process |
| M5 | Catalog wire-up (only if the owner orders it) | A later design | Not in this plan by default |

## 12. Open questions for the owner

1. Does the owner accept option C (GitHub identity plus append-only JSONL) for admin authentication?
2. Does the owner want a new GitHub App, a private repo, branch protection, or `CODEOWNERS`? Those need new access.
3. Is an orphan lane branch acceptable, or does the owner want a named live parent that is not `main`, `nightly`, or `wip/one-build`?
4. The kept ELF files have no public publication. Does the owner want the hash record only, or a later published ELF? Publication needs a clear owner order.
5. The kept build logs do not name the compiler. Does the owner have a toolchain string to store in `elf_provenance.toolchain`?
6. The MAIN installer in PR #880 still treats 0.0.26 as withdrawn. The nightly catalog offers 94197fc8 as an owner-test candidate. Does the owner want those two channels to stay different?
7. For `owner-hardware-approval` on one model, does a quoted Elite-only approval apply to Elite only, and never to Pro?
8. The relayed report is not a quotation. Does the owner want to add a quoted statement with a timestamp and a link, for a later approval review? That statement still does not by itself verify the installed hash.
9. Who is the standing admin actor besides `gobbajogger`? This design names no other actor.
10. Does the owner allow this workstream to start M1 on the box only, with no push, after the owner accepts this design?

