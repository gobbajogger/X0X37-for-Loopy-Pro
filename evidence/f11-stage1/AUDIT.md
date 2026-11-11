# Stage 1 audit of validated source af494784

Audited tree: `af4947846df7233c68a6be72c4d0c634052a2752` (validated source published on nightly; not the diverged firmware `nightly` tip).
Content tree (Stage-4 DEV CHECKED records): `1da3bbfbbf0df52256acceecbca2e064a1ef821478a33d78591e01813229643d`.
Git tree (`source_tree`): `c12a7e747b79aa4619466764ecccc88285d4c10d`.
Firmware tree: `bcc21733ea77097a787cfa6cc617fbdf1dca5da1`.
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`.
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this record: `cursor/f11-bf007-stage1-fb90`.
This thread: `bc-1751adc0-5254-5401-a54e-48e433f242b9` (next-cycle Stage-1 audit after F10 Stage 7 publish).
Prior cycle audited SHA: `d0d00da23c3ce2268f06f31f8c062716d5a37ad0` (F10 selected BF-006; now fixed in `af494784`).
Evidence-bearing base: remote tip of `cursor/f10-bf006-stage7-fb90` (docs/evidence only; packages authenticate `af494784` / tree `1da3bbfb…`).

No production source was edited. Production `main` and production GitHub Pages were not updated. No device was flashed. Firmware `nightly` was not force-pushed (still diverged / FF refused at `08d3164a…`).

## Published page (re-verified this audit)

Installer: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/

| Check | Result |
| --- | --- |
| `latest` in live `versions.json` | `nightly-af494784-dev` |
| Label / `hardware_tested` | `DEV / HARDWARE TEST — DEVICE NOT TESTED` / false |
| Elite bundled re-fetch | size 651396, sha256 `9cba1f3d0bebc2113d59b9bbb3f1ca1a67bb396183be279b74486319ddae8e11` **MATCH** |
| Pro bundled re-fetch | size 651392, sha256 `5dda4eecd74d96237a3d457a026d7bc04eb3ed570705179ab8e6c15a198fa739` **MATCH** |
| Stage-4 blank Elite / Pro (records) | `de0bb077…` / `25bebb26…` (unchanged from Stage-4 DEV CHECKED) |
| `controller_sha256` (bundled) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| Host tip | `artifacts/nightly-dev` @ `2363c07f334747208831923a4f9fa4332cbbe472` |
| Pages tip | `f45cae0956fba10218ed54d9b95ba5a8c7a4b63e` |
| Catalog `source_commit` / `source_tree` | `af494784…` / `c12a7e74…` **MATCH** |
| Live install HTML markers | `af494784`, `BF-006`, `nightly-af494784-dev`, `DEVICE NOT TESTED` **present** |
| Served install HTML sha256 | `190a9a9ac18a30e6b4fd9091cdcda34cb4b65aeaa0d351368084012688bdb52e` |
| Installer fingerprint (qualification) | `4d29566e14c3f252bf52047676e993367e3d155aa1a94a45157ec07d77e47dc8` |
| Dual-digest distinct (FP-001) | **YES** (source FP ≠ served HTML; do not reopen FP-001) |
| Result | **LIVE_PAGE_MATCH** |

Independent claim review (internal Task): `bc-b123ad99-27b7-5044-b4c6-efd6eccf2b4e` — **8/8 PASS**.
Research (internal Task): `bc-cc91a2a4-a49a-5c31-964b-ccda991e032e` — select BF-007.
Stage-7 evidence local review: Project store `internal/pr-local-reviews/pr-256.md` — cite when present.
Package bytes local review (unchanged): Project store `internal/pr-local-reviews/pr-253.md` — Overall **PASS**.

Cited blobs at `af494784`:

| File | Blob |
| --- | --- |
| `firmware/src/app/project.c` | `1000a0d88e04553295b82d724a63670434b67748` (**BF-007 baseline match**) |
| `firmware/src/dsp/drum808.c` | `90f463295fb1a642bad81700d73fc22eb8ec7a38` (BF-006 fixed) |
| `firmware/src/dsp/fastmath.h` | `7d2e41bf70590bafa8af0a7376bb912bf763b098` (PR #28 still open; not selected) |
| `firmware/src/seq/sequencer.c` | `fd3f65306df166494d10facf8ddae058287cc17b` (BF-005 fixed) |
| `web/make_site.py` | present (BF-003 fixed; delete still deferred) |
| `tools/check_installer_digests.py` | present (FP-001 fixed) |

Host witness (this audit): PR #26 lane harness `tests/host/bugfinder_legacy_bdpan_motion_test.c` against tip `project.c` — **FAIL** exit 1, 171 checks / 3 FAIL (legacy 909/808 BD PAN motion indices). Scratch-only reorder: migrate BD PAN sound+lane after arrangement/stream load — **PASS** exit 0, 171/171. Logs: `bf007-witness.log`, `bf007-scratch-clear.log`. Scratch is not a committed fix.

## Selected defect

**BF-007. Loading a legacy project loses 909/808 Bass Drum PAN motion mapping.**

Oldest open audit/bugfinder finding after deferred-list exhaustion on `af494784`. BF-001–BF-006 and FP-001 are fixed and are not selected. Deferred items from `evidence/f10-stage2/DEFERRED.md` remain excluded, design, optional, decision-open, not-required, docs debt, or separate younger open (PR #28) — none are selected ahead of BF-007 under the prior-cycle bugfinder order.

## Evidence

Definition (docs-only lane, open draft PR #26): `docs/audit/bugfinder-legacy-bdpan-motion-20261023.md` on `lane/bugfinder-reports-20261023` (copy: `evidence/f11-stage1/bf007-definition.md`). Baseline main `c9718ae8…`; `project.c` blob `1000a0d88e04553295b82d724a63670434b67748` — identical at audited SHA `af494784`.

In `firmware/src/app/project.c`:

- `sound_bd_sends()` (~493–504) migrates both BD PAN sound values and matching motion lane indices (`l->i = pan + 2`).
- `read_project()` invokes it for 909 (`snd_rev < 3`, pan=8) and 808 (`snd_rev < 4`, pan=7) at ~549–556 immediately after `OBJ_SOUND`.
- Legacy motion lanes and compressed arrangement load only afterward (~561–578 via `load_stream` / `load_arrange1`). Lane migration therefore sees defaults.
- Old projects retain motion index 8 for 909 (expected 10) and 7 for 808 (expected 9), now corresponding to send controls. Static PAN values still migrate.

Executable witness: tip FAIL 3/171; scratch move of `sound_bd_sends` calls to after arrangement/stream load PASS 171/171.

Log: `evidence/f11-stage1/live-verify.log`.

## Affected scope

- `firmware/src/app/project.c` — split or reorder so lane migration runs only after arrangement/lanes are loaded; keep static PAN migration correct (scratch moved both migrations after load and cleared the harness).
- Host regressions: promote or add `bugfinder_legacy_bdpan_motion_test.c` (red on tip / green after repair); keep existing project-stage / project-runtime suites green.
- Out of scope: production `main`, production Pages, device flash, BF-001–BF-006, FP-001, withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, deleting Felucca-era `make_site.py` entirely, nightly-repo second qualification field, historical F06 “Installer FP” wording rewrite, merging open PR #28 `fastmath.h` into this Stage-2 change.

## Proposed fix

**Decision for this cycle: repair BF-007 only** (legacy BD PAN motion migration order).

1. Ensure BD PAN motion lane indices migrate only after pattern/arrangement (and motion lanes) are loaded.
2. Prefer splitting static sound-value migration from lane migration if needed so static PAN still migrates at the sound-load site; scratch proved moving both after arrangement load also clears the harness (171/171).
3. Do not fold open PR #28 `fastmath.h` edits into this repair.
4. Add or wire host tests that fail on today’s tip (lane index 8/7 retained) and pass after the repair; keep existing project suites green.
5. Implement on a new `cursor/<slug>-stage2-fb90` branch from the pushed tip of `cursor/f11-bf007-stage1-fb90`. Use `X0X_DEV_PADCAL=1` for any firmware/package rebuild in later stages.

## Approvals

The full-cycle procedure (`AGENTS.md` + `docs/rules/workflow.md`), the Stage-7 successor prompt, and the deferred-order rule select the oldest open bugfinder finding after deferred exhaustion without asking. Standing owner pre-approval covers properly tested Project merges of that selected repair. This stage does not merge production `main`, publish production Pages, or flash.

Approved for Stage 2: BF-007 legacy BD PAN motion migration order + host regressions as above.

Not approved in this repair: PR #28 fastmath UB, withheld asset cache, unmerged data-safety physical-endpoint lanes, HTML initial-loader resume paths, lengthening `SLOT_PROBE`, deleting `web/make_site.py` entirely, reopening BF-001–BF-006 or FP-001, force-pushing firmware `nightly`, production Pages.

## Missing decisions

None block the BF-007 migration-order repair above. Optional later (not this Stage 2): whether Felucca-era `make_site.py` should be deleted entirely vs remaining installer-only; whether longer `SLOT_PROBE` budgets are worth a separate hardening cycle; historical F06 “Installer FP” wording rewrite as docs-only; whether/when to land open PR #28 fastmath UB repair as its own cycle.

## Defects considered and not selected

### Already fixed in `af494784`

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
| BF-006 808 `d8_exp2_cr` defined exponent bias | Present (`37bab7d1`; drum808 blob `90f46329…`). Closed. |

### Deferred list (`evidence/f10-stage2/DEFERRED.md`)

| Item | Disposition |
| --- | --- |
| Withheld asset cache | Excluded / not an open tip source defect (prior audits). |
| Unmerged data-safety identity lanes | Explicitly not imported. |
| HTML resume `find(IS_OTA)` | Left as initial/interrupted-loader resume by F02 design. |
| Longer `SLOT_PROBE` budgets | Optional hardening only after F03 fail-closed (still 1.0s/2). |
| Delete Felucca-era `make_site.py` entirely | Decision still open after BF-003 installer-only trim; callers still import helpers — not a tip FAIL. |
| Nightly-repo second qualification field | Not required: FP-001 dual-digest + host check. |
| Historical F06 Stage-7 “Installer FP” wording rewrite | Docs debt; current Stage-7 CHECKS already dual-labels; not a tip FAIL. |
| Open PR #28 `fastmath.h` signed-shift UB | Separate occurrence / younger deferred entry relative to BF-007 selection order; tip still unfixed (`7d2e41bf…`) — **not selected** this cycle. |

### Recorded failing checks

No open **FAIL** on this validated SHA outside GitHub Actions (ignored). Stage-3 F10 regressions ALL_PASS; Stage-4 Elite/Pro DEV CHECKED; Stage-7 published live with matching **package** hashes. Nightly FF REFUSED is divergence, not a package/host FAIL. Optional Pillow/ASan host gaps are not-run where missing, not FAIL.

### Younger / separate open (not selected)

| Item | Disposition |
| --- | --- |
| PR #28 `fastmath.h` `fm_exp2_nf` signed shift | Still open; tip blob matches unfixed baseline; leave for its own cycle. |

## Stage 2 limit

Implement only BF-007 (legacy BD PAN motion migration order + host tests), on a new stage-2 branch from the pushed tip of `cursor/f11-bf007-stage1-fb90`. Do not take other deferred or separate-lane items in that same change.
