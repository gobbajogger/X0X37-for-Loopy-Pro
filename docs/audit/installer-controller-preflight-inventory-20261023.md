# X0X37 Pages DEV inventory and Bluetooth-controller preflight — 2026-10-23

## Scope and immutable remote identity

Read-only inventory and static/install-path audit; **not** hardware testing, stable release validation, firmware compilation, a live site change, or an authorization to flash.

- Public repository: `gobbajogger/X0X37-for-Loopy-Pro`.
- Source main: `c9718ae833eb64a5191c350922593df8a36017c9` — **SOURCE ONLY** as a target build; latest-source does not mean latest validated firmware.
- Published site branch: `gh-pages@dcb41f2ead9e2166ec3dddcd658e445d22a5fac9`.
- Catalog blob: `e573799f5d870005c7e170b71dcce435a585ce74`; installer HTML blob: `e3a2c4c54e719513bdefbc5fe1760abe344f7f0d`, unchanged versus previous inventory.
- Latest recoverable **DEV CHECKED / DEVICE NOT TESTED** firmware from `e894804dfdb0bb66c7247eb1846313a215422c9f`, recorded source tree `67c15750402e5ea6a5893d422e5ade1ce023001953bbe04378f34fc6cf774097`, DEV configuration (`X0X_DEV_PADCAL=1`). The *existing model-specific* validation records report 6,244 B code free, 492 B static RAM free and 8,324 B pool free (>= 8,192 B minimum). No independent rebuild for this inventory run.
- Current published versions each have `hardware_tested=false`. No attributable same-artifact device PASS; PR #23 reports healthy SMC connection without a matched installed firmware/controller hash. GitHub Releases collection is empty: **no verified stable release**.

## Exact published package inventory (Git object check)

The SHA-256 values were previously computed from the actual package bytes; **this run** reconfirmed the unchanged Git blob SHA, file size and manifest SHA-256/size, instead of independently downloading and rehashing the web-delivered bytes.

| Lifecycle | Model / build UTC | Bytes | SHA-256 | Git blob |
| --- | --- | ---: | --- | --- |
| LATEST AVAILABLE DEV; DEV CHECKED; DEVICE NOT TESTED | Elite, 2026-10-22 16:38 | 651396 | `19cf000582b67f6e66295c4f0c70512c493432e87f3c88c817f938f67e138c64` | `c5c16255d35d6a1d8deda0efde1c4f4361d2cb1f` |
| LATEST AVAILABLE DEV; DEV CHECKED; DEVICE NOT TESTED | Pro, 2026-10-22 16:38 | 651392 | `ca458e0e4fe216ec349566d4a122ad65498d78aabc21c9e01dfe8d7e8494d56f` | `6b5d3d9cbcf49e2b91a36ad6681536432ad02fea` |
| SUPERSEDED; DEV CHECKED; DEVICE NOT TESTED | Elite, 2026-10-22 16:29 | 651396 | `b14ae4145359743d62b1cb789b7c9f8b2b83664b79bcc462ba4e1be29ff4402c` | `24bb17e4a7e972d6038dba5ce2fc9584c700e008` |
| SUPERSEDED; DEV CHECKED; DEVICE NOT TESTED | Pro, 2026-10-22 16:29 | 651392 | `8008b633b96e5c33a4bb7dbb7adb98b5e7315720c1d64446664209aadeccee57` | `ddb8fc0ee68fbc671de26537cc1f01e75e6cd873` |

All four still exist under `gh-pages/firmware/`. The older version and its associated rollback comparison remain intact. Older source/checkpoint records without retrievable packages remain **SOURCE ONLY**, **MISSING ARTIFACT** or **BUILD FAILED** according to existing historical evidence; no invented install links.

### Known later source is not a verified later artifact

- PR #19 `2b310479c73c1c7dad8feb64c94109fd757c6110`: **SOURCE ONLY**; embedded evidence directory still contains the older `e894804` firmware Git blobs.
- PR #28 `832f8dbb0655cba8b9fd512be34c1e89ace185aa`: source repair and claimed offline projection builds, **SOURCE ONLY** until full exact-commit DEV records and same-artifact packaging are available. Its repository tree still contains the older `e894804` firmware Git blobs. Do not reassign a prior record to PR #28.
- Current `main` has no matching checked target artifact; do not conflate repository recency with validated firmware.

## Confirmed controller omission in installer path — FLASH NO-GO

