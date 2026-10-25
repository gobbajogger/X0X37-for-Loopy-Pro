# Workflow handoff to stage 4

Receiving stage: **4 Validate** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f03-slot-probe-stage3-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-3 owner: `bc-21ba0310-0c10-5552-a8e2-35032c15b732`.
Audited / validated source SHA: `4ee8f37714b0892f2c70d157fda9328da393e987`.
Content tree: `7ad3a50da8abac890215dae3c28197a2fa90041b8615e828b6b3c6e18060fb83`.
Git tree: `c63af7972f3ca6e5d4e8ac3aac5e70abc8178265`.
Tested implement tip: `a03cfab3a423ab5c65a6546afd4993fe51754ee7` plus this stage-3 evidence tip on `cursor/f03-slot-probe-stage3-fb90`.
Repair commit: `4f18983a`.
Test evidence: `evidence/f03-stage3/CHECKS.md`.
Deferred (unchanged): `evidence/f03-stage2/DEFERRED.md`.
Successor prompt: `evidence/f03-stage3/SUCCESSOR-STAGE4.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 4 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY`; coordinator CreateAgent is required.

## Test summary

F03 only. Required slot-probe fail-closed regressions and affected installer checks all PASS under a dedicated test-run subagent (`bc-833de9db-27d0-5ba5-88a9-f5a30914f4ae`). No failing required check, so no failure-investigation subagent and no return to stage 2.

## Scope for stage 4

Build Elite and Pro development packages (`X0X_DEV_PADCAL=1`) from the tested candidate on `cursor/f03-slot-probe-stage3-fb90`. Record exact package and controller identities, budgets, checks, and logs. Host PASS is not a package PASS. Do not implement deferred items. Do not merge production `main`, publish production Pages, or flash a device.

When the affected development packages are DEV CHECKED, write the stage-5 successor prompt, push it, and start exactly one owner-visible stage-5 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
