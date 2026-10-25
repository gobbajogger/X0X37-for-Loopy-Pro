# Workflow handoff to stage 4

Receiving stage: **4 Validate** (new cloud thread; F10 after stage-3 BF-006 test).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f10-bf006-stage3-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-3 owner: `bc-3c0568a9-8766-54a3-873f-df4a542e0ec5`.
Audited / validated source SHA: `d0d00da23c3ce2268f06f31f8c062716d5a37ad0`.
Content tree (pre-repair package baseline): `62339630bc6e133da464eed7924645c0bfc09e4de66a8bccafdc60fd2d660c83`.
Git tree (pre-repair): `e06299abf0f35f24c771d011d07c2e74de613369`.
Firmware tree (pre-repair): `32a36f68d1ce7a3c304b447475afe7e2bbb116cf`.
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`.
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Tested implement tip: `af4947846df7233c68a6be72c4d0c634052a2752` plus this stage-3 evidence tip on `cursor/f10-bf006-stage3-fb90`.
BF-006 repair commit: `37bab7d1` (drum808 blob `90f46329…` + `tests/host/bugfinder_drum808_exp2_test.c` + `drum808-exp2` in `tests/run_tests.sh` + `docs/audit/bugfinder-drum808-exp2-20261023.md`).
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005).
Test evidence: `evidence/f10-stage3/CHECKS.md`.
Implement evidence: `evidence/f10-stage2/CHECKS.md`, `evidence/f10-stage2/DEFERRED.md`.
Audit record: `evidence/f10-stage1/AUDIT.md`.
Successor prompt: `evidence/f10-stage3/SUCCESSOR-STAGE4.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 4 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY` (API create expected ×2 → 401); coordinator CreateAgent is required.

## Test summary

BF-006 only. Host regressions ALL_PASS under dedicated test-run subagent `bc-030b39b2-b267-5127-b260-cc6f988e6f2e`: gcc UBSan harness exit 0 with nine numeric exp2 controls and no `left shift of negative value -3`; `X0X_TESTS="drum808-exp2 drum-pan" sh tests/run_tests.sh` exit 0; `d8_exp2_cr` uses `b.u += (uint32_t)(int32_t)n << 23;` with fractional series / clamping unchanged (blob `90f46329…`). No failing required check, so no failure-investigation subagent and no return to stage 2.

## Scope for stage 4

Build Elite and Pro development packages (`X0X_DEV_PADCAL=1`) from the tested candidate on `cursor/f10-bf006-stage3-fb90`. Record exact package and controller identities, budgets, checks, and logs. Host PASS is not a package PASS. Do not implement deferred items. Do not reopen BF-001–BF-005 or FP-001. Do not take BF-007. Do not merge production `main`, publish production Pages, or flash a device.

When the affected development packages are DEV CHECKED, write the stage-5 successor prompt, push it, and start exactly one owner-visible stage-5 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
