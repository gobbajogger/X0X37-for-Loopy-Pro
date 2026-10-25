# Stage 1 audit of validated source de11b6d7 (F13 after F12 LIVE_PAGE_MATCH)

Audited tree (primary): `de11b6d7e3ac3322d2db689675d9ed27ed667785` — validated / published BF-008 source with LIVE_PAGE_MATCH `nightly-de11b6d7-dev` and Stage 6 PASS record `e88f1ce2eb88ae4a` on Elite_900.
Git `source_tree`: `c143a9f8f5b5d0f6804995829ccbe515c77c6103`.
Firmware tree: `9195dce5c4beb5fa8aa564504c8d1d114c8264cd`.
Cross-check tip: `origin/nightly` @ `5e6d7e4ea387c42722d2704bdb43ef04f7caeda3` (post-#279 reunify + later BF merges; contains `de11b6d7` as ancestor). Same unfixed `master.c` blob `b5557b6c…` as the audited SHA.
Why this tip: workflow Stage 1 audits the named validated/published SHA even when `nightly` moves; reunify made `de11b6d7` an ancestor of `nightly`, so both tips agree on this defect.
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this record: `cursor/f13-stage1-audit-237f`.
This thread: `bc-09959f6e-dc18-5575-baa6-444dfdf698cc` (A4 / FCDC-187 successor Stage 1).
Prior cycle audited / published SHA: `de11b6d7…` (F12 selected BF-008; now fixed in tip).
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007), `de11b6d7` (BF-008).

No production source was edited. Production `main` and production GitHub Pages were not updated. No device was flashed. Firmware `nightly` was not force-pushed. Do not steal #279/#281 sole-owner leftover Arp/Fx work.

## Published page (re-verified this audit)

Installer: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/

| Check | Result |
| --- | --- |
| `latest` in live catalog | `nightly-de11b6d7-dev` |
| Label / `hardware_tested` | `DEV / HARDWARE TEST — DEVICE NOT TESTED` / false |
| `source_commit` / `source_tree` | `de11b6d7…` / `c143a9f8…` **MATCH** tip |
| Elite bundled re-fetch | size 651396, sha256 `4841110a…d18a` **MATCH** |
| Pro bundled re-fetch | size 651392, sha256 `b35d5875…c720` **MATCH** |
| `controller_sha256` (bundled) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| Stage 6 local install | PASS `e88f1ce2eb88ae4a` Elite blank `829bb6da…` / ctl `956e3e21…` (Project store `internal/f12-stage6-flash.md`) |
| Result | **LIVE_PAGE_MATCH** |

Log: `evidence/f13-stage1/live-verify.log`.

Cited blobs at `de11b6d7`:

| File | Blob |
| --- | --- |
| `firmware/src/dsp/master.c` | `b5557b6c41bdf45275547a3b6c5431d5252f62f0` (**BF-012 baseline**; no OFF→ON ring clear) |
| `tests/host/master_test.c` | `76bc500af87d52cbddcfb97fee7c5b95276f6ae2` (no BF-012 case) |
| `firmware/src/dsp/fastmath.h` | BF-008 fixed (`b.u += (uint32_t)n << 23;`) |
| `web/make_site.py` | present (BF-003 trim done; full delete still deferred / decision-open) |

PR #68 repair tip blobs: `master.c` `f91a9473…`, `master_test.c` `df2f13b3…` @ `99543b84…`.

Host witness (this audit): tip `master` baseline PASS; tip + PR #68 regression overlay **FAIL** mono/stereo stale peak `0.500000`; scratch PR #68 sources **PASS**. Logs: `bf012-witness.log`, `bf012-scratch-clear.log`, `master-tip-baseline.log`. Scratch is not a committed fix. `X0X_TEST_JOBS=8`.

## Selected defect

**BF-012. MASTER LIMIT OFF→ON replays stale look-ahead ring audio (`firmware/src/dsp/master.c`).**

Oldest open audit/bugfinder finding after deferred-list exhaustion on `de11b6d7`. BF-001–BF-008 and FP-001 are fixed and are not selected. Deferred items from `evidence/f12-stage2/DEFERRED.md` remain excluded, design, optional, decision-open, not-required, or docs debt — none are tip source FAILs ahead of this occurrence. Open PR #68 tracks this exact tip FAIL (same `master.c` blob on audited SHA and on current `origin/nightly`).

## Evidence

Definition: open PR #68 plus `evidence/f13-stage1/bf012-definition.md`. Baseline tip blob `b5557b6c…` identical at audited SHA and at `origin/nightly`.

In `firmware/src/dsp/master.c` `master_set`:

- Bypass leaves look-ahead rings unadvanced.
- Tip has no clear when `i == MST_LIMIT && v > 0 && !m->pot[MST_LIMIT]`.
- Expected: silent input after OFF→ON yields peak 0.0; fresh signal still passes after look-ahead; LIMIT=ON while already ON does not reset the live ring.

