# Workflow handoff to stage 3

Receiving stage: **3 Test** (new cloud thread; F12 after stage-2 BF-008 implement).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f12-stage2-bf008-1207`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-2 owner: `bc-5b881bc0-b2cc-5ef5-8da9-e7db915ac6a8`.
Audited / validated source SHA: `4f96b0d9f5f11f700ea3fc935800b8714e11f35d`.
Content tree: `f27c89e1ac8e11ab57c5ae7e49145ce19b964082bf3ca9a700a99bfa8a97091e`.
Git tree (pre-repair): `7b35932d5f0adbceeb35111f2136855aa54ebc9b`.
Firmware tree (pre-repair): `5b1a21f07563b0262ece6728051cb0436bd4bfa1`.
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`.
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Repair commits already in tip: `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007) — not re-opened.
BF-008 implement: defined unsigned exponent shift in `firmware/src/dsp/fastmath.h` (blob `a5c26c95…`) + `tests/host/run_fastmath_ubsan.sh` + `fastmath-ubsan` in `tests/run_tests.sh` (three paths only from open PR #28).
Implement evidence: `evidence/f12-stage2/CHECKS.md`, `DEFERRED.md`, `bf008-red.log`, `bf008-green.log`, `host-unit.log`, `fastmath-numeric.log`.
Audit record: `evidence/f12-stage1/AUDIT.md`, `bf008-definition.md`.
Successor prompt: `evidence/f12-stage2/SUCCESSOR-STAGE3.txt`.
Successor agent id: none.
Successor URL: none.
Successor source: pending coordinator CreateAgent (pod has no `CURSOR_API_KEY`).
Handoff status: **incomplete** until an owner-visible Stage 3 id is recorded on this remote tip.

## Repair summary

BF-008 only. `fm_exp2_nf` applies the integer exponent with `b.u += (uint32_t)n << 23;` so negative `n` is defined. Host UBSan fails on the unrepaired tip blob and passes after repair; numeric `fastmath` stays green. Deferred items in `evidence/f12-stage2/DEFERRED.md`.

## Scope for stage 3

Run the BF-008 host checks on the implement tip (`CC=gcc` if default `cc` lacks clang UBSan runtime). Separate subagents for the test run and for failure investigation. Do not implement deferred items. Do not reopen BF-001–BF-007 or FP-001. Do not merge production `main`, publish production Pages, or flash a device.

When checks pass, write the stage-4 successor prompt, push it, and start exactly one owner-visible stage-4 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
