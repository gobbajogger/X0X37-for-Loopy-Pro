# Workflow handoff to stage 1

Receiving stage: **1 Audit** (new cloud thread; next cycle after F10 publish).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f10-bf006-stage7-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-7 owner: `bc-c926071e-0ab5-513d-bff4-01cca78cb685`.
Validated **source** SHA (published / next audit): `af4947846df7233c68a6be72c4d0c634052a2752`.
Content tree: `1da3bbfbbf0df52256acceecbca2e064a1ef821478a33d78591e01813229643d`.
Git source_tree: `c12a7e747b79aa4619466764ecccc88285d4c10d`.
Firmware tree: `bcc21733ea77097a787cfa6cc617fbdf1dca5da1`.
BF-006 repair commit: `37bab7d1`.
Live: pending LIVE_PAGE_MATCH.
Elite page sha256: `9cba1f3d0bebc2113d59b9bbb3f1ca1a67bb396183be279b74486319ddae8e11`.
Pro page sha256: `5dda4eecd74d96237a3d457a026d7bc04eb3ed570705179ab8e6c15a198fa739`.
Blank Elite/Pro: `de0bb077…` / `25bebb26…`.
Label: `DEV / HARDWARE TEST — DEVICE NOT TESTED`; `hardware_tested` false.
Evidence: `evidence/f10-stage7/CHECKS.md`, `live-verify.log`, `independent-qa-local-review.md`.
Successor prompt: `evidence/f10-stage7/SUCCESSOR-STAGE1.txt` (written after LIVE_PAGE_MATCH).
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until LIVE_PAGE_MATCH and an owner-visible Stage 1 id is recorded on this remote tip.

## Scope for stage 1

Audit validated source SHA `af494784…` against the published nightly page hashes. Select the next defect per workflow (oldest deferred not fixed in that SHA, then failing check, then oldest open finding). Do not reopen BF-001–BF-006 or FP-001. Do not flash. Leave production unchanged.
