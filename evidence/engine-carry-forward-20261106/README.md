# Chicago/Miami carry-forward evidence — 2026-11-06

This evidence packet preserves a locally prepared engine integration patch and executed test receipts for the existing #698 integrator. It does not modify the shared source branch or select a release candidate.

Base proposal tree: `2e38c67d3dbf1b905085580229e395433aefd92d`.
Resulting local candidate tree: `01194a7f7fd0043833bf657756939819e23c99ca`.
Patch SHA-256: `4a904d439829c9333dfcc02485bed52ba4e0b717f4ae3cc422c0fcad2c4a36b2`.

- [Report and exact exclusions](REPORT.md)
- [Patch](engine-carry-forward.patch), [identity/file list](identity.json), [donor blob map](donor-blob-map.json)
- [Checksums](deliverable-sha256.json)
- Audio: 17 scenarios, 50 audio/state files bit-identical to the exact proposal; [final log](audio-final.log).
- Native SDK V1.2.1 Elite and Pro DEV builds PASS; pool free 8,440 B, only 248 B above the 8,192 B gate. App image +7,300 B, static RAM +524 B, pool use +160 B versus the proposal. Recheck after other lanes compose.
- Three host timing rounds and a separate ASan/UBSan comparison PASS. Host timing is measured; AC79 timing, device performance and dropout acceptance are not measured.
- No approximate X0X_FAST_FMA or restored-insert architecture; newer EQ/VCA/control/idle behavior retained. Complete scoped exclusions and unrun gates are in the report.

`LOCAL_PREPARATION_NOT_PUSHED` in the preserved identity/report describes the source candidate: the candidate source is not on a shared branch. This evidence-only delivery makes its patch and receipts durable. It is not independent QA, DEV CHECKED, firmware publication, installation or device acceptance.

Existing #698 integrator owns selection and composition. Existing independent QA must review the resulting immutable source. No raw PCM, firmware binaries, private headers, SDK/toolchain bytes, credentials or device data are included.
