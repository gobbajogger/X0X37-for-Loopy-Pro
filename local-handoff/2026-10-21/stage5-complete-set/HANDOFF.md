# Local stage-5 complete-set integration accepted

The sole integrator merged the accepted local DEV candidate and finished
development-default rules lane. Both maintained DEV records verify against the
complete merged revision. One fresh LOCAL stage-6 designated installer receives
ownership. No further merge is needed before installation.

Stage-5 owner: `01a159e5-62bf-7b63-86f7-9f4c77cc199f`.
Integration worktree: `/Users/mac/Documents/x0x37/local_integration_stage5_20261021`.
Branch: `lane/mb-local-integration-stage5-20261021`.
Complete-set merge: `5bd60cc93522aac167dfa2eb37ada798280256af`, with parents
`72089c23be50f96a6ef4f1ef08d204fabec88430` and
`c33463cd8c7b01ccb9ab458436d69040c6cbb351`.
This enclosing handoff commit adds only Markdown. Its exact revision and the one
returned successor ID are recorded in
`/Users/mac/Documents/x0x37/local-integration-stage5-start.json`.
Checked non-Markdown source identity:
`6502208820644dd132be8013958281eba569b2718af2470c0e6b7d4e00adf914`.

Read `/Users/mac/Documents/x0x37/AGENTS.md`, `agent-rules.md`, then matching current
workflow/shared-work/commit/integration/install rules. Owner correction:
“We are not doing a release install. Only a dev build and install.” Every build
and install defaults to `X0X_DEV_PADCAL=1`. Skip EVERY real full gate, including
trial, force and owner-validation, and every release configuration. Cloud execution
stays stopped/archived. Every future stage is a fresh LOCAL chat.

## Complete set and evidence

The history-preserving merge adds exactly the eight Markdown paths from the
finished `lane/dk-rules-dev-default` at `c33463c` (implementation `f708dc5`).
All eight resulting blobs match that reviewed lane. The newer capture-reuse
preference and unchanged device-bug rule remain present alongside the dev default.
Its saved audit records the earlier independent review PASS and mirror rollout.
No additional rule repair or firmware edit was made here.

Private stage-5 evidence:
`/Users/mac/Documents/x0x37/local_integration_stage5_evidence_20261021`.
`verify-merge.py`, `verify-merge.log` and `summary.json` record the comparison;
`final-checks.json` and `handoff-commit-verification.log` cover the enclosing
handoff revision. `evidence-sha256.json` freezes the private evidence files.

- All 219 frozen manifest entries match: stage-4 35, local repair 14, previous
  local stage-4 47, independent local stage-3 36, inherited 52+17+18.
- Each record's 105 dependency entries, five reference fingerprints and two
  validator files match current bytes after mapping original container mounts to
  their local sources. This includes full toolchain/SDK contents, external plans,
  evidence and scripts. The required compiler, eleven SDK payloads, SDK commit,
  four clean pinned references, immutable image and stopped successful container
  were also checked. There are 112 distinct local content comparisons.
- Both exact packages/controllers match their record and actual-artifact checks.
  Original Linux tool/Python/platform/configuration fingerprints remain recorded;
  no new build or runtime test suite was needed for this Markdown-only merge.
- `prefs_lint.py` passes all 18 entries; all 21 local Markdown links resolve;
  `git diff --check` passes. Merge content preserves both owner preferences.
- Required firmware ancestry was checked against the completeness audit list:
  clip isolation/velocity, 303 velocity/ADSR and auto-slide, SMC dedup/drum banks,
  reachability/pickup, song actions, feedback/read-only rows, controller/backup
  guards, workflow identity, receive/SMK reconciliation, capture reuse and both
  accepted fixture-repair stages. Required firmware was already in `354d85c`.
- Cancelled browser `ed494755` and separate web-controller `44c0f12` are absent
  from reachable history. No old `b627b83`/`85b0734` merge, refs-oscuro, proprietary
  dependency or generated binary was added. The only new tracked paths are
  Markdown. The dirty primary checkout's HEAD, status and full diff hash match
  the initial snapshot; no other worktree was edited.

The first private comparison harness assumed cancelled browser object `ed494755`
was locally available for an ancestry command. It is absent. The read-only harness
was corrected to inspect reachable full commit IDs, covering both absent and
available excluded objects, then passed. The initial log and disposition remain
private. No candidate or validation record changed for that harness correction.

Read the accepted `stage4-dev-checked/HANDOFF.md` and inherited
`stage3-fixture-retest/HANDOFF.md`, `stage2-fixture-repair/HANDOFF.md`, and
`stage4-validation/HANDOFF.md` under this same `local-handoff/2026-10-21` directory.
Retain their scoped host passes, mutation controls and actual package/controller
checks; do not repeat unrelated suites or target diagnostics for Markdown.

## Exact development artifacts

Accepted private root:
`/Users/mac/Documents/x0x37/local_fixture_stage4_evidence_20261021`.
Each model directory contains its original `record.json`, package and `ctl.img`.
Records were generated by the maintained validator, not copied or fabricated.

