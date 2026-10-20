# Local stage-4 development validation handoff

Stage 4 is not accepted. Two scoped validation fixtures require repair before a
DEV CHECKED record can be issued. Independent development target, package and
controller diagnostics pass. No firmware or test source was changed here.

The owner corrected this task: “We are not doing a release install. Only a dev
build and install.” This supersedes the earlier release/full-gate handoff.
Current rule 11 also requires development by default and skips the full gate,
including trial/owner-validation, without an explicit current release-build
request. Do not restart a full gate or build a release configuration.

Owner: local chat `01a159b0-dda9-7951-aca6-a82762a70ebe`.
Input: `354d85cb3b237f2dd3ad8f5595feff00fc2e83c1`, branch
`lane/mb-local-stage4-20261021`, in
`/Users/mac/Documents/x0x37/local_stage4_20261021`.
This enclosing handoff commit adds Markdown only. The three prior evidence
manifests (52, 17 and 18 entries) were reverified with no mismatches. The input
changes only evidence compared with stage-3-tested `1c6847412b8a380858d2eaace50e8dfe36b7495a`.

Current maintained rules entry: `/Users/mac/Documents/x0x37/AGENTS.md`, then
`agent-rules.md`. Keep fresh local threads, isolated lanes, source/model/queue/
data guards and rules 17–18. All cloud execution remains stopped.

## Results and evidence

Private evidence root:
[/Users/mac/Documents/x0x37/local_stage4_validation_20261021](/Users/mac/Documents/x0x37/local_stage4_validation_20261021).

- `dev/elite-plan.json`, `dev/pro-plan.json`, `dev/run-dev.sh`: scoped development
  validation plans and exact runner setup. The Elite attempt refused `check-0`
  after 24 passing selected tests and two fixture failures. No `record.json`
  exists; Pro's DEV record attempt did not run after that refusal.
- `dev/elite/check-0.log`: final selected-test results. Receive passed strict
  ASan/UBSan; capture replay, production SMC reachability, visibility, SMK
  isolation, diagnostics, INLOG, SMK update contract, installer, slot backup,
  slot migration and controller readback checks pass. The runner's prebuild
  Pro-package skip note is outside the selected check set; actual packages
  were subsequently built and checked independently as below.
- `dev/failure-disposition.json`: exact failures, required dispositions, current
  owner scope, outstanding rules lane and completeness-audit context.
- `dev/investigation/dev-record-contract.log`: all 12 scratch development-record
  contract tests pass. Their synthetic DEV CHECKED text is test output only.
- `dev/run-target-diagnostics.sh`, `dev/target-diagnostics/*`: fresh Elite and Pro
  development builds, each actual package's identity/stock flash map/CRC checks,
  Python/JavaScript controller-insertion checks, controller build and locked
  settings all pass. `artifact-identity.json` records SHA-256, CRC32, size and
  budgets. These diagnostic artifacts are not admitted for installation.
- `input-preflight.json`: approved compiler SHA-256, exact SDK commit and all
  eleven required SDK payload hashes, four clean pinned DSP references, source
  comparison and original container identity. Its initial gate-mode description
  is superseded by `dev/owner-scope.json` and the owner's correction above.
- `evidence-sha256.json`: 47 evidence-file hashes; proprietary binaries and the
  local source bundle are excluded. The artifact JSON separately hashes binaries.

Budgets, all bytes: Elite dev code free 2472; Pro dev code free 2488. Both have
524 static RAM free and 8364 pool headroom (minimum 8192). Controller image:
65492 of 65536, 44 flash bytes free; core RAM 25824 of 26368, 544 bytes free.

Artifact CRC32 values: Elite dev `b7aca7d2`, Pro dev `b0dcfc0f`, controller whole
file `9c07ae67`; controller header checksum `77b7a496` is a distinct value.
Current package hashes differ from historical cloud artifacts; do not substitute
historical bytes or evidence. Controller SHA-256 is
`956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`.

