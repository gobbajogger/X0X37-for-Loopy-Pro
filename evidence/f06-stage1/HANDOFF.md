# Workflow handoff to stage 2

Receiving stage: **2 Implement** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f06-bf003-stage1-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-1 owner: `bc-a76a9628-9a50-5b04-a7a4-ba134f41d9e1`.
Audited / validated source SHA: `ab90f47ccad0f213114d6a98c1bc6179be2ebb35`.
Content tree: `0ee03318a0b64a1e535b36fad284558529ff7e3f95ff069efed86bbcf308518c`.
Git tree: `50c381c528cae9692deff4a712071f7dc6acb020`.
Repair commits already in tip: `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03) — not re-opened.
Audit record: `evidence/f06-stage1/AUDIT.md`.
Checks: `evidence/f06-stage1/CHECKS.md`.
Prior publish evidence: `evidence/f05-stage7/CHECKS.md`, `HANDOFF.md`, `live-verify.log`, `independent-qa-local-review.md`.
Deferred younger items: withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, Stage-7 installer FP pin mismatch (`evidence/f05-stage2/DEFERRED.md`).
Successor prompt: `evidence/f06-stage1/SUCCESSOR-STAGE2.txt`.
Successor agent id: `bc-764bccc6-54f9-58a3-bf70-6f95773c3460` (F06 BF-003 implement; desktop; startingRef `cursor/f06-bf003-stage1-fb90`).
Successor URL: https://cursor.com/agents/bc-764bccc6-54f9-58a3-bf70-6f95773c3460
Handoff status: **complete**. Owner-visible Stage 2 id is on this remote tip. Stage 1 stops here; does not start Stage 3.

Selected defect: **BF-003**. Retained editor test / site-builder references missing `web/editor.html`. **Approved repair: retire** (do not restore). Strip editor-dependent paths in `web/test_web.mjs` and `web/make_site.py`; keep useful non-editor `fm1pkg`/`fm1ota` checks if they still apply; run local host/node checks. Decision closed by this audit from intentional X0X deletion `23775375` and unregistered suite status.

## Scope

Stage 2 only. Apply the approved BF-003 retire fix. Do not restore `editor.html` / `editor.c`. Do not implement other deferred items. Do not merge production `main`, publish production Pages, or flash a device. Do not change firmware, controller, or package bytes.

When checked commits are on the remote, write the stage-3 successor prompt, push it, and start exactly one owner-visible stage-3 Cloud Agent under the same handoff rule (coordinator CreateAgent if no API key). An internal Task subagent does not complete the handoff.
