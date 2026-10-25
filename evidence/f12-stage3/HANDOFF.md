# Workflow handoff to stage 4

Receiving stage: **4 Validate** (new cloud thread; F12 after stage-3 BF-008 test).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f12-stage3-bf008-1207`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-3 owner: `bc-d454af26-4104-5460-becb-bcf94a34df5c`.
Audited / validated source SHA: `4f96b0d9f5f11f700ea3fc935800b8714e11f35d`.
Content tree: `f27c89e1ac8e11ab57c5ae7e49145ce19b964082bf3ca9a700a99bfa8a97091e`.
Git tree (pre-repair): `7b35932d5f0adbceeb35111f2136855aa54ebc9b`.
Firmware tree (pre-repair): `5b1a21f07563b0262ece6728051cb0436bd4bfa1`.
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`.
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Tested implement tip: `de11b6d7e3ac3322d2db689675d9ed27ed667785` plus this stage-3 evidence tip on `cursor/f12-stage3-bf008-1207`.
BF-008 repair: `firmware/src/dsp/fastmath.h` blob `a5c26c95cad0d91a548a8db073f59777e36d4e86` (`b.u += (uint32_t)n << 23;` in `fm_exp2_nf`) + `tests/host/run_fastmath_ubsan.sh` + `fastmath-ubsan` in `tests/run_tests.sh`.
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007).
Test evidence: `evidence/f12-stage3/CHECKS.md`.
Implement evidence: `evidence/f12-stage2/CHECKS.md`, `evidence/f12-stage2/DEFERRED.md`.
Audit record: `evidence/f12-stage1/AUDIT.md`, `bf008-definition.md`.
Successor prompt: `evidence/f12-stage3/SUCCESSOR-STAGE4.txt`.
Successor agent id: `bc-eacdb970-e51c-553e-b0f8-fd49e9201626`.
Successor URL: https://cursor.com/agents/bc-eacdb970-e51c-553e-b0f8-fd49e9201626
Successor source: coordinator CreateAgent (owner-visible; F12 package validate).
Local-env review: `bc-964ab6a2-d8a9-50af-9f5c-2241cbc9f1ec` (PR #271 subscribed).
Handoff status: **complete** — owner-visible Stage 4 id on this remote tip.

## Test summary

BF-008 only. Host regressions ALL_PASS under dedicated test-run subagent `bc-b1c2637f-2c21-59cb-8ad9-b502b5b9b0c4`: blob `a5c26c95…` with `b.u += (uint32_t)n << 23;`; `CC=gcc sh tests/host/run_fastmath_ubsan.sh` exit 0; `X0X_TESTS="fastmath fastmath-ubsan" CC=gcc sh tests/run_tests.sh` exit 0. No failing required check, so no failure-investigation subagent and no return to stage 2.

## Scope for stage 4

Build Elite and Pro development packages (`X0X_DEV_PADCAL=1`) from the tested candidate on `cursor/f12-stage3-bf008-1207`. Record exact package and controller identities, budgets, checks, and logs. Host PASS is not a package PASS. Do not implement deferred items. Do not reopen BF-001–BF-007 or FP-001. Do not fold unrelated PR #28 base-lane history. Do not merge production `main`, publish production Pages, or flash a device from this stage. Owner advance-approved flash + nightly publish apply later (Stage 5/6/7); still no production main.

When the affected development packages are DEV CHECKED, write the stage-5 successor prompt, push it, and start exactly one owner-visible stage-5 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
