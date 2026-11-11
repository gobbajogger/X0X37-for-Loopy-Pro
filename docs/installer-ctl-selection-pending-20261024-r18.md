# Installer UX: controller-selection pending/failed file must not silently install without Bluetooth

Evidence revision: `installer-ctl-selection-pending-20261024-r18`.

## Consumed exact input
- Firmware repository: `gobbajogger/X0X37-for-Loopy-Pro`, existing source-only installer PR #102 branch `lane/installer-nightly-download-guard-20261024`.
- Producer parent commit `a5eed65297f7894f9cdb2092a0a70ebe4c642c62`; **full ROOT tree** `0c60ae03a82b26d5b0b8d9747f9a20b3071bb1bb`.
- Unchanged **firmware/ subtree** `19840d87388fb1f5734d9765bf8a2c0b7d3ebaaf`; also unchanged BLE and assets inputs.
- Original page blob `91a29addf582d820d85140e6ac075318b7300c4a`; existing registered controller regression `3e92b412161646ed3eae38233241bbc864ded748`.
- Source exact Git controller-handler FNV1a32 `d1b86dcd`; Install handler `a6122e61`.

## Before → After
**Before:** Choosing a local `ctl.img` clears the selected controller and then asynchronously reads it. The authorized DEV **Install** button remains enabled. Pressing it before file verification runs firmware installation with `ctl=null` and quietly drops Bluetooth. On invalid/read-failed controller inputs Install likewise remains available with no explicit acceptance of a no-Bluetooth install. Canceling a new file dialog also discards an existing verified controller.

**After:** As soon as a file is really selected, the DEV Install button is disabled and the controller-selection state stays unresolved until the latest controller is verified or the user explicitly chooses **Remove controller** to opt out of Bluetooth. Invalid and read-failed inputs retain this gate. Clicking Install checks the unresolved flag in addition to disabled state. A canceled file chooser retains a previously accepted image. Slow/stale async responses cannot resurrect an outdated controller; write lock and model/controller SHA gates remain intact.

## Real producer commands/exits (local Linux x86_64)
1. `node /mnt/data/x0x37_installer_ctl_run18/test_ctl_pending.cjs` — **EXIT 0**. Expected reproduced BEFORE unsafe plain firmware write, AFTER **6/6 PASS** async controller selection/error/clear/stale/focus-of-intent scenarios. Exact patched controller/Install handler source windows FNV32 `57b49b90` and `b9761a7d`; match committed full page.
2. `node /mnt/data/x0x37_installer_ctl_run18/build_browser_fixture.cjs` — **EXIT 0**. Generated bounded source-exact DOM fixture.
3. `python3 /mnt/data/x0x37_installer_ctl_run18/test_browser_ctl_pending.py` — **EXIT 0**, Chromium 144 with Playwright **12/12 PASS** at CSS widths 320/390/1280, English/Japanese synthetic status, pending and invalid controller, keyboard Remove, no horizontal overflow.
4. `node --check /mnt/data/x0x37_installer_ctl_run18/test_ctl_pending.cjs`, `node --check /mnt/data/x0x37_installer_ctl_run18/build_browser_fixture.cjs`, `python3 -m py_compile /mnt/data/x0x37_installer_ctl_run18/test_browser_ctl_pending.py` — **EXIT 0** each.
5. Connected V8 executed all **22/22 registered exact-source controller regression cases** with Node `vm` API adapter against proposed complete page blob, syntax parsed full inline HTML module — PASS. **This is not a native Node full-repository run**.
6. `node web/test_install_slots.mjs` — **NOT RUN** on exact complete Git root. Generated-site full Chromium, physical keyboard, installer OTA/backup aggregate, independent QA, firmware compiler and HTTP Pages deployment — **NOT RUN**.

Native log SHA256: `node.log` = `6e5af5e77624f4c1c1ebaf0aeb1deb0a40bb6d04af98663878ee6a65227bc73b`; `chromium.log` = `f82c43ea3bbdde73255e6247d64a1b03002437524a24ee9dca3fdeb0e8605d7d`. Controlled source-window harness SHA256 `fdd7cb72f3401bc7e1b8520915e0b006cc3acd9a5f6b3635ee41ca149fccbac4`; Chromium script SHA256 `d35bd2a1dc4219f3dab46743c46019363baa5c1e0e57d2bd37d74a6a91ab7d46`.

## Source delivery and boundaries
- Full proposed page Git blob `7c5d48f005e40711f2a87de919332e3ca19a1919`.
- Registered controller regression Git blob `05fe716c384bb7ea387dee4c0c824bddc015296b` (17 existing + 5 new = 22, authenticated source).
- Browser-only patch. No new firmware/controller binary built, installed, flashed, hosted, approved, or promoted. The pre-existing Release importer remains a no-op and the direct Download link continues using a verified Blob only after SHA/model/product checks. NIGHTLY approvals remain empty.
- Downstream: **Independent Firmware QA** must replay complete `node web/test_install_slots.mjs`, full generated browser DOM and backup/OTA/model/wrong-controller/revocation negatives against exact new HEAD. Sole Safe Merge integrates reviewed firmware source via protected NIGHTLY route. Separate NIGHTLY site producer owns equivalent site source; sole site merger/Pages publisher alone control website/main/gh-pages and eventual served HTTP verification.
- PR #102 stays SOURCE-COMPLETE, READY FOR REVIEW, not independently approved, not integrated or live.