1. Both DEV records require separately produced `ctl.img` of **65,492 B**, SHA-256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` (the historical independent byte audit checked its header, CRC, ABI and bounds). Current hosted firmware packages have an empty 64 KiB Bluetooth-controller region.
2. Live `install/index.html` `updateInstallGate()` accepts the selected validated package after DEV acknowledgement with **no controller-presence requirement**. The click handler uses `let img=image; if(ctl) img=insertCtl(image,ctl)`; otherwise it sends the unchanged image to `run()`.
3. `main:web/fm1pkg.js` `insertCtl()` fills that region with `0xFF` and sets controller bytes *only if* `ctl` exists. `main:web/fm1ota.js` supplies the selected image to the OTA loader. This is a source-confirmed risk of losing Bluetooth/SMC firmware after install, **not a device-reproduced failure**.
4. PR #20/#24/#31 UI code continues to make `ctl` optional. The controller guard is not yet shown implemented and verified on any of these branches.
5. The current `main:.github/workflows/pages.yml` can force-push `gh-pages`; draft PR #30 proposes a guard but is not merged. The designated Pages publication owner must reconcile this hazard before any candidate publication. Do **not** use the existing force-publish script.

**Required remedy (installer/catalog owner, not this audit worker):** for controller-requiring model packages, reject or disable installation until a validated *matching* controller image is selected; permit legacy controller-less devices/packages only with an explicitly tested authoritative compatibility rule. Add both positive and negative browser tests: missing controller, invalid CRC/ABI, model mismatch, superseded release, selection races, user cancellation, backup failures, interrupted update, and unchanged slots. Re-verify assembled image bytes, flash limits, and all model/data guards. Never suppress this safety failure with a mere warning or acknowledgement. Maintain old version and rollback links.

## Execution and provenance this run

- GitHub exact refs, PRs, releases, published catalog, installer source, binary Git blob identities, archived evidence records and deployment/job summaries were independently reread.
- GitHub Pages dynamic build+deployment `39822941019` succeeded for `gh-pages@dcb41f2`; published static HTTP responses were **not** re-downloaded due to unavailable access from this environment. This must **not** be called an independently verified live SHA-256 download.
- PR #20 regression run `39849080018` has a failed job and zero artifacts. Historical only: owner policy disallows GitHub Actions test execution, and it was not used for local verification.
- Current-tree static assertions: **PASS** (four unchanged Git blobs, all manifest size/hash fields, version lifecycle labels, model presence, no hardware PASS, missing-controller UI paths). No actual device write.
- Native local Linux x86-64 runner: extracted the historical source archive safely, excluding 12 out-of-tree symlinks. Verified the relevant test/OTA/package files have Git blob identities matching current main. `node --check web/fm1pkg.js`: **PASS**. `python3 -B tests/install_slots_test.py`: **PASS** (17 sample-preservation scenarios). `python3 -B tests/ble_pkg_test.py`: **SKIPPED** because the checkout lacks `build/x0x-smk37.fwsc`; not a pass. The historical checkout as a whole is not the current main tree.
- No new Elite/Pro target builds were run in this inventory-only cycle. Pinned proprietary SDK/toolchain files were not copied into Git; current main exact source and the DSP reference pins remain separate full-build prerequisites.
- Direct Pages HTTP read was unavailable, and shell `git ls-remote` failed DNS resolution; verified GitHub ref/metadata access continued through the GitHub connector.
- No firmware, controller, installer code, model guard, sample, old catalog version, `main`, `gh-pages`, release or hardware was changed. This document is evidence only on this isolated development lane.

## Next safe owner-controlled action

No catalog update warranted without new verified artifact bytes. Installer owner implements/test-proves controller-dependent fail-closed selection on its existing stacked lane. Designated safe Pages owner independently validates the **merged** site, preserves every old package and performs non-force fast-forward publication only after all pre-hardware criteria pass. Sole firmware integrator controls any source merges. Maintain **DEVICE NOT TESTED** until exact package+controller+device logs can be attributed. This worker continues the existing hourly inventory without creating duplicate commits when the state remains unchanged.

## 2026-10-23: exact successful Pages Actions deployment-archive bytes

A read-only catalog-worker check downloaded the actual successful GitHub Pages Actions artifact `github-pages` (#12444763487) from run [39822941019](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/actions/runs/39822941019) for `gh-pages@dcb41f2ead9e2166ec3dddcd658e445d22a5fac9`. ZIP length **2,795,239 bytes**, SHA-256 `e32b52aba55e649565a6c3f9a7e85312ce6e069a637fe19359539085dbe750c7`; inner tar 3,399,680 bytes, SHA-256 `26ce545f5f6c72ebe6652aea781574fd7c546a72e5deebbe4699468c323b9542`. Native Python verified ZIP CRC, 57 safe TAR entries, and 46 regular site files without duplicate/traversal/symlink members. Exact extracted `install/index.html`, `versions.json` and four Elite/Pro `.fwsc` files **all matched the six immutable Git blob SHA-1 values** already recorded above. Every firmware package was separately rehashed for SHA-256, model identity, CRC16, loader and sample-slot CRC32 against the catalog, **PASS 4/4**. All four still decrypt to blank 64-KiB controller blocks, and the deploy archive includes no `ctl.img`; online Bluetooth/SMC-dependent flashing stays **NO-GO**. An Actions upload plus deploy job is **not** proof of the actual served HTTP bytes, browser preflash success or a device PASS. Original report and structured verification manifest: ChatGPT Library `/LoopyX0X37/LoopyX0X37-Pages-Deployment-Artifact-Attestation-20261023.md` and `LoopyX0X37-Pages-Deployment-Artifact-Integrity-20261023.json` (no firmware bytes included in those records).

Additional independent source-only observation: unmerged `lane/stage4-test-selector-failclosed-20261023@349af1c64c971baa65c5b77baa26b99e79c18ee7` is one commit beyond PR #28, changes only a fastmath comment and n=128 host test; its retained `.fwsc` blobs and records still belong to `e894804dfdb0bb66c7247eb1846313a215422c9f`. Do **not** relabel these as a new exact-source build. The catalog's historical source-only #19 SHA `877f512...` trails current #19 head by two CI-only commits; neither difference authorizes an installer version change. Latest and rollback remain DEV CHECKED, DEVICE NOT TESTED; no stable release. No changes to `main`, `gh-pages`, packages or the installer from this documentation update.
