# PR #102 — direct Download uses SHA-verified browser bytes, not unchecked host URL
Evidence revision: `installer-pr102-verified-byte-download-20261024-r11`.

## Defect and repair
Consumed live installer owner branch `lane/installer-nightly-download-guard-20261024@f020c069ec4ee46ec1b6fe3b7e13291bd6cc090f`, complete parent ROOT Git tree `e1ea09f62670a82557944f31bb12dce5b2db08fd`; independent compiled `firmware/` subtree `19840d87388fb1f5734d9765bf8a2c0b7d3ebaaf` remains unchanged. Old installer `web/x0x_installer.html` blob `7b71be676f088ed7d9a07086f97273bee0750783`.

The old source `refreshMeta()` assigned `link.href = pkg.pkg` for a publisher-approved catalog tuple **before** HTTP fetch, browser SHA-256 and exact model/product verification. A direct click could download different bytes if the approved HTTP target was changed, even though the installer would refuse mismatched bytes. This is a downstream browser download-integrity defect; no currently selectable public candidate is implied because the compiled approval allowlist is empty. Native scoped RED baseline reproduced the direct remote URL exposure without a fetch.

New installer source blob `91a29addf582d820d85140e6ac075318b7300c4a` uses `verifiedDownload` to keep a local browser object URL only after `loadSelectedPackage()` completes its approved model/version, HTTP, SHA256 and exact product checks. `refreshMeta()` never exposes a raw remote `pkg.pkg` link, old object URLs are revoked when changing/resetting selection, and a click-time guard rejects approval/model/version/package changes. The existing `mergeGitHubReleases()` remains a no-op; arbitrary local stock/custom `.fwsc` input and separate backup/model/confirmation gates remain accessible. The direct download now requires browser fetch/CORS to succeed, which is intentionally fail-closed when integrity cannot be checked. Registered regression source is `web/test_install_nightly_download_guard.mjs` blob `dcbecf3420b3ec12c77a3bceb619f86ff332d915`.

## Actually executed producer tests
Native local machine and command exit codes, 2026-10-24 (source windows reconstructed from exact GitHub parent blob and patch-checked against the above new installer source blob):
- `node --check verified-download-window.js`: exit **0** (window syntax).
- `node --check test_verified_download.cjs`: exit **0**.
- `node test_baseline_unverified_link.cjs`: exit **0**, expected **RED** reproduced the pre-verification unapproved *bytes* direct href in old source.
- `node test_verified_download.cjs`: exit **0**, **10/10** controlled positive/negative cases for pending fetch/hash, SHA/product mismatch, denied Release, HTTP failure, revocation, model switching, current-identity race, keyboard click and object-URL cleanup.
- `python3 test_verified_download_browser.py`: exit **0**, **30/30 real Chromium** source-window scenarios at 320/390/1280 pixels × EN/JA (verified Blob download, prehash absence, revocation, keyboard navigation, local input visibility, overflow).
- `node test-revocation-exact-excerpt.cjs`: exit **0**, **14/14** earlier Release/revocation source-window cases.
- `python3 browser_release_approval.py`: exit **0**, **24/24** earlier browser publisher-approval source-window cases.
- Exact assembled *complete inline HTML script* syntax: **PASS** using JavaScript Function parser, with `/*CATALOG*/` replaced by `{}` and `/*LIB*/` removed; not full generated-site integration.
- Source-equivalence check against new full HTML: FNV1a32 `9a9b763b` verifiedDownload guard, `69a315ba` refreshMeta window, `43f2a7f3` loadSelectedPackage window. These matched the source windows tested with native Node and real Chromium.
No physical-device testing, site deployment or network artifact-byte check was performed. Complete reconstructed-repository native `node web/test_install_slots.mjs`, generated-site Chromium, Web MIDI/backup/OTA integration and independent QA were NOT RUN by this producer; native git checkout attempts could not resolve github.com, though connected GitHub API worked. Do not interpret scope-limited tests or source-syntax PASS as full-install/device PASS.

## Disposition and next owners
**SOURCE-COMPLETE / producer-scoped-checked / READY FOR INDEPENDENT REVIEW; downstream QA, NIGHTLY source integration, publication, HTTP and hardware remain separate.** The compiled publisher allowlist remains empty; no real user-installable NIGHTLY model is claimed.
1. **Independent Firmware QA:** consume exact remote PR #102 new HEAD, independently run all native Node suite, full rendered Chromium with 320/390/1280, EN/JA, same-image and changed/failed URLs, model/controller/backup/confirmation/WebMIDI/OTA negative cases; post a Git exact-head review (not inferred).
2. **Sole Firmware Safe Merge owner:** review dependency/stack and integrator conditions; do not treat QA-pending PR as approved, leave firmware nightly/main untouched until authorized.
3. **Separate NIGHTLY Website Source owner:** implement *equivalent* verified-Blob direct-download change via OWN reviewed website source branch/PR, preserving catalog history, local file import and publication safeguards. Do not self-merge production or write website main/gh-pages.
4. **Sole NIGHTLY Pages publisher:** only after independent prehardware source/license/QA/model/controller/backup/OTA/immutable hosted HTTP byte gates PASS may link a model-specific `DEV / HARDWARE TEST — DEVICE NOT TESTED` build. Prior owner physical PASS/per-build consent is NOT needed for NIGHTLY; production requires same-byte owner hardware proven and distinct approval.

No vendor toolchain/SDK, binary `.fwsc`, `ctl.img`, samples or credentials are in this browser-only change. No protected refs/Pages/firmware/physical hardware changed.