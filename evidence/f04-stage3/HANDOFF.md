# Workflow handoff to stage 4

Receiving stage: **4 Validate** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f04-bf001-stage3-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-3 owner: `bc-ead74ca6-9020-58cf-9542-8b8091a1c878`.
Audited / validated source SHA: `a03cfab3a423ab5c65a6546afd4993fe51754ee7`.
Content tree: `1755a2dd6d8b79e7405ab58919cc4d60a69ca11a4ae6601030b0f5de963f170e`.
Git tree: `85e159c5365b14c48e9f3df7d2089e8f2eb560d4`.
Tested implement tip: `0d79d63485dc6585800a71ef716d095445f5a1fc` plus this stage-3 evidence tip on `cursor/f04-bf001-stage3-fb90`.
BF-001 repair commit: `d11b74fc`.
Repair commit already in tip (F03; not re-opened): `4f18983a`.
Test evidence: `evidence/f04-stage3/CHECKS.md`.
Deferred (unchanged): `evidence/f04-stage2/DEFERRED.md`.
Successor prompt: `evidence/f04-stage3/SUCCESSOR-STAGE4.txt`.
Successor agent id: `bc-5cbf81ce-9d35-5c9f-8919-36e1f7b2a870`.
Successor URL: `https://cursor.com/agents/bc-5cbf81ce-9d35-5c9f-8919-36e1f7b2a870`.
Handoff status: **complete**. Owner-visible Stage 4 created by the Project coordinator (pod had no `CURSOR_API_KEY`). Base: `cursor/f04-bf001-stage3-fb90` @ `e9ff9cb7766316307060d323acf74e4c0bdd153c`. Source: desktop.

## Test summary

BF-001 only. Required local `.fwsc` / `ctl.img` selection-generation regressions and affected installer checks all PASS under a dedicated test-run subagent (`bc-ef042592-7de9-5b6d-8f3d-848569f55437`). No failing required check, so no failure-investigation subagent and no return to stage 2.

## Scope for stage 4

Build Elite and Pro development packages (`X0X_DEV_PADCAL=1`) from the tested candidate on `cursor/f04-bf001-stage3-fb90`. Record exact package and controller identities, budgets, checks, and logs. Host PASS is not a package PASS. Do not implement deferred items. Do not merge production `main`, publish production Pages, or flash a device.

When the affected development packages are DEV CHECKED, write the stage-5 successor prompt, push it, and start exactly one owner-visible stage-5 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
