# Workflow handoff to stage 4

Receiving stage: **4 Validate** (new cloud thread; F09 after stage-3 BF-005 test).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f09-bf005-stage3-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-3 owner: `bc-ae6fbd79-7084-5960-9b7c-1f7fce57cc2b`.
Audited / validated source SHA: `8a74ff52cba842c3f2d8365600286f94e6bccdf0`.
Content tree (pre-repair package baseline): `1a1e7a22d136fef6e30702c9d3207d22798a6dd382717acc5ab0f40d2f235acf`.
Git tree (pre-repair): `33d6950b0a378525f2a8b592672275625893239f`.
Firmware tree (pre-repair): `581d1cb213e6ceda3e9fbbcdc324ef8dce67c42f`.
Audited firmware content tree (package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`.
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Tested implement tip: `d0d00da23c3ce2268f06f31f8c062716d5a37ad0` plus this stage-3 evidence tip on `cursor/f09-bf005-stage3-fb90`.
BF-005 repair commit: `94f4a012` (sequencer blob `fd3f6530…` + `tests/host/bugfinder_clock_handover_test.c` + `clock-handover` in `tests/run_tests.sh`).
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004).
Test evidence: `evidence/f09-stage3/CHECKS.md`.
Implement evidence: `evidence/f09-stage2/CHECKS.md`, `evidence/f09-stage2/DEFERRED.md`.
Audit record: `evidence/f09-stage1/AUDIT.md`.
Successor prompt: `evidence/f09-stage3/SUCCESSOR-STAGE4.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 4 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY`; coordinator CreateAgent is required.

## Test summary

BF-005 only. Host regressions ALL_PASS under dedicated test-run subagent `bc-a4520fef-24a0-59c3-a24d-66631c79eb1d`: `X0X_TESTS="sequencer clk-lock clock-handover" sh tests/run_tests.sh` exit 0 with `old_pending=0` and `old_pending=3` both PASS; clk-lock green; owner-matching `seq_clk_drop` clears `ext_pending` and non-owning drop leaves pending (code review + suite). No failing required check, so no failure-investigation subagent and no return to stage 2.

## Scope for stage 4

Build Elite and Pro development packages (`X0X_DEV_PADCAL=1`) from the tested candidate on `cursor/f09-bf005-stage3-fb90`. Record exact package and controller identities, budgets, checks, and logs. Host PASS is not a package PASS. Do not implement deferred items. Do not reopen BF-001–BF-004 or FP-001. Do not take BF-006–BF-007. Do not merge production `main`, publish production Pages, or flash a device.

When the affected development packages are DEV CHECKED, write the stage-5 successor prompt, push it, and start exactly one owner-visible stage-5 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
