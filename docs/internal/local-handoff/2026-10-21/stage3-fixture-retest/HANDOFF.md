# Local stage-3 fixture retest

Stage 3 accepts the repaired test candidate. Both fixture repairs pass independent
checks; no production source or guard changed. Exact schema-2 stage-4 development
validation remains required before install admission.

Owner: local chat `01a159c8-6ae9-7b71-a391-516af6ff85e4`.
Tested candidate: `8e06a9ccdeb4d5801b7933002f23cdf68ed2d590`.
Repair: `61e1b7dfda44ff38c9149e0456f37d42dfdc03c9`.
Lane: `lane/mb-local-fixture-stage3-20261021`, worktree
`/Users/mac/Documents/x0x37/local_fixture_stage3_20261021`.
This enclosing commit adds only this Markdown handoff. The exact checkpoint and
one returned successor ID are recorded in
`/Users/mac/Documents/x0x37/local-fixture-stage3-start.json`.

Current rules entry: `/Users/mac/Documents/x0x37/AGENTS.md`, then
`agent-rules.md`. Current maintained rules and owner instructions supersede stale
checkout text. The owner said: “We are not doing a release install. Only a dev
build and install.” Builds and installs default to development. Skip every real
full gate, including trial, force and owner-validation, and every release build.
All continuation stays local; cloud execution remains stopped/archived.

## Independent acceptance

Private evidence: `/Users/mac/Documents/x0x37/local_fixture_stage3_evidence_20261021`.
`summary.json` gives results; `evidence-sha256.json` hashes the scripts, reports
and logs. The queue and input helpers had separate scopes and private containers;
the stage owner owns aggregate acceptance and inherited evidence.

- Queue/provenance helper: `sh tests/host/test_queue_guard.sh` passes 10/10 in
  clean and `MODEL=elite BOARD=smk37` contexts; shell syntax passes. Two independent
  disposable mutations remove the identity line or capture HEAD C instead of
  install B. Both yield expected exit 1: all five queue-negative assertions and
  both positives fail on provenance refusal. This proves the repaired assertions
  cannot accept an unrelated refusal and provenance must identify B.
- Validation-input helper: `python3 tests/host/test_validation_inputs.py` passes
  4/4 in each clean, Elite/SMK-37 and Pro/SMK-37 context. Source review confirms
  gate startup clears MODEL/BOARD while `dev_validate.py` injects those selectors
  for check commands. Disposable restoration of the original environment filter
  reproduces inherited-selector reuse failure. The first private control harness
  expected setup exit 1; actual exit is 2. The assertion was corrected only in
  private evidence, then the control-only rerun passed. Candidate tests had
  already passed; both attempts and the disposition are preserved.
- Aggregate direct consumer: `sh tests/run_tests.sh install-gate-guard` passes
  23/23 in clean and inherited Elite contexts. No actual package test is claimed
  by this runner; its prebuild package-skip note is outside the selected scope.
- `git diff --check` passes. The worktree was clean before this Markdown handoff.

Fixture SHA-256 values:
`tests/host/test_queue_guard.sh` =
`1c8af0ad78f77ee94519c1027dafadff9b7bbc9a621dce1aae324507fa8e17c1`;
`tests/host/test_validation_inputs.py` =
`73ce5512f1e976b4b9007c46456970ecb7397a30aeb7574e3dbe43652f4305d4`.
No new source repair is needed. Mutation logs intentionally contain FAIL output;
their harnesses verify those failures. They are not candidate failures.

Environment: existing offline `x0x37-validation-curl:local`, image ID
`sha256:6371d922fca8a479e9b914e3591514f4fd14c68760b21c60e9befac843c02d44`;
Linux x86-64, cc 12.2.0, Python 3.11.2, Node 18.20.4. Settings:
`CC=cc CXX=c++ X0X_HOST_DEFS=-D_POSIX_C_SOURCE=200809L X0X_TEST_JOBS=1`, private
paths/locks, strict ASan/UBSan. Containers with prefix
`x0x37-local-fixture-stage3-` are stopped. Successful runs exited 0; the original
input-helper container exited 1 for the private harness assertion above, resolved
by the control-only successor container (exit 0). Candidate source mounts
were read-only and each test used its own private copy and scratch repositories.

## Evidence reuse and stage 4

