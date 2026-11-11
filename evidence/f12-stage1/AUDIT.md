# Stage 1 audit of validated source 4f96b0d9

Audited tree: `4f96b0d9f5f11f700ea3fc935800b8714e11f35d` (validated source published on nightly; not the diverged firmware `nightly` tip).
Content tree (Stage-4 DEV CHECKED records): `f27c89e1ac8e11ab57c5ae7e49145ce19b964082bf3ca9a700a99bfa8a97091e`.
Git tree (`source_tree`): `7b35932d5f0adbceeb35111f2136855aa54ebc9b`.
Firmware tree: `5b1a21f07563b0262ece6728051cb0436bd4bfa1`.
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`.
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this record: `cursor/f12-stage1-audit-1207`.
This thread: `bc-3fbb0761-3307-572f-a879-07db26c01207` (next-cycle Stage-1 audit after F11 Stage 7 publish).
Prior cycle audited SHA: `af4947846df7233c68a6be72c4d0c634052a2752` (F11 selected BF-007; now fixed in `4f96b0d9`).
Evidence-bearing base: remote tip of `cursor/f11-bf007-stage7-179d` (docs/evidence only; packages authenticate `4f96b0d9` / tree `f27c89e1…`).

No production source was edited. Production `main` and production GitHub Pages were not updated. No device was flashed. Firmware `nightly` was not force-pushed (still diverged / FF refused at `08d3164a…`).

## Published page (re-verified this audit)

Installer: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/

| Check | Result |
| --- | --- |
| `latest` in live catalog | `nightly-4f96b0d9-dev` |
| Label / `hardware_tested` | `DEV / HARDWARE TEST — DEVICE NOT TESTED` / false |
| Elite bundled re-fetch | size 651396, sha256 `4841110a274e2212d56abf9234cf0d921b45d915d1ff25fad0727e52edb7d18a` **MATCH** |
| Pro bundled re-fetch | size 651392, sha256 `b35d587557d9f1d4c72b2c81bc944a1dd129bee625e98ae35f06fe7f653fc720` **MATCH** |
| Stage-4 blank Elite / Pro (records) | `829bb6da…` / `d2186cb0…` (unchanged from Stage-4 DEV CHECKED) |
| `controller_sha256` (bundled) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| Host tip | `artifacts/nightly-dev` @ `02f4f4f48408db96e01b143d5410b7fb00a5a70a` |
| Pages tip | `48ffd27ebaceff48494cb3145c6699bb1a5c1def` |
| Catalog `source_commit` / `source_tree` | `4f96b0d9…` / `7b35932d…` **MATCH** |
| Live install HTML markers | `4f96b0d9`, `BF-007`, `nightly-4f96b0d9-dev`, `DEVICE NOT TESTED` **present** |
| Served install HTML sha256 | `f381efb6ed5eab50e530ea0fce56331d439efc39418350936f21c4cafdd286f3` |
| Installer fingerprint (qualification) | `4d29566e14c3f252bf52047676e993367e3d155aa1a94a45157ec07d77e47dc8` |
| Dual-digest distinct (FP-001) | **YES** (source FP ≠ served HTML; do not reopen FP-001) |
| Result | **LIVE_PAGE_MATCH** |

Research (internal Task): `bc-92c55624-604c-59b1-bd5b-2ed41ff7fa17` — SELECT BF-008.
Witness (internal Task): `bc-2b17e9ee-82af-57da-93f7-92ac3ec18f94` — tip UBSan FAIL / scratch PASS.
Stage-7 evidence local review: Project store `internal/pr-local-reviews/pr-268.md` (LIVE digests MATCH; Codex not gate).
Package bytes local review (unchanged): Project store `internal/pr-local-reviews/pr-264.md` — Overall **PASS**.
BF-007 host regression on tip: `legacy-bdpan-motion` 171/171 PASS (`bf007-regression.log`).

Cited blobs at `4f96b0d9`:

| File | Blob |
| --- | --- |
| `firmware/src/dsp/fastmath.h` | `7d2e41bf70590bafa8af0a7376bb912bf763b098` (**BF-008 baseline match**; still `b.i += n << 23;`) |
| `firmware/src/app/project.c` | `240a4b1735166dfb990b52c26686c7ea5ad4317e` (BF-007 fixed) |
| `firmware/src/dsp/drum808.c` | `90f463295fb1a642bad81700d73fc22eb8ec7a38` (BF-006 fixed) |
| `firmware/src/seq/sequencer.c` | `fd3f65306df166494d10facf8ddae058287cc17b` (BF-005 fixed) |
| `web/make_site.py` | present (BF-003 fixed; delete still deferred) |
| `tools/check_installer_digests.py` | present (FP-001 fixed) |

PR #28 repair tip blob: `a5c26c95cad0d91a548a8db073f59777e36d4e86` (`b.u += (uint32_t)n << 23;`).

Host witness (this audit): minimal GCC UBSan harness calling `fm_exp2f(-3.0f)` against tip `fastmath.h` — **FAIL** exit 1, `left shift of negative value -3` at line 57. Scratch-only replace with `b.u += (uint32_t)n << 23;` — **PASS** exit 0, `fm_exp2f(-3) = 0.125`. Logs: `bf008-witness.log`, `bf008-scratch-clear.log`. Scratch is not a committed fix. Numeric `fastmath_test.c` without UBSan already passes on tip (`fastmath-numeric.log`) and is not the witness.

## Selected defect

**BF-008. `fm_exp2_nf` applies a signed left shift of a possibly negative exponent (`b.i += n << 23;`).**

Oldest open audit/bugfinder finding after deferred-list exhaustion on `4f96b0d9`. BF-001–BF-007 and FP-001 are fixed and are not selected. Deferred items from `evidence/f11-stage2/DEFERRED.md` remain excluded, design, optional, decision-open, not-required, or docs debt — none are tip source FAILs ahead of this occurrence. Open PR #28 tracks this exact tip FAIL and is now the oldest remaining selectable defect.

## Evidence

Definition: open PR #28 (`lane/smc-fastmath-ubsan-20261023`) plus `evidence/f12-stage1/bf008-definition.md`. Baseline tip blob `7d2e41bf…` identical at audited SHA `4f96b0d9`.

In `firmware/src/dsp/fastmath.h` `fm_exp2_nf` (~43–59):

- After the fractional `2^f` polynomial, the integer exponent `n` is applied with `b.i += n << 23;` (~57).
- A valid negative input such as `x=-3.0f` yields `n=-3`. Left-shifting a negative signed integer is undefined C behavior.
- Expected `fm_exp2f(-3.0f) = 0.125f` with no UB. Host UBSan fails at line 57 before any hardware test.
- This is a different source occurrence from BF-006 (`drum808.c` `d8_exp2_cr`), already fixed in tip. Repairing 808 alone does not clear this `fastmath.h` line.

Executable witness: tip FAIL under GCC UBSan; scratch unsigned cast of that one shift exits 0 with `0.125`.

Log: `evidence/f12-stage1/live-verify.log`.

## Affected scope

- `firmware/src/dsp/fastmath.h` — `fm_exp2_nf` must apply the exponent bias without a signed left shift of a possibly negative `n` (same intent as PR #28 / scratch: `b.u += (uint32_t)n << 23;`).
- Host regressions: add or wire `tests/host/run_fastmath_ubsan.sh` + selectable `fastmath-ubsan` in `tests/run_tests.sh` (red on tip / green after repair under UBSan); keep existing `fastmath` numeric suite green.
- Out of scope: production `main`, production Pages, device flash, BF-001–BF-007, FP-001, withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, deleting Felucca-era `make_site.py` entirely, nightly-repo second qualification field, historical F06 “Installer FP” wording rewrite, merging unrelated PR #28 base-stack commits beyond the fastmath fix + UBSan harness.

## Proposed fix

**Decision for this cycle: repair BF-008 only** (`fm_exp2_nf` uses a defined shift for the exponent bias).

1. Replace `b.i += n << 23;` in `fm_exp2_nf` with a defined form such as `b.u += (uint32_t)n << 23;` (scratch-proven; matches open PR #28 head).
2. Do not change the fractional series / clamping intent of `fm_exp2_nf` / `fm_exp2f`.
3. Prefer cherry-picking or re-applying only the fastmath.h + `run_fastmath_ubsan.sh` + `tests/run_tests.sh` wiring from PR #28 onto a fresh stage-2 branch from this audit tip — do not import the PR’s unrelated base-lane history as the cycle tip.
4. Add or wire host tests that fail on today’s tip under UBSan (`x=-3` left-shift of −3) and pass after the repair; keep existing fastmath numeric suites green.
5. Implement on a new `cursor/<slug>-stage2-1207` branch from the pushed tip of `cursor/f12-stage1-audit-1207`. Use `X0X_DEV_PADCAL=1` for any firmware/package rebuild in later stages.

## Approvals

The full-cycle procedure (`AGENTS.md` + `docs/rules/workflow.md`), the Stage-7 successor prompt, and the deferred-order rule select the oldest open finding after deferred exhaustion without asking. Standing owner pre-approval covers properly tested Project merges of that selected repair. This stage does not merge production `main`, publish production Pages, or flash.

Approved for Stage 2: BF-008 `fm_exp2_nf` defined exponent bias shift + host UBSan regressions as above.

Not approved in this repair: withheld asset cache, unmerged data-safety physical-endpoint lanes, HTML initial-loader resume paths, lengthening `SLOT_PROBE`, deleting `web/make_site.py` entirely, reopening BF-001–BF-007 or FP-001, force-pushing firmware `nightly`, production Pages.

## Missing decisions

None block the BF-008 unsigned-shift repair above. Optional later (not this Stage 2): whether Felucca-era `make_site.py` should be deleted entirely vs remaining installer-only; whether longer `SLOT_PROBE` budgets are worth a separate hardening cycle; historical F06 “Installer FP” wording rewrite as docs-only; whether open PR #28 should be closed as superseded once this cycle’s tip lands the same fix.

## Defects considered and not selected

### Already fixed in `4f96b0d9`

| Item | Disposition |
| --- | --- |
| BF-001 local `.fwsc` / ctl selection race | Present (`d11b74fc`). Closed. |
| BF-002 scoped quickcheck project suites | Present (`ab90f47c`). Closed. |
| BF-003 retire stale editor test / site-builder | Present (`1325f5d3`). Closed. |
| F03 fail-closed slot layout probe | Present (`4f18983a`). Closed. |
| F02 fail-closed loader attach | Present (ancestor). Closed. |
| FP-001 dual-digest reconciliation | Present (`6df0f81f`). Closed. Source FP ≠ served HTML still true by design. |
| BF-004 MIDI owner Note Off gate | Present (`ce690772`). Closed. |
| BF-005 clock handover pending clear | Present (`94f4a012`). Closed. |
| BF-006 808 `d8_exp2_cr` defined exponent bias | Present (`37bab7d1`). Closed. |
| BF-007 legacy BD PAN motion migration order | Present (`78a0452e`; project.c blob `240a4b17…`). Closed. |

### Deferred list (`evidence/f11-stage2/DEFERRED.md`)

| Item | Disposition |
| --- | --- |
| Withheld asset cache | Excluded / not an open tip source defect (prior audits). |
| Unmerged data-safety identity lanes | Explicitly not imported. |
| HTML resume `find(IS_OTA)` | Left as initial/interrupted-loader resume by F02 design. |
| Longer `SLOT_PROBE` budgets | Optional hardening only after F03 fail-closed (still 1.0s/2). |
| Delete Felucca-era `make_site.py` entirely | Decision still open after BF-003 installer-only trim; callers still import helpers — not a tip FAIL. |
| Nightly-repo second qualification field | Not required: FP-001 dual-digest + host check. |
| Historical F06 Stage-7 “Installer FP” wording rewrite | Docs debt; current Stage-7 CHECKS already dual-labels; not a tip FAIL. |
| Open PR #28 `fastmath.h` signed-shift UB | **Selected as BF-008** this cycle (oldest remaining tip FAIL after BF-007 closed). |

### Recorded failing checks

No open **FAIL** on this validated SHA outside GitHub Actions (ignored) and the selected BF-008 UBSan witness. Stage-3 F11 regressions ALL_PASS for BF-007; Stage-4 Elite/Pro DEV CHECKED; Stage-7 published live with matching **package** hashes. Nightly FF REFUSED is divergence, not a package/host FAIL. Optional Pillow/ASan host gaps are not-run where missing, not FAIL.

## Stage 2 limit

Implement only BF-008 (`fm_exp2_nf` defined shift + UBSan host wiring), on a new stage-2 branch from the pushed tip of `cursor/f12-stage1-audit-1207`. Do not take other deferred items in that same change.
