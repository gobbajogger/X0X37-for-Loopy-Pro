# Release status workflow

This directory holds one release record for each exact firmware build, a JSON schema, an append-only audit log, a validator, a catalog generator, an admin action tool, a promotion evaluator, and tests.

It does not install firmware. It does not write to a device. It does not publish a package. It does not edit the nightly catalog, the MAIN installer, `docs/ROADMAP.md`, or `.github/workflows`.

Admin authentication is option C.

Option C has one admin: the GitHub login `gobbajogger`. There is no other principal.

The JSONL `actor` field is a label only. It is not authentication.

The validator checks the GitHub login on each git commit that added lines to `audit.jsonl`. It uses the GitHub API, or local git to list commits and then the GitHub API to read the login. If that history or the GitHub login is unavailable, the validator reports `actor not verified` and does not pass.

A local copy or an off-git audit file is not tamper-proof. Do not treat the hash chain or the JSONL actor field as proof of the admin.

No new credential, GitHub App, or permission is required.

Each accepted state change must add a new audit line.

A later line may retract an earlier line.

No line is rewritten.

Seed lines have `commit_sha` set to `null` so those lines are not rewritten. The containing Git commit is what the actor check uses.

Each record's `states` must equal the latest audit `to_states` for that `record_id`.

## Layout

```
release-status/README.md
release-status/schema/release-record.schema.json
release-status/schema/audit-event.schema.json
release-status/records/0.0.26-94197fc8.json
release-status/records/0.0.26-600b3b45.json
release-status/records/0.0.25.1-79280e76.json
release-status/audit/audit.jsonl
release-status/tools/validate_release_status.py
release-status/tools/generate_catalog.py
release-status/tools/admin_action.py
release-status/tools/evaluate_promotion.py
release-status/catalog/catalog.json
release-status/catalog/index.html
release-status/tests/test_schema.py
release-status/tests/test_validator.py
release-status/tests/test_promotion_gates.py
release-status/tests/test_owner_reported_elite.py
release-status/tests/test_generate_catalog.py
release-status/tests/test_admin_action.py
release-status/tests/test_evaluate_promotion.py
```

This tree lives on the orphan branch `lane/rp-release-status-workflow-20261113` in `gobbajogger/X0X37-for-Loopy-Pro`. The branch holds only `release-status/`. It is not based on `main`, `nightly`, `gh-pages`, or `wip/one-build`.

## How to run

From this directory, or from the branch root:

```
python3 release-status/tools/validate_release_status.py \
  --records release-status/records \
  --schema release-status/schema/release-record.schema.json \
  --audit release-status/audit/audit.jsonl
```

Exit `0` means every record and the audit file are valid, and the GitHub login on each audit-adding commit is `gobbajogger`. Exit `1` is a schema, evidence, or actor-verification error; the script prints the rule and the record id. Exit `2` is a usage or file error. On a bare directory with no git history, the exit is `1` and the message starts with `actor not verified`.

`--propose CHANGE.json` checks one state change in memory and does not write the record. The change file must include a new `audit_event` whose `actor` is `gobbajogger` and whose `actor_identity` is `github`.

`--audit-previous FILE` refuses any rewrite of an existing audit line. When that flag is omitted and the audit file is inside a git checkout, the validator uses the previous commit's `audit.jsonl` by default (HEAD if the working tree is dirty, otherwise HEAD^).

A bare run that is not in a git checkout, and that does not pass `--audit-previous`, cannot detect a rehashed rewrite of the audit file. The validator then prints a warning. The hash chain still checks internal consistency, but a rewrite that also repairs the chain will pass that check. That is one reason a local or off-git log is not tamper-proof. The same bare run also reports `actor not verified`.

```
python3 -m unittest discover -s release-status/tests -v
```

The tools have no third-party dependencies.

### Catalog generator

This command builds a read-only catalog and a static HTML status page from the records:

```
python3 release-status/tools/generate_catalog.py   --records release-status/records   --out-json release-status/catalog/catalog.json   --out-html release-status/catalog/index.html
```

The page shows states, exact source and artifact hashes, model identity with confidence, and owner-reported events with their sources. It shows withdrawn builds. It states that an Elite report never implies Pro. The generator does not publish the page. The generator does not promote a build. Emulation and an upload are not hardware qualification.

`--dry-run` prints the catalog JSON and does not write files.

### Admin action tool

This command prepares one authenticated audit event. Dry-run is the default. Dry-run writes nothing.

```
python3 release-status/tools/admin_action.py   --records release-status/records   --audit release-status/audit/audit.jsonl   --record-id nightly-79280e76-dev   --action record-update   --note "example"   --evidence-ref "https://example.invalid/ref"
```

The actor is the GitHub login verified through the GitHub API. For a proposed commit, the tool reads the authenticated user. For an existing commit, the tool reads the commit author. If that login is unavailable, or is not `gobbajogger`, the result is `actor not verified` and the tool refuses `--commit`. The JSONL `actor` field is not authentication.

`--commit` produces a GitHub commit on an allowed feature branch only, through the GitHub tree and blob API. It does a fast-forward update. It refuses `main`, `nightly`, `gh-pages`, and `wip/one-build`. It writes only paths under `release-status/`. The validator still refuses an illegal promotion. The tool never promotes a build automatically.

### Promotion evaluator

This command reports, for each record and its channel, which owner-required gates are pass, fail, or missing:

```
python3 release-status/tools/evaluate_promotion.py   --records release-status/records
```

`--json` prints JSON. The report is output only. The evaluator never promotes. It never infers hardware qualification from an upload or from emulation. An Elite result never implies Pro. Channels stay distinct. A withdrawn record cannot be promoted. If every owner-required gate is pass, the owner still decides.

## Owner browser admin, model-scoped outcomes, and promotion dry-run (PR #883 continuation)

The new actual local HTTP admin is under `release-status/admin/` and
`release-status/tools/admin_http.py`. It is **not a GitHub Pages-only mockup**:
the browser makes real JSON requests to a loopback service that validates
record/history/audit proposals, can write a consented anonymous report to a
private local JSONL file, and optionally prepares an authenticated branch
commit through the existing GitHub API writer. Nothing runs on a device.

Start the default, non-publishing, non-writing owner console:

```sh
python3 release-status/tools/admin_http.py --port 8765
```

Open `http://127.0.0.1:8765/`. A server-side `GH_TOKEN` or `GITHUB_TOKEN`
environment variable must refer to **the authenticated GitHub account
gobbajogger** to enable owner editing. No token is accepted from the
browser and no credential is committed or displayed. Without verified
owner identity the interface is read-only.

The owner selects an **exact record, Elite/Pro model and package SHA**,
then separately records install, restart, observed restart *and* boot,
test with no issue observed, or test with an issue. These are
**owner-reported history events**, not readback of installed bytes and not
hardware approval. A form can optionally include a model-explicit owner
quotation and direct source link. The preview checks the full record and
an append-only audit event, then shows confidence and gates. By default,
every operation is a dry-run. An explicit one-time feature-branch commit
requires both `--allow-commit` and `--expected-head` pinning the **current
full branch SHA** plus the verified owner GitHub token. There is no
auto-merge or ref update to `main`, `nightly`, or `gh-pages`.

The public, concise, dark-only release-status view is
`release-status/public/index.html`. It reads the generated catalog,
displays tested/reported/missing/withdrawn status for each model,
and hides package hashes and detailed provenance inside an expandable
section. It is **not** deployed to the public installer by this PR.
The owner admin offers an explicit, read-only promotion-candidate
dry-run: `GET /api/promotion-candidate?catalog_id=nightly-94197fc8-dev`.
The pure `promotion_candidate.py` reports each model's missing gates.
Even a passing snapshot always has `deployment_authority=not-confirmed`,
`promotion_authorized=false` and `promotion_performed=false`. Actual
promotion authority, publication procedure, reviewer/owner gates and
branch protections remain separate, outside this thread.

