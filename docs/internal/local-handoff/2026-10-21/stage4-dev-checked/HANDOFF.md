# Local stage-4 development validation accepted

Stage 4 accepts exact schema-2 DEV CHECKED records for both maintained models,
including the intended controller. No source repair is required. One fresh LOCAL
stage-5 sole integrator receives the complete candidate and the finished rules lane.

Owner: local chat `01a159cd-9785-7c50-af8b-00f97045b0b0`.
Build input: `55eb830c424ab02548ebe4612680f44c2f7dba8b`, branch
`lane/mb-local-fixture-stage4-20261021`, worktree
`/Users/mac/Documents/x0x37/local_fixture_stage4_20261021`.
This enclosing commit adds only this Markdown handoff. Exact handoff commit and
one returned successor ID are recorded in
`/Users/mac/Documents/x0x37/local-fixture-stage4-start.json`.
Checked non-Markdown source identity:
`6502208820644dd132be8013958281eba569b2718af2470c0e6b7d4e00adf914`.

Read `/Users/mac/Documents/x0x37/AGENTS.md`, `agent-rules.md`, and matching current
rules first. Owner correction: “We are not doing a release install. Only a dev
build and install.” Default every build/install to `X0X_DEV_PADCAL=1`. Skip EVERY
real full gate, including trial, force and owner-validation, and every release
configuration. Cloud execution stays stopped/archived. Future stages remain fresh
local chats. Current owner instructions supersede old checkout/gate prose.

## Exact records and artifacts

Private evidence root:
`/Users/mac/Documents/x0x37/local_fixture_stage4_evidence_20261021`.
`summary.json` records acceptance, artifact identities, budgets and exact host
verification commands. `evidence-sha256.json` freezes the evidence files; binary
identities are recorded separately in the records and summary. No proprietary
dependency or generated binary is included in this handoff commit.

- Elite: `/Users/mac/Documents/x0x37/local_fixture_stage4_evidence_20261021/elite/record.json`. Validation-input identity
  `a3af32a9b8470fb7391624065329126d3c5e18272f186a2c8252e9ee55f9427b`. Package SHA-256
  `2c06e32ce6f08d481c6c7caf0ee74b5057291fcb470f545f6000fbae2db0cde3`, 651396 bytes,
  CRC32 `b7aca7d2`. Code free 2472 bytes;
  static RAM free 524; pool headroom 8364 (minimum 8192).
- Pro: `/Users/mac/Documents/x0x37/local_fixture_stage4_evidence_20261021/pro/record.json`. Validation-input identity
  `950c8be4df45ca6b3c25d7058fc28685a5a9ecfe044e281f5052b2ee0a504fe7`. Package SHA-256
  `722d502216451da7524f7f000a36d3504dea24e4d8a711664d6b7e201dfbb351`, 651392 bytes,
  CRC32 `b0dcfc0f`. Code free 2488 bytes;
  static RAM free 524; pool headroom 8364 (minimum 8192).

Each record directory contains its model package and `ctl.img`, plus all five
logged checks/builds. Controller SHA-256 (both models):
`956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`.
It is 65492/65536 bytes (44 bytes free); whole-file CRC32 `9c07ae67`.
Header checksum `77b7a496` is distinct. Core RAM is 25824/26368 bytes (544 free).
No `--no-controller` shortcut was used. Current raw SMK packages require explicit
insertion of the validated controller; install with matching `--ctl`, never omit
it to save checks. These new artifacts happen to match prior local diagnostic
bytes; install admission comes from the new exact records, not old diagnostics.

## Checked scope and reuse

`elite-plan.json`, `pro-plan.json`, `run-dev.sh` and `inputs/` give exact commands,
reasons and explicit dependencies. Every record was produced by the unchanged
`tools/dev_validate.py`; none was copied or fabricated.

- `check-0.log`: verifies all 184 inherited manifest entries, source comparison,
  immutable local image/tools/settings, compiler, required SDK payloads and four
  clean pinned references. Only the two repaired fixtures and Markdown differ
  from `354d85cb3b237f2dd3ad8f5595feff00fc2e83c1`. Production, guards, unrelated
  tests and build inputs match. Retain 24 prior selected host behavior/installer/
  data/controller passes, 12 DEV-record contract tests and accepted independent
  stage-3 mutation controls. Frozen inherited evidence was not modified.
- `check-1.log`: reruns queue-guard, validation-inputs and install-gate-guard for
  each model in the actual DEV runner environment; all pass. Its prebuild package
  skip note is outside that selection; real package checks follow.
