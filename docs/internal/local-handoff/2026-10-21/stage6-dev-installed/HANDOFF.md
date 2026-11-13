# Local stage-6 development installation verified

The designated installer installed the complete accepted development candidate
on the connected SMK-37 Elite and verified application-loader completion,
controller readback and preservation of every complete loop-slot region.
Stage 6 is complete. Exactly one fresh LOCAL stage 7 receives Git-push ownership.
No further firmware merge, build, installation or test cycle is required.

Owner: `01a159e9-d466-7eb1-b43c-dccb1f9f57bc`.
Installed revision: `b3112a8c152dabb1ac8b7f88a4746380895b5e4d`.
Worktree: `/Users/mac/Documents/x0x37/local_install_stage6_20261021`.
Branch: `lane/mb-local-install-stage6-20261021`.
This enclosing commit adds only this handoff and the `BLE_ATTEMPTS.md` ledger entry.
Its exact revision and the single receiving chat ID are recorded in
`/Users/mac/Documents/x0x37/local-install-stage6-start.json`.
Checked non-Markdown source identity remains
`6502208820644dd132be8013958281eba569b2718af2470c0e6b7d4e00adf914`.

## Current scope and complete set

Read `/Users/mac/Documents/x0x37/AGENTS.md`, then `agent-rules.md` and matching
workflow/shared-work/commit/integration/install rules. Current owner correction:
“We are not doing a release install. Only a dev build and install.” Every build
or install defaults to `X0X_DEV_PADCAL=1`. Skip EVERY real full gate, including
trial, force and owner-validation, and every release configuration. Cloud execution
stays stopped/archived; future stages must be fresh LOCAL chats.

The accepted complete-set merge is `5bd60cc93522aac167dfa2eb37ada798280256af`,
with accepted stage 4 `72089c23be50f96a6ef4f1ef08d204fabec88430` and finished rules
`c33463cd8c7b01ccb9ab458436d69040c6cbb351` (implementation `f708dc5`). Its accepted
stage-5 handoff is the installed `b3112a8`. No further merge was required or made.
The full firmware ancestry is already present: clip isolation/velocity, 303
velocity/ADSR and auto-slide, SMC dedup/drum banks and reachability/pickup,
song actions, help/read-only feedback, controller/data guards, workflow identity,
receive/SMK reconciliation, capture reuse and accepted fixture repairs.
Both capture-reuse and development-default preferences remain present.

Read inherited complete evidence and scope in
[stage 5](../stage5-complete-set/HANDOFF.md),
[stage 4](../stage4-dev-checked/HANDOFF.md),
[stage 3](../stage3-fixture-retest/HANDOFF.md),
[stage 2](../stage2-fixture-repair/HANDOFF.md), and
[prior validation](../stage4-validation/HANDOFF.md).
Do not blindly merge `b627b83`/`85b0734`; cancelled browser `ed494755`, separate
web-controller `44c0f12`, refs-oscuro, proprietary SDK/toolchain and generated
binaries stay excluded from Git.

Stage-5 private evidence is
`/Users/mac/Documents/x0x37/local_integration_stage5_evidence_20261021`.
Its final manifest actually contains 14 files; all 14 match in stage 6.
Retain its 219 inherited manifest checks and complete SDK/toolchain/reference
mapping comparisons, exact record verification, source audit and stopped
successful container evidence. No unrelated suite or target diagnostic reran.

## Actual installation and data preservation

Private stage-6 evidence:
`/Users/mac/Documents/x0x37/local_install_stage6_evidence_20261021`.
`install-record.json`, `install.log`, `preflight.json`, `before/snapshot.json`,
`after/snapshot.json`, `controller-readback-64KiB.bin` and the `.x0xslots` archives
record actual device access, exact arguments, bytes, CRCs and times.
The private manifest freezes the evidence; no binary is tracked in Git.

- Actual intended device: `SMK-37 Elite_900`, USB-MIDI port `X0X FM-1`.
  Original exact record:
  `/Users/mac/Documents/x0x37/local_fixture_stage4_evidence_20261021/elite/record.json`.
  Validation-input identity
  `a3af32a9b8470fb7391624065329126d3c5e18272f186a2c8252e9ee55f9427b`.
  Package `elite/x0x-smk37-elite.fwsc`, 651396 B, CRC32 `b7aca7d2`, SHA-256
  `2c06e32ce6f08d481c6c7caf0ee74b5057291fcb470f545f6000fbae2db0cde3`.
- Explicit matching `elite/ctl.img`, 65492 B, CRC32 `9c07ae67`, SHA-256
  `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`.
  Header checksum `77b7a496` is distinct. The full 64 KiB readback with erased
  padding matches byte for byte, CRC32 `4a400899`, SHA-256
  `7f560b3bd709d45a920e80e24aed27a2d2852700cab2678dc4cffd2bd4e0c210`.
