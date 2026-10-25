# Stage 1 audit of validated source d0d00da2

Audited tree: `d0d00da23c3ce2268f06f31f8c062716d5a37ad0` (validated source published on nightly; not the diverged firmware `nightly` tip).
Content tree (Stage-4 DEV CHECKED records): `62339630bc6e133da464eed7924645c0bfc09e4de66a8bccafdc60fd2d660c83`.
Git tree (`source_tree`): `e06299abf0f35f24c771d011d07c2e74de613369`.
Firmware tree: `32a36f68d1ce7a3c304b447475afe7e2bbb116cf`.
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`.
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this record: `cursor/f10-bf006-stage1-fb90`.
This thread: `bc-6b7eb25f-60be-5d9e-8fa7-d8b9111b5e63` (next-cycle Stage-1 audit after F09 Stage 7 publish).
Prior cycle audited SHA: `8a74ff52cba842c3f2d8365600286f94e6bccdf0` (F09 selected BF-005; now fixed in `d0d00da2`).
Evidence-bearing base: remote tip of `cursor/f09-bf005-stage7-fb90` (docs/evidence only; packages authenticate `d0d00da2` / tree `62339630…`).

No production source was edited. Production `main` and production GitHub Pages were not updated. No device was flashed. Firmware `nightly` was not force-pushed (still diverged / FF refused at `08d3164a…`).

## Published page (re-verified this audit)

Installer: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/

| Check | Result |
| --- | --- |
| `latest` in live `versions.json` | `nightly-d0d00da2-dev` |
| Label / `hardware_tested` | `DEV / HARDWARE TEST — DEVICE NOT TESTED` / false |
| Elite bundled re-fetch | size 651396, sha256 `9cba1f3d0bebc2113d59b9bbb3f1ca1a67bb396183be279b74486319ddae8e11` **MATCH** |
| Pro bundled re-fetch | size 651392, sha256 `5dda4eecd74d96237a3d457a026d7bc04eb3ed570705179ab8e6c15a198fa739` **MATCH** |
| Stage-4 blank Elite / Pro (records) | `de0bb077…` / `25bebb26…` (unchanged from Stage-4 DEV CHECKED) |
| `controller_sha256` (bundled) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| Host tip | `artifacts/nightly-dev` @ `2363c07f334747208831923a4f9fa4332cbbe472` |
| Pages tip | `4f468d16c2a8c67be8817db0637d909b2fd3db85` |
| Catalog `source_commit` / `source_tree` | `d0d00da2…` / `e06299ab…` **MATCH** |
| Live install HTML markers | `d0d00da2`, `BF-005`, `nightly-d0d00da2-dev`, `DEVICE NOT TESTED` **present** |
| Served install HTML sha256 | `472b486350b3a05a33a5459bcc95d705287a688c85ee460df60f84c48a0c31f9` |
| Installer fingerprint (qualification) | `4d29566e14c3f252bf52047676e993367e3d155aa1a94a45157ec07d77e47dc8` |
| Dual-digest distinct (FP-001) | **YES** (source FP ≠ served HTML; do not reopen FP-001) |

Independent claim review (internal Task): `bc-a4b5bfab-9e69-5adc-9677-9d2dfe23e20a` — **12/12 PASS**.
Stage-7 evidence local review: Project store `internal/pr-local-reviews/pr-245.md` — Overall **PASS**.
Package bytes local review (unchanged): Project store `internal/pr-local-reviews/pr-243.md` — Overall **PASS**.

Cited blobs at `d0d00da2`:

| File | Blob |
| --- | --- |
| `firmware/src/dsp/drum808.c` | `4b873a79cb050a789cfa26c2490ca8cf61c94ecb` (**BF-006 baseline match**) |
| `firmware/src/seq/sequencer.c` | `fd3f65306df166494d10facf8ddae058287cc17b` (BF-005 fixed) |
| `firmware/src/app/engine.c` | (BF-004 fixed; not reopened) |
| `web/make_site.py` | `e98904afc8e38dbce693fffd26f97da1e809aca4` (BF-003 fixed; delete still deferred) |
| `tools/check_installer_digests.py` | present (FP-001 fixed) |

Host witness (this audit): PR #26 lane `tests/host/bugfinder_drum808_exp2_test.c` against tip `drum808.c` with GCC 14 `-fsanitize=undefined -fno-sanitize-recover=all` — **FAIL** `left shift of negative value -3` at `drum808.c:165` (exit 1). Scratch-only replace of that line with `b.u += (uint32_t)(int32_t)n << 23;` — **PASS** (exit 0; nine exp2 numeric controls). Logs: `bf006-witness.log`, `bf006-scratch-clear.log`. Scratch is not a committed fix.

## Selected defect

**BF-006. 808 `d8_exp2_cr` signed left shift is undefined behavior.**

Oldest open audit/bugfinder finding after deferred-list exhaustion on `d0d00da2`. BF-001, BF-002, BF-003, FP-001, BF-004, and BF-005 are fixed and are not selected. Deferred items from `evidence/f09-stage2/DEFERRED.md` remain excluded, design, optional, decision-open, not-required, or docs debt (see below) — none are tip source FAILs.

## Evidence

Definition (docs-only lane, open draft PR #26): `docs/audit/bugfinder-drum808-exp2-20261023.md` on `lane/bugfinder-reports-20261023` (copy: `evidence/f10-stage1/bf006-definition.md`). Baseline main `c9718ae8…`; drum808 blob `4b873a79…` — identical at audited SHA `d0d00da2`.

In `firmware/src/dsp/drum808.c` `d8_exp2_cr` (~146–167):

- After the correctly-rounded fractional exp2 series, the integer exponent `n` is applied with `b.i += (int32_t)n << 23;` (~165).
- A valid negative input such as `x=-3.0f` yields `n=-3`. Left-shifting a negative signed integer is undefined C behavior.
- Expected `d8_exp2_cr(-3.0f) = 0.125f` with no UB. Host UBSan fails at line 165 before any hardware test.
- This is a different source occurrence from the `fastmath.h` signed-shift UB tracked on open PR #28 (`lane/smc-fastmath-ubsan-20261023`); repairing fastmath alone does not clear this 808 line.

Executable witness (PR #26 lane harness): GCC UBSan exit 1 on tip; scratch unsigned cast of that one shift exits 0 with nine numeric controls PASS.

Log: `evidence/f10-stage1/live-verify.log`.

## Affected scope

- `firmware/src/dsp/drum808.c` — `d8_exp2_cr` must apply the exponent bias without a signed left shift of a possibly negative `n` (same intent as the scratch: `b.u += (uint32_t)(int32_t)n << 23;` or equivalent defined form).
- Host regressions: promote or add `bugfinder_drum808_exp2_test.c` (red on tip / green after repair under UBSan); keep existing `drum808` / pan host suites green.
- Out of scope: production `main`, production Pages, device flash, BF-001–BF-005, FP-001, withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, deleting Felucca-era `make_site.py` entirely, nightly-repo second qualification field, historical F06 “Installer FP” wording rewrite, BF-007, duplicating or merging open PR #28 fastmath edits in this Stage-2 change (unless Stage 2 discovers the 808 fix alone is insufficient for the selected host probe — it is sufficient per scratch).

## Proposed fix

**Decision for this cycle: repair BF-006 only** (`d8_exp2_cr` uses a defined shift for the exponent bias).

1. Replace `b.i += (int32_t)n << 23;` in `d8_exp2_cr` with a defined form such as `b.u += (uint32_t)(int32_t)n << 23;` (scratch-proven).
2. Do not change the fractional series / clamping intent of `d8_exp2_cr`.
3. Do not fold unrelated open PR #28 `fastmath.h` edits into this repair unless required for the selected host probe (not required: scratch of the 808 line alone clears UBSan for this harness).
4. Add or wire host tests that fail on today’s tip under UBSan (`x=-3` left-shift of −3) and pass after the repair; keep the nine numeric exp2 controls and existing drum808 suites green.
5. Implement on a new `cursor/<slug>-stage2-fb90` branch from the pushed tip of `cursor/f10-bf006-stage1-fb90`. Use `X0X_DEV_PADCAL=1` for any firmware/package rebuild in later stages.

## Approvals

The full-cycle procedure (`AGENTS.md` + `docs/rules/workflow.md`), the Stage-7 successor prompt, and the deferred-order rule select the oldest open bugfinder finding after deferred exhaustion without asking. Standing owner pre-approval covers properly tested Project merges of that selected repair. This stage does not merge production `main`, publish production Pages, or flash.

Approved for Stage 2: BF-006 `d8_exp2_cr` defined exponent bias shift + host regressions as above.

Not approved in this repair: BF-007, withheld asset cache, unmerged data-safety physical-endpoint lanes, HTML initial-loader resume paths, lengthening `SLOT_PROBE`, deleting `web/make_site.py` entirely, reopening BF-001–BF-005 or FP-001, force-pushing firmware `nightly`, production Pages, merging open PR #28 as part of this cycle’s Stage-2 change.

## Missing decisions

None block the BF-006 unsigned-shift repair above. Optional later (not this Stage 2): whether Felucca-era `make_site.py` should be deleted entirely vs remaining installer-only; whether longer `SLOT_PROBE` budgets are worth a separate hardening cycle; historical F06 “Installer FP” wording rewrite as docs-only; whether/when to land open PR #28 fastmath UB repair as its own cycle; BF-007 legacy BD PAN motion migration order.

## Defects considered and not selected

### Already fixed in `d0d00da2`

| Item | Disposition |
| --- | --- |
| BF-001 local `.fwsc` / ctl selection race | Present (`d11b74fc`). Closed. |
| BF-002 scoped quickcheck project suites | Present (`ab90f47c`). Closed. |
| BF-003 retire stale editor test / site-builder | Present (`1325f5d3`). Closed. |
| F03 fail-closed slot layout probe | Present (`4f18983a`). Closed. |
| F02 fail-closed loader attach | Present (ancestor). Closed. |
| FP-001 dual-digest reconciliation | Present (`6df0f81f`). Closed. Source FP ≠ served HTML still true by design. |
| BF-004 MIDI owner Note Off gate | Present (`ce690772`). Closed. |
| BF-005 clock handover pending clear | Present (`94f4a012`; sequencer blob `fd3f6530…`). Closed. |

### Deferred list (`evidence/f09-stage2/DEFERRED.md`)

| Item | Disposition |
| --- | --- |
| Withheld asset cache | Excluded / not an open tip source defect (prior audits). |
| Unmerged data-safety identity lanes | Explicitly not imported. |
| HTML resume `find(IS_OTA)` | Left as initial/interrupted-loader resume by F02 design. |
| Longer `SLOT_PROBE` budgets | Optional hardening only after F03 fail-closed (still 1.0s/2). |
| Delete Felucca-era `make_site.py` entirely | Decision still open after BF-003 installer-only trim; callers still import helpers — not a tip FAIL. |
| Nightly-repo second qualification field | Not required: FP-001 dual-digest + host check. |
| Historical F06 Stage-7 “Installer FP” wording rewrite | Docs debt; current Stage-7 CHECKS already dual-labels; not a tip FAIL. |

### Recorded failing checks

No open **FAIL** on this validated SHA outside GitHub Actions (ignored). Stage-3 F09 regressions ALL_PASS; Stage-4 Elite/Pro DEV CHECKED; Stage-7 published live with matching **package** hashes. Nightly FF REFUSED is divergence, not a package/host FAIL. Optional Pillow/ASan host gaps are not-run where missing, not FAIL (this audit’s GCC UBSan runtime was present and used).

### Younger open bugfinder (not selected)

| Item | Disposition |
| --- | --- |
| BF-007 legacy BD PAN motion migration order | Still open on PR #26 lane; `project.c` blob `1000a0d8…` still matches at `d0d00da2`; younger than BF-006. |

## Stage 2 limit

Implement only BF-006 (`d8_exp2_cr` defined exponent bias shift + host tests), on a new stage-2 branch from the pushed tip of `cursor/f10-bf006-stage1-fb90`. Do not take other deferred or younger bugfinder items in that same change.