Read both inherited handoffs at
`local-handoff/2026-10-21/stage2-fixture-repair/HANDOFF.md` and
`local-handoff/2026-10-21/stage4-validation/HANDOFF.md`.
`aggregate/reuse-verification.json` records exact changed paths and retained
coverage. Relative to prior local validation input
`354d85cb3b237f2dd3ad8f5595feff00fc2e83c1`, only these two test fixtures and
Markdown handoffs differ. All production, guard, build and unrelated test inputs
are byte-identical. The local image/tool settings match. Reverified manifests:
stage-2 local 14 entries, prior local stage-4 47, inherited earlier 52+17+18;
all 148 hashes match. Retain the prior 24 passing selected tests and 12 scratch
DEV-record contract tests; do not rerun unrelated suites for these fixture edits.
Stage 3 independently reran the affected checks and direct install-guard consumer.

Fresh stage 4 must use `tools/dev_validate.py` to issue exact schema-2 DEV CHECKED
records for Elite and Pro packages and their intended controller, with budgets,
identities and logged checks. It may verify and reuse matching source/host results
through a scoped plan with explicit external evidence dependencies. Do not copy
or fabricate a record or successful log. Review the plan against the complete
candidate and record reuse justification. Real package/controller assertions and
required development builds remain stage-4 work. Prior target diagnostics are
non-admitted until this succeeds. No FM-1 target build is required.

Prior target artifacts in
`/Users/mac/Documents/x0x37/local_stage4_validation_20261021/dev/target-diagnostics/artifact-identity.json`
belong to `354d85c`: Elite code free 2472 bytes, Pro 2488; both static RAM free
524 and pool headroom 8364 >=8192. Controller 65492/65536 bytes (44 free), core
RAM 25824/26368 (544 free). CRC32: Elite `b7aca7d2`, Pro `b0dcfc0f`, controller
whole-file `9c07ae67` (header checksum `77b7a496` is distinct). Never substitute
historical cloud artifacts with different hashes. Stopped prior stage-4 containers
retain private build caches, not install admission.

Approved compiler `/Users/mac/.jieli/toolchain` SHA-256
`c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f`;
SDK `/Users/mac/fw-AC79_AIoT_SDK` at
`d179b4484759423312073f5fbb232501aa491047`, prior eleven payload hashes matched.
Four clean pinned DSP refs are under `/Users/mac/Documents/x0x37/validation_refs`.
Stage 4 rechecks required inputs for its records. No vendor download or proprietary
Git content is permitted. Use actual local paths and isolated worktrees/copies.

## Complete set and one receiving owner

Finished Markdown-only rules lane `lane/dk-rules-dev-default` at
`c33463cd8c7b01ccb9ab458436d69040c6cbb351` (implementation
`f708dc5d55b000b31f297ec0a2a386bac3108cc1`) remains outside the candidate and
must reach the sole final integrator. Audit reports zero-drift synced mirrors and
independent review PASS. Preserve newer capture-reuse preference and development
default. Completeness audit `01a159bb-df40-7982-97e0-10cbbd936431` reported required
firmware changes already present in `354d85c`. Do not blindly merge old `b627b83`
or `85b0734`. Cancelled browser `ed494755`, separate web-controller `44c0f12`,
refs-oscuro and proprietary dependencies/binaries remain excluded.

Physical SMC controls still failed per owner. Prior UX/controller evidence is
historical; saved replay/synthetic HCI/MTU-23 simulation is not hardware PASS.
No new hands-on capture is required.

Before successor creation, recheck live project activity and
`local-fixture-stage3-start.json`, `local-fixture-repair-start.json` and
`local-stage4-start.json` in the workspace. Only this stage-3 owner creates exactly
one fresh LOCAL stage-4 DEV validation chat, records its ID, verifies it starts
and ends stage-3 execution. No main push, installation or publication occurred.

After exact DEV CHECKED acceptance, stage 4 creates exactly one fresh local stage-5
sole integrator. Stage 5 includes the completed rules lane, compares merged inputs
with validation and creates stage 6 for development installation, data preservation
and readback. Missing hardware leaves installation pending. Stage 7 follows verified
installation and performs a normal verified fast-forward push only to authorized
`https://github.com/gobbajogger/X0X37-for-Loopy-Pro.git`, `main`. No early main push,
release build or publication. Carry rules 17–18 and this one-successor requirement
through every handoff. New source repairs require fresh stage 2 then fresh stage 3;
do not restart stage 1 or advance a failed stage as accepted.
