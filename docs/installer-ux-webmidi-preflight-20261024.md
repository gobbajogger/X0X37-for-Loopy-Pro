# X0X37 installer UX — immediate Web MIDI browser preflight

**Scope:** Browser-only source change; no target firmware, Bluetooth controller or installation algorithm changed. GPL-3.0-only existing notices retained.

**Tested source parent:** firmware repository `gobbajogger/X0X37-for-Loopy-Pro` draft PR #97 head `8344f586e57512f651965f3ac21545b24a6c6241`, tree `f73dd236fd1788a87c4e5d1a55a4f5e5158a2303`. Local candidate `web/x0x_installer.html` Git blob `2085be1b58281241596a408fe5ee3842dfa74ccc` (old `f24b574ad42c1351752881943fe4df206a5efa41`).

## Before → after

In a browser without the Web MIDI API, Connect USB remained available and the installer waited until the user clicked to report incompatibility. Now the connection panel immediately displays **Web MIDI unavailable** with concrete Chrome/Edge desktop and USB data-cable guidance (English or Japanese), and its unavailable Connect control cannot accidentally initiate scanning. On browsers with Web MIDI, the existing Connect/Retry/Rescan workflow is unchanged. The empty firmware catalog stays noninstallable; local `.fwsc` confirmation and model guards are retained.

## Executed checks (native Linux x86-64)

- `python3 test_install_webmidi_preflight.py candidate_accessible.html before` → **exit 1 expected baseline regression**, no unsupported-browser notice (`before.log`, SHA256 `80f7913c500069375897d09aedfa2c4fc60e4116ec662486be5cc880865fa519`).
- `python3 test_install_webmidi_preflight.py candidate_preflight.html after` → **exit 0**: Chromium 144 at 320/390/1280; unsupported and supported API paths, EN/JA text, focus preservation, disabled unsupported scan, no overflow, empty selector remains fail-closed (`after.log`, SHA256 `44a0d946662ff3098cb0dc4a9635780b0e482940f3d3889c20df599b1ac3d6b5`).
- `node test_install_connection_retry.mjs candidate_accessible.html` → **exit 1 expected baseline** on the new unsupported-browser assertion (`node-before.log`, SHA256 `2531ed445e78301389f1651cf00f2a9ce3bef0033d1f961aea3e512df3ebb202`).
- `node test_install_connection_retry.mjs candidate_preflight.html` → **exit 0**, previously existing Web MIDI retry/rescan/mid-scan/write-lock cases plus new supported/unsupported EN/JA path (`retry-final.log`, SHA256 `c95ca1e937400ac81af24811f810256be2b27b2267ddfde2f10abfca1864697f`).
- `node test_install_journey.mjs candidate_preflight.html` → **exit 0**, seven workflow states (`journey-after.log`, SHA256 `f7c4fe6e5cb7927287fcaade50ba4cd92c463c2fa707498b19a46e04a75a5c60`).
- `node test_install_local_entry.mjs candidate_preflight.html` → **exit 0**, five local picker errors/cancel/write-lock/focus cases (`local-entry.log`, SHA256 `868c394779897d1ff585823212684d53fce79806dad23323116eb2eff17cb7bc`).
- `node source/test_install_public_eligibility.mjs candidate_preflight.html` → **exit 0**, 20 production eligibility/forgery/wrong-model/report/controller/Release-import negatives (`eligibility.log`, SHA256 `bf9e698b4fcfb81f8ba414fa1e38bebd933e7f2d31bafd501f8b905a6c9dbb1b`).
- `node --check inline-check.js`, `node --check test_install_connection_retry.mjs`, and `python3 -m py_compile test_install_webmidi_preflight.py` → all **exit 0**.

**Native Elite/Pro firmware builds:** NOT_RUN (no firmware source change). **Physical device flashing:** NOT_RUN. **Independent technical QA:** PENDING. **Merged firmware `nightly`:** NO. **Either Pages deployment/HTTP served bytes:** NOT_CHANGED / NOT_VERIFIED.

**Next consumer:** independent QA should replay tests against remote source blobs, then sole Safe Merge should reconcile this stacked draft with PR #97 and integrate through firmware `nightly` when eligible. NIGHTLY website mirror and site publication belong to their separate owners.
