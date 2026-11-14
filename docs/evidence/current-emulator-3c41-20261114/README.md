# Current-3c41 emulator review: source and text evidence

This archive preserves an independently performed, bounded exercise of actual Elite and Pro firmware packages, rather than presenting historical screens or proposed UI as working firmware. It lets firmware and design maintainers recover the exact stimuli and inspect the reported clip-confirmation and held-note findings before changing an instrument the musician relies on.

The original reviewer completed this scope before this archive stage. Its 33 source/text files and control-mapping provenance record are copied byte-for-byte. `MANIFEST.json` identifies those originals and this archive's added documentation/check record. The scripts and receipts are historical evidence, not a newly hardened general-purpose tool or acceptance of every intended stimulus. The reviewer retained initial calibration attempts and documented the ignored tap `count` field and encoder pulse limits.

## Exact scope

- Actual exercised firmware source: `3c41ef4bedf9701f549ce2752b73306e2fab1eb8`, root `fa0c3fe75483534f12839eeb8f5e29ec9cb52c6d`.
- Separately inspected design proposal: #921 at `a8986ac2d364bb59a38e068381e18ca068fe7c8d`, root `1a79869e1df001561a1a8ac3e4571c8e4c708fde`. It was not implemented or interaction-validated in this exercise.
- Maintained policy and archive parent: `83fc6d60289869116f93352cf845a9a7e271cdba`, root `555d0913145263800fc062eab23b83e4dfecfed9`.
- Original reviewer reports 15 isolated runs and 183,572 emulated milliseconds; each recorded command exited 0. This archive validates receipt consistency and byte identity, but does not rerun those emulator commands.

These are historical candidate identities. The newer `d158444101d569f9ee64ba05d7cb54f24de1be30` candidate and its native result are not qualified by the older 3c41 exercise. Source-only preservation does not integrate firmware, approve the proposal, fix either observed symptom or establish physical-device acceptance.

Read `stage1534/fresh-review/REVIEW.md` first. Its findings distinguish current observed behavior, proposal dependencies and unrun work. `run-summary.json`, each model's JSON receipt and `COMMANDS.md` preserve stimulus, timing, input hashes, LCD hashes and end states. `SOURCE.json` points to immutable upstream mapping source; its implementation is not vendored here. The exact older local WASM was not independently rebuilt from that upstream revision.

## What is deliberately absent

No firmware/controller package, SDK, compiler, WASM or worklet implementation, third-party implementation copy, raw PCM/audio, RGB565 capture, PNG image, saved instruction file, account information or Library identifier is included. References to absent capture filenames in the original review remain historical evidence pointers, not functioning public image links. Raw images and original package inputs remain with the existing review owner/coordinator; this text archive alone does not reproduce the reviewer's visual inspection.

## Reproduction without overwriting evidence

Make a separate disposable copy of this whole archive directory before replay. **The original scripts write receipts and captures beside themselves and can overwrite their historical filenames. Never replay in this preserved evidence directory.** Do not substitute another firmware version into an older receipt.

In that disposable archive-root copy, independently supply the exact approved local package/runtime inputs at the relative paths and SHA-256 values in `stage1534/fresh-review/LAUNCH.md`. Those bytes are not downloaded, regenerated or distributed by this archive. Use Node v24.22.1 for the original interface. Pillow is required only to render newly captured LCD images. Then follow `LAUNCH.md` from the disposable archive root, using a unique exercise tag. No browser, physical device, network service or persistent flash write is needed by the harness. Supplied worklet and WASM are executable inputs; hash verification is necessary and does not establish fresh upstream rebuild provenance.

## Preservation checks

`PRESERVATION-CHECKS.json` records this stage's byte, JSON, syntax, receipt-total and scope checks. They are archive checks, not fresh emulator tests, audio parity, browser latency, release/resource qualification or hardware checks. Original findings were already routed to the existing firmware/design owners. They retain implementation/integration ownership; this archive appoints no competing writer.
