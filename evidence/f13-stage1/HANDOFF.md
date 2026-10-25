# Workflow handoff to stage 2

Receiving stage: **2 Implement** (new cloud thread; F13 after F12 LIVE_PAGE_MATCH + Stage 6 PASS).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f13-stage1-audit-237f`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-1 owner: `bc-09959f6e-dc18-5575-baa6-444dfdf698cc`.
Validated **source** SHA (audit baseline; packages authenticate this): `de11b6d7e3ac3322d2db689675d9ed27ed667785`.
Git `source_tree`: `c143a9f8f5b5d0f6804995829ccbe515c77c6103`.
Firmware tree: `9195dce5c4beb5fa8aa564504c8d1d114c8264cd`.
Cross-check `origin/nightly` (post-reunify): `5e6d7e4ea387c42722d2704bdb43ef04f7caeda3` — contains `de11b6d7`; same unfixed `master.c` blob.
BF-008 already in tip: `de11b6d7` (`fastmath.h` defined unsigned shift) — do not reopen.
Selected defect: **BF-012** — MASTER LIMIT OFF→ON stale look-ahead in `firmware/src/dsp/master.c` blob `b5557b6c…`.
Evidence: `evidence/f13-stage1/AUDIT.md`, `CHECKS.md`, `live-verify.log`, `bf012-definition.md`, `bf012-witness.log`, `bf012-scratch-clear.log`, `master-tip-baseline.log`.
Evidence PR: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/291
Successor prompt: `evidence/f13-stage1/SUCCESSOR-STAGE2.txt`.
Successor agent id: none.
Successor URL: none.
Successor source: pending coordinator CreateAgent (this Stage 1 does not implement Stage 2).
Handoff status: **incomplete** until an owner-visible Stage 2 id is recorded on this remote tip.

## Scope for stage 2

Implement only BF-012 on a new `cursor/<slug>-stage2-237f` branch from the pushed tip of `cursor/f13-stage1-audit-237f`. Clear look-ahead rings on LIMIT off→on (PR #68 intent); add host regressions; use `X0X_DEV_PADCAL=1`. Do not reopen BF-001–BF-008 or FP-001. Do not flash. Leave production unchanged. Do not take Arp/Fx sole-owner leftovers.

Deferred / excluded items: withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, delete Felucca-era `make_site.py` entirely, nightly-repo second qualification field, historical F06 Stage-7 “Installer FP” wording rewrite (`evidence/f12-stage2/DEFERRED.md`).
