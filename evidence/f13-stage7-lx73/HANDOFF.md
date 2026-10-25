# Workflow handoff to stage 1 (F15)

Receiving stage: **1 Audit** (new cloud thread; next cycle after F13 LX-73 publish).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f13-lx73-stage7-237f`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-7 owner: `bc-981370f0-380d-5980-8f03-f70bac9128d6`.
Validated **source** SHA (published / next audit): `092589b84fd455355216cb0de93c4782236f7524`.
Content tree: `25ada1f02e2e289072fab8db464b715e09f9c0712bded561af008bc88d8c54af`.
Git source_tree: `9382fb1276e320d9f9d290dbad187fa08cdd224c`.
Firmware tree: `a5ba08046700be5cdf2751b7afc1197411b24121`.
Live: **LIVE_PAGE_MATCH** `nightly-092589b8-dev` (Pages tip `2fd5541f…`).
Elite page sha256: `3d107710cde229d7faf3e007ae14e324d7ea5ce209a802642dacf40608b2895e`.
Pro page sha256: `60dc7fda13fdaa3ee456736588e18b2d9e7e50cc34a800504bde678e14ba12d3`.
Blank Elite/Pro: `9c1168cf…` / `3a29723e…`.
Label: `DEV / HARDWARE TEST — DEVICE NOT TESTED`; `hardware_tested` false.
Evidence: `evidence/f13-stage7-lx73/CHECKS.md`, `live-verify.log`, `independent-qa-local-review.md`, `safety-gates-note.md`.
Successor prompt: `evidence/f13-stage7-lx73/SUCCESSOR-STAGE1.txt`.
Successor agent id: none.
Successor URL: none.
Successor source: pending coordinator CreateAgent (pod has no `CURSOR_API_KEY`).
Handoff status: **incomplete** until an owner-visible Stage 1 id is recorded on this remote tip.

## Scope for stage 1

Audit validated source SHA `092589b8…` against the published nightly page hashes. Select the next defect per workflow (oldest deferred not fixed in that SHA, then failing check — including supporting f03 FakeSMK `SlotError` noted in Stage-7 safety log — then oldest open finding). Do not reopen BF-012 or LX-73 package work. Do not flash. Leave production unchanged.
