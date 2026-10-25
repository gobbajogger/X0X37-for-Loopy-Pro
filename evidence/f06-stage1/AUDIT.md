# Stage 1 audit of validated source ab90f47c

Audited tree: `ab90f47ccad0f213114d6a98c1bc6179be2ebb35` (validated source published on nightly; not the diverged firmware `nightly` tip).
Content tree (Stage-4 DEV CHECKED records): `0ee03318a0b64a1e535b36fad284558529ff7e3f95ff069efed86bbcf308518c`.
Git tree (`source_tree`): `50c381c528cae9692deff4a712071f7dc6acb020`.
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this record: `cursor/f06-bf003-stage1-fb90`.
This thread: `bc-a76a9628-9a50-5b04-a7a4-ba134f41d9e1` (next-cycle Stage-1 audit after F05 Stage 7 publish).
Prior cycle audited SHA: `0d79d63485dc6585800a71ef716d095445f5a1fc`.
Evidence-bearing base: remote tip of `cursor/f05-bf002-stage7-fb90` (docs/evidence only; packages authenticate `ab90f47c`).

No production source was edited. Production `main` and production GitHub Pages were not updated. No device was flashed. Firmware `nightly` was not force-pushed (still diverged / FF refused at `08d3164a…`).

## Published page (re-verified this audit)

Installer: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/

| Check | Result |
| --- | --- |
| `latest` in live `versions.json` | `nightly-ab90f47c-dev` |
| Label / `hardware_tested` | `DEV / HARDWARE TEST — DEVICE NOT TESTED` / false |
| Elite bundled re-fetch | size 651396, sha256 `8ba23a89f50c70b631927077ec4cfe7c98593870035455261842f1b245b82278` **MATCH** |
| Pro bundled re-fetch | size 651392, sha256 `d90932115ff47cf421b0ef5caab3b93e730da5810e5b26d4c3ecd2c61c58fecb` **MATCH** |
| Stage-4 blank Elite / Pro (records) | `d3b4cac3…` / `0b09605f…` (unchanged from Stage-4 DEV CHECKED) |
| `controller_sha256` (bundled) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| Host tip | `artifacts/nightly-dev` @ `f917b636ba00d7dbef18fd00d1f0af15299a0a24` |
| Pages tip | `e10823e38ec9e56f5ec381822b1597f7364a95d9` |
| Catalog `source_commit` / `source_tree` | `ab90f47c…` / `50c381c5…` **MATCH** |
| Live install HTML markers | `d11b74fc`, `BF-001`, `selection generations`, `nightly-ab90f47c-dev` **present** |
| Live install HTML sha256 | `0439c05891eb804ceb94c626a8eba919926f11d7454a81e706197f1466bde28b` (raw.githubusercontent same tip **identical**) |
| Stage-7 qualification `installer_sha256` pin | `2240be8702176544945a4b66171269e958b47259fb275654d2f6ff7125f637ee` — **does not match** the tip’s `install/index.html` bytes |

Observation: Stage-7’s recorded installer fingerprint does not equal the served (and git) install HTML on the same Pages tip. Package digests, catalog identity, and BF-001 content markers still match. This is an evidence-pin mismatch, not a package FAIL and not selected ahead of BF-003.

Cited blobs at `ab90f47c`:

| File | Blob |
| --- | --- |
| `web/test_web.mjs` | `7fbe81bf4905600c92613db7321449106f3e7a31` |
| `web/make_site.py` | `593bf56b4dde7f3a5138fe522210ec85d1159b46` |
| `tools/quickcheck_plan.py` | `e19d9b2240ab0c5796ef5e4486371a90b4e8aa5e` (BF-002 fixed: project suite routes) |
| `web/x0x_installer.html` | `a8d2c75b96dd4a6a096bae48bbc15cdaeb0ca22f` (BF-001 fixed) |
| `web/editor.html` | **absent** |
| `firmware/src/editor.c` | **absent** |

