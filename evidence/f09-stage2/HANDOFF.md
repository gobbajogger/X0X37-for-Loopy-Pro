# Workflow handoff to stage 3

Receiving stage: **3 Test** (new cloud thread; F09 after stage-2 BF-005 implement).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f09-bf005-stage2-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-2 owner: `bc-42c7795e-c06a-577e-b9f6-2b9f2b368ed0`.
Audited / validated source SHA: `8a74ff52cba842c3f2d8365600286f94e6bccdf0`.
Content tree (pre-repair package baseline): `1a1e7a22d136fef6e30702c9d3207d22798a6dd382717acc5ab0f40d2f235acf`.
Git tree (pre-repair): `33d6950b0a378525f2a8b592672275625893239f`.
Firmware tree (pre-repair): `581d1cb213e6ceda3e9fbbcdc324ef8dce67c42f`.
Audited firmware content tree (package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`.
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Repair commits already in tip: `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004) — not re-opened.
BF-005 implement: owner-matching `seq_clk_drop` clears `ext_pending` in `firmware/src/seq/sequencer.c` (blob `fd3f6530…`) + `tests/host/bugfinder_clock_handover_test.c` + `clock-handover` in `tests/run_tests.sh`.
Implement evidence: `evidence/f09-stage2/CHECKS.md`, `evidence/f09-stage2/DEFERRED.md`, `bf005-red.log`, `bf005-green.log`, `clk-lock.log`, `host-unit.log`, `nonowning-drop.log`.
Audit record: `evidence/f09-stage1/AUDIT.md`.
Successor prompt: `evidence/f09-stage2/SUCCESSOR-STAGE3.txt`.
Successor agent id: `bc-ae6fbd79-7084-5960-9b7c-1f7fce57cc2b` (coordinator CreateAgent; owner-visible).
Successor URL: https://cursor.com/agents/bc-ae6fbd79-7084-5960-9b7c-1f7fce57cc2b
startingRef: `cursor/f09-bf005-stage2-fb90`.
API CreateAgent (this pod): attempt1/2 **401** (no `CURSOR_API_KEY`) — see `api-create-stage3-attempt{1,2}.json`.
Handoff status: **complete** — owner-visible Stage 3 id recorded on this remote tip. Stage 2 releases ownership.

## Repair summary

BF-005 only. When the locked MIDI clock source’s link ends, `seq_clk_drop` now clears queued `ext_pending` with `clk_lock`, so a new Bluetooth Start/Clock is not accelerated by stale USB ticks. Non-owning drops leave pending unchanged. Host regression fails on the unfixed tip and passes after repair; `clk-lock` / sequencer stay green. Deferred items in `evidence/f09-stage2/DEFERRED.md`.

## Scope for stage 3

Run the BF-005 host checks on the implement tip. Separate subagents for the test run and for failure investigation. Do not implement deferred items. Do not reopen BF-001–BF-004 or FP-001. Do not take BF-006–BF-007. Do not merge production `main`, publish production Pages, or flash a device.

When checks pass, write the stage-4 successor prompt, push it, and start exactly one owner-visible stage-4 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
