# Workflow handoff to stage 4

Receiving stage: **4 Validate** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f08-bf004-stage3-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-3 owner: `bc-85452fe2-cacc-5d1e-a23d-189f00c0f7c9`.
Audited / validated source SHA: `6df0f81fbbe0adc3207f67d912d2fe0ab676be04`.
Content tree (pre-repair package baseline): `69ccb3d77a718d32191cf17a75bce6454dad807ef9bb76f6050e6532a0bff3ad`.
Git tree (pre-repair): `4dc57f74141fd1f01728a0001025b6e76065bf19`.
Audited firmware content tree (package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`.
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Tested implement tip: `8a74ff52cba842c3f2d8365600286f94e6bccdf0` plus this stage-3 evidence tip on `cursor/f08-bf004-stage3-fb90`.
BF-004 repair commit: `ce690772` (engine blob `ab6d9667…` + `tests/scenarios/midiowner.x0x`).
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001).
Test evidence: `evidence/f08-stage3/CHECKS.md`.
Implement evidence: `evidence/f08-stage2/CHECKS.md`, `evidence/f08-stage2/DEFERRED.md`.
Audit record: `evidence/f08-stage1/AUDIT.md`.
Successor prompt: `evidence/f08-stage3/SUCCESSOR-STAGE4.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 4 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY`; coordinator CreateAgent is required.

## Test summary

BF-004 only. Host regressions ALL_PASS under dedicated test-run subagent `bc-d9a62eaf-b661-5df9-9a5c-893e387d4a33`: `build_host`, `midiowner.x0x` (stale non-owner Off leaves hold; owner Off / different-note / same-owner / TRS green), `usbnotes.x0x`, `blenotes.x0x`, and engine Note Off `src_held` gate confirmed for `src<2` with TRS `src>=2` release preserved. No failing required check, so no failure-investigation subagent and no return to stage 2.

## Scope for stage 4

Build Elite and Pro development packages (`X0X_DEV_PADCAL=1`) from the tested candidate on `cursor/f08-bf004-stage3-fb90`. Record exact package and controller identities, budgets, checks, and logs. Host PASS is not a package PASS. Do not implement deferred items. Do not reopen BF-001–BF-003 or FP-001. Do not take BF-005–BF-007. Do not merge production `main`, publish production Pages, or flash a device.

When the affected development packages are DEV CHECKED, write the stage-5 successor prompt, push it, and start exactly one owner-visible stage-5 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
