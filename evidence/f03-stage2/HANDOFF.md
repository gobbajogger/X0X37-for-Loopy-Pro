# Workflow handoff to stage 3

Receiving stage: **3 Test** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f03-slot-probe-stage2-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-2 owner: `bc-94c505d8-1ca0-5d88-b61f-049933c106b9`.
Audited / validated source SHA: `4ee8f37714b0892f2c70d157fda9328da393e987`.
Content tree: `7ad3a50da8abac890215dae3c28197a2fa90041b8615e828b6b3c6e18060fb83`.
Git tree: `c63af7972f3ca6e5d4e8ac3aac5e70abc8178265`.
Implement evidence: `evidence/f03-stage2/CHECKS.md`, `evidence/f03-stage2/DEFERRED.md`.
Audit record: `evidence/f03-stage1/AUDIT.md`.
Successor prompt: `evidence/f03-stage2/SUCCESSOR-STAGE3.txt`.
Successor agent id: `bc-21ba0310-0c10-5552-a8e2-35032c15b732`.
Successor URL: https://cursor.com/agents/bc-21ba0310-0c10-5552-a8e2-35032c15b732
Successor source: `desktop` (coordinator CreateAgent). Name: `F03 stage3 tests`. `startingRef`: `cursor/f03-slot-probe-stage2-fb90`.
Handoff status: **complete**. Owner-visible Stage 3 is recorded on this remote tip. Stage 2 ownership ends.

## Repair summary

F03 only. On the same-keyboard slot-preserving install path, a failed sample-slot layout probe (timeout / SlotError / parse) fails closed before any write. `--no-slots` remains the explicit skip. Successful empty / no-risk parses after a successful layout read still skip backup. Native: `tools/fm1_install.py` `slot_probe` / `plan_slots` / `run`. Web: `web/x0x_breaks.js` `installKeepingSlots`. Regressions: `tests/install_f03_slot_probe_test.py`, `web/test_f03_slot_probe.mjs`.

## Scope for stage 3

Run the F03 regressions and the affected installer checks. Separate subagents for the test run and for failure investigation. Do not implement deferred items. Do not merge production `main`, publish production Pages, or flash a device.

When checks pass, write the stage-4 successor prompt, push it, and start exactly one owner-visible stage-4 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
