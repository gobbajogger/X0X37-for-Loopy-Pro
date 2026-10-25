# Workflow handoff to stage 2

Receiving stage: **2 Implement** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f04-bf001-stage1-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-1 owner: `bc-b24569bd-4f94-5a38-abb3-ed5ce76d4012`.
Audited / validated source SHA: `a03cfab3a423ab5c65a6546afd4993fe51754ee7`.
Content tree: `1755a2dd6d8b79e7405ab58919cc4d60a69ca11a4ae6601030b0f5de963f170e`.
Git tree: `85e159c5365b14c48e9f3df7d2089e8f2eb560d4`.
Repair commit already in tip: `4f18983a` (F03 — not re-opened).
Audit record: `evidence/f04-stage1/AUDIT.md`.
Checks: `evidence/f04-stage1/CHECKS.md`.
Prior publish evidence: `evidence/f03-stage7/CHECKS.md`, `HANDOFF.md`, `live-verify.log`.
Deferred younger items: BF-002, BF-003, withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets (`evidence/f03-stage2/DEFERRED.md`).
Successor prompt: `evidence/f04-stage1/SUCCESSOR-STAGE2.txt`.
Successor agent id: `bc-d910ead2-0c33-5313-adea-956b263a6fae`.
Successor URL: https://cursor.com/agents/bc-d910ead2-0c33-5313-adea-956b263a6fae
Handoff status: **complete** (owner-visible desktop Stage 2 recorded by Project coordinator CreateAgent; base `cursor/f04-bf001-stage1-fb90`).

Selected defect: **BF-001**. Local `.fwsc` (and related) file selections can finish out of order so an older package replaces the most recently chosen one. Approved repair: selection generation token before `arrayBuffer`; clear stock / require fresh confirmation; ignore stale completions; preferably harden `ctlfile` the same way. Reimplement on this tip — do not stack-merge draft PR #31.

## Scope

Stage 2 only. Apply the approved BF-001 fix in `web/x0x_installer.html` with a host race regression. Do not implement BF-002–BF-003 or other deferred items. Do not merge production `main`, publish production Pages, or flash a device. Do not change firmware, controller, or package bytes.

When checked commits are on the remote, write the stage-3 successor prompt, push it, and start exactly one owner-visible stage-3 Cloud Agent under the same handoff rule (coordinator CreateAgent if no API key). An internal Task subagent does not complete the handoff.