- `check-2.log`: builds prerequisites and checks each actual model identity,
  stock flash map/region table and CRCs; Python/JavaScript package tests pass.
  Builds a real controller and checks its ABI/header/CRC/budgets, actual insertion
  parity, explicit installer selection, application bytes, erased tail, unchanged
  bootloader bytes and removal restoring the original package.
- `build.log` and `controller.log`: the validator's mandatory final builds pass.
  Complete input fingerprints match before/after. `verify-records.py` then verifies
  both records through the maintained verifier on macOS and proves the actual
  package/controller assertions cover the exact final artifact bytes.
- Source exclusion checks pass in both container and host repositories. No
  FM-1 target build was required. No full gate or release build ran.

Local environment: offline `x0x37-validation-curl:local`, image
`sha256:6371d922fca8a479e9b914e3591514f4fd14c68760b21c60e9befac843c02d44`,
Linux x86-64, cc 12.2.0, Python 3.11.2/Pillow 9.4.0, Node 18.20.4.
`CC=cc CXX=c++ X0X_HOST_DEFS=-D_POSIX_C_SOURCE=200809L X0X_TEST_JOBS=1`, private
locks, strict ASan/UBSan. Container network disabled; every SDK file came from
the mounted hash-verified local SDK. See `container-setup.json`, `runtime-tools.log`
and `inputs/input-preflight.json`. Approved compiler SHA-256
`c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f`;
SDK `/Users/mac/fw-AC79_AIoT_SDK` commit
`d179b4484759423312073f5fbb232501aa491047`, all eleven payload hashes verified.
Four reference pins under `/Users/mac/Documents/x0x37/validation_refs` remain clean.
The owned container `x0x37-local-fixture-stage4-20261021` exits after both models.

## Complete set and receiving owner

Stage 5 MUST include Markdown-only `lane/dk-rules-dev-default` at
`c33463cd8c7b01ccb9ab458436d69040c6cbb351` (implementation
`f708dc5d55b000b31f297ec0a2a386bac3108cc1`). Its audit records zero-drift synced
mirrors and independent review PASS. `rules-lane-review.json` confirms only eight
Markdown paths differ from the common `354d85c` base. Preserve the newer capture-
reuse preference as well as the development default. Do not blindly merge old
`b627b83` or `85b0734`. Completeness audit chat
`01a159bb-df40-7982-97e0-10cbbd936431` found required firmware already in `354d85c`.
Cancelled browser `ed494755`, separate web-controller `44c0f12`, refs-oscuro and
proprietary dependencies/binaries stay excluded.

Physical SMC controls still failed per owner. Prior UX/controller evidence is
historical; saved replay, synthetic HCI and MTU-23 simulation are not hardware
PASS. No new hands-on capture is required. DEV CHECKED is not installation,
owner device PASS, release PASS or publication authorization.

Recheck live activity and all four `local-*-start.json` predecessor/current records
before creating the successor; inspect an uncertain creation before retrying.
Only this stage-4 owner creates exactly one fresh LOCAL stage-5 sole integrator,
records its returned ID and verifies it starts, then ends execution. No competing
integrator receives ownership. Preserve all existing checkouts/edits; the primary
`x0x37` checkout remains dirty and was not changed by this stage. No remote fetch,
external push or device write occurred. Commit author is
`genkigenki <3080081+genkigenki@users.noreply.github.com>`.

Stage 5 merges the complete set in its isolated lane and compares resulting
source/configuration/tools/dependencies/artifacts with these checked inputs.
A Markdown-only/new commit identity can reuse the records through
`tools/dev_validate.py verify --commit MERGED ...`; any changed non-Markdown
checked input returns to the earliest affected fresh stage. Never advance failed
validation. New source repairs require fresh stage 2 then stage 3; do not restart
stage 1 or silently repeat an already completed merge.

After merge, stage 5 creates exactly one fresh LOCAL stage 6 for development
installation, owner-data backup/preservation, device lock, model/write-range
checks, exact image/controller readback and every loop-slot CRC comparison.
Missing device leaves install pending, never skips to push. Stage 7 follows
verified installation in another fresh LOCAL chat and performs only a normal
verified fast-forward push to the owner-authorized
`https://github.com/gobbajogger/X0X37-for-Loopy-Pro.git`, `main`.
No early main push, release build or publication. Carry rules 17–18 and the
one-successor requirement through every handoff.
