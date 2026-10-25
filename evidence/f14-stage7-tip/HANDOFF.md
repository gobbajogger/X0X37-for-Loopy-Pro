# Workflow handoff to stage 1

Receiving stage: **1 Audit** (new cloud thread; next cycle after F14 tip publish).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f14-tip-stage7-237f`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-7 owner: `bc-361901cd-c08a-5e68-b94a-71213f72f0f9`.
Validated **source** SHA (published / next audit): `b48cd8002a6d4d509010e04b8a8f1cfcbbdee64a`.
Content tree: `4eef581e267c582897df5910196671be2a04ac24a1d0a11acd6086fe583f3e60`.
Git source_tree: `16b3caf29fc3548c290ce97afaed9f44214c1ab5`.
Firmware tree: `1924e2cff82c4f1737fbe7a8188af1a3b229ae01`.
Live: **LIVE_PAGE_MATCH** `nightly-b48cd800-dev` (Pages tip `a338960c…`).
Elite page sha256: `81aaf361cb11717ad0bcee325453e361064506e4d6720781e8eb8ffcc1dadae2`.
Pro page sha256: `e0ac6b68e117ac42eac97944430d1a7ed2cacfba38336676096fa8a09f57319a`.
Blank Elite/Pro: `4dc49bad…` / `3f3b5c00…`.
Label: `DEV / HARDWARE TEST — DEVICE NOT TESTED`; `hardware_tested` false.
Evidence: `evidence/f14-stage7-tip/CHECKS.md`, `live-verify.log`, `independent-qa-local-review.md`, `safety-gates-note.md`.
Successor prompt: `evidence/f14-stage7-tip/SUCCESSOR-STAGE1.txt`.
Successor agent id: none.
Successor URL: none.
Successor source: pending coordinator CreateAgent (pod has no `CURSOR_API_KEY`).
Handoff status: **incomplete** until an owner-visible Stage 1 id is recorded on this remote tip.

## Scope for stage 1

Audit validated source SHA `b48cd800…` against the published nightly page hashes. Select the next defect per workflow (oldest deferred not fixed in that SHA, then failing check — including tip `07773682` RAM BUILD_FAILED and supporting f03 FakeSMK `SlotError` noted in Stage-7 safety log — then oldest open finding). Do not reopen BF-013 tip package work or invent packages for `07773682` without a Stage-2 RAM repair. Do not flash. Leave production unchanged.