### Optional installer outcome contract

`release-status/installer/outcome-bridge.mjs` is a working, testable
browser event adapter. In the **authorized installer integration**, load
the module using a same-origin path and bind
`createOutcomeReporter({catalogId,model,packageSha256,consent})` to the
installer's actual transfer callbacks. `attachInstallerEvents(target,reporter)`
consumes the `dnbox:installer-outcome` event and distinguishes:

- `transfer-started` and `transfer-completed`: browser transport only.
- `restart-observed`: a separate restart observation **after** transfer.
- `boot-observed`: only after restart and a separate
  `bootConfirmation="manual"` or `"device-readback"` callback.
- `failed`: explicit failure, never turned into boot or approval.

Reports require a live, explicit opt-in and go only to the same-origin
`/api/installer-report` receiver. That receiver is **disabled by default**;
for local tests the operator uses `--enable-reports`, writing a
permissions-restricted JSONL file under the operator's local user account.
Data is limited to an ephemeral run UUID, ordered bounded event number,
event type/time, exact catalog/model/package identity and the fixed
`anonymous-unverified` confidence. No IP, MAC, serial, USB identity,
raw device proof, cookie, browser fingerprint or firmware bytes are
persisted. Consent refusal produces no network request. Duplicate or
out-of-order reports are rejected, including after local service restart,
and receiver storage is capped at 4 MB.

**Integration still needed:** the separate authorized installer writer
must wire actual transfer/restart/boot callbacks and a visible opt-in
control to this adapter, and decide a legitimate HTTPS same-origin
receiver/auth/retention deployment boundary. A public HTTPS GitHub Pages
installer must not be assumed to communicate with this development-only
HTTP loopback service (mixed content and browser network restrictions).
No credential, new hosting bill, unattended device test, or public
deployment is introduced here. The existing installer source and the
independently owned website PR #890 are untouched. An anonymous event
**never authorizes verified install, owner approval or promotion**.

### Independent QA proof and audit chronology

For `owner-hardware-approval`, every positive
`hardware_approvals[]` entry now requires its **own** schema-valid
`model`, matching `source_sha` and that model's `image_sha256`, the
literal quoted owner words explicitly naming only that model, an actual
timezone-aware ISO-8601 timestamp, and a direct HTTPS
`source_link` field. A model-less quotation, a field named `ref` or
free-form `source`, or an Elite quote attached to Pro cannot pass.
Receipt presence is not external authentication of quotation provenance;
human/connector source verification and promotion authority are separate.

