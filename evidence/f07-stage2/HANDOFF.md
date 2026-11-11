# Workflow handoff to stage 3

Receiving stage: **3 Test** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f07-fp-pin-stage2-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-2 owner: `bc-d180cf42-3a0d-5bf5-b694-aa94e5cf04ef`.
Audited / validated source SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Content tree: `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`.
Git tree: `6b6401f5c57ed4142bce1adbc3ceab2f99a80b6e`.
Repair commits already in tip: `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003) — not re-opened.
FP-001 implement commit: see branch tip (`Reconcile FP-001 installer source vs served HTML digests`).
Implement evidence: `evidence/f07-stage2/CHECKS.md`, `evidence/f07-stage2/DEFERRED.md`, `evidence/f07-stage2/digests.json`.
Audit record: `evidence/f07-stage1/AUDIT.md`.
Successor prompt: `evidence/f07-stage2/SUCCESSOR-STAGE3.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 3 id is recorded on this remote tip. Pod has no usable `CURSOR_API_KEY`; coordinator CreateAgent is required.

## Repair summary

FP-001 only. Dual-digest reconciliation: evidence records `installer_source_fingerprint` (`8caf8724…`, nightly `installerFingerprint()` / `qualification.installer_sha256`) separately from `served_install_html_sha256` (`66b56ba5…`). Added `tools/check_installer_digests.py` + host unit tests; live `--fetch` PASS; conflation of a single Installer FP rejected. Nightly prepare:candidate source-fingerprint semantics unchanged. No firmware / controller / package bytes changed. Deferred items in `evidence/f07-stage2/DEFERRED.md`.

## Scope for stage 3

Run the FP-001 host checks on the implement tip. Separate subagents for the test run and for failure investigation. Do not implement deferred items. Do not reopen BF-001–BF-003. Do not merge production `main`, publish production Pages, or flash a device. Do not change firmware, controller, or package bytes.

When checks pass, write the stage-4 successor prompt, push it, and start exactly one owner-visible stage-4 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
