# Firmware NIGHTLY installer: fail closed on model/approval revocation (installer-nightly-revocation-20261024-r1)

**Producer role:** X0X37 Installer Improvements. **Scope:** firmware-repository browser installer only; existing PR #102 / `lane/installer-nightly-download-guard-20261024` targeting `nightly`. 
**Consumed immutable producer:** e507b09f820db3eb77bc373222b51663467c5d71. Whole repository root `5b862e7960bc6a64bc7b575802d81b0e13e6e86a`; separate compiled `firmware/` subtree `19840d87388fb1f5734d9765bf8a2c0b7d3ebaaf` is unaffected by these web/test/docs changes. Current live firmware NIGHTLY base `58b5b45a2dd90581de7a674c331ca5cbe8a67c7e`.

## Executable failure and repair

The existing per-model explicit package approval gate correctly rejects an unapproved GitHub Releases auto-import, missing source/QA/controller/host evidence, and mismatched model/hash. However, independently executing its **actual Git blob** `web/x0x_installer.html@0843cf0121752d81aaa641fd8a7babbdb018c9c8` with a synthetic approved Elite publisher tuple showed that:
- `models.elite.revoked=true` (or nonboolean `"true"`) was admitted;
- `approval.revoked=true` was admitted;
- catalog `revoked="true"` was admitted;
- nonboolean package `fromRelease="true"` was admitted;
- catalog `fromRelease=true` was admitted.
These could leave direct `#download` available to a subsequently revoked or explicitly Release-origin candidate once a real allowlist exists. **Current real publisher allowlist remains empty**, so there is no actual installable NIGHTLY package or currently live newly exposed firmware.

The smallest repair refuses any present revocation or implicit-Release marker unless it is the literal boolean `false`, on catalog record, per-model package and publisher approval tuple. The same `approvedNightlyModels()` predicate still drives selector, direct link, and package auto-fetch. No automatic GitHub Releases import is enabled; local arbitrary `.fwsc` selection retains its independent model/backup/confirm guards.

## Actually executed checks

- Connected GitHub-source V8, source `0843cf0121752d81aaa641fd8a7babbdb018c9c8`, baseline **6/12** negative/positive synthetic model/approval scenarios violated expected admission; proposed exact source **12/12** passed, including positive approved Elite (hardware untested).
- Native Linux x86_64 Node **v22.16.0**: `node --check /mnt/data/x0x37-installer-r11/approved-nightly-exact-proposed.js` **exit 0**; `node --check /mnt/data/x0x37-installer-r11/test-revocation-exact-excerpt.cjs` **exit 0**; `node /mnt/data/x0x37-installer-r11/test-revocation-exact-excerpt.cjs` **exit 0**, **12/12**, six expected baseline unsafe cases. Copied exact proposed `approvedNightlyModels` source excerpt length **1921**, FNV32 **fd535469**, independently matched connected candidate bytes. No network or devices.
- Native Chromium via Python Playwright: `python3 /mnt/data/x0x37-installer-r11/browser-revocation-excerpt.py` **exit 0**, **60/60** scoped minimal-DOM cases, 320/390/1280px × EN/JA, synthetic approved/revoked/Release/QA/model/host checks, keyboard link focus, no horizontal overflow, local file-input presence. A first *test harness* attempt failed because the minimal fixture's CSS improperly overrode `[hidden]`; corrected fixture and full rerun passed. **These are excerpt DOM simulations**, not full generated site.
- Connected V8 async-transformed registered `.mjs` parser **PASS**. Added eight registered regressions in `web/test_install_nightly_download_guard.mjs`.
- **NOT RUN:** complete native `node web/test_install_nightly_download_guard.mjs`, `node web/test_install_slots.mjs`, generated-site Chrome/Edge EN/JA, Web MIDI reconnect/backup/OTA/confirmation integration, hardware/device, hosted HTTP/hash. Native environment lacks an exact PR #102 Git checkout and GitHub DNS; earlier `curl` exit 6. Source-excerpt PASS is NOT full-tree QA or publication.
- Browser-only source: native Elite/Pro firmware compilation belongs to independent Hourly Build and must not await this downstream site gate.

## Next Git consumer and gates

**Independent Firmware QA:** read this exact PR head, run `node web/test_install_nightly_download_guard.mjs`, `node web/test_install_slots.mjs` and applicable whole-root Node/Chromium/backup/OTA/multilingual regressions; review immutable approval tuple, model/publisher revoke, local import and source license. Then **sole firmware Safe Merge** may integrate only through its reviewed `nightly` process. **Separate NIGHTLY website source owner** must implement this equivalent filter through website's own independently checked PR #1; sole website merger/Pages publisher retain their separate checks. **NIGHTLY public firmware** still requires real model-matching package/controller, independent full prehardware QA, lawful immutable host and HTTP bytes/hash; owner physical PASS is NOT required for owner-operated DEV hardware testing. Production remains separately same-byte owner device-tested and approved.

**Disposition:** SOURCE-COMPLETE, SCOPED-PRODUCER-CHECKED, READY FOR INDEPENDENT QA; NOT independently approved, complete browser QA, merged, compiled, hosted, published, installed, or device tested.
