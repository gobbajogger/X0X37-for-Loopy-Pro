# Stage 1 audit of validated source 8a74ff52

Audited tree: `8a74ff52cba842c3f2d8365600286f94e6bccdf0` (validated source published on nightly; not the diverged firmware `nightly` tip).
Content tree (Stage-4 DEV CHECKED records): `1a1e7a22d136fef6e30702c9d3207d22798a6dd382717acc5ab0f40d2f235acf`.
Git tree (`source_tree`): `33d6950b0a378525f2a8b592672275625893239f`.
Firmware tree: `581d1cb213e6ceda3e9fbbcdc324ef8dce67c42f`.
Audited firmware content tree (pre-BF-004 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`.
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this record: `cursor/f09-bf005-stage1-fb90`.
This thread: `bc-2ac9852a-4eac-553f-906c-d7ecf1a4d46e` (next-cycle Stage-1 audit after F08 Stage 7 publish).
Prior cycle audited SHA: `6df0f81fbbe0adc3207f67d912d2fe0ab676be04` (F08 selected BF-004; now fixed in `8a74ff52`).
Evidence-bearing base: remote tip of `cursor/f08-bf004-stage7-fb90` (docs/evidence only; packages authenticate `8a74ff52` / tree `1a1e7a22…`).

No production source was edited. Production `main` and production GitHub Pages were not updated. No device was flashed. Firmware `nightly` was not force-pushed (still diverged / FF refused at `08d3164a…`).

## Published page (re-verified this audit)

Installer: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/

| Check | Result |
| --- | --- |
| `latest` in live `versions.json` | `nightly-8a74ff52-dev` |
| Label / `hardware_tested` | `DEV / HARDWARE TEST — DEVICE NOT TESTED` / false |
| Elite bundled re-fetch | size 651396, sha256 `d7767bebf1413b5d6fb918ba3893335a7b9fae86078ce895aa3b72df48bedb93` **MATCH** |
| Pro bundled re-fetch | size 651392, sha256 `b0cecbcff42589cdc71231a16098b7cfc8301a8562aba25362d552b3dcdc5fa0` **MATCH** |
| Stage-4 blank Elite / Pro (records) | `aab3cd8e…` / `066e2b1a…` (unchanged from Stage-4 DEV CHECKED) |
| `controller_sha256` (bundled) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| Host tip | `artifacts/nightly-dev` @ `82622c1f9561bd23bbf785c165d66e11e68820ca` |
| Pages tip | `e296e8834aff9f9f358fc6f63360abbff83334d4` |
| Catalog `source_commit` / `source_tree` | `8a74ff52…` / `33d6950b…` **MATCH** |
| Live install HTML markers | `8a74ff52`, `BF-004`, `nightly-8a74ff52-dev`, `DEVICE NOT TESTED` **present** |
| Served install HTML sha256 | `aec5dc78f6d4fe198b8e32e7939c7c0533a5c585f4eeb04adadc6fad78379b31` |
| Installer fingerprint (qualification) | `4d29566e14c3f252bf52047676e993367e3d155aa1a94a45157ec07d77e47dc8` |
| Dual-digest distinct (FP-001) | **YES** (source FP ≠ served HTML; do not reopen FP-001) |

Independent claim review (internal Task): `bc-2c8dbc00-6019-5cab-bf1c-52d90930ef4f` — **12/12 PASS**.
Research helper: `bc-733122bc-0711-5a24-abd5-8d03e225ad6c` — deferred exhaustion → BF-005.
Stage-7 evidence local review: Project store `internal/pr-local-reviews/pr-234.md` — Overall **PASS**.
Package bytes local review (unchanged): Project store `internal/pr-local-reviews/pr-232.md` — Overall **PASS**.

Cited blobs at `8a74ff52`:

| File | Blob |
| --- | --- |
| `firmware/src/seq/sequencer.c` | `83b03f73139d8e679c94526c66b390b6e5bc2f39` (**BF-005 baseline match**) |
| `firmware/src/app/engine.c` | `ab6d9667e9cd40ae12cc8c8ece8d7bb12be29216` (BF-004 fixed) |
| `web/make_site.py` | `e98904afc8e38dbce693fffd26f97da1e809aca4` (BF-003 fixed; delete still deferred) |
| `tools/check_installer_digests.py` | present (FP-001 fixed) |

Host witness (this audit): `tests/host/bugfinder_clock_handover_test.c` from PR #26 lane against tip `sequencer.c` — **FAIL** `old_pending=3` (exit 1). Scratch-only clear of `ext_pending` inside owner-matching `seq_clk_drop` — **PASS** (exit 0). Logs: `bf005-witness.log`, `bf005-scratch-clear.log`. Scratch is not a committed fix.

## Selected defect

**BF-005. Queued old-source MIDI clocks accelerate a new source after disconnect.**

Oldest open audit/bugfinder finding after deferred-list exhaustion on `8a74ff52`. BF-001, BF-002, BF-003, FP-001, and BF-004 are fixed and are not selected. Deferred items from `evidence/f08-stage2/DEFERRED.md` remain excluded, design, optional, decision-open, not-required, or docs debt (see below) — none are tip source FAILs.

## Evidence

Definition (docs-only lane, open draft PR #26): `docs/audit/bugfinder-clock-handover-20261023.md` on `lane/bugfinder-reports-20261023` (copy: `evidence/f09-stage1/bf005-definition.md`). Baseline main `c9718ae8…`; sequencer blob `83b03f73…` — identical at audited SHA `8a74ff52`.

In `firmware/src/seq/sequencer.c`:

- `seq_ext_clock` increments shared `ext_pending` for Clock F8 (~744).
- `seq_clk_drop` at ~792–796 clears only `clk_lock` when the dropping source owned the lock; it does **not** clear `ext_pending`.
- After USB disconnect, Bluetooth Start + new Bluetooth Clocks can drain the old USB pending ticks via `seq_advance`, advancing 909 phase too early.

Executable witness (PR #26 lane harness, not wired into the passing suite while unfixed): fails with `old_pending=3` / `observed=2` vs expected step 1 on the 4th fresh Bluetooth clock; control `old_pending=0` passes.

Log: `evidence/f09-stage1/live-verify.log`.

## Affected scope

- `firmware/src/seq/sequencer.c` — `seq_clk_drop` must drop queued `ext_pending` when the locked source’s link ends (owner-matching path), so a new source’s Start/Clock is not accelerated by stale ticks.
- Host regressions: promote or add `bugfinder_clock_handover_test.c` (red on tip / green after repair); keep `clk_lock` / sequencer host suites green.
- Out of scope: production `main`, production Pages, device flash, BF-001–BF-004, FP-001, withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, deleting Felucca-era `make_site.py` entirely, nightly-repo second qualification field, historical F06 “Installer FP” wording rewrite, BF-006/BF-007.

## Proposed fix

**Decision for this cycle: repair BF-005 only** (`seq_clk_drop` clears pending external clocks with the lock).

1. When `seq_clk_drop` clears `clk_lock` for the ending source, also clear `s->ext_pending` (scratch-proven; both control and regression then pass).
2. Do not clear pending clocks for a drop of a non-owning source (preserve existing `clk_lock` ownership semantics).
3. Keep `seq_ext_clock` / Start / Continue / internal-clock paths unchanged in intent.
4. Add or wire host tests that fail on today’s tip (`old_pending=3` accelerates new Bluetooth owner) and pass after the repair; keep `old_pending=0` and existing `clk_lock` suites green.
5. Implement on a new `cursor/<slug>-stage2-fb90` branch from the pushed tip of `cursor/f09-bf005-stage1-fb90`. Use `X0X_DEV_PADCAL=1` for any firmware/package rebuild in later stages.

## Approvals

The full-cycle procedure (`AGENTS.md` + `docs/rules/workflow.md`), the Stage-7 successor prompt, and the deferred-order rule select the oldest open bugfinder finding after deferred exhaustion without asking. Standing owner pre-approval covers properly tested Project merges of that selected repair. This stage does not merge production `main`, publish production Pages, or flash.

Approved for Stage 2: BF-005 `seq_clk_drop` pending-clock clear + host regressions as above.

Not approved in this repair: BF-006, BF-007, withheld asset cache, unmerged data-safety physical-endpoint lanes, HTML initial-loader resume paths, lengthening `SLOT_PROBE`, deleting `web/make_site.py` entirely, reopening BF-001–BF-004 or FP-001, force-pushing firmware `nightly`, production Pages.

## Missing decisions

None block the BF-005 pending-clear above. Optional later (not this Stage 2): whether Felucca-era `make_site.py` should be deleted entirely vs remaining installer-only; whether longer `SLOT_PROBE` budgets are worth a separate hardening cycle; historical F06 “Installer FP” wording rewrite as docs-only; exact TRS multi-source clock edge cases if host probes expose them beyond the USB→Bluetooth handover witness.

## Defects considered and not selected

### Already fixed in `8a74ff52`

| Item | Disposition |
| --- | --- |
| BF-001 local `.fwsc` / ctl selection race | Present (`d11b74fc`). Closed. |
| BF-002 scoped quickcheck project suites | Present (`ab90f47c`). Closed. |
| BF-003 retire stale editor test / site-builder | Present (`1325f5d3`). Closed. |
| F03 fail-closed slot layout probe | Present (`4f18983a`). Closed. |
| F02 fail-closed loader attach | Present (ancestor). Closed. |
| FP-001 dual-digest reconciliation | Present (`6df0f81f`). Closed. Source FP ≠ served HTML still true by design. |
| BF-004 MIDI owner Note Off gate | Present (`ce690772`; engine blob `ab6d9667…`). Closed. |

### Deferred list (`evidence/f08-stage2/DEFERRED.md`)

| Item | Disposition |
| --- | --- |
| Withheld asset cache | Excluded / not an open tip source defect (prior audits). |
| Unmerged data-safety identity lanes | Explicitly not imported. |
| HTML resume `find(IS_OTA)` | Left as initial/interrupted-loader resume by F02 design. |
| Longer `SLOT_PROBE` budgets | Optional hardening only after F03 fail-closed (still 1.0s/2). |
| Delete Felucca-era `make_site.py` entirely | Decision still open after BF-003 installer-only trim; callers still import helpers — not a tip FAIL. |
| Nightly-repo second qualification field | Not required: FP-001 dual-digest + host check. |
| Historical F06 Stage-7 “Installer FP” wording rewrite | Docs debt; current F08 Stage-7 CHECKS already dual-labels; not a tip FAIL. |

### Recorded failing checks

No open **FAIL** on this validated SHA outside GitHub Actions (ignored). Stage-3 F08 regressions ALL_PASS; Stage-4 Elite/Pro DEV CHECKED; Stage-7 published live with matching **package** hashes. Nightly FF REFUSED is divergence, not a package/host FAIL. Optional Pillow host gaps are not-run, not FAIL.

### Younger open bugfinder (not selected)

| Item | Disposition |
| --- | --- |
| BF-006 drum808 `d8_exp2_cr` signed shift UB | Still open on PR #26 lane; younger than BF-005. |
| BF-007 legacy BD PAN motion migration order | Still open; younger. |

## Stage 2 limit

Implement only BF-005 (`seq_clk_drop` clears `ext_pending` with the lock + host tests), on a new stage-2 branch from the pushed tip of `cursor/f09-bf005-stage1-fb90`. Do not take other deferred or younger bugfinder items in that same change.