Local environment: Colima x86-64 was started; Docker images already existed.
Development checks/builds used `x0x37-validation-curl:local`, Linux x86-64,
Debian cc 12.2.0, Python 3.11, Pillow 9.4.0 and Node 18.20.4. Environment:
`CC=cc CXX=c++ X0X_HOST_DEFS=-D_POSIX_C_SOURCE=200809L X0X_TEST_JOBS=1`.
Strict receive sanitizer options remained enabled. SDK `/Users/mac/fw-AC79_AIoT_SDK`
and toolchain `/Users/mac/.jieli/toolchain` matched approved identities. No
vendor download occurred. Four references at `/Users/mac/Documents/x0x37/validation_refs`
match the required pins; unchanged DSP reference tests were not newly run.

Owned containers `x0x37-local-stage4-dev-20261021` (failed check) and
`x0x37-local-stage4-builds-20261021` (diagnostic builds PASS) are stopped and
retain private source/build state. They contain no pending device operation.
The initial trial gate ended in linked-worktree Git ownership setup refusal
before any target build, with no gate result. Its failure is historical setup
context only; the corrected dev scope does not require a gate rerun.

## Required fresh repair and retest

Create exactly one fresh LOCAL stage-2 fixture-repair owner, then a fresh stage 3
with separately scoped test/investigation helpers, then a fresh stage-4 DEV
validation owner. No stage-5 successor is appropriate before acceptance.

1. `tests/host/test_queue_guard.sh` constructs a scratch PASS fixture without the
   `Validation inputs` line or `validation-inputs.json` now required by
   `tools/require_gate_pass.sh`. Two positive cases fail before queue checks;
   the negative cases can pass for the wrong reason. Repair the disposable
   fixture using the valid synthetic source/mode provenance pattern in
   `test_install_gate_guard.sh`; keep production admission unchanged. Ensure
   negative cases prove queue-specific refusal. Never write a real PASS record.
2. `tests/host/test_validation_inputs.py` preserves outer `MODEL`/`BOARD` in
   `self.env`; baseline capture includes them, while gate startup removes them.
   The expected reuse then falls through to its intentionally absent compiler.
   Normal clean-env execution passes 4/4; `MODEL=elite BOARD=smk37` reproduces the
   failure. Logs are in `dev/investigation/identity-{clean,model}-env.*`. Isolate
   the scratch environment or use the same effective environment in capture
   and runner; preserve mutation, reuse, force/trial and purpose assertions.

Do not change firmware or production guards to make these fixtures pass. The
repair and independent retest must include the environment used by
`dev_validate.py`. Reuse matching unrelated passes; do not restart stage 1.
Final stage 4 must issue exact schema-2 DEV CHECKED package/controller records
for the intended models and preserve budgets. Stage 5 then receives sole
integration ownership, stages 6 development install and 7 verified normal
fast-forward private-main push remain separate and mandatory. Missing device
leaves installation pending. No fallback install, release build or publication.

## Complete set and ownership

Completeness audit `01a159bb-df40-7982-97e0-10cbbd936431` reported all identified
firmware changes included in `354d85c`. The finished Markdown-only rules lane
`lane/dk-rules-dev-default` at `c33463cd8c7b01ccb9ab458436d69040c6cbb351`
(implementation `f708dc5d55b000b31f297ec0a2a386bac3108cc1`) is still separate.
Its audit records mirror rollout, zero drift and independent review PASS.
Carry this lane to the sole final integrator and include it when handed off.
Preserve both the newer capture-reuse preference and development-default rule.
Do not blindly merge old `b627b83`/`85b0734` branches; their reconciled patch IDs
differ. Cancelled browser `ed494755` and separate web-controller `44c0f12` stay
excluded, as do refs-oscuro and proprietary dependencies/binaries.

The rules rollout added discovery wrappers to this lane's `AGENTS.md` and
`docs/SKILLS.md`. Those two existing uncommitted edits are preserved and excluded
from this handoff commit. Other worktrees and maintained checkout edits remain.
No integration branch was modified; no external push or device write occurred.

Latest activity check showed this validation chat active; auxiliary audit,
archive and rules chats were idle, with no competing repair or integrator.
Receiving owner and returned successor ID are recorded in
[/Users/mac/Documents/x0x37/local-stage4-start.json](/Users/mac/Documents/x0x37/local-stage4-start.json).
Check this record and live activity before successor creation to avoid duplicates.

Prior installed UX/controller evidence does not establish current physical SMC
operation: the owner had reported controls still did not change parameters.
Saved payload replay with synthetic HCI and MTU-23 resegmentation simulation
remain distinct from hardware PASS. No new hands-on capture is required.
