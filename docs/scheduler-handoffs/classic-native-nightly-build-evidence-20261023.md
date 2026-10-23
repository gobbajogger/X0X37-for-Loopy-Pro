# Current NIGHTLY firmware subtree: real native Elite/Pro builds — 2026-10-23

**Classification: DEV COMPILED / TARGETED HOST TESTS PASS / NOT INSTALLABLE / NOT PUBLISHED.** This is a Git-first handoff containing the *actual* native test/build results. This commit adds documentation only. No firmware, controller, SDK, vendor library, toolchain, samples, backup or package bytes are committed or hosted.

## Source/provenance (important qualification)

- Source repo: `gobbajogger/X0X37-for-Loopy-Pro`.
- Live firmware NIGHTLY at first build: `c9718ae833eb64a5191c350922593df8a36017c9`.
- Live NIGHTLY then advanced to `58b5b45a2dd90581de7a674c331ca5cbe8a67c7e` by *docs-only* merge PR #60. Its `firmware/` Git tree is unchanged.
- Verified remote current firmware subtree, including files/modes/structure: `19840d87388fb1f5734d9765bf8a2c0b7d3ebaaf`.
- The available bootstrap contains **historical** source tree `152ca944e9cd3d0913906a1d3fa9d8d2826ea639` in a synthetic single-commit Git bundle (not a real GitHub commit). Four `firmware/src/` files differed. Each was reconstructed from both the GitHub old and new blobs via fail-closed line edits, with each local `git hash-object` checked against the **current** remote Git blob: `smc_mixer.c=b89e5c12062a3dc3459eeb660d205f607273aab6`; `ui.c=0c661c870f537348070fd3aaff665ced3bc97cd0`; `ble_midi_central.c=9afc57c6cf19e42f3e66ff7c1aa9201c89e1939a`; `ble_midi_central.h=9eb708e1ca90995180c9da2a9ce7cbe85cee4431`.
- After patching all four, the **entire locally computed `firmware/` subtree SHA** matched live NIGHTLY exactly (`19840d...`), not just individual files. The accompanying build inputs `build.sh`, `tools/build.py`, `tools/fm1pkg_make.py`, generator and asset blobs were compared with remote and unchanged.
- **Important scope:** the *full* repository Git tree does **not** equal current NIGHTLY, because the bootstrap's docs, web and some tests are historical. Selected changed BLE/SMC regression test files were also reconstructed to exact live blob SHAs, but not every remote test/source/doc file was imported. This is a verified current-firmware-subtree build, **not** an exact-full-checkout QA attestation. Do not promote until an independent worker obtains and checks the complete live SHA.

## Executed local native builds and target budgets

Environment: Linux x86-64; genuine pinned JieLi toolchain ZIP SHA-256 `ed379c483468c44f10cf3be7db32356b9623e16320a021cf2a4e06ec96ab5f19`, `pi32v2` clang 4.0.1; private owner-provided SDK V1.2.1 pinned input kit SHA-256 `2570c8f9472968afb16ea94372a05edda2e1f9c72d22b2431688c87d906c4805` (`kit.py verify` successful). The user's separate SDK V1.2.0 ZIP is **not interchangeable** with this pinned kit.

Environment variables: `JIELI_TOOLCHAIN=<local unpacked toolchain>`, `AC79_SDK=<local verified V1.2.1 SDK inputs>`, `SDK_LOCAL=$AC79_SDK`, `X0X_JOBS=2`.

Commands executed against the projected verified firmware tree (all **EXIT 0**):
- `BOARD=smk37 MODEL=elite X0X_DEV_PADCAL=1 bash build.sh --board smk37`
- `BOARD=smk37 MODEL=pro X0X_DEV_PADCAL=1 bash build.sh --board smk37`
- `CTL_NATIVE=1 CTL_NO_CACHE=1 bash ble/ctl/build_ctl.sh`

