# Installer Improvements R27 — local file reads bounded before allocation (2026-10-25)

## Owner | action | done evidence | stop
- **Owner:** existing firmware installer/UI browser-source specialist, isolated `gobbajogger/X0X37-for-Loopy-Pro` PR #102 (`lane/installer-nightly-download-guard-20261024`).
- **Action:** reject unreasonably large, empty or uninspectable local controller `ctl.img` and user-selected/dropped firmware `.fwsc` **before** calling `File.arrayBuffer()`; preserve guarded manual-file install behavior for normally sized files and preserve all remote immutable approved-Blob/Release gates.
- **Producer done evidence:** exact Git source/test blob readback, source-window native Node RED/GREEN, registered-suite V8-adapted replay, no changed compiled files.
- **Stop/next owner:** existing **Independent Firmware QA** must run complete current source `node web/test_install_slots.mjs` and applicable Chromium/mobile/keyboard/MIDI/backup/OTA integration, independently review both negative/positive paths and issue exact-SHA disposition. Sole firmware Safe Merge integrates separately. No self-approval, device install, production/main/gh-pages, or website owner edits.

## Immutable source and actual scope
- Prior PR HEAD: `a2dee9e2de29ed9ca51baef0a6f19a04bf79d79a`.
- Four ordered, nonforced existing-owner source/test commits: `e77cd4dfdf970bd1e47490ed304c4c22e8e58564`, `cb5e9c87ea8897e94e5d1d34b9dab9f0970461ba`, `0853e518a06650dd8d9cca6a9e2b0de89c64e6c5`, `eb08b5de2e74dc8bf82795d1dc78dea76589b740`.
- Pushed source blobs at last code HEAD: `web/x0x_installer.html@1b974e45f197236ca236327a0cee6b79269fc430`, `web/test_install_nightly_controller_selection.mjs@6bb0bdc903b0e9277fa421fab5d37a5da3d19095`, `web/test_install_nightly_local_selection.mjs@18f1ad68b1220c4cc68b9315d5d1cd4b4f3311b0`. Remote readback verified all three.
- Connected Git compare prior HEAD→last code HEAD: **4 commits, three browser-only paths**, zero `firmware/`, `ble/`, `assets/`, toolchain, build-input, controller or binary changes. No binary rebuild/requalification claimed.
- Actual complete current Git root tree ID was **not exposed by the available connector**; downstream owner must retrieve and pin it before root-level QA/integration. Do not substitute the historical root.
- Website PR #8 was independently inspected: **merged/closed**, last producer head `5f7d6a5aa0d878bf267c48494bf5c32352710998`; separate current website PR #9 and catalog PR #3 have their own producers/merger. No website branch was edited or taken over.

## Reproduced defect and repair
- Before: for an oversized synthetic File, both existing local upload paths invoked `arrayBuffer()` and could load it before later structural/integrity checks. Source-exact controlled RED: **2/2** preallocation defects reproduced (controller and local firmware), no real device.
- After: both reject `File.size` that is missing, non-finite/non-integer, zero, negative, unsafe or **over 16 MiB**, before allocation, using EN/JA warning. This is a resource cap, **not** a substitute for format/CRC/SHA, model, consent, backup/restore or OTA guards.
- Controller refusal keeps Install disabled until explicit **Remove controller**; firmware refusal keeps manual consent/Install disabled. Normally sized controller/firmware files are still read by existing guarded paths. Drag/drop and file picker use the same `selectStockFile`.
- Updated the registered controller fixture to provide `guardApprovedSelection` for the R26 click-handler mock; its previously missing mock function caused a V8-adapted test failure unrelated to hardware. No runtime acceptance relaxed.
- The publisher allowlist remains **empty**, GitHub Release import **no-op**, remote download only after exact source/model/controller/package-size/immutable-host checks and SHA-verified Blob. No installable NIGHTLY claimed.

## Actual checks (scope is important)
- Native Linux x86-64 **Node v22.16.0**, actual command `node --check source_windows.cjs`, `node --check test_native.cjs`, `node --check build_fixture.cjs`: all **exit 0**.
- `node test_native.cjs`: **exit 0, 20/20** exact proposed Git-source-window assertions, including missing/zero/negative/NaN/oversize and positive valid File, controller remove, no MIDI.
- Native harness `source_windows.cjs` SHA-256 `6ee654cecfa0e83fa3bc9d523d824d1b19e36b457d93d188a6eec1b384c69d67`; Node test SHA-256 `26ef38829602dc7a8b284a028eaf2823e5546d0fae2148cfdc0d184b566b6094`; log SHA-256 `2ab8619f8edc90fbe274004d8521b26a5e84e3978e317a489f352eec2915bc3b`. Exact controller source window length 1794/FNV1a32 `e94de6f0`; stock window length 767/FNV1a32 `4912e603`; independently compared to pushed full page.
- Fetched exact pushed page plus registered test sources through GitHub connector. V8 adapter executing the **registered complete test source bodies** using readFile/VM/assert shims (not native Node): controller **24/24 PASS**, local-selection **8/8 PASS**; earlier controller adapter initially failed because the R26 mock omitted `guardApprovedSelection`, then fixture was repaired and both suites passed.
- Existing source-exact R25 *unchanged remote admission window* (FNV1a32 `a890c0ae`) was replayed with previously checked native cached package: Node **30/30 exit 0**, real Playwright Chromium **120/120 exit 0** at 320/390/1280 EN/JA synthetic Elite/Pro. **Those are prior unaffected admission controls, not Chromium of this local-file patch.**
- Attempted new browser fixture `python3 test_chromium.py`; Chromium `Page.goto(file://...): net::ERR_BLOCKED_BY_ADMINISTRATOR`. This operation was stopped without alternate URL/transport or workaround. **R27 Chromium NOT PASSED**. Do not infer browser acceptance.
- Full clone via ordinary git network was unavailable: `Could not resolve host: github.com`; separate raw-content tool access returned disabled; GitHub connector exact Git source readback succeeded. **Native full-repo `node web/test_install_slots.mjs`, complete Chromium installer, WebMIDI reconnect, model/slot backup/restore, OTA denial, independent QA, hardware and hosted HTTPS UNRUN**.

## Downstream packet
- Recipient: existing **Independent Firmware QA** (exact code HEAD `eb08b5de2e74dc8bf82795d1dc78dea76589b740` plus this docs-only receipt commit).
- Recheck current PR head and file Git SHAs; fetch complete root tree, re-execute both new suites natively from full checkout and broader existing aggregator. Independently probe 16 MiB boundary, malformed File metadata, chooser/drop equivalence, actual browser 320/390/1280, EN/JA, model/controller, MIDI/writer/backup/OTA guards and manual-file positive controls. Do not treat a V8 adapter as Chromium.
- Sole firmware Safe Merge remains integration owner; separate website source owner/Main Merger and sole Pages publisher receive only source compatibility after independent review. Production requires separately approved attributable same-byte matching-model physical device PASS. SOURCE ONLY / DEVICE NOT TESTED / NOT QUALIFIED.
