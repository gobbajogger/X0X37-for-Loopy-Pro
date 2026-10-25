# Workflow handoff to stage 4

Receiving stage: **4 Validate** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f05-bf002-stage3-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-3 owner: `bc-267d1e71-e911-5c7d-aecc-36f285bdf065`.
Audited / validated source SHA: `0d79d63485dc6585800a71ef716d095445f5a1fc`.
Content tree: `1b067cadd1e7887415354c4f384a51f93d6437c367574e2cee52b0b44d829479`.
Git tree: `603e2a2b1442a8214cd97b03d32bf57fc158feac`.
Tested implement tip: `19e8f6656743aa2d0cb7302715d87ff4d596070d` plus this stage-3 evidence tip on `cursor/f05-bf002-stage3-fb90`.
BF-002 repair commit: `ab90f47c`.
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `4f18983a` (F03).
Test evidence: `evidence/f05-stage3/CHECKS.md`.
Implement evidence: `evidence/f05-stage2/CHECKS.md`, `evidence/f05-stage2/DEFERRED.md`.
Audit record: `evidence/f05-stage1/AUDIT.md`.
Successor prompt: `evidence/f05-stage3/SUCCESSOR-STAGE4.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 4 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY`; coordinator CreateAgent is required.

## Test summary

BF-002 only. Required planner host tests, `projects-web` / `projects-page` suites, and offline `select()` probe all PASS under a dedicated test-run subagent (`bc-f6f403e7-14e6-5641-be29-050849d92e6d`). No failing required check, so no failure-investigation subagent and no return to stage 2.

## Scope for stage 4

Build Elite and Pro development packages (`X0X_DEV_PADCAL=1`) from the tested candidate on `cursor/f05-bf002-stage3-fb90`. Record exact package and controller identities, budgets, checks, and logs. Host PASS is not a package PASS. The BF-002 repair is planner/host only (`tools/quickcheck_plan.py` + host fixture); firmware / controller / package bytes should match the audited tip unless a package gate proves otherwise. Do not implement deferred items (BF-003 needing retire-vs-restore, withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets). Do not merge production `main`, publish production Pages, or flash a device.

When the affected development packages are DEV CHECKED, write the stage-5 successor prompt, push it, and start exactly one owner-visible stage-5 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
