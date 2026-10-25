# Stage 1 audit of validated source 6df0f81f

Audited tree: `6df0f81fbbe0adc3207f67d912d2fe0ab676be04` (validated source published on nightly; not the diverged firmware `nightly` tip).
Content tree (Stage-4 DEV CHECKED records): `69ccb3d77a718d32191cf17a75bce6454dad807ef9bb76f6050e6532a0bff3ad`.
Git tree (`source_tree`): `4dc57f74141fd1f01728a0001025b6e76065bf19`.
Audited firmware content tree (unchanged package bytes): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`.
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this record: `cursor/f08-bf004-stage1-fb90`.
This thread: `bc-78e96d95-98c8-5fe4-b4f0-287bc2ad068d` (next-cycle Stage-1 audit after F07 Stage 7 publish).
Prior cycle audited SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d` (F07 selected FP-001; now fixed).
Evidence-bearing base: remote tip of `cursor/f07-fp-pin-stage7-fb90` (docs/evidence only; packages authenticate `6df0f81f` / tree `69ccb3d7…`).

No production source was edited. Production `main` and production GitHub Pages were not updated. No device was flashed. Firmware `nightly` was not force-pushed (still diverged / FF refused at `08d3164a…`).

## Published page (re-verified this audit)

Installer: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/

| Check | Result |
| --- | --- |
| `latest` in live `versions.json` | `nightly-6df0f81f-dev` |
| Label / `hardware_tested` | `DEV / HARDWARE TEST — DEVICE NOT TESTED` / false |
| Elite bundled re-fetch | size 651396, sha256 `8ba23a89f50c70b631927077ec4cfe7c98593870035455261842f1b245b82278` **MATCH** |
| Pro bundled re-fetch | size 651392, sha256 `d90932115ff47cf421b0ef5caab3b93e730da5810e5b26d4c3ecd2c61c58fecb` **MATCH** |
| Stage-4 blank Elite / Pro (records) | `d3b4cac3…` / `0b09605f…` (unchanged from Stage-4 DEV CHECKED) |
| `controller_sha256` (bundled) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| Host tip | `artifacts/nightly-dev` @ `f917b636ba00d7dbef18fd00d1f0af15299a0a24` |
| Pages tip | `d6a237c43cb6830d8cf0cb51e567181821630a28` |
| Catalog `source_commit` / `source_tree` | `6df0f81f…` / `4dc57f74…` **MATCH** |
| Live install HTML markers | `6df0f81f`, `FP-001`, `nightly-6df0f81f-dev`, `DEVICE NOT TESTED` **present** |
| Served install HTML sha256 | `d0866927e2b5baccfc3b2cde422c3f4eea2101a930cafd4de1c77ffd681cb54d` |
| Installer source fingerprint | `8caf87247afb79dda2107f3074d73da7f2512f5d14e55db32b9285baadc38157` |
| Dual-digest distinct (FP-001) | **YES** (source FP ≠ served HTML; do not reopen FP-001) |

Observation: F07 publish updated the served install HTML (was `66b56ba5…` at F07 Stage-2 live fetch; now `d0866927…`). Source fingerprint `8caf8724…` is unchanged. FP-001 dual-digest tooling and evidence pattern remain the gate — historical F06 Stage-2 `digests.json` served pin is expected to fail `--fetch` against today’s HTML and is not a new package FAIL.

Independent claim review (internal Task): `bc-5b578719-45a1-5aa8-9ae1-25087ce3609e` — **12/12 PASS**.
Research helper: `bc-cc5e637f-6d3f-5a99-9d08-759ddf037f08` — deferred exhaustion → BF-004.
Stage-7 evidence local review: Project store `internal/pr-local-reviews/pr-225.md` — Overall **PASS**.
Package bytes local review (unchanged): Project store `internal/pr-local-reviews/pr-222.md` — Overall **PASS**.

Cited blobs at `6df0f81f`:

| File | Blob |
| --- | --- |
| `firmware/src/app/engine.c` | `555480d9c81ca8b088842bfa6bac93e85865d49a` (**BF-004 baseline match**) |
| `web/make_site.py` | `e98904afc8e38dbce693fffd26f97da1e809aca4` (BF-003 fixed; delete still deferred) |
| `tools/check_installer_digests.py` | present (FP-001 fixed) |

## Selected defect

**BF-004. Stale Note Off interrupts the current MIDI owner.**

Oldest open audit/bugfinder finding after deferred-list exhaustion on `6df0f81f`. BF-001, BF-002, BF-003, and FP-001 are fixed and are not selected. Deferred items from `evidence/f07-stage2/DEFERRED.md` remain excluded, design, optional, decision-open, not-required, or docs debt (see below) — none are tip source FAILs.

## Evidence

