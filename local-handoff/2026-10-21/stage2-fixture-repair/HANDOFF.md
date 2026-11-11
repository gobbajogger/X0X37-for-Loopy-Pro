# Local stage-2 validation fixture repair

The two authorized fixture repairs pass their focused checks. Production code
and admission guards are unchanged. Independent stage 3 and exact schema-2
stage-4 development validation remain required; no install admission exists.

Owner: `01a159c3-3502-7741-a98a-1078792ffa34`, local stage 2.
Base: `5b3c47c6452bc606346c2dcd1823b15bbfa58117`.
Repair: `61e1b7dfda44ff38c9149e0456f37d42dfdc03c9`.
Lane: `lane/mb-local-fixture-repair-20261021`, worktree
`/Users/mac/Documents/x0x37/local_fixture_repair_20261021`.
This enclosing handoff commit changes Markdown only. Use its exact checkpoint
from `/Users/mac/Documents/x0x37/local-fixture-repair-start.json`.

Current rules entry: `/Users/mac/Documents/x0x37/AGENTS.md`, then
`agent-rules.md`. Current maintained rules supersede stale checkout instructions.
The owner said: “We are not doing a release install. Only a dev build and install.”
All builds and installs default to development. Skip every real full-gate
invocation, including trial, force and owner-validation, and release builds.
Scratch routing tests use fake inputs and an absent compiler; they do not run
a real gate or build. All continuation stays local; cloud execution is stopped.

## Repairs and checks

- `tests/host/test_queue_guard.sh`: add synthetic source/mode provenance only
  to its disposable result fixture. Capture the install commit B, not scratch
  HEAD C. Require each negative case to report both an open-queue-line refusal
  and its specific missing, unknown or non-ancestor commit cause.
- `tests/host/test_validation_inputs.py`: remove inherited `MODEL` and `BOARD`
  from the scratch environment so baseline capture matches gate startup.
  Keep all existing identity mutations, reuse, force/trial and purpose routing.

Private evidence:
`/Users/mac/Documents/x0x37/local_fixture_repair_evidence_20261021`.
`summary.json` records exact file hashes, scope, commands and results;
`run-focused.sh` records the reproducible Linux-container setup.

- Clean environment: `sh tests/run_tests.sh queue-guard validation-inputs
  install-gate-guard` passes queue 10/10, identity 4/4, install guard 23/23.
- The same selection with `MODEL=elite BOARD=smk37` passes all three counts.
- `MODEL=pro BOARD=smk37 python3 tests/host/test_validation_inputs.py` passes 4/4.
- `run-negative-control.py` removes provenance only from a disposable source
  copy. All five queue-negative assertions correctly fail for that unrelated
  refusal; the script verifies exit 1 and the diagnostics. This proves the old
  false positives are gone. `negative-control.log` is intentionally failing
  fixture output, not a candidate-test failure.
- `sh -n tests/host/test_queue_guard.sh` and `git diff --check` pass.

Logs are under `clean/`, `elite/`, and `pro-identity.log`. The new manifest
hashes these logs and scripts. The focused runner's prebuild Pro-package skip
note is outside the selected check set; no package test is claimed here.

Environment: `x0x37-validation-curl:local`, image ID
`sha256:6371d922fca8a479e9b914e3591514f4fd14c68760b21c60e9befac843c02d44`,
Linux x86-64, cc 12.2.0, Python 3.11.2, Node 18.20.4. Set `CC=cc CXX=c++
X0X_HOST_DEFS=-D_POSIX_C_SOURCE=200809L X0X_TEST_JOBS=1`, private evidence paths
and locks. Strict ASan/UBSan remains enabled. The owned focused-check container
`x0x37-local-fixture-repair-20261021` is stopped after exit 0.

No firmware, production guard, SDK, toolchain or feature changes occurred. No
real PASS row, real full gate, target build, release build, install, external
push or publication occurred. Other worktrees and edits were preserved.

## Inherited acceptance and complete set

