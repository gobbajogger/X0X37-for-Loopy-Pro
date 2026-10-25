# Workflow handoff to stage 1

Receiving stage: **1 Audit** (new cloud thread; next cycle after F11 publish).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f11-bf007-stage7-179d`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-7 owner: `bc-a7d005ad-7a34-54fb-bd61-45fd0eb0179d`.
Validated **source** SHA (published / next audit): `4f96b0d9f5f11f700ea3fc935800b8714e11f35d`.
Content tree: `f27c89e1ac8e11ab57c5ae7e49145ce19b964082bf3ca9a700a99bfa8a97091e`.
Git source_tree: `7b35932d5f0adbceeb35111f2136855aa54ebc9b`.
Firmware tree: `5b1a21f07563b0262ece6728051cb0436bd4bfa1`.
BF-007 repair commit: `78a0452e`.
Live: **LIVE_PAGE_MATCH** `nightly-4f96b0d9-dev` (Pages tip `48ffd27e…`).
Elite page sha256: `4841110a274e2212d56abf9234cf0d921b45d915d1ff25fad0727e52edb7d18a`.
Pro page sha256: `b35d587557d9f1d4c72b2c81bc944a1dd129bee625e98ae35f06fe7f653fc720`.
Blank Elite/Pro: `829bb6da…` / `d2186cb0…`.
Label: `DEV / HARDWARE TEST — DEVICE NOT TESTED`; `hardware_tested` false.
Evidence: `evidence/f11-stage7/CHECKS.md`, `live-verify.log`, `independent-qa-local-review.md`, `safety-gates-note.md`.
Cloud pages worker: `bc-1b875c0d-27fd-5a1e-91ec-eb99da1303c9`.
Successor prompt: `evidence/f11-stage7/SUCCESSOR-STAGE1.txt`.
Successor agent id: none.
Successor URL: none.
Successor source: pending coordinator CreateAgent (pod has no `CURSOR_API_KEY`).
Handoff status: **incomplete** until an owner-visible Stage 1 id is recorded on this remote tip.

## Scope for stage 1

Audit validated source SHA `4f96b0d9…` against the published nightly page hashes. Select the next defect per workflow (oldest deferred not fixed in that SHA, then failing check, then oldest open finding). Do not reopen BF-001–BF-007 or FP-001. Do not flash. Leave production unchanged. Stage 6 flash remains Mac LaunchAgent after DEV OK — owner advance-approved.
