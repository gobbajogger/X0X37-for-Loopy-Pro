# Release status workflow

This directory holds one release record for each exact firmware build, a JSON schema, an append-only audit log, a validator, and tests.

It does not install firmware. It does not write to a device. It does not publish a package. It does not edit the nightly catalog, the MAIN installer, `docs/ROADMAP.md`, or `.github/workflows`.

Admin authentication is option C: the GitHub identity `gobbajogger` plus this append-only JSONL audit file. No new credential, GitHub App, or permission is required. Each accepted state change must add a new audit line. A later line may retract an earlier line. No line is rewritten. The Git commit that contains the line binds it to the GitHub identity. Seed lines have `commit_sha` set to `null` so those lines are not rewritten after the first commit; the containing commit is the bind.

## Layout

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

This tree lives on the orphan branch `lane/rp-release-status-workflow-20261113` in `gobbajogger/X0X37-for-Loopy-Pro`. The branch holds only `release-status/`. It is not based on `main`, `nightly`, `gh-pages`, or `wip/one-build`.

## How to run

From this directory, or from the branch root:

```
python3 release-status/tools/validate_release_status.py \
  --records release-status/records \
  --schema release-status/schema/release-record.schema.json \
  --audit release-status/audit/audit.jsonl
```

Exit `0` means every record and the audit file are valid. Exit `1` is a schema or evidence error; the script prints the rule and the record id. Exit `2` is a usage or file error.

`--propose CHANGE.json` checks one state change in memory and does not write the record. `--audit-previous FILE` refuses any rewrite of an existing audit line.

```
python3 -m unittest discover -s release-status/tests -v
```

The validator has no third-party dependencies.

## States

Flags on one record are independent. The validator sets or accepts a flag only when the evidence for that flag exists.

| Flag | Meaning |
|---|---|
| `built` | Images exist. For a bundled package, `controller_check` must be `PASS` for each model. |
| `gate-passed` | The strict emu360 gate passed on the exact images (`applies_to_image=exact-image`, 0 late, worst time under 11609 µs). This is an emulator result, not a hardware result. A result on other inputs (for example d408) does not pass. |
| `published-nightly` | The nightly catalog published this exact package. |
| `installation-reported` | A report of an install. The model and the build may lack a verified identity. |
| `install-verified` | Model and exact-build confidence are `verified`, and the installed image hash (or a firmware GIT readback, or a backup field that names the hash) is verified. An inferred, reported, or absent field fails. A USB port name is not proof. A "current nightly" guess is not proof. |
| `boot-restart-reported` | A report says boot or restart occurred. If identity is not verified, the flag lacks verification and is not a clean success for any promotion gate. A web install success alone is not boot, restart, or hardware proof. |
| `owner-hardware-approval` | The owner's own words, quoted, with a timestamp and a direct source link, and `install-verified` is already true. A relay with no quotation does not satisfy this gate. |
| `withdrawn` | Do not offer this exact build for install. A withdrawn record cannot gain a new promotion flag. |

Promotion flags are `gate-passed`, `published-nightly`, `install-verified`, and `owner-hardware-approval`. Missing gate evidence blocks `gate-passed`. An unresolved conflict (generic rule, used only with a synthetic record in tests) blocks hardware promotion. The 0.0.26 seed has no conflict.

Identity confidence values are `verified`, `inferred`, `reported`, and `absent`.

History events are ordered by `sequence`. The current state is the event with `is_current=true` (the latest report). Exact timestamps are stored when known. When the source does not give a time, `time` is `null` and `time_precision` is `unknown`.

## Seed records

### `nightly-94197fc8-dev` (0.0.26)

Source `94197fc839b00aff47b98ca02f3c6396e4a6eabf`. Exact-image emu360 strict PASS: Elite 11561 µs, Pro 11525 µs, 0 late. Controller bundle check PASS. Live nightly catalog latest id.

History, both relayed via Orchestrator, owner words not quoted:

1. Earlier, recovery-history (not current): stuck on restart with a black screen for 1–2 minutes; 0.0.25.1 recovered the device; upgrade-path cause not proven; M-Flasher worked; the FM-1 USB label did not prove a wrong model. The 0.0.25.1 recovery build is a reference only. Its hashes come from the live nightly `versions.json` (`nightly-79280e76-dev`, source `79280e763341d03ba43266d3e25dad66f12de860`).
2. Latest, current state: owner-reported install, automatic restart, and boot at normal speed on the owner's SMK. Model Elite is inferred. Build `94197fc8` is inferred from the current nightly. Installed hash is absent.

States: `built`, `gate-passed`, `published-nightly`, `installation-reported`, and `boot-restart-reported` are true. `install-verified` and `owner-hardware-approval` are false. `withdrawn` is false. Promotion stays blocked because the installed hash is absent, the model is unverified, and there is no owner hardware approval.

ELF provenance is hashes only, `published=false`. The toolchain is `unknown` because the kept build logs do not name the compiler. ELF bytes are not in this tree and must not be added.

### `nightly-600b3b45-dev` (0.0.26, withdrawn)

Source `600b3b45f8d59c3432d958238b05753eb6f2fcb5`. Published by nightly PR #116, withdrawn by nightly PR #120 after boot and restart failures, replaced by `nightly-94197fc8-dev`. `withdrawn` is true. The validator refuses a promotion of this record.

`built` is false: the 600b seed has no `package_nightly --check` result, so `controller_check` is `NOT_RUN`, and a bundled package cannot be `built` without a PASS. `controller_size` 61552 is taken from the same controller SHA-256 as the 94197fc8 record. `package_sha256` equals `image_sha256`, following the 0.0.26 package pattern.

## ELF files

Do not add ELF binaries. Do not add `.fwsc` bytes. A release record may attach `elf_provenance` with hashes and build facts only.
