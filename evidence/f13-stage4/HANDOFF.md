# Workflow handoff to stage 5

Receiving stage: **5 Merge** (new cloud thread; F13 after stage-4 BF-012 validate).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f13-bf012-stage4-237f`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-4 owner: `bc-952af67e-b6f6-5318-a3b3-ba4de860c800`.
Stage-3 owner: `bc-7c49b76a-97dd-57c9-9a5a-784be039d70a`.
Stage-2 owner: `bc-a0181730-1daf-5e94-826a-e4339ed37abf`.
Stage-1 owner: `bc-09959f6e-dc18-5575-baa6-444dfdf698cc`.
Audited / validated source SHA: `de11b6d7e3ac3322d2db689675d9ed27ed667785`.
**Install / merge / package SHA (authoritative):** `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675`.
That is the Stage-3 tested tip DEV CHECKED packages were built from. Fast-forward firmware `nightly` to **this** SHA. Pass **this** SHA to Stage-6 install / `dev_validate.py verify --commit`.
**Evidence tip (not install/merge):** branch head of `cursor/f13-bf012-stage4-237f` after Stage-4 added `evidence/f13-stage4/` logs/JSON/records (e.g. `d344ad12…` and later clarify commits). Carries HANDOFF / CHECKS / records only. Do **not** fast-forward `nightly` to the evidence tip. Do **not** pass the evidence tip as `--commit` (content tree includes non-`.md` logs/JSON → REFUSED / wrong tree).
BF-012 repair commit: `6636cca1` (`master.c` blob `f91a9473…`, `master_test.c` blob `df2f13b3…`).
Package validation commit (= install/merge SHA): `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675`.
Content tree at validation commit: `519ccdf21b46db9198993587cb9334960e67ce483641c744c5caf71b1c769141`.
Firmware tree: `5facb3b4405d119a269a05224b9268adfa1baa13`.
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007), `de11b6d7` (BF-008).
Package evidence: `evidence/f13-stage4/CHECKS.md`, `elite/record.json`, `pro/record.json`.
Host test evidence: `evidence/f13-stage3/CHECKS.md` (ALL_PASS).
Deferred (unchanged): `evidence/f13-stage2/DEFERRED.md` / `evidence/f13-stage3/DEFERRED.md`.
Successor prompt: `evidence/f13-stage4/SUCCESSOR-STAGE5.txt`.
Successor agent id: none (Stage 5 already steered separately; do not start a second Stage 5 from this clarify).
Successor URL: none.
Successor source: Stage 5 steered outside this clarify.
Handoff status: package SHA locked; evidence tip only carries records.

## Package summary

Elite and Pro development packages (`X0X_DEV_PADCAL=1`) are **DEV CHECKED** for install/merge SHA `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675`. Pool free 8324 B (>= 8192). Firmware tree changed for BF-012; package SHA-256 differ from the live nightly as expected. Controller SHA-256 unchanged. Host PASS alone was not treated as a package PASS.

- Elite: `x0x-smk37-elite.fwsc` sha256 `d9a9ccf4698d9519316d90b36f6158101f7f1c5cd96ba50f6c1be6ef8693ebcc`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; identity SMK-37 Elite_900
- Pro: `x0x-smk37.fwsc` sha256 `a6a2387c712e95772ab1ff2f184f6b21357f71da08f0414f8d698b5f94309498`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; identity SMK-37 Pro_900

## Scope for stage 5

Fast-forward firmware `nightly` to **`b42bcba46ee7255c18f18c10dd1f99b9fc9bb675`** (package validation / install SHA), not the Stage-4 evidence tip. Save the local stage-6 install prompt with package and controller hashes and that same SHA. Do not flash from the cloud thread. Do not implement deferred items. Do not reopen BF-001–BF-008 or FP-001. Do not steal Arp/Fx #279/#281 leftovers. Do not merge production `main` or publish production Pages. Stage 5 already steered — do not launch another.
