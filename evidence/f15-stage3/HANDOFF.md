# Workflow handoff to stage 4

Receiving stage: **4 Validate** (new cloud thread; F15 after stage-3 BF-034 test).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f15-bf034-slotprobe-stage3-237f`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-3 owner: `bc-dfce289d-9e57-50f6-9219-39a84a11217e`.
Stage-2 implement owner (do not resume): `bc-c8e90cb7-9be8-5cea-b2d1-4ef65d9c75df`.
Stage-1 audit owner (do not resume): `bc-6c51c5fd-d995-54b7-baf7-b5033919009d`.
Audited / validated source SHA: `092589b84fd455355216cb0de93c4782236f7524`.
Content tree: `25ada1f02e2e289072fab8db464b715e09f9c0712bded561af008bc88d8c54af`.
Git source_tree: `9382fb1276e320d9f9d290dbad187fa08cdd224c`.
Firmware tree: `a5ba08046700be5cdf2751b7afc1197411b24121`.
Tested implement tip: `0f7f03b52e223e3a1f7ae3a21d46dc0ca62fefa4` plus this stage-3 evidence tip on `cursor/f15-bf034-slotprobe-stage3-237f`.
BF-034 repair commit: `7fb4b71d` — FakeSMK mute/empty SMP_INFO harness + F03 assertion alignment (`tests/install_test.py`, `tests/install_f03_slot_probe_test.py` only).
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007), `de11b6d7` (BF-008), BF-012/#308/092589b8, LX-73 package publish, BF-013 ancestry.
Test evidence: `evidence/f15-stage3/CHECKS.md`, `test-run-RESULT.md`, host logs.
Implement evidence: `evidence/f15-stage2/CHECKS.md`, `DEFERRED.md`.
Audit record: `evidence/f15-stage1/AUDIT.md`, `bf034-definition.md`.
Successor prompt: `evidence/f15-stage3/SUCCESSOR-STAGE4.txt`.
Successor agent id: none.
Successor URL: none.
Successor source: pending coordinator CreateAgent (pod has no `CURSOR_API_KEY`).
Handoff status: **incomplete** until an owner-visible Stage 4 id is recorded on this remote tip.

## Test summary

BF-034 only. Host regressions **ALL_PASS** under dedicated test-run subagent `bc-32baa265-d030-5c35-96a0-a96c0826c941` with `X0X_DEV_PADCAL=1` `X0X_TEST_JOBS=8`:
- `python3 tests/install_f03_slot_probe_test.py` exit 0
- `python3 tests/install_test.py` exit 0 (missing/malformed SMP_INFO fail-closed + full suite)
- `python3 tests/x0x_slots_info_preflight_test.py` exit 0 (31/31)
- `python3 tests/install_slots_test.py` exit 0
FakeSMK mute/`slot_info=None` → no SMP_INFO; default empty `slot_info` → successful empty parse. Production `fm1_install.py` / `x0x_slots.py` unchanged. No failing required check → no failure-investigation subagent → no return to stage 2.

## Scope for stage 4

Build Elite and Pro development packages (`X0X_DEV_PADCAL=1`) from the tested candidate on `cursor/f15-bf034-slotprobe-stage3-237f`. Record exact package and controller identities, budgets, checks, and logs. Host PASS is not a package PASS. Do not implement deferred items. Do not reopen BF-001–BF-013 or FP-001. Do not lengthen SLOT_PROBE budgets. Do not steal Arp/Fx leftovers. Do not merge production `main`, publish production Pages, or flash a device from this stage.

When the affected development packages are DEV CHECKED, write the stage-5 successor prompt, push it, and start exactly one owner-visible stage-5 Cloud Agent under the same handoff rule (or ask coordinator CreateAgent if no API key). An internal Task subagent does not complete the handoff.
