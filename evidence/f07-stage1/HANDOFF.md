# Workflow handoff to stage 2

Receiving stage: **2 Implement** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f07-fp-pin-stage1-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-1 owner: `bc-b1215ac4-b3bd-5a72-a25e-e97bb3d629dd`.
Audited / validated source SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Content tree: `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`.
Git tree: `6b6401f5c57ed4142bce1adbc3ceab2f99a80b6e`.
Repair commits already in tip: `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003) — not re-opened.
Audit record: `evidence/f07-stage1/AUDIT.md`.
Checks: `evidence/f07-stage1/CHECKS.md`.
Prior publish evidence: `evidence/f06-stage7/CHECKS.md`, `HANDOFF.md`, `live-verify.log`, `independent-qa-local-review.md`.
Independent claim review: internal Task `bc-4fb732c3-0749-525e-91d0-f846466c1b04` (13/13 PASS).
Stage-7 local review: Project store `internal/pr-local-reviews/pr-215.md` (PASS).
Deferred younger / excluded items: withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, delete Felucca-era `make_site.py` entirely (`evidence/f06-stage2/DEFERRED.md`).
Successor prompt: `evidence/f07-stage1/SUCCESSOR-STAGE2.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 2 id is recorded on this remote tip. Pod has no usable `CURSOR_API_KEY` (API create ×2 → 401; logs `api-create-stage2-attempt{1,2}.json`); coordinator CreateAgent is required.

Selected defect: **FP-001**. Stage-7 qualification `installer_sha256` (`8caf8724…`, nightly `installerFingerprint()` source composite) does not equal served `install/index.html` sha256 (`66b56ba5…`). **Approved repair: dual-digest reconciliation** — record source fingerprint and served HTML digest separately; add a local host check that prevents conflating them; keep prepare:candidate source-fingerprint semantics. Do not change package/controller/firmware bytes.

## Scope

Stage 2 only. Apply the approved FP-001 fix. Do not implement other deferred items. Do not merge production `main`, publish production Pages, or flash a device. Do not change firmware, controller, or package bytes. Do not reopen BF-001–BF-003.

When checked commits are on the remote, write the stage-3 successor prompt, push it, and start exactly one owner-visible stage-3 Cloud Agent under the same handoff rule (coordinator CreateAgent if no API key). An internal Task subagent does not complete the handoff.