- Both original records verified against `b3112a8` using maintained
  `tools/dev_validate.py verify`. Elite DEV/queue admission passed again during
  preflight and actual install. The maintained installer ran with exact
  `--commit b3112a8c152dabb1ac8b7f88a4746380895b5e4d --dev-record ELITE_RECORD
  --config dev --ctl ELITE_CTL --backup EXPLICIT_ALL_LOOPS_ARCHIVE --yes`, in a
  watched foreground TTY. Exit 0; no guard replacement, waiver, mocked backend,
  force flag, cached controller, `--no-ctl` or slot skip was used.
- The prewrite owner notice was sent. The tool held its exclusive hardware lock;
  the same lock remained held through all postinstall snapshots. The original
  loop data was archived and validated before writing, independently of the
  installer's mandatory verified archive. Archive images match each other and
  the postinstall images. Slot 1 has three loops, slot 2 two, slot 3 is empty.
- Complete slot bytes (including unused space) are identical before and after:
  slot 1, 81920 B, CRC32 `ee599248`; slot 2, 122880 B, `8a124cb7`; slot 3,
  45056 B, `7e7f9415`. Loop-image CRCs `961626d5`/`9a09fdb9` also match. No restore
  or migration was needed. All five imported loops are preserved.
- Image readback uses the device loader's byte comparison of every 4 KiB sector
  in `[0x4000,0x9D000)` before FINISH_WRITE, then expected runtime identity.
  All 153 sectors completed. This is device-side image readback, not an
  independent host application dump; current USB firmware exposes no such dump.
  Controller readback was additionally captured and compared on the host.
- Selected Felucca loader and independent RAM flash guards exclude the bootloader
  `[0,0x4000)`, calibration/BTIF, stock resources and key_mac. Controller insertion
  leaves package head bytes unchanged. No raw head-reference file was available,
  so the tool reported that host head-hash comparison unrun. No reference was
  fabricated and no bootloader write occurred. Stage-4 exact package/controller
  assertions and unchanged source/range guards remain evidence.
- Elite code free 2472 B; Pro 2488 B. Both static RAM free 524 B and pool headroom
  8364 >=8192 B. Controller 44 B free; core RAM 544 B free. No budget, feature,
  sample quality, licensing or target guard was changed.

The unchanged Pro record/package/controller remain at the same evidence root,
`pro/record.json`, `pro/x0x-smk37.fwsc`, `pro/ctl.img`; Pro was not installed.
Pro identity `950c8be4df45ca6b3c25d7058fc28685a5a9ecfe044e281f5052b2ee0a504fe7`,
package SHA-256 `722d502216451da7524f7f000a36d3504dea24e4d8a711664d6b7e201dfbb351`,
651392 B, CRC32 `b0dcfc0f`. Both models use the identical controller above.

No new build, full gate, release configuration, remote fetch, main push or
publication occurred in stage 6. No source or artifact repair was required.
The only private preflight formatter error used a nonexistent `identity` key;
it was corrected to the existing `sha256` key and rerun before installation.
No validator, record or checked source changed. The primary dirty checkout's
HEAD/status/diff hash remain unchanged.

Physical SMC controls still failed per the owner. Prior UX/controller results
are historical; replay/synthetic HCI/MTU-23 simulation is not hardware PASS.
SMC behavior on this installation is NOT TESTED. No new hands-on capture is
required. DEV CHECKED and verified install do not grant owner device PASS,
release packaging or publication authorization.

## Exactly one receiving stage 7

Only stage 6 creates exactly one fresh LOCAL stage-7 Git-push owner after this
verified installation. Recheck live project activity and all predecessor/start
records before creation; inspect uncertain creation rather than retrying.
Record its returned ID, verify it starts, then end stage-6 execution. The receiving
stage 7 performs its stage itself and must not create another stage-7 copy.

Stage 7 performs ONLY a normal verified fast-forward push of the enclosing
Markdown-only handoff commit to the owner-authorized
`https://github.com/gobbajogger/X0X37-for-Loopy-Pro.git`, `main`, and verifies the
remote ref equals that exact commit. Use an isolated local lane/worktree from
this handoff revision in the shared Git database; preserve other checkouts.
Check the destination/ref/ancestry and maintained push guards. Never force push,
rewrite remote history, bypass a refusal, add a merge, repeat install, run a full
gate, build release, publish packages, or start another audit. If push is blocked,
report its actual cause and leave it pending. Finish this workflow after the
verified remote ref; no automatic successor is required.

A changed commit ID or Markdown-only edit can reuse the original DEV evidence.
Any changed checked source/config/tools/dependencies/artifacts returns to the
earliest affected fresh LOCAL stage; source repairs need fresh stage 2 then 3,
never stage 1 or advancement with failed checks. Retain rule 17 stage ownership
and rule 18 exactly-one-successor behavior. Commit author:
`genkigenki <3080081+genkigenki@users.noreply.github.com>`.
