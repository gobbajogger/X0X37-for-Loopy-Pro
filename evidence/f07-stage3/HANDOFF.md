# Workflow handoff to stage 4

Receiving stage: **4 Validate** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f07-fp-pin-stage3-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-3 owner: `bc-43698d6f-ea60-5e58-9d62-84179d683504`.
Audited / validated source SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Content tree: `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`.
Git tree: `6b6401f5c57ed4142bce1adbc3ceab2f99a80b6e`.
Tested implement tip: `4f5321e551bde642605af652e0a9c59d20c72469` plus this stage-3 evidence tip on `cursor/f07-fp-pin-stage3-fb90`.
FP-001 repair commit: `6df0f81f` (Reconcile FP-001 installer source vs served HTML digests).
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003).
Test evidence: `evidence/f07-stage3/CHECKS.md`.
Implement evidence: `evidence/f07-stage2/CHECKS.md`, `evidence/f07-stage2/DEFERRED.md`, `evidence/f07-stage2/digests.json`.
Audit record: `evidence/f07-stage1/AUDIT.md`.
Successor prompt: `evidence/f07-stage3/SUCCESSOR-STAGE4.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 4 id is recorded on this remote tip. Pod has no usable `CURSOR_API_KEY`; coordinator CreateAgent is required.

## Test summary

FP-001 only. Dual-digest host checks ALL_PASS under dedicated test-run subagent `bc-9db661e1-544d-5c67-a5d4-d7e205b580db`: unit tests, live `--fetch` (served HTML `66b56ba5…` matches live page; source fingerprint `8caf8724…` remains unequal), `installer_sha256`-alone conflation rejected (exit 1), quickcheck planner route present, tracked firmware/product paths unchanged vs `1325f5d3`. No failing required check, so no failure-investigation subagent and no return to stage 2.

## Scope for stage 4

Build Elite and Pro development packages (`X0X_DEV_PADCAL=1`) from the tested candidate on `cursor/f07-fp-pin-stage3-fb90`. Record exact package and controller identities, budgets, checks, and logs. Host PASS is not a package PASS. The FP-001 repair is host dual-digest tooling / evidence only (`tools/check_installer_digests.py`, host tests, docs, `digests.json`); firmware / controller / package bytes should match the audited tip unless a package gate proves otherwise. Do not implement deferred items (withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, delete Felucca-era `make_site.py` entirely). Do not reopen BF-001, BF-002, or BF-003. Do not merge production `main`, publish production Pages, or flash a device.

When the affected development packages are DEV CHECKED, write the stage-5 successor prompt, push it, and start exactly one owner-visible stage-5 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
