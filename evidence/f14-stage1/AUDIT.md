# Stage 1 audit of validated source b42bcba4 (F14 after F13 LIVE_PAGE_MATCH)

Audited tree (primary): `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675` — validated / published BF-012 source with LIVE_PAGE_MATCH `nightly-b42bcba4-dev`.
Git `source_tree`: `a15afc439fc8b7bccd62d50403b925add45ae9e5`.
Content tree (non-`.md`, Stage-4/7 DEV CHECKED): `519ccdf21b46db9198993587cb9334960e67ce483641c744c5caf71b1c769141`.
Firmware tree: `5facb3b4405d119a269a05224b9268adfa1baa13`.
Cross-check tip: `origin/nightly` @ `bd5b7236…` (contains BF-013 merge `f62ff102` / PR #74; audited published SHA does **not**).
Why this tip: workflow Stage 1 audits the named validated/published SHA even when firmware `nightly` diverged or moved; Stage 5 refused FF of `nightly` to `b42bcba4`.
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this record: `cursor/f14-bf013-stage1-237f`.
This thread: `bc-4c124351-a5c1-53ea-a243-b3b478f7bc15` (F14 Stage 1 after F13 Stage 7).
Prior cycle audited / published SHA: `b42bcba4…` (F13 selected BF-012; now fixed in tip).
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007), `de11b6d7` (BF-008), `6636cca1`/`b42bcba4` (BF-012).

No production source was edited. Production `main` and production GitHub Pages were not updated. No device was flashed. Firmware `nightly` was not force-pushed. Do not steal #279/#281 sole-owner leftover Arp/Fx work (`bc-8efb2f94…`).

## Published page (re-verified this audit)

Installer: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/

| Check | Result |
| --- | --- |
| `latest` in live catalog | `nightly-b42bcba4-dev` |
| Label / `hardware_tested` | `DEV / HARDWARE TEST — DEVICE NOT TESTED` / false |
| `source_commit` / `source_tree` | `b42bcba4…` / `a15afc43…` **MATCH** tip |
| Elite bundled re-fetch | size 651396, sha256 `82ee29d3…131b14` **MATCH** |
| Pro bundled re-fetch | size 651392, sha256 `bc6c0422…bfad34` **MATCH** |
| `controller_sha256` (bundled) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| Result | **LIVE_PAGE_MATCH** |

Log: `evidence/f14-stage1/live-verify.log`.

Cited blobs at `b42bcba4`:

| File | Blob |
| --- | --- |
| `firmware/src/dsp/fxbus.c` | `4c7485391d3a462ebe9f4907abfaf4867ce80b54` (**BF-013 baseline**; `>20` rejects exact 20 BPM) |
| `tests/host/stereo_test.c` | `cdb25a6f099c2f4797acf0004637ce6d2810fcea` (no BF-013 cases) |
| `firmware/src/dsp/master.c` | BF-012 fixed (`f91a9473…`) |
| `web/make_site.py` | present (BF-003 trim done; full delete still deferred / decision-open) |

PR #74 repair blobs: `fxbus.c` `49a89483…`, `stereo_test.c` `c8bc1092…`.

Host witness (this audit): tip `stereo` baseline PASS; tip + PR #74 regression overlay **FAIL** four 20 BPM assertions; scratch PR #74 sources **PASS**. Logs: `bf013-witness.log`, `bf013-scratch-clear.log`, `stereo-tip-baseline.log`. Scratch is not a committed fix. `X0X_TEST_JOBS=8`.

## Selected defect

**BF-013. Delay sync rejects exact 20 BPM (`firmware/src/dsp/fxbus.c`).**

Oldest open audit/bugfinder finding after deferred-list exhaustion on `b42bcba4`. BF-001–BF-008, BF-012, and FP-001 are fixed in the audited tip and are not selected. Deferred items from `evidence/f13-stage2/DEFERRED.md` remain excluded, design, optional, decision-open, not-required, or docs debt — none are tip source FAILs ahead of this occurrence. PR #74 / lane `bugfinder-fx-delay-min-tempo-20261023-2115` tracks this exact tip FAIL (same `fxbus.c` blob `4c748539…` on audited SHA). The fix is already on diverged `origin/nightly`; the published validated SHA still lacks it.

## Evidence

Definition: open PR #74 history plus `evidence/f14-stage1/bf013-definition.md`. Baseline tip blob `4c748539…` at audited SHA.

In tip `firmware/src/dsp/fxbus.c`:

- `fxbus_set_bpm` uses `bpm > 20.0f` (rejects 20).
- `fx_retime` uses `f->bpm > 20.0f ? f->bpm : 120.0f` (substitutes 120 at saved 20).
- Expected: comparisons `>= 20.0f` so UI/sequencer minimum tempo stays synchronized.

