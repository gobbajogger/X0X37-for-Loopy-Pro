# Workflow handoff to stage 3

Receiving stage: **3 Test** (new cloud thread; F15 after stage-2 BF-034 implement).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f15-bf034-slotprobe-stage2-237f`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-2 owner: `bc-c8e90cb7-9be8-5cea-b2d1-4ef65d9c75df`.
Stage-1 audit owner (do not resume): `bc-6c51c5fd-d995-54b7-baf7-b5033919009d`.
Audited / validated source SHA: `092589b84fd455355216cb0de93c4782236f7524`.
Content tree: `25ada1f02e2e289072fab8db464b715e09f9c0712bded561af008bc88d8c54af`.
Git source_tree: `9382fb1276e320d9f9d290dbad187fa08cdd224c`.
Firmware tree: `a5ba08046700be5cdf2751b7afc1197411b24121`.
Repair: **BF-034 only** — FakeSMK mute/empty SMP_INFO harness + F03 assertion alignment.
Implement evidence: `evidence/f15-stage2/CHECKS.md`, `DEFERRED.md`, `f03-slotprobe-green.log`, `install-test-green.log`, `slots-info-preflight-green.log`, `install-slots-green.log`.
Audit record: `evidence/f15-stage1/AUDIT.md`, `bf034-definition.md`, witness logs.
Successor prompt: `evidence/f15-stage2/SUCCESSOR-STAGE3.txt`.
Successor agent id: none.
Successor URL: none.
Successor source: pending coordinator CreateAgent (pod has no `CURSOR_API_KEY`).
Handoff status: **incomplete** until an owner-visible Stage 3 id is recorded on this remote tip.

## Repair summary

FakeSMK no longer answers CMD_INFO with invalid `[0,0]`. Mute (`answer_slots=False`) and `slot_info=None` produce no SMP_INFO reply → fail closed via timeout / SlotError. Default empty `slot_info` is a successful empty parse (no backup). Production `fm1_install.py` / `x0x_slots.py` unchanged. Host: `install_f03_slot_probe_test.py` exit 0; `install_test.py` fail-closed negatives + suite PASS. Deferred items in `DEFERRED.md`.

## Scope for stage 3

Test the BF-034 harness repair on the implement tip. Separate subagents for the test run and for failure investigation. Do not implement deferred items. Do not reopen BF-001–BF-013 or FP-001. Do not lengthen SLOT_PROBE budgets. Do not steal Arp/Fx leftovers. Do not merge production `main`, publish production Pages, or flash a device.

When checks pass, write the stage-4 successor prompt, push it, and start exactly one owner-visible stage-4 Cloud Agent under the same handoff rule (or ask coordinator CreateAgent if no API key). An internal Task subagent does not complete the handoff.