Executable witness: tip FAIL under PR #68 host regression overlay; scratch clear of rings on re-enable exits 0.

## Affected scope

- `firmware/src/dsp/master.c` — on LIMIT off→on, zero mono/right look-ahead rings and reset limiter hold/env/gain state (same intent as PR #68).
- `tests/host/master_test.c` — add mono/stereo OFF→ON stale-audio + live-ring-preserve cases.
- Out of scope: production `main`, production Pages, device flash, BF-001–BF-008, FP-001, deferred non-FAIL items, Arp/Fx sole-owner leftovers from #279/#281, merging unrelated PR #68 base history beyond the two paths above, force-push `nightly`.

## Proposed fix

**Decision for this cycle: repair BF-012 only.**

1. When LIMIT transitions from off to on in `master_set`, clear `la` / `la_r`, reset `la_pos` / `hold` / `env` / `inv_env`, set `gain = 1.0f` (scratch-proven; matches open PR #68 head).
2. Do not change limiter knee / ceiling / look-ahead length intent.
3. Prefer re-applying only `firmware/src/dsp/master.c` + `tests/host/master_test.c` from PR #68 onto a fresh stage-2 branch from this audit tip — do not import unrelated base-lane history as the cycle tip.
4. Host tests must FAIL on today’s tip and PASS after the repair (`X0X_TEST_JOBS≥8`).
5. Implement on a new `cursor/<slug>-stage2-237f` branch from the pushed tip of `cursor/f13-stage1-audit-237f`. Use `X0X_DEV_PADCAL=1` for any firmware/package rebuild in later stages.

## Approvals

The full-cycle procedure (`AGENTS.md` + `docs/rules/workflow.md`) and the deferred-order rule select the oldest open finding after deferred exhaustion without asking. Standing owner pre-approval covers properly tested Project merges of that selected repair. This stage does not merge production `main`, publish production Pages, or flash.

Approved for Stage 2: BF-012 limiter OFF→ON ring clear + host regressions as above.

Not approved in this repair: withheld asset cache, unmerged data-safety physical-endpoint lanes, HTML initial-loader resume paths, lengthening `SLOT_PROBE`, deleting `web/make_site.py` entirely, reopening BF-001–BF-008 or FP-001, Arp/Fx leftover ports, force-pushing firmware `nightly`, production Pages.

## Missing decisions

None block the BF-012 repair above. Optional later (not this Stage 2): whether Felucca-era `make_site.py` should be deleted entirely vs remaining installer-only; whether longer `SLOT_PROBE` budgets are worth a separate hardening cycle; historical F06 “Installer FP” wording rewrite as docs-only; whether open PR #68 should be closed as superseded once this cycle’s tip lands the same fix.

## Defects considered and not selected

### Already fixed in `de11b6d7`

| Item | Disposition |
| --- | --- |
| BF-001 … BF-008, FP-001, F02/F03 | Present in tip. Closed. Do not reopen. |
| Open PR #28 fastmath signed-shift | Tip already has defined unsigned shift (BF-008). Superseded as tip FAIL. |

### Deferred list (`evidence/f12-stage2/DEFERRED.md`)

| Item | Disposition |
| --- | --- |
| Withheld asset cache | Excluded / not an open tip source defect. |
| Unmerged data-safety identity lanes | Explicitly not imported. |
| HTML resume `find(IS_OTA)` | Left as initial/interrupted-loader resume by F02 design. |
| Longer `SLOT_PROBE` budgets | Optional hardening only after F03 fail-closed. |
| Delete Felucca-era `make_site.py` entirely | Decision still open after BF-003 installer-only trim; callers still import helpers — **not a tip FAIL** (do not select ahead of BF-012). |
| Nightly-repo second qualification field | Not required: FP-001 dual-digest + host check. |
| Historical F06 Stage-7 “Installer FP” wording rewrite | Docs debt; not a tip FAIL. |

### Recorded failing checks

No open **package/host FAIL** on this validated SHA outside GitHub Actions (ignored) and the selected BF-012 host witness. Stage 4 Elite/Pro DEV CHECKED for BF-008; Stage 6 PASS; Stage 7 LIVE_PAGE_MATCH. Optional Pillow/ASan host gaps are not-run where missing, not FAIL. Younger BF merges on `origin/nightly` (BF-013+) are already landed there and are not selected ahead of older open BF-012 on the published tip.

### Explicitly not stolen

#279 reunify leftovers and #281 amp ADSR / Arp / Fx sole-owner work — out of this Stage 1 selection.

## Stage 2 limit

Implement only BF-012 (`master_set` OFF→ON ring clear + host regressions), on a new stage-2 branch from the pushed tip of `cursor/f13-stage1-audit-237f`. Do not take other deferred items in that same change.