Executable witness: tip FAIL under PR #74 host regression overlay; scratch `>=20` clear exits 0.

## Affected scope

- `firmware/src/dsp/fxbus.c` — change the two `> 20.0f` tempo gates to `>= 20.0f` (same intent as PR #74).
- `tests/host/stereo_test.c` — add 20 BPM acceptance / 1125 ms dotted-eighth / subdivision retime / 19 BPM reject / high-tempo update cases.
- Out of scope: production `main`, production Pages, device flash, BF-001–BF-008, BF-012, FP-001, deferred non-FAIL items, Chicago Arp/Fx sole-owner leftovers from #279/#281, merging unrelated nightly history beyond the two paths above, force-push `nightly`.

## Proposed fix

**Decision for this cycle: repair BF-013 only.**

1. In `fxbus_set_bpm` and `fx_retime`, use `>= 20.0f` instead of `> 20.0f`.
2. Do not extend tempo acceptance below 20 BPM.
3. Prefer re-applying only `firmware/src/dsp/fxbus.c` + `tests/host/stereo_test.c` from PR #74 onto a fresh stage-2 branch from this audit tip — do not import unrelated nightly-only history as the cycle tip.
4. Host tests must FAIL on today’s tip and PASS after the repair (`X0X_TEST_JOBS≥8`).
5. Implement on a new `cursor/<slug>-stage2-237f` branch from the pushed tip of `cursor/f14-bf013-stage1-237f`. Use `X0X_DEV_PADCAL=1` for any firmware/package rebuild in later stages.

## Approvals

The full-cycle procedure (`AGENTS.md` + `docs/rules/workflow.md`) and the deferred-order rule select the oldest open finding after deferred exhaustion without asking. Standing owner pre-approval covers properly tested Project merges of that selected repair. This stage does not merge production `main`, publish production Pages, or flash.

Approved for Stage 2: BF-013 delay 20 BPM sync + host regressions as above.

Not approved in this repair: withheld asset cache, unmerged data-safety physical-endpoint lanes, HTML initial-loader resume paths, lengthening `SLOT_PROBE`, deleting `web/make_site.py` entirely, reopening BF-001–BF-008 / BF-012 / FP-001, Chicago Arp/Fx leftover ports, force-pushing firmware `nightly`, production Pages.

## Missing decisions

None block the BF-013 repair above. Optional later (not this Stage 2): whether Felucca-era `make_site.py` should be deleted entirely vs remaining installer-only; whether longer `SLOT_PROBE` budgets are worth a separate hardening cycle; historical F06 “Installer FP” wording rewrite as docs-only; how Stage 5 will eventually FF firmware `nightly` onto the published lineage after XIP/budget unlock.

## Defects considered and not selected

### Already fixed in `b42bcba4`

| Item | Disposition |
| --- | --- |
| BF-001 … BF-008, BF-012, FP-001, F02/F03 | Present in tip. Closed. Do not reopen. |

### Deferred list (`evidence/f13-stage2/DEFERRED.md`)

| Item | Disposition |
| --- | --- |
| Withheld asset cache | Excluded / not an open tip source defect. |
| Unmerged data-safety identity lanes | Explicitly not imported. |
| HTML resume `find(IS_OTA)` | Left as initial/interrupted-loader resume by F02 design. |
| Longer `SLOT_PROBE` budgets | Optional hardening only after F03 fail-closed. |
| Delete Felucca-era `make_site.py` entirely | Decision still open after BF-003 installer-only trim; callers still import helpers — **not a tip FAIL** (do not select ahead of BF-013). |
| Nightly-repo second qualification field | Not required: FP-001 dual-digest + host check. |
| Historical F06 Stage-7 “Installer FP” wording rewrite | Docs debt; not a tip FAIL. |
| Open PR #68 unrelated base-lane history | BF-012 already re-applied as two paths; closed as tip FAIL. |

### Recorded failing checks

No open **package/host FAIL** on this validated SHA outside GitHub Actions (ignored) and the selected BF-013 host witness. Stage 4 Elite/Pro DEV CHECKED for BF-012; Stage 7 LIVE_PAGE_MATCH. Optional Pillow/ASan / host-build rusage gaps are not-run or host-only noise where missing, not candidate FAIL. Younger BF merges already on `origin/nightly` (BF-014+) are not selected ahead of older open BF-013 on the published tip.

### Explicitly not stolen

#279 reunify leftovers and #281 amp ADSR / Arp / Fx sole-owner work (`bc-8efb2f94…`) — out of this Stage 1 selection. BF-013 is the bugfinder delay-tempo finding, not that Chicago leftover port lane.

## Stage 2 limit

Implement only BF-013 (`fxbus` `>=20` tempo gates + stereo host regressions), on a new stage-2 branch from the pushed tip of `cursor/f14-bf013-stage1-237f`. Do not take other deferred items in that same change.
