# Workflow handoff to stage 2

Receiving stage: **2 Implement** (new cloud thread; F12 after F11 nightly publish).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f12-stage1-audit-1207`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-1 owner: `bc-3fbb0761-3307-572f-a879-07db26c01207`.
Validated **source** SHA (audit baseline; packages authenticate this): `4f96b0d9f5f11f700ea3fc935800b8714e11f35d`.
Content tree: `f27c89e1ac8e11ab57c5ae7e49145ce19b964082bf3ca9a700a99bfa8a97091e`.
Git source_tree: `7b35932d5f0adbceeb35111f2136855aa54ebc9b`.
Firmware tree: `5b1a21f07563b0262ece6728051cb0436bd4bfa1`.
BF-007 already in tip: `78a0452e` (project.c blob `240a4b17…`) — do not reopen.
Selected defect: **BF-008** — `fm_exp2_nf` signed left-shift UB in `firmware/src/dsp/fastmath.h` blob `7d2e41bf…`.
Evidence: `evidence/f12-stage1/AUDIT.md`, `CHECKS.md`, `live-verify.log`, `bf008-definition.md`, `bf008-witness.log`, `bf008-scratch-clear.log`.
Research: `bc-92c55624-604c-59b1-bd5b-2ed41ff7fa17`.
Witness: `bc-2b17e9ee-82af-57da-93f7-92ac3ec18f94`.
Stage-7 local review cite: Project store `internal/pr-local-reviews/pr-268.md`.
Package local review cite: Project store `internal/pr-local-reviews/pr-264.md` PASS.
Successor prompt: `evidence/f12-stage1/SUCCESSOR-STAGE2.txt`.
Successor agent id: none.
Successor URL: none.
Successor source: pending coordinator CreateAgent (pod has no `CURSOR_API_KEY`).
Handoff status: **incomplete** until an owner-visible Stage 2 id is recorded on this remote tip.

## Scope for stage 2

Implement only BF-008 on a new `cursor/<slug>-stage2-1207` branch from the pushed tip of `cursor/f12-stage1-audit-1207`. Replace `b.i += n << 23;` with `b.u += (uint32_t)n << 23;` (or equivalent defined form); wire `fastmath-ubsan` host harness from open PR #28 without importing unrelated base-lane history; use `X0X_DEV_PADCAL=1`. Do not reopen BF-001–BF-007 or FP-001. Do not flash. Leave production unchanged.

Deferred / excluded items: withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, delete Felucca-era `make_site.py` entirely, nightly-repo second qualification field, historical F06 Stage-7 “Installer FP” wording rewrite (`evidence/f11-stage2/DEFERRED.md`).
