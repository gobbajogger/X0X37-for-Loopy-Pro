# Workflow handoff to stage 1

Receiving stage: **1 Audit** (new cloud thread; next cycle after F09 publish).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f09-bf005-stage7-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-7 owner: `bc-bb6e5cd5-b939-53c7-a83d-6b012405e45c`.
Validated **source** SHA (published / next audit): `d0d00da23c3ce2268f06f31f8c062716d5a37ad0`.
Content tree: `62339630bc6e133da464eed7924645c0bfc09e4de66a8bccafdc60fd2d660c83`.
Git source_tree: `e06299abf0f35f24c771d011d07c2e74de613369`.
Firmware tree: `32a36f68d1ce7a3c304b447475afe7e2bbb116cf`.
BF-005 repair commit: `94f4a012`.
Live: **LIVE_PAGE_MATCH** `nightly-d0d00da2-dev` (Pages tip `4f468d16…`).
Elite page sha256: `9cba1f3d0bebc2113d59b9bbb3f1ca1a67bb396183be279b74486319ddae8e11`.
Pro page sha256: `5dda4eecd74d96237a3d457a026d7bc04eb3ed570705179ab8e6c15a198fa739`.
Blank Elite/Pro: `de0bb077…` / `25bebb26…`.
Label: `DEV / HARDWARE TEST — DEVICE NOT TESTED`; `hardware_tested` false.
Evidence: `evidence/f09-stage7/CHECKS.md`, `live-verify.log`, `independent-qa-local-review.md`.
Cloud pages worker: `bc-e4d74847-d6da-53e4-b413-96cb0e585799`.
Successor prompt: `evidence/f09-stage7/SUCCESSOR-STAGE1.txt`.
Successor agent id: `bc-6b7eb25f-60be-5d9e-8fa7-d8b9111b5e63` (coordinator CreateAgent; pod had `NO_CURSOR_API_KEY`).
Successor URL: https://cursor.com/agents/bc-6b7eb25f-60be-5d9e-8fa7-d8b9111b5e63
Successor source: `desktop` (owner-visible; visibility `user`; name `F10 stage1 audit`).
Handoff status: **complete** — owner-visible Stage 1 id is on this remote tip. Stage 7 ownership ends.

## Scope for stage 1

Audit validated source SHA `d0d00da2…` against the published nightly page hashes. Select the next defect per workflow (oldest deferred not fixed in that SHA, then failing check, then oldest open finding). Do not reopen BF-001–BF-005 or FP-001. Do not flash. Leave production unchanged.