Read the prior complete handoff at
`local-handoff/2026-10-21/stage4-validation/HANDOFF.md` and private evidence at
`/Users/mac/Documents/x0x37/local_stage4_validation_20261021`.
Its 47-entry `evidence-sha256.json` was reverified here with zero mismatches.
That stage 4 refused check 0 after 24/26 selected passes and issued no record.
The two fixture defects above were the only failing selected checks.
Its 12 scratch DEV-record contract tests passed. Reuse unchanged evidence;
do not repeat unrelated suites or target work merely for these fixture edits.

Independent target/package/controller diagnostics passed for input
`354d85cb3b237f2dd3ad8f5595feff00fc2e83c1`. They remain non-admitted until fresh
stage 4 issues exact schema-2 DEV CHECKED package/controller records for the
final candidate. Elite code free 2472 bytes; Pro 2488; both static RAM free 524
and pool headroom 8364 >=8192. Controller 65492/65536 bytes (44 free), core RAM
25824/26368 (544 free). Artifact hashes are in prior
`dev/target-diagnostics/artifact-identity.json`. CRC32: Elite `b7aca7d2`, Pro
`b0dcfc0f`, controller whole-file `9c07ae67`; header `77b7a496` is distinct.
Never substitute historical cloud packages with different hashes.

Approved toolchain `/Users/mac/.jieli/toolchain`, compiler SHA-256
`c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f`;
SDK `/Users/mac/fw-AC79_AIoT_SDK` at
`d179b4484759423312073f5fbb232501aa491047`, all 11 payload hashes matched.
Four clean pinned DSP refs are under `/Users/mac/Documents/x0x37/validation_refs`.
Use existing local Colima/Docker; do not download vendors or commit proprietary
content. No FM-1 target build is required. Previous stopped stage-4 containers
retain private source/build caches.

Finished Markdown-only rules lane `lane/dk-rules-dev-default` at
`c33463cd8c7b01ccb9ab458436d69040c6cbb351` (implementation
`f708dc5d55b000b31f297ec0a2a386bac3108cc1`) still needs inclusion by the sole
final integrator. Its audit records zero-drift mirrors and independent PASS.
Preserve newer capture-reuse preference and development-default rules.
Completeness audit `01a159bb-df40-7982-97e0-10cbbd936431` reported required
firmware changes present in `354d85c`. Do not blindly merge old `b627b83` or
`85b0734`; reconciled patch IDs differ. Cancelled browser `ed494755`, separate
web-controller `44c0f12`, refs-oscuro and proprietary binaries stay excluded.

Prior installed UX/controller evidence is historical. Physical SMC controls
still failed per owner. Saved replay, synthetic HCI and MTU-23 simulation are
not hardware PASS. No new hands-on capture is required.

## Receiving stage and successor

Exactly one fresh LOCAL stage-3 owner receives this repair. Check live project
chats and `/Users/mac/Documents/x0x37/local-fixture-repair-start.json` before
creating a successor; only this stage-2 owner creates its stage-3 successor.
Record its returned ID and verify it starts, then end stage-2 execution.

Stage 3 must use separately scoped helpers for queue/provenance tests and
validation-input environment/reuse tests. Start from the exact handoff commit
in an isolated lane. Independently verify affected tests, including the
`MODEL=elite BOARD=smk37` dev_validate environment; inherit matching unrelated
passes and investigate any unexpected failure before broadening scope. New
source repairs go to a fresh stage 2. Do not restart stage 1.

After acceptance, stage 3 creates exactly one fresh LOCAL stage-4 DEV validation
chat with exact candidate, justified check reuse, models, budgets and evidence.
Stage 4 must issue schema-2 DEV CHECKED records before creating exactly one
stage-5 sole integrator. Stage 5 includes the completed rules lane and preserves
firmware/data/model guards, then creates separate stage 6 for development
installation, data preservation and readback. Missing device leaves installation
pending. Stage 7 follows verified installation and performs normal verified
fast-forward push only to the authorized private destination
`https://github.com/gobbajogger/X0X37-for-Loopy-Pro.git`, `main`.
Checkpoint transfer authorization does not allow early main push. No release
build or publication is authorized. Carry the one-successor requirement forward.
