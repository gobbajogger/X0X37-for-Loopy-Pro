# Workflow handoff to stage 4

Receiving stage: **4 Validate** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f06-bf003-stage3-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-3 owner: `bc-fd451c4d-a054-5318-bad5-ee55cb5dbabe`.
Audited / validated source SHA: `ab90f47ccad0f213114d6a98c1bc6179be2ebb35`.
Content tree: `0ee03318a0b64a1e535b36fad284558529ff7e3f95ff069efed86bbcf308518c`.
Git tree: `50c381c528cae9692deff4a712071f7dc6acb020`.
Tested implement tip: `212b7d5c58752e275f753347c23f932f9d44a9a6` plus this stage-3 evidence tip on `cursor/f06-bf003-stage3-fb90`.
BF-003 repair commit: `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03).
Test evidence: `evidence/f06-stage3/CHECKS.md`.
Implement evidence: `evidence/f06-stage2/CHECKS.md`, `evidence/f06-stage2/DEFERRED.md`.
Audit record: `evidence/f06-stage1/AUDIT.md`.
Successor prompt: `evidence/f06-stage3/SUCCESSOR-STAGE4.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 4 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY`; coordinator CreateAgent is required.

## Test summary

BF-003 only. Required `node web/test_web.mjs`, `web/make_site.py` dry run (synthetic FM-1_900; no editor.html copy), and absence of `web/editor.html` / `firmware/src/editor.c` all PASS under a dedicated test-run subagent (`bc-805b42a9-5c75-5437-b1a6-bb36920433fb`). Editor-dependent harness sections were removed, not skipped. No failing required check, so no failure-investigation subagent and no return to stage 2.

## Scope for stage 4

Build Elite and Pro development packages (`X0X_DEV_PADCAL=1`) from the tested candidate on `cursor/f06-bf003-stage3-fb90`. Record exact package and controller identities, budgets, checks, and logs. Host PASS is not a package PASS. The BF-003 repair is web harness / site-builder only (`web/test_web.mjs`, `web/make_site.py`, comment updates); firmware / controller / package bytes should match the audited tip unless a package gate proves otherwise. Do not implement deferred items (withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, Stage-7 installer FP pin reconciliation). Do not restore the editor. Do not reopen BF-001 or BF-002. Do not merge production `main`, publish production Pages, or flash a device.

When the affected development packages are DEV CHECKED, write the stage-5 successor prompt, push it, and start exactly one owner-visible stage-5 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