- Elite: `elite/record.json`, `elite/x0x-smk37-elite.fwsc`, `elite/ctl.img`.
  Validation-input identity
  `a3af32a9b8470fb7391624065329126d3c5e18272f186a2c8252e9ee55f9427b`.
  Package SHA-256 `2c06e32ce6f08d481c6c7caf0ee74b5057291fcb470f545f6000fbae2db0cde3`,
  651396 bytes, CRC32 `b7aca7d2`; code free 2472 bytes.
- Pro: `pro/record.json`, `pro/x0x-smk37.fwsc`, `pro/ctl.img`.
  Validation-input identity
  `950c8be4df45ca6b3c25d7058fc28685a5a9ecfe044e281f5052b2ee0a504fe7`.
  Package SHA-256 `722d502216451da7524f7f000a36d3504dea24e4d8a711664d6b7e201dfbb351`,
  651392 bytes, CRC32 `b0dcfc0f`; code free 2488 bytes.
- Both: static RAM free 524, pool headroom 8364 >=8192 bytes.
  Identical controller SHA-256
  `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`,
  65492/65536 bytes, 44 free; core RAM 25824/26368, 544 free.
  Whole-file CRC32 `9c07ae67`; header checksum `77b7a496` is distinct.

The raw model package needs the validated controller inserted with explicit
`--ctl MODEL_DIRECTORY/ctl.img`. Never omit it, use a cached controller, or select
`--no-ctl` to save checks. Controller readback must compare the complete 64 KiB
block including erased padding; its block CRC is distinct from image-file CRC.

Verify the final revision with the unchanged maintained tool:
`python3 tools/dev_validate.py verify --repo INTEGRATION_WORKTREE --commit FINAL
--record MODEL_RECORD --fwsc MODEL_PACKAGE --ctl MODEL_CONTROLLER`.
Exact expanded commands/results for both models are in private summaries/logs.
An enclosing Markdown-only commit reuses the same records. Any substantive change
to checked source/config/tools/dependencies/artifacts returns to the earliest
affected fresh stage. New source repairs require fresh stage 2 then stage 3;
never restart stage 1, bypass guards or advance failed validation.

Build environment retained: offline `x0x37-validation-curl:local`, image
`sha256:6371d922fca8a479e9b914e3591514f4fd14c68760b21c60e9befac843c02d44`,
Linux x86-64 cc 12.2.0, Python 3.11.2/Pillow 9.4.0, Node 18.20.4;
`CC=cc CXX=c++ X0X_HOST_DEFS=-D_POSIX_C_SOURCE=200809L X0X_TEST_JOBS=1`.
Strict ASan/UBSan and private locks were retained by stage 4. Approved compiler
SHA-256 `c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f`;
SDK `/Users/mac/fw-AC79_AIoT_SDK` at `d179b4484759423312073f5fbb232501aa491047`;
references `/Users/mac/Documents/x0x37/validation_refs`. No FM-1 target build.

## One receiving installer and next stage

Only this stage-5 owner creates exactly one fresh LOCAL stage 6 after final
acceptance. Recheck live project activity and all predecessor/current start
records before creating it. Record the returned ID and verify it starts, then
end stage-5 execution. Inspect an uncertain creation before any retry.
The stage-6 successor performs installation itself; it must not duplicate its stage.

Stage 6 is the designated development installer. Use its own isolated lane at
the final handoff commit; preserve all other checkouts and edits. No remote fetch,
main push, release build or publication is required or authorized in stage 6.
Read maintained installer/tool headers and current install rules. Identify the
actual intended SMK-37 model and use its matching exact package/record/controller.
Before writing, notify the owner briefly, hold the device lock, verify bootloader
protection and write ranges, and back up/verify owner data and every loop slot.
Use guarded `tools/fm1_install.py` with exact `--commit FINAL --dev-record FILE
--config dev --ctl FILE` and explicit full backup; run in a foreground TTY or
named watched session. Preserve queue/model/source/data guards; never bypass a
refusal, use a fallback, write the bootloader or skip slot/controller checks.

Record final revision/configuration/model/artifact CRCs and validation record in
the install ledger. Verify image/controller readback and compare every loop-slot
CRC after installation before claiming preservation. A missing device or required
readback leaves install pending; it NEVER silently skips to push.

Physical SMC controls still failed per the owner. Prior UX/controller results are
historical. Replay/synthetic HCI/MTU-23 simulation is not hardware PASS. No new
hands-on capture is required. DEV CHECKED and a verified install do not themselves
prove owner-tested SMC behavior or authorize release/publication.

After verified installation, only stage 6 creates exactly one fresh LOCAL stage 7.
Stage 7 performs ONLY a normal verified fast-forward push to owner-authorized
`https://github.com/gobbajogger/X0X37-for-Loopy-Pro.git`, `main`, and verifies the
remote ref. No early push, force push, release build, public packaging or publication.
Carry rules 17–18, full scope/evidence/identities and one-successor ownership through
every handoff. Commit author:
`genkigenki <3080081+genkigenki@users.noreply.github.com>`.
