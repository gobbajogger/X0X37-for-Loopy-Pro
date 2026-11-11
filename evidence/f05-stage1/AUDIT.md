# Stage 1 audit of validated source 0d79d634

Audited tree: `0d79d63485dc6585800a71ef716d095445f5a1fc` (validated source published on nightly; not the diverged firmware `nightly` tip).
Content tree (Stage-4 DEV CHECKED records): `1b067cadd1e7887415354c4f384a51f93d6437c367574e2cee52b0b44d829479`.
Git tree (`source_tree`): `603e2a2b1442a8214cd97b03d32bf57fc158feac`.
BF-001 repair commit present in tip: `d11b74fc`.
F03 repair commit present in tip: `4f18983a` (not re-opened).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this record: `cursor/f05-bf002-stage1-fb90`.
This thread: `bc-bd9cb2ca-4784-5d48-968d-bee42f593ff1` (next-cycle Stage-1 audit after F04 Stage 7 publish).
Prior cycle audited SHA: `a03cfab3a423ab5c65a6546afd4993fe51754ee7`.
Evidence-bearing base: remote tip of `cursor/f04-bf001-stage7-fb90` (docs/evidence only; packages authenticate `0d79d634`).

No production source was edited. Production `main` and production GitHub Pages were not updated. No device was flashed. Firmware `nightly` was not force-pushed (still diverged / FF refused at `08d3164a…`).

## Published page (re-verified this audit)

Installer: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/

| Check | Result |
| --- | --- |
| `latest` in live `versions.json` | `nightly-0d79d634-dev` |
| Label / `hardware_tested` | `DEV / HARDWARE TEST — DEVICE NOT TESTED` / false |
| Elite bundled re-fetch | size 651396, sha256 `8ba23a89f50c70b631927077ec4cfe7c98593870035455261842f1b245b82278` **MATCH** |
| Pro bundled re-fetch | size 651392, sha256 `d90932115ff47cf421b0ef5caab3b93e730da5810e5b26d4c3ecd2c61c58fecb` **MATCH** |
| Stage-4 blank Elite / Pro (records) | `d3b4cac3…` / `0b09605f…` (unchanged from Stage-4 DEV CHECKED) |
| `controller_sha256` (bundled) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| Host tip | `artifacts/nightly-dev` @ `f917b636ba00d7dbef18fd00d1f0af15299a0a24` |
| Pages tip | `38b5d2a7d6380a4b9fd9ea1ff28d45b53d826569` |
| Catalog `source_commit` / `source_tree` | `0d79d634…` / `603e2a2b…` **MATCH** |
| Live install HTML markers | `d11b74fc` and `selection generations` **present** |
| Live install HTML sha256 | `3cd34559ee18de416171c0d6c51b914bf9e231f01cea3d98dd3e8b319af80145` (raw.githubusercontent same tip **identical**) |
| Stage-7 qualification `installer_sha256` pin | `2240be8702176544945a4b66171269e958b47259fb275654d2f6ff7125f637ee` — **does not match** the tip’s `install/index.html` bytes |

Observation: Stage-7’s recorded installer fingerprint does not equal the served (and git) install HTML on the same Pages tip. Package digests, catalog identity, and BF-001 content markers still match. This is an evidence-pin mismatch, not a package FAIL and not selected ahead of BF-002.

Cited blobs at `0d79d634`:

| File | Blob |
| --- | --- |
| `tools/quickcheck_plan.py` | `52f1f06faddea0f6bf6c2c89c9b41c50906a66fd` |
| `web/test_web.mjs` | `7fbe81bf4905600c92613db7321449106f3e7a31` |
| `web/x0x_installer.html` | `a8d2c75b96dd4a6a096bae48bbc15cdaeb0ca22f` (BF-001 fixed: `stockLoadSerial` / `ctlLoadSerial`) |

