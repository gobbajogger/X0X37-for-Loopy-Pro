# Workflow handoff to stage 3

Receiving stage: **3 Test** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f04-bf001-stage2-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-2 owner: `bc-d910ead2-0c33-5313-adea-956b263a6fae`.
Audited / validated source SHA: `a03cfab3a423ab5c65a6546afd4993fe51754ee7`.
Content tree: `1755a2dd6d8b79e7405ab58919cc4d60a69ca11a4ae6601030b0f5de963f170e`.
Git tree: `85e159c5365b14c48e9f3df7d2089e8f2eb560d4`.
Repair commit already in tip: `4f18983a` (F03 — not re-opened).
Implement evidence: `evidence/f04-stage2/CHECKS.md`, `evidence/f04-stage2/DEFERRED.md`.
Audit record: `evidence/f04-stage1/AUDIT.md`.
Successor prompt: `evidence/f04-stage2/SUCCESSOR-STAGE3.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 3 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY`; coordinator CreateAgent is required.

## Repair summary

BF-001 only. Local `.fwsc` (and `ctl.img`) selections increment a generation before `arrayBuffer`; clear stock / uncheck+disable confirm / disable Install on new selection; after read and digest apply state only if the generation still matches (newest wins; stale completions no-op). Host regression: `web/test_install_local_file.mjs` (no real MIDI, no device write). Reimplemented on this tip — draft PR #31 was not stack-merged.

## Scope for stage 3

Run the BF-001 regression and the affected installer checks. Separate subagents for the test run and for failure investigation. Do not implement deferred items (BF-002, BF-003 needing retire-vs-restore, withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets). Do not merge production `main`, publish production Pages, or flash a device.

When checks pass, write the stage-4 successor prompt, push it, and start exactly one owner-visible stage-4 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