BF definition source (not on tip; open docs-only PR): [PR #26](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/26) / `docs/audit/bugfinder-watch-2026-10-23.md` on `lane/bugfinder-reports-20261023`.

## Selected defect

**BF-003. Retained editor test / site-builder references a file absent from the tree.**

Oldest deferred defect from `evidence/f05-stage2/DEFERRED.md` that is **not** already fixed in `ab90f47c`. BF-001 (`d11b74fc`) and BF-002 (`ab90f47c`) are fixed and are not selected.

## Evidence

`web/test_web.mjs` line 28 unconditionally reads `web/editor.html`:

```28:28:web/test_web.mjs
const html = readFileSync(join(HERE, "editor.html"), "utf8");
```

That path is absent at `ab90f47c` (and on this tip). Local reproduction:

| Probe | Result |
| --- | --- |
| `node web/test_web.mjs` | exit **1**, `ENOENT` open `web/editor.html` |
| `web/editor.html` at `ab90f47c` | **absent** |
| `firmware/src/editor.c` at `ab90f47c` | **absent** |
| Registered in `tests/run_tests.sh` | **no** (stale optional suite) |
| `web/make_site.py` | still `shutil.copy(HERE / "editor.html", …)` |
| Deletion history | `web/editor.html` removed in `23775375` (X0X fork from Felucca; instrument replaced; Felucca non-GPL assets removed) |

Log: `evidence/f06-stage1/bf003-repro.log`.

`tools/quickcheck_plan.py` on this tip already selects `projects-web` / `projects-page` for project sources (BF-002 closed).

## Affected scope

- `web/test_web.mjs`: remove or retire the editor-protocol / editor-UI sections that require `editor.html`. Keep non-editor `fm1pkg.js` / `fm1ota.js` checks only if they still run without the missing file and remain useful; do not silently `skip` a required registered suite (this suite is not registered).
- `web/make_site.py`: stop copying / publishing a `webapp/editor` tree that depends on missing `editor.html` (installer-only output, or explicit retired-editor path). The script is already Felucca-era (`FM-1_9xx` identity); do not expand it into a new product site.
- Comment / docs touch-ups only as needed so they do not claim a live `web/editor.html` check (`tools/sampleio.py` header, `web/EDITOR_PROTOCOL.md` as historical protocol notes).
- Out of scope: firmware, controller image bytes, package application bytes, production `main`, production Pages, device flash, BF-001, BF-002, withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, Stage-7 installer fingerprint pin reconciliation (evidence-only unless Stage 2 finds it blocks the repair), restoring Felucca `editor.html` / `editor.c`.

## Proposed fix

**Decision for this cycle: retire** (do not restore the Felucca editor).

Rationale recorded here so Stage 2 is not blocked waiting on the owner:

1. `web/editor.html` was **intentionally deleted** in the X0X conversion commit `23775375`, with firmware `editor.c` also absent on the validated tip.
2. The suite is **not** wired into `tests/run_tests.sh`, so it is a stale retained reference, not a current default gate.
3. Restoring the editor would reintroduce a removed Felucca surface with licence/provenance risk the bugfinder already flagged; that is a product decision outside this defect’s minimal repair.
4. Bugfinder disposition (PR #26): “either restore … or retire the obsolete test/site-builder links. Do not silently skip a required test as a fix.” Retirement of the obsolete links matches that second option.

Stage-2 implementation outline:

1. Retire editor-dependent code paths in `web/test_web.mjs` (delete those sections or the whole editor harness) so `node web/test_web.mjs` exits 0 without `editor.html`, while preserving any still-valid `fm1pkg` / `fm1ota` checks that do not need the editor.
2. Update `web/make_site.py` so it no longer requires or copies `editor.html`.
3. Adjust stale comments that assert a live editor byte-check.
4. Run the affected local host/node checks on the resulting head (at least `node web/test_web.mjs` and any make_site dry invocation that proves the editor copy is gone). No GitHub Actions.
5. Implement on a new `cursor/<slug>-stage2-fb90` branch from the pushed tip of `cursor/f06-bf003-stage1-fb90`.

No firmware build required. Development configuration if ever needed: `X0X_DEV_PADCAL=1`.

## Approvals

The full-cycle procedure (`AGENTS.md` + `docs/rules/workflow.md`), the Stage-7 successor prompt, and the deferred-order rule select BF-003 as the next defect without asking. PR #26 already records the finding and the retire-versus-restore disposition. This audit **approves the retire path** for Stage 2 based on the intentional X0X deletion and unregistered suite status above. Standing owner pre-approval covers properly tested Project merges of that selected repair. This stage does not merge production `main`, publish production Pages, or flash.

Not approved in this repair: restoring `editor.html` / `editor.c`, withheld asset cache, unmerged data-safety physical-endpoint lanes, HTML initial-loader resume paths, lengthening `SLOT_PROBE`, reopening BF-001 or BF-002, rewriting Stage-7 qualification pins as a Stage-2 source change, silently skipping while leaving the ENOENT path in place.

## Missing decisions

None block the **retire** repair selected above. Prior cycles left retire-versus-restore open; this audit closes that choice as **retire**. Optional later (not this Stage 2): whether Stage-7’s mismatched installer fingerprint pin should be corrected in a docs/evidence pass; whether Felucca-era `make_site.py` should be deleted entirely versus installer-only trimming.

## Defects considered and not selected

### Already fixed in `ab90f47c`

| Item | Disposition |
| --- | --- |
| BF-001 local `.fwsc` / ctl selection race | Present (`d11b74fc`). Closed on this SHA. Live page still carries markers. |
| BF-002 scoped quickcheck project suites | Present (`ab90f47c`). Closed on this SHA. Local `select()` includes `projects-web` / `projects-page`. |
| F03 fail-closed slot layout probe | Present (`4f18983a`). Closed on this SHA. |
| F02 fail-closed loader attach | Present (ancestor). Closed on this SHA. |

### Deferred list (`evidence/f05-stage2/DEFERRED.md`)

| Item | Disposition |
| --- | --- |
| BF-003 | **Selected.** Still present on tip; reproduced; retire approved. |
| Withheld asset cache | Excluded / not an open tip source defect (prior audits). |
| Unmerged data-safety identity lanes | Explicitly not imported. |
| HTML resume `find(IS_OTA)` | Left as initial/interrupted-loader resume by F02 design; not a commanded-transition wait. |
| Longer `SLOT_PROBE` budgets | Optional hardening only after F03 fail-closed; still 1.0s/2 native and 1000ms/2 web. |
| Stage-7 installer fingerprint pin mismatch vs served HTML | Evidence-pin discrepancy only; not a package FAIL and not selected over BF-003. |

### Recorded failing checks

No open **FAIL** on this validated SHA outside GitHub Actions (ignored) and the unreproduced optional `node web/test_web.mjs` ENOENT that **is** BF-003. Stage-3 F05 regressions ALL_PASS; Stage-4 Elite/Pro DEV CHECKED; Stage-7 published live with matching package hashes. Nightly FF REFUSED is divergence, not a package/host FAIL. Optional Pillow host gaps are not-run, not FAIL.

## Stage 2 limit

Implement only BF-003 **retire** (test + site-builder link cleanup), on a new stage-2 branch from the pushed tip of `cursor/f06-bf003-stage1-fb90`. Do not restore the editor. Do not take other deferred items in that same change. Do not change firmware, controller, or package bytes.