BF definition source (not on tip; open docs-only PR): [PR #26](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/26) / `docs/audit/bugfinder-watch-2026-10-23.md` on `lane/bugfinder-reports-20261023`.

## Selected defect

**BF-002. Scoped quickcheck omits project web regressions.**

Oldest deferred defect from `evidence/f04-stage2/DEFERRED.md` that is **not** already fixed in `0d79d634`. BF-001 is fixed (`d11b74fc`) and is not selected.

## Evidence

`tools/quickcheck_plan.py` maps every `web/` path to `breaks-web breaks-page install-slots-web` and never names `projects-web` or `projects-page`:

```30:30:tools/quickcheck_plan.py
    ('web/', 'breaks-web breaks-page install-slots-web'),
```

`select()` indexes only under `tests/` for basename inference; it does not walk `web/test_projects*.mjs`, so edits to `web/x0x_projects.js`, `web/x0x_projects.html`, or shared `web/x0x_breaks.js` do not pull in the project suites. Those suites are defined in `tests/run_tests.sh` (`projects-web` → `node web/test_projects.mjs`, `projects-page` → `node web/test_projects_page.mjs`).

Local reproduction on this tip (planner blob matches `0d79d634`):

| Changed path | `projects-web` / `projects-page` selected? |
| --- | --- |
| `web/x0x_projects.js` | **no** / **no** |
| `web/x0x_projects.html` | **no** / **no** |
| `web/x0x_breaks.js` | **no** / **no** |

Log: `evidence/f05-stage1/bf002-repro.log`.

## Affected scope

- `tools/quickcheck_plan.py`: add explicit project-suite routes for project JS/HTML and the shared `x0x_breaks.js` dependency (and when project tests themselves change).
- `tests/host/test_quickcheck_plan.py`: fixture that advertises both project commands; assert both suites are selected for production-module edits; keep unrelated UI changes scoped.
- Out of scope: firmware, controller image bytes, package application bytes, production `main`, production Pages, device flash, BF-001 (already fixed), BF-003, withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, Stage-7 installer fingerprint pin reconciliation (evidence-only unless Stage 2 finds it blocks the repair).

## Proposed fix

1. Route project sources (`web/x0x_projects.js`, `web/x0x_projects.html`, and shared `web/x0x_breaks.js` as needed) so scoped quickcheck includes `projects-web` and `projects-page`.
2. Extend `tests/host/test_quickcheck_plan.py` with a red/green fixture for those routes.
3. Run the affected planner host tests and the project web/page suites on the resulting head (local host/node only; no GitHub Actions).
4. Implement on a new `cursor/<slug>-stage2-fb90` branch from the pushed tip of `cursor/f05-bf002-stage1-fb90`.

No firmware build required. Development configuration if ever needed: `X0X_DEV_PADCAL=1`.

## Approvals

The full-cycle procedure (`AGENTS.md` + `docs/rules/workflow.md`), the Stage-7 successor prompt, and the deferred-order rule select BF-002 as the next defect without asking. PR #26 already records the finding and suggested fix outline. Standing owner pre-approval covers properly tested Project merges of that selected repair. This stage does not merge production `main`, publish production Pages, or flash.

Not approved in this repair: BF-003 (needs retire-vs-restore decision), withheld asset cache, unmerged data-safety physical-endpoint lanes, HTML initial-loader resume paths, lengthening `SLOT_PROBE`, reopening BF-001, rewriting Stage-7 qualification pins as a Stage-2 source change.

## Missing decisions

None block the BF-002 planner-route repair. BF-003 still needs a retire-versus-restore decision before any Stage-2 change to `web/test_web.mjs` / editor — that decision is **not** required for BF-002. Optional: whether Stage-7’s mismatched installer fingerprint pin should be corrected in a later docs/evidence pass (not this Stage-2 code repair).

## Defects considered and not selected

### Already fixed in `0d79d634`

| Item | Disposition |
| --- | --- |
| BF-001 local `.fwsc` / ctl selection race | Present (`d11b74fc`, `stockLoadSerial` / `ctlLoadSerial`). Closed on this SHA. Live page still carries markers. |
| F03 fail-closed slot layout probe | Present (`4f18983a`). Closed on this SHA. |
| F02 fail-closed loader attach | Present (ancestor). Closed on this SHA. |

### Deferred list (`evidence/f04-stage2/DEFERRED.md`)

| Item | Disposition |
| --- | --- |
| BF-002 | **Selected.** Still present on tip; reproduced. |
| BF-003 | Younger. Stale `web/test_web.mjs` → missing `editor.html`. Needs retire/restore decision. Defer. |
| Withheld asset cache | Excluded / not an open tip source defect (prior audits). |
| Unmerged data-safety identity lanes | Explicitly not imported. |
| HTML resume `find(IS_OTA)` | Left as initial/interrupted-loader resume by F02 design; not a commanded-transition wait. |
| Longer `SLOT_PROBE` budgets | Optional hardening only after F03 fail-closed; still 1.0s/2 native and 1000ms/2 web. |

### Recorded failing checks

No open **FAIL** on this validated SHA outside GitHub Actions (ignored). Stage-3 F04 regressions ALL_PASS; Stage-4 Elite/Pro DEV CHECKED; Stage-7 published live with matching package hashes. Nightly FF REFUSED is divergence, not a package/host FAIL. Optional Pillow host gaps are not-run, not FAIL. The Stage-7 installer fingerprint pin mismatch is an evidence discrepancy against matching live bytes — not selected over BF-002.

## Stage 2 limit

Implement only BF-002, with the planner route and host fixture above, on a new stage-2 branch from the pushed tip of `cursor/f05-bf002-stage1-fb90`. Do not take BF-003 or other deferred items in that same change. Do not change firmware, controller, or package bytes.
