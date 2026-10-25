# Workflow handoff (Stage 7 — page not live yet)

Receiving stage: **7 Publish nightly retry** if the live page does not update; **1 Audit** only after LIVE_PAGE_MATCH.
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f09-bf005-stage7-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-7 owner: `bc-bb6e5cd5-b939-53c7-a83d-6b012405e45c`.
Validated **source** SHA (publish / next audit): `d0d00da23c3ce2268f06f31f8c062716d5a37ad0`.
Content tree: `62339630bc6e133da464eed7924645c0bfc09e4de66a8bccafdc60fd2d660c83`.
Git source_tree: `e06299abf0f35f24c771d011d07c2e74de613369`.
Firmware tree: `32a36f68d1ce7a3c304b447475afe7e2bbb116cf`.
BF-005 repair commit: `94f4a012`.
Live: **PENDING** (prepared candidate; cloud gh-pages 403). Parent tip `e296e883…`.
Elite page sha256 (bundled): `9cba1f3d0bebc2113d59b9bbb3f1ca1a67bb396183be279b74486319ddae8e11`.
Pro page sha256 (bundled): `5dda4eecd74d96237a3d457a026d7bc04eb3ed570705179ab8e6c15a198fa739`.
Blank Elite/Pro: `de0bb077…` / `25bebb26…`.
Label: `DEV / HARDWARE TEST — DEVICE NOT TESTED`; `hardware_tested` false.
Evidence: `evidence/f09-stage7/CHECKS.md`, `prepare-candidate.log`, `independent-qa-local-review.md`.
Pages prompt: Project store `internal/f09-stage7-mac-pages-push.md`.
Successor prompt: none yet (do not start Stage 1 until LIVE_PAGE_MATCH; on permanent page failure write SUCCESSOR-STAGE7.txt retry).
Handoff status: **incomplete** — live page not updated; Stage 7 retains ownership.

## Scope

Publish/serve `nightly-d0d00da2-dev` with matching hashes. Do not wait for stage 6. Do not flash. Leave production unchanged.
