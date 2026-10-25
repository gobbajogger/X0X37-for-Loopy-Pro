# Workflow handoff to stage 7

Receiving stage: **7 Publish nightly** (new cloud thread). Stage 6 is a local install prompt only (no cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f08-bf004-stage5-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-5 owner: `bc-1940db7e-48d9-5cd0-9460-96008b16579d`.
Audited / validated source SHA (pre-BF-004): `6df0f81fbbe0adc3207f67d912d2fe0ab676be04`.
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Validated **source** SHA (publish / next audit / install commit): `8a74ff52cba842c3f2d8365600286f94e6bccdf0`.
Content tree: `1a1e7a22d136fef6e30702c9d3207d22798a6dd382717acc5ab0f40d2f235acf`.
Firmware tree: `581d1cb213e6ceda3e9fbbcdc324ef8dce67c42f`.
Audited firmware content tree (pre-BF-004 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`.
Evidence-bearing tip: this stage-5 branch tip (records/logs/handoff only). Do **not** treat it as the install or nightly FF target.
BF-004 repair commit: `ce690772`.
Package validation commit: `8a74ff52`.
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001).
Package evidence: `evidence/f08-stage4/CHECKS.md`, `elite/record.json`, `pro/record.json`.
Merge evidence: `evidence/f08-stage5/CHECKS.md`.
Host test evidence: `evidence/f08-stage3/CHECKS.md` (ALL_PASS).
Deferred (unchanged): `evidence/f08-stage2/DEFERRED.md`.
Stage-6 local prompt: `evidence/f08-stage5/SUCCESSOR-STAGE6.txt` (also `stage6-install-prompt.json`).
Successor prompt: `evidence/f08-stage5/SUCCESSOR-STAGE7.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 7 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY`; CreateAgent attempts 1–2 recorded as `api-create-stage7-attempt{1,2}.json` (`NO_CURSOR_API_KEY`). Coordinator CreateAgent is required. Stage 5 retains ownership.

## Nightly

Firmware `nightly` fast-forward to validated source SHA `8a74ff52…` was **REFUSED** (non-fast-forward vs `08d3164a…`). No force-push. Stage 7 still publishes from the validated source SHA and package hashes below.

## Packages (DEV CHECKED)

- Elite: `x0x-smk37-elite.fwsc` sha256 `aab3cd8e245dedaf561cd973b3ce05aac33eb84ba0077b9b81cb1d9aa3b260b4`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; pool_free 8324; identity SMK-37 Elite_900
- Pro: `x0x-smk37.fwsc` sha256 `066e2b1aeb6ed4abc8b3fba48606eaec1b42aa029a29ce07ffb0049ad7064a1e`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; pool_free 8324; identity SMK-37 Pro_900

## Scope for stage 7

Publish the valid Elite and Pro development packages for validated source SHA `8a74ff52…` to the nightly installer page. Leave production `main` and production GitHub Pages unchanged. Do not wait for stage 6. After the live page matches, start exactly one owner-visible stage-1 Cloud Agent under the handoff rule.