Actual outputs, **not** Git-hosted and **not** independently distributable without checks:
- `x0x-smk37-elite.fwsc`: **651396 bytes**, identity `SMK-37 Elite_900`; SHA-256 `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b`.
- `x0x-smk37.fwsc`: **651392 bytes**, identity `SMK-37 Pro_900`; SHA-256 `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9`.
- Private locally built `ctl.img`: **65492 bytes / 65536 max**, CRC32 `77b7a496`, SHA-256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`, `tools/add_ctl.py --check` passed; **must not be published** (JieLi controller license / user-provided local browser assembly).
- Build-budget logs: app code area 552672 bytes with 6676 bytes free; app image 545996 bytes; RAM data+bss 97812/98304 (**492 bytes free**); pool 335740/344064 (**8324 bytes free**); controller image **44 bytes free**. All actual build tool budget checks reported OK. These are tight budgets and future source edits require revalidation.

## Actually executed host/install safety regressions

Native `bash tests/run_tests.sh` invoked with pinned `AC79_SDK`, `JIELI_TOOLCHAIN`, `SDK_LOCAL`, `X0X_TEST_JOBS=1` against projected firmware + individually Git-blob-verified current test files; final explicit process exit **0**, final output **`all tests passed`**; **17 selected checks**:

`ble-midi-receive`, `smc-reachability`, `smc-mapping`, `smc-drum-mutes`, `smc-clip-visibility`, `smk-package`, `smk-ota-entry`, `smk-ota-entry-model1`, `smk-ota-entry-model2`, `smk-update-loader`, `smk-build-package`, `smk-build-package-elite`, `ble`, `ble-package`, `slot-backup`, `installer-slots`, `installer-ctl`.

Additional checks: `tests/host/ble_midi_receive_test.sh` **exit 0** with current receive+signal tests; `tests/host/smc_reachability_test.sh` **exit 0**; `tools/add_ctl.py ctl.img --check` **exit 0**. Current test versions are necessary: the old snapshot tests fail on intentionally revised BLE pairing/MTU/parameter-update and SMC help-overlay behavior. Selected current test blob hashes are verified against live remote, not silently patched assertions.

**Not run / not proven:** independent QA's complete exact-head test suite, release gate, physical Elite/Pro test, same-byte device/controller/backup/readback, current NIGHTLY browser/Node/Chromium source-and-embedded-catalog integration, external artifact hosting and HTTP downloads. The tests above include simulated installer and backup negative cases but are **not** universal installation safety certification. No firmware flash occurred.

## Root cause and next consumer queue

- Compilers/SDK/target budgets are **not** blockers; both actual current firmware tree target builds succeeded.
- Firmware builder source acquisition is fragile: the Library bootstrap is stale and native container cannot clone from GitHub over shell DNS. Connected GitHub Git-tree/blob API can independently reconstruct and verify current firmware sources. Replace this ad hoc bridge with a durable exact-full-SHA source bundle/transfer so schedulers never substitute stale source or claim full HEAD.
- A scheduled producer has not delivered a reviewed, immutable, legally distributable candidate and controller strategy to the NIGHTLY Pages publisher. The browser-only NIGHTLY repository deliberately has an empty installable catalog and forbids committed firmware/controller images. NIGHTLY policy and catalog drafts await reconciliation; do not bypass them. NIGHTLY firmware source `nightly` is distinct from the browser NIGHTLY repo.
- Firmware `.fwsc` contains GPL-3.0 X0X and JieLi Apache-2.0 SDK components: distributed packages need complete corresponding source, `LICENSE`, `LICENSES/Apache-2.0.txt` and `LICENSING.md`. The controller `ctl.img` includes closed JieLi code and must remain owner-locally generated/selected unless actual redistribution rights are proven. The vendor-provided local picker is the existing supported strategy.
- **Independent QA:** verify the full current firmware nightly commit tree (currently `58b5b45...`, possibly advancing), all impacted positive/negative checks and USB failure/backups against exact inputs and hashes; separately review outstanding critical PRs, especially controller/installer controls. Do not infer independent QA from this producer's run.
- **Build producer:** with QA-accepted exact full source and pinned inputs, rebuild unique versioned DEV package bytes; publish only metadata/immutable lawful firmware download URL via the proper channel; no `ctl.img` distribution.
- **Catalog Sync / Nightly Main Merger:** reconcile existing NIGHTLY browser-only PRs (policy #2, status/catalog #3, continuity #5) and require exact accepted evidence. Integrate only within designated role after required browser/QA checks; do not declare branches live.
- **Sole NIGHTLY Pages publisher:** only after all independent prehardware, license and immutable-hosting gates pass, expose `DEV / HARDWARE TEST — DEVICE NOT TESTED`, model, hash and backup safeguards; verify URL/HTTP bytes. Hardware proof is NOT a prerequisite to NIGHTLY, but production main and picker remain hardware-proven and owner-approved.

**Status for each model:** `DEV BUILT + SCOPED HOST PASS / QA FULL-SHA + DISTRIBUTION + SITE PENDING / NO INSTALLABLE DOWNLOAD YET`.
**Site:** `GH-PAGES NOT CHANGED / HTTP NOT VERIFIED`.
**Hardware:** `DEVICE NOT TESTED`.

X0X37-Role: Classic native build recovery
X0X37-Stage: VERIFIED-FIRMWARE-SUBTREE-BUILD / TARGETED-HOST-PASS / INDEPENDENT-QA-PENDING
X0X37-Consumer: Independent QA; Hourly Build; Build Catalog Sync; NIGHTLY Main Merger; sole NIGHTLY Pages publisher
X0X37-Next: Check full live source SHA and independent package/installer/distribution gates before offering any flashable candidate
