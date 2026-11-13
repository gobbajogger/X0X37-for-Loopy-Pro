# Stage 1 audit of validated source 1325f5d3

Audited tree: `1325f5d34340ea877e65ab86f3663f5dea052c0d` (validated source published on nightly; not the diverged firmware `nightly` tip).
Content tree (Stage-4 DEV CHECKED records): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`.
Git tree (`source_tree`): `6b6401f5c57ed4142bce1adbc3ceab2f99a80b6e`.
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this record: `cursor/f07-fp-pin-stage1-fb90`.
This thread: `bc-b1215ac4-b3bd-5a72-a25e-e97bb3d629dd` (next-cycle Stage-1 audit after F06 Stage 7 publish).
Prior cycle audited SHA: `ab90f47ccad0f213114d6a98c1bc6179be2ebb35`.
Evidence-bearing base: remote tip of `cursor/f06-bf003-stage7-fb90` (docs/evidence only; packages authenticate `1325f5d3`).

No production source was edited. Production `main` and production GitHub Pages were not updated. No device was flashed. Firmware `nightly` was not force-pushed (still diverged / FF refused at `08d3164a…`).

## Published page (re-verified this audit)

Installer: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/

| Check | Result |
| --- | --- |
| `latest` in live `versions.json` | `nightly-1325f5d3-dev` |
| Label / `hardware_tested` | `DEV / HARDWARE TEST — DEVICE NOT TESTED` / false |
| Elite bundled re-fetch | size 651396, sha256 `8ba23a89f50c70b631927077ec4cfe7c98593870035455261842f1b245b82278` **MATCH** |
| Pro bundled re-fetch | size 651392, sha256 `d90932115ff47cf421b0ef5caab3b93e730da5810e5b26d4c3ecd2c61c58fecb` **MATCH** |
| Stage-4 blank Elite / Pro (records) | `d3b4cac3…` / `0b09605f…` (unchanged from Stage-4 DEV CHECKED) |
| `controller_sha256` (bundled) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| Host tip | `artifacts/nightly-dev` @ `f917b636ba00d7dbef18fd00d1f0af15299a0a24` |
| Pages tip | `cff694f3792d196267c2dc8ce2d86c6aef7d5e9e` |
| Catalog `source_commit` / `source_tree` | `1325f5d3…` / `6b6401f5…` **MATCH** |
| Live install HTML markers | `1325f5d3`, `BF-003`, `nightly-1325f5d3-dev`, `DEVICE NOT TESTED` **present** |
| Live / raw `install/index.html` sha256 | `66b56ba5accd1309664320c91db52c28438080a97b2bdb3fa31f6b830f3ca554` (identical) |
| Stage-7 qualification `installer_sha256` pin | `8caf87247afb79dda2107f3074d73da7f2512f5d14e55db32b9285baadc38157` — **does not match** served HTML |
| Nightly main `installerFingerprint()` | `8caf8724…` (**equals** qualification pin; composite of web installer sources + render scripts) |

Observation: Stage-7’s recorded `installer_sha256` is the nightly **source composite fingerprint** (`installerFingerprint()`), not the sha256 of the served `install/index.html`. Package digests, catalog identity, and BF-003 content markers still match. This is the deferred Stage-7 installer FP pin reconciliation — now the oldest open deferred defect after BF-001–BF-003.

Independent claim review (internal Task): `bc-4fb732c3-0749-525e-91d0-f846466c1b04` — **13/13 PASS**.
Stage-7 evidence local review: Project store `internal/pr-local-reviews/pr-215.md` — Overall **PASS**.

Cited blobs at `1325f5d3`:

| File | Blob |
| --- | --- |
| `web/test_web.mjs` | `f611a25f17edafd5e1cd3d62312037ffcd53390d` (BF-003 fixed) |
| `web/make_site.py` | `e98904afc8e38dbce693fffd26f97da1e809aca4` (BF-003 fixed) |
| `tools/quickcheck_plan.py` | `e19d9b2240ab0c5796ef5e4486371a90b4e8aa5e` (BF-002 fixed) |
| `web/x0x_installer.html` | `a8d2c75b96dd4a6a096bae48bbc15cdaeb0ca22f` (BF-001 fixed) |

## Selected defect

**FP-001. Stage-7 installer fingerprint pin does not match served install HTML.**

Oldest deferred defect from `evidence/f06-stage2/DEFERRED.md` that is **not** already fixed in `1325f5d3` and is still an open tip evidence defect. BF-001 (`d11b74fc`), BF-002 (`ab90f47c`), and BF-003 (`1325f5d3`) are fixed and are not selected. Older deferred items remain excluded or design/optional (see below).

## Evidence

Nightly `scripts/render-qualified-nightly.mjs` defines `installerFingerprint()` as the sha256 of a JSON list of path→content digests for `web/x0x_installer.html`, `web/fm1pkg.js`, `web/fm1ota.js`, `web/x0x_breaks.js`, `web/x0x_breaks.html`, `scripts/render-qualified-nightly.mjs`, and `scripts/legacy-controller-packages.json`. `validateCatalog` requires `qualification.installer_sha256 === installerFingerprint()`.

On current nightly `main` that fingerprint is `8caf8724…`. F06 Stage 7 wrote that value into `evidence/f06-stage7/qualification.json` and CHECKS as “Installer FP”. The live (and gh-pages tip) `install/index.html` bytes hash to `66b56ba5…`. They are different artifacts:

| Artifact | Digest |
| --- | --- |
| Qualification / source composite FP | `8caf87247afb79dda2107f3074d73da7f2512f5d14e55db32b9285baadc38157` |
| Served `install/index.html` | `66b56ba5accd1309664320c91db52c28438080a97b2bdb3fa31f6b830f3ca554` |
| Match? | **NO** |

The same class of mismatch was recorded in F05/F06 Stage-1 audits (live HTML ≠ Stage-7 pin) and deferred behind BF-002/BF-003. Historical pin `2240be87…` also never equalled the then-served HTML; F06 replaced the pin with the current source fingerprint without introducing a served-HTML digest field.

Log: `evidence/f07-stage1/live-verify.log`.

## Affected scope

- Stage-7 / qualification evidence recording in this firmware repo (`evidence/*/qualification.json`, CHECKS, package-manifest, handoff language): stop presenting `installer_sha256` (source composite) as if it were the served install HTML digest; record **both** values.
- Local host verify used by Stage 1 / Stage 7: assert package digests as today, and assert an explicit `served_install_html_sha256` against fetched `install/` HTML; keep source-fingerprint check against nightly `installerFingerprint()` separately.
- Optional narrow change in `gobbajogger/x0x37-for-Loopy-Pro_nightly` only if Stage 2 needs a second qualification field or docs clarification so prepare:candidate and live verify cannot be conflated — do not weaken package gates.
- Out of scope: firmware, controller image bytes, package application bytes, production `main`, production Pages, device flash, BF-001, BF-002, BF-003, withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, deleting Felucca-era `make_site.py` entirely, restoring `editor.html` / `editor.c`.

## Proposed fix

**Decision for this cycle: split and reconcile the two digests** (do not pretend they are one pin).

1. Name them explicitly in Stage-7 evidence: `installer_source_fingerprint` (= current `qualification.installer_sha256` / nightly `installerFingerprint()`) and `served_install_html_sha256` (= sha256 of published `install/index.html`).
2. Keep `prepare:candidate` / `validateCatalog` using the source fingerprint unchanged unless a nightly-repo schema extension is required; do not break that gate by stuffing the HTML digest into `installer_sha256`.
3. Add a local host check (firmware and/or nightly) that fails when evidence claims a single “Installer FP” equals both, or when the recorded served HTML digest ≠ a fresh fetch of the live (or prepared) `install/index.html`.
4. Update F07 Stage-7 republish evidence (after Stage 2–5 as required) so CHECKS/HANDOFF list both digests; LIVE_PAGE_MATCH for HTML uses the served digest.
5. Implement on a new `cursor/<slug>-stage2-fb90` branch from the pushed tip of `cursor/f07-fp-pin-stage1-fb90`.

No firmware build required for the evidence/process repair. Development configuration if ever needed: `X0X_DEV_PADCAL=1`.

## Approvals

The full-cycle procedure (`AGENTS.md` + `docs/rules/workflow.md`), the Stage-7 successor prompt, and the deferred-order rule select this Stage-7 installer FP pin reconciliation as the next defect without asking. Standing owner pre-approval covers properly tested Project merges of that selected repair. This stage does not merge production `main`, publish production Pages, or flash.

Approved for Stage 2: dual-digest recording + local host check that prevents conflating source fingerprint with served HTML; optional nightly docs/schema clarification that preserves `installer_sha256` semantics for prepare:candidate.

Not approved in this repair: withheld asset cache, unmerged data-safety physical-endpoint lanes, HTML initial-loader resume paths, lengthening `SLOT_PROBE`, reopening BF-001–BF-003, deleting `web/make_site.py` entirely, restoring the editor, changing Elite/Pro package or controller bytes, force-pushing firmware `nightly`.

## Missing decisions

None block the dual-digest reconciliation above. Optional later (not this Stage 2): whether Felucca-era `make_site.py` should be deleted entirely versus remaining installer-only; whether longer `SLOT_PROBE` budgets are worth a separate hardening cycle.

## Defects considered and not selected

### Already fixed in `1325f5d3`

| Item | Disposition |
| --- | --- |
| BF-001 local `.fwsc` / ctl selection race | Present (`d11b74fc`). Closed on this SHA. |
| BF-002 scoped quickcheck project suites | Present (`ab90f47c`). Closed on this SHA. |
| BF-003 retire stale editor test / site-builder | Present (`1325f5d3`). Closed on this SHA. `node web/test_web.mjs` exits 0. |
| F03 fail-closed slot layout probe | Present (`4f18983a`). Closed on this SHA. |
| F02 fail-closed loader attach | Present (ancestor). Closed on this SHA. |

### Deferred list (`evidence/f06-stage2/DEFERRED.md`)

| Item | Disposition |
| --- | --- |
| Withheld asset cache | Excluded / not an open tip source defect (prior audits). |
| Unmerged data-safety identity lanes | Explicitly not imported. |
| HTML resume `find(IS_OTA)` | Left as initial/interrupted-loader resume by F02 design; not a commanded-transition wait. |
| Longer `SLOT_PROBE` budgets | Optional hardening only after F03 fail-closed; still 1.0s/2 native and 1000ms/2 web. |
| Stage-7 installer fingerprint pin mismatch vs served HTML | **Selected (FP-001).** Reproduced: qual `8caf8724…` ≠ served `66b56ba5…`. |
| Delete Felucca-era `make_site.py` entirely | Younger / decision left open after BF-003 installer-only trim; not selected over FP-001. |
| Restore `editor.html` / `editor.c` | Explicitly rejected by F06 Stage 1. |

### Recorded failing checks

No open **FAIL** on this validated SHA outside GitHub Actions (ignored). Stage-3 F06 regressions ALL_PASS; Stage-4 Elite/Pro DEV CHECKED; Stage-7 published live with matching **package** hashes. Nightly FF REFUSED is divergence, not a package/host FAIL. Optional Pillow host gaps are not-run, not FAIL. The installer FP pin mismatch is an evidence/process defect against matching live package bytes — selected now that BF-001–BF-003 are closed.

## Stage 2 limit

Implement only FP-001 dual-digest reconciliation (evidence fields + local host check; optional nightly clarification), on a new stage-2 branch from the pushed tip of `cursor/f07-fp-pin-stage1-fb90`. Do not take other deferred items in that same change. Do not change firmware, controller, or package bytes.
