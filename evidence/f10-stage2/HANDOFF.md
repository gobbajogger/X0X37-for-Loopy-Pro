# Workflow handoff to stage 3

Receiving stage: **3 Test** (new cloud thread; F10 after stage-2 BF-006 implement).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f10-bf006-stage2-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-2 owner: `bc-b0f3069d-5192-5bcd-a58d-cb6673598cd8`.
Audited / validated source SHA: `d0d00da23c3ce2268f06f31f8c062716d5a37ad0`.
Content tree (pre-repair package baseline): `62339630bc6e133da464eed7924645c0bfc09e4de66a8bccafdc60fd2d660c83`.
Git tree (pre-repair): `e06299abf0f35f24c771d011d07c2e74de613369`.
Firmware tree (pre-repair): `32a36f68d1ce7a3c304b447475afe7e2bbb116cf`.
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`.
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Repair commits already in tip: `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005) — not re-opened.
BF-006 implement: `d8_exp2_cr` defined shift in `firmware/src/dsp/drum808.c` (blob `90f46329…`) + `tests/host/bugfinder_drum808_exp2_test.c` + `drum808-exp2` in `tests/run_tests.sh` + `docs/audit/bugfinder-drum808-exp2-20261023.md`.
Implement evidence: `evidence/f10-stage2/CHECKS.md`, `evidence/f10-stage2/DEFERRED.md`, `bf006-red.log`, `bf006-green.log`, `host-unit.log`, `drum808-compile.log`.
Audit record: `evidence/f10-stage1/AUDIT.md`.
Successor prompt: `evidence/f10-stage2/SUCCESSOR-STAGE3.txt`.
Successor agent id: `bc-3c0568a9-8766-54a3-873f-df4a542e0ec5` (name: F10 BF-006 tests; coordinator CreateAgent; startingRef `cursor/f10-bf006-stage2-fb90`).
Successor URL: https://cursor.com/agents/bc-3c0568a9-8766-54a3-873f-df4a542e0ec5
Handoff status: **complete** — owner-visible Stage 3 id recorded on this remote tip. Pod had no `CURSOR_API_KEY` (attempts ×2 → `NO_CURSOR_API_KEY`); coordinator CreateAgent supplied the successor. PR #251 subscribed; local-env review `bc-91b0f44b-8fa4-5170-9287-39a6e578cc98` running. Do not create another Stage 3.

## Repair summary

BF-006 only. `d8_exp2_cr` now applies the integer exponent with `b.u += (uint32_t)(int32_t)n << 23;`, so a valid negative input such as `x=-3.0f` no longer left-shifts a negative signed integer. Host regression fails on the unfixed tip under UBSan and passes after repair; nine numeric exp2 controls and `drum-pan` stay green. Deferred items in `evidence/f10-stage2/DEFERRED.md`.

## Scope for stage 3

Run the BF-006 host checks on the implement tip. Separate subagents for the test run and for failure investigation. Do not implement deferred items. Do not reopen BF-001–BF-005 or FP-001. Do not take BF-007. Do not merge production `main`, publish production Pages, or flash a device.

When checks pass, write the stage-4 successor prompt, push it, and start exactly one owner-visible stage-4 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