Definition (docs-only lane, open draft PR #26): `docs/audit/bugfinder-midi-owner-20261023.md` on `lane/bugfinder-reports-20261023` (copy: `evidence/f08-stage1/bf004-definition.md`). Baseline main `c9718ae8…`; engine blob `555480d9…` — identical at audited SHA `6df0f81f`.

In `firmware/src/app/engine.c` `midi_in`:

- Note On for a 303 transfers ownership: clears `src_held[0]` and `src_held[1]` for that bit, then sets `src_held[src]` when `src < 2` (lines ~609–616).
- Note Off for the same pitch releases the voice whenever `bass_held[k] == d1`, clearing **both** `src_held` sides **without** checking that the Note Off’s `src` owns the bit (lines ~617–622).
- BREAK slice Note On/Off similarly updates/clears `src_held` without requiring Note Off to come from the owning USB/Bluetooth source (lines ~624–630).
- Link-drop path (`pkt` empty) correctly releases only `src_held[src]` (lines ~537–549) — the stale Note Off path does not mirror that ownership check.

Bugfinder host scenarios (prior run): 303A expected held 1 got 0; BREAK expected 256 got 0; different-note control and existing USB/Bluetooth scenarios exit 0. This audit confirms the source still matches the finding baseline; it does not re-run target firmware or device tests.

Log: `evidence/f08-stage1/live-verify.log`.

## Affected scope

- `firmware/src/app/engine.c` — Note Off (and BREAK off) must honor `src_held` ownership for USB (`src==0`) / Bluetooth (`src==1`); protect valid owner Note Off; preserve source-2 (TRS) behavior as the bugfinder notes.
- Host regressions under `X0X_HOST` using `engine_midi_held()` — add red/green scenarios for stale Note Off from the non-owner source vs owner release and different-note control.
- Out of scope: production `main`, production Pages, device flash, BF-001–BF-003, FP-001, withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, deleting Felucca-era `make_site.py` entirely, nightly-repo second qualification field, historical F06 “Installer FP” wording rewrite, BF-005/BF-006/BF-007.

## Proposed fix

**Decision for this cycle: repair BF-004 only** (MIDI owner Note Off gating).

1. On 303 Note Off, release the held note only when the Note Off’s `src` owns that bit in `src_held` (or when ownership rules explicitly allow TRS/`src>=2` as approved in Stage 2 design), matching the link-drop path’s per-source release.
2. On BREAK slice Note Off, same ownership gate before clearing the slice / `src_held` bits.
3. Keep Note On ownership transfer and link-drop cleanup unchanged in intent.
4. Add host tests that fail on today’s tip (stale non-owner Note Off silences current owner) and pass after the repair; keep different-note and same-owner release green.
5. Implement on a new `cursor/<slug>-stage2-fb90` branch from the pushed tip of `cursor/f08-bf004-stage1-fb90`. Use `X0X_DEV_PADCAL=1` for any firmware/package rebuild in later stages.

## Approvals

The full-cycle procedure (`AGENTS.md` + `docs/rules/workflow.md`), the Stage-7 successor prompt, and the deferred-order rule select the oldest open bugfinder finding after deferred exhaustion without asking. Standing owner pre-approval covers properly tested Project merges of that selected repair. This stage does not merge production `main`, publish production Pages, or flash.

Approved for Stage 2: BF-004 ownership-gated Note Off / BREAK off + host regressions as above.

Not approved in this repair: BF-005, BF-006, BF-007, withheld asset cache, unmerged data-safety physical-endpoint lanes, HTML initial-loader resume paths, lengthening `SLOT_PROBE`, deleting `web/make_site.py` entirely, reopening BF-001–BF-003 or FP-001, force-pushing firmware `nightly`, production Pages.

## Missing decisions

None block the BF-004 ownership gate above. Optional later (not this Stage 2): exact TRS/`src>=2` policy if host probes expose an edge; whether Felucca-era `make_site.py` should be deleted entirely vs remaining installer-only; whether longer `SLOT_PROBE` budgets are worth a separate hardening cycle; historical F06 “Installer FP” wording rewrite as docs-only.

## Defects considered and not selected

### Already fixed in `6df0f81f`

| Item | Disposition |
| --- | --- |
| BF-001 local `.fwsc` / ctl selection race | Present (`d11b74fc`). Closed. |
| BF-002 scoped quickcheck project suites | Present (`ab90f47c`). Closed. |
| BF-003 retire stale editor test / site-builder | Present (`1325f5d3`). Closed. |
| F03 fail-closed slot layout probe | Present (`4f18983a`). Closed. |
| F02 fail-closed loader attach | Present (ancestor). Closed. |
| FP-001 dual-digest reconciliation | Present (`6df0f81f`). Closed. Source FP ≠ served HTML still true by design. |

### Deferred list (`evidence/f07-stage2/DEFERRED.md`)

| Item | Disposition |
| --- | --- |
| Withheld asset cache | Excluded / not an open tip source defect (prior audits). |
| Unmerged data-safety identity lanes | Explicitly not imported. |
| HTML resume `find(IS_OTA)` | Left as initial/interrupted-loader resume by F02 design. |
| Longer `SLOT_PROBE` budgets | Optional hardening only after F03 fail-closed. |
| Delete Felucca-era `make_site.py` entirely | Decision still open after BF-003 installer-only trim; callers (`make_pages` / `publish_nightly` / `test_breaks_page`) still import helpers — not a tip FAIL. |
| Nightly-repo second qualification field | Not required: FP-001 dual-digest + host check. |
| Historical F06 Stage-7 “Installer FP” wording rewrite | Docs debt; current F07 Stage-7 CHECKS already dual-labels; not a tip FAIL. |

### Recorded failing checks

No open **FAIL** on this validated SHA outside GitHub Actions (ignored). Stage-3 F07 regressions ALL_PASS; Stage-4 Elite/Pro DEV CHECKED; Stage-7 published live with matching **package** hashes. Nightly FF REFUSED is divergence, not a package/host FAIL. Optional Pillow host gaps are not-run, not FAIL.

### Younger open bugfinder (not selected)

| Item | Disposition |
| --- | --- |
| BF-005 `seq_clk_drop` / `ext_pending` | Still open on PR #26 lane; younger than BF-004. |
| BF-006 drum808 `d8_exp2_cr` signed shift UB | Still open; younger. |
| BF-007 legacy BD PAN motion migration order | Still open; younger. |

## Stage 2 limit

Implement only BF-004 (ownership-gated Note Off / BREAK off + host tests), on a new stage-2 branch from the pushed tip of `cursor/f08-bf004-stage1-fb90`. Do not take other deferred or younger bugfinder items in that same change.
