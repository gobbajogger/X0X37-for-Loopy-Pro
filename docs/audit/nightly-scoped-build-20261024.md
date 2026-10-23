# NIGHTLY native build — scoped source-verified evidence, 2026-10-24

Status: **BUILD_SUCCEEDED_UNREVIEWED_SCOPED_SOURCE_VERIFIED**. Exactly Git-verified compiled source subtree and build assets/tools; **not full Git HEAD**, not independent QA approved, not downloadable/installer-qualified, device **NOT TESTED**.

Remote source observed during build: `gobbajogger/X0X37-for-Loopy-Pro` firmware `nightly` commit `58b5b45a2dd90581de7a674c331ca5cbe8a67c7e`, root tree `71b759dd206d844cceed9e40ae68966142720e9c`. Reconstructed `firmware/` tree was written through a **temporary Git index** and checked byte-identical against remote SHA `19840d87388fb1f5734d9765bf8a2c0b7d3ebaaf`. `assets/` tree `0e721b227239d885a77a12ef24d9f001604e3eb3` and `ble/` tree `d0479e103e206e0e9649a7c734af8f1f11a718ca` match GitHub. All 15 listed build/generator/controller tool blobs match GitHub. Historical unrelated repository files remain, so full-root checkout identity is **NOT ASSERTED**.

All native build commands exited 0 (JieLi Clang 4.0.1, pinned AC79 SDK V1.2.1 11/11 files verified):

- Elite: `BOARD=smk37 MODEL=elite X0X_DEV_PADCAL=1 bash build.sh --board smk37` → `651396` bytes, SHA256 `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b`.
- Pro: `BOARD=smk37 MODEL=pro X0X_DEV_PADCAL=1 bash build.sh --board smk37` → `651392` bytes, SHA256 `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9`.
- Controller (local private only): `CTL_NATIVE=1 CTL_NO_CACHE=1 bash ble/ctl/build_ctl.sh` → `65492` bytes, SHA256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`, CRC32 `77b7a496`, zero unresolved symbols.

Code area free `6676 B`, static RAM `97812/98304` (**492 free**), pool `335740/344064` (**8324 free**), controller `65492/65536` (**44 free**). All extremely tight thresholds must be rechecked after source changes.

## Executed tests

1. Firmware package / OTA: ble-midi-receive, smk-package, smk-ota-entry, smk-ota-entry-model1, smk-ota-entry-model2, smk-update-loader, smk-build-package, smk-build-package-elite — **8/8 PASS**.
2. SMC / BLE: ble-midi-receive, smc-reachability, smc-mapping, smc-drum-mutes, smc-clip-visibility — **5/5 PASS**.
3. BLE / backup / installer: ble, ble-package, slot-backup, installer-slots, installer-ctl — **5/5 PASS**.
4. Exactly **17 distinct named host checks** executed (BLE MIDI receive overlaps groups 1 and 2), all local process exits 0. All tests now use exact GitHub NIGHTLY test blob revisions for affected tests.
5. Offline delta source helper 13/13 unit tests PASS; REST source fetch helper 12/12 PASS.

**Evidence:** `docs/audit/nightly-scoped-build-20261024.json` includes every source/tool SHA and model/command/artifact/log SHA256. Host build and test logs were generated in native Linux private output and are retained in the separate owner-facing evidence ZIP; no firmware/vendor binary is embedded in Git. A preliminary SMC run using **stale historical test files** failed; after exact GitHub test-byte reconciliation, the above current host tests passed. The false earlier failures must not be quietly relabeled as firmware bugs or omitted from the audit.

**Downstream incomplete:** full root source checkout and independent critical QA, full installer/OTA/backup/restore interruption/model mismatch/browser negative checks, legality of distributing a JieLi-built controller, lawful hosting of immutable artifact bytes, site catalog/Pages HTTP hash verification. NIGHTLY may expose a software-qualified **DEV / HARDWARE TEST — DEVICE NOT TESTED** without prior owner physical test, but no binary is yet qualified or published by this stage. Production `main` needs owner hardware PASS and separate promotion permission.