A future `install-verified` claim requires **independent** model-specific
`install_verifications[]` with `claimed_model`, matching
`model_proof_model`, proof type, proof observation time/direct source,
verified model and build confidence, and an installed-image hash matching
the claimed model (or the exact source commit's firmware GIT readback).
Legacy single-model `install_verification` is accepted only for its own
claimed model, never copied to the other. USB product labels and browser
ACK cannot prove hardware identity. Existing owner Elite reports for
0.0.25.1 and 0.0.26 remain **reported only**; Pro is **untested** and
installed image hashes are **unconfirmed**.

Append-only JSONL `time` records the **historical event time** and is
allowed to precede an earlier appended event. Every newly generated
admin event also stores `appended_at` as the validated, monotonic UTC
append instant. Both require real timezone-aware ISO-8601 dates. Older
adopted audit lines remain byte-for-byte immutable; they are not sorted,
backdated, or silently rewritten.

Focused executable tests:

```sh
python3 -m unittest discover -s release-status/tests -p 'test_*.py' -v
node --test release-status/tests/test_outcome_bridge.mjs
```

No firmware build, device write, emulator or full host suite is relevant
to this release-status-only change. Do not mislabel these source tests as
live installer hardware acceptance.

## Finish line

The finish-line artifacts include the generated catalog, the static public status view, and the local authenticated owner/admin service with the opt-in outcome bridge:

- `release-status/catalog/catalog.json`
- `release-status/catalog/index.html`
- `release-status/public/index.html`
- `release-status/admin/index.html` and `release-status/admin/app.js`
- `release-status/tools/admin_http.py` and `release-status/installer/outcome-bridge.mjs`

Generate them again with `generate_catalog.py` after a record change. The committed files must match a fresh generation.

This pull request does not publish the page. This pull request does not merge into main. This pull request does not promote a firmware release.

Remaining owner-gated actions:

1. Merge pull request #883 into main. That merge is the owner's decision.
2. After that merge, a designated publisher can publish the generated status page. A possible location is a GitHub Pages path on the main site, for example `https://gobbajogger.github.io/X0X37-for-Loopy-Pro/release-status/`. This workstream does not write to `gh-pages` and does not publish that page.
3. Firmware release promotion is not part of this pull request. The promotion evaluator only reports gates.

## States

Flags on one record are independent. The validator sets or accepts a flag only when the evidence for that flag exists.

| Flag | Meaning |
|---|---|
| `built` | Images exist. For a bundled package, `controller_check` must be `PASS` for each model. |
| `gate-passed` | The strict emu360 gate passed on the exact images (`applies_to_image=exact-image`, 0 late, worst time under 11609 µs). The evidence must also carry `elite_image_sha256` and `pro_image_sha256` equal to the record images. This is an emulator result, not a hardware result. A result on other inputs (for example d408), or a d408 result relabeled as exact-image without those matching hashes, does not pass. |
| `published-nightly` | The nightly catalog published this exact package. |
| `installation-reported` | A report of an install. The model and the build may lack a verified identity. |
| `install-verified` | Model and exact-build confidence are `verified`. A stored installed hash must equal `models.<claimed model>.image_sha256`. A firmware GIT readback of `source_sha`, or a backup field that names that image hash, is also accepted. An inferred, reported, or absent field fails. A confidence flag alone is not a hash. A USB port name or a product string is not model proof, regardless of flags. Model proof must be one of: `board_id`, `model_id`, `flash_id`, `mac`, `firmware_readback`, `backup_field`. A "current nightly" guess is not proof. |
| `boot-restart-reported` | A report says boot or restart occurred. If identity is not verified, the flag lacks verification and does not satisfy any promotion gate. A web install success alone is not boot, restart, or hardware proof. |
| `owner-hardware-approval` | The owner's own words, quoted, with a timestamp and a direct source link, and `install-verified` is already true. A relay with no quotation does not satisfy this gate. Approval is scoped to one exact model and one exact build. An Elite approval never implies a Pro approval. The overall flag is true only when every model on the record has its own scoped approval. |
| `withdrawn` | Do not offer this exact build for install. A withdrawn record cannot gain a new promotion flag. |

Promotion flags are `gate-passed`, `published-nightly`, `install-verified`, and `owner-hardware-approval`. Missing gate evidence blocks `gate-passed`. An unresolved conflict (generic rule, used only with a synthetic record in tests) blocks hardware promotion. The 0.0.26 seed has no conflict.

Identity confidence values are `verified`, `inferred`, `reported`, and `absent`.

History events are ordered by `sequence`. The current state is the event with `is_current=true` (the latest report). Exact timestamps are stored when known. When the source does not give a time, `time` is `null` and `time_precision` is `unknown`.

## Seed records

### `nightly-94197fc8-dev` (0.0.26)

Source `94197fc839b00aff47b98ca02f3c6396e4a6eabf`. Exact-image emu360 strict PASS: Elite 11561 µs, Pro 11525 µs, 0 late, with elite and pro image SHA-256 on the evidence item. Controller bundle check PASS. Live nightly catalog latest id.

History:

1. Earlier, recovery-history (not current): stuck on restart with a black screen for 1–2 minutes; 0.0.25.1 recovered the device; upgrade-path cause not proven; M-Flasher worked; the FM-1 USB label did not prove a wrong model. Relayed via Orchestrator, owner words not quoted. The 0.0.25.1 recovery build is a reference only. Its hashes come from the live nightly `versions.json` (`nightly-79280e76-dev`, source `79280e763341d03ba43266d3e25dad66f12de860`).
2. Intermediate, install-and-boot-report (not current): install, automatic restart, and boot at normal speed on the owner's SMK. The event source is `owner-reported via Dot relay`. This is not a quotation of the owner. No exact time was available. Model Elite was inferred at the time of this event. Installed hash is absent.
3. Latest, current state: the owner stated in the Orchestrator chat that he tested 0.0.25.1 and 0.0.26 and used Elite. Quotes, 2026-11-13, Asia/Vladivostok (UTC+10): at 23:20, "Bro I tested a 25,1 and 26 already"; at 23:22, "I used elite". Relayed by Orchestrator. Model Elite is owner-stated and not hardware-field verified. Installed hash is absent. Pro was not tested. An Elite report never implies Pro. This is not a full functional suite pass.

States: `built`, `gate-passed`, `published-nightly`, `installation-reported`, and `boot-restart-reported` are true. `install-verified` and `owner-hardware-approval` are false. `withdrawn` is false. Identity confidence: model `reported`, build `inferred`, installed_hash `absent`. The channel is `nightly-hardware-test`. This is not a promotion to MAIN.

ELF provenance is hashes only, `published=false`. The toolchain is `null`. The kept build logs do not name the compiler. The validator does not infer a toolchain. ELF bytes are not in this tree and must not be added.

### `nightly-79280e76-dev` (0.0.25.1, stable MAIN)

Source `79280e763341d03ba43266d3e25dad66f12de860` on branch `release/0.0.25.1`. The channel in this workflow is `stable-MAIN`. This record does not publish. It does not edit the MAIN installer or PR #880.

Hashes come from the live nightly `versions.json` row.

The owner reported testing 0.0.25.1 on an SMK-37 Elite. Quotes, 2026-11-13, Asia/Vladivostok (UTC+10): at 23:20, "Bro I tested a 25,1 and 26 already"; at 23:22, "I used elite". Source: `owner in Orchestrator chat, relayed by Orchestrator`. The installed package hash was not confirmed. Pro was not tested. An Elite report never implies Pro.

`installation-reported` is true. `install-verified` and `owner-hardware-approval` are false. A test report is not hardware approval. Identity confidence: model `reported`, build `absent`, installed_hash `absent`. The nightly catalog label DEVICE NOT TESTED is unchanged and is not a full hardware qualification.

### `nightly-600b3b45-dev` (0.0.26, withdrawn)

Channel in this workflow: `withdrawn`. Source `600b3b45f8d59c3432d958238b05753eb6f2fcb5`. Published by nightly PR #116 (merge 13 November 2026, 01:34:16 UTC). Withdrawn by nightly PR #120 (merge 13 November 2026, 05:44:03 UTC) after boot and restart failures. Replaced by `nightly-94197fc8-dev`. `withdrawn` is true. The validator refuses a promotion of this record.

`built` is true. `controller_check` is `PASS` for both models. Sources for that PASS are the nightly PR #116 body, the 600b publish-branch catalog (`package_nightly --check PASS both`), and the independent review's `--check` receipt.

`controller_size` 61552 comes from the 600b publish-branch catalog and from the historic 600b packages. It is not taken from the 94197fc8 record.

`package_sha256` equals `image_sha256`. This was verified against the historic `.fwsc` bytes (Elite `25b78701…`, Pro `528dac38…`). CRC-32 of those bytes is Elite `5c668fbb` and Pro `407cc185`.

## ELF files

Do not add ELF binaries. Do not add `.fwsc` bytes. A release record may attach `elf_provenance` with hashes and build facts only.
