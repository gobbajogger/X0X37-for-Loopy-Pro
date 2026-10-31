# Installer NIGHTLY post-OTA reboot identity — 2026-10-24

Source producer evidence revision: `installer-nightly-postreboot-model-20261024-r1`.

Consumed authoritative `gobbajogger/X0X37-for-Loopy-Pro` `lane/installer-nightly-download-guard-20261024@5d130ebe40b6d893b0a779dcb376fd15f4ea2f29`; complete Git **ROOT** tree `ac7bab054afddc5b6defa86450da39a3b09c3781`, distinct compiled `firmware/` **SUBTREE** `19840d87388fb1f5734d9765bf8a2c0b7d3ebaaf`. This change is browser-side updater/Node regression only; neither target firmware nor controller was rebuilt, flashed, or published.

## Finding and repair

The previous `Updater.install` waited for *any* non-OTA MIDI identity on reboot. With the intended Pro still absent and an unrelated FM-1 already connected, it selected FM-1 and raised `mismatch`. The manual `.fwsc` install UI currently interprets a mismatched returned version as successful stock/custom installation; an unrelated device could therefore produce **false success**. The inverse FM-1 / SMK case is equivalent. This was reproduced against the exact prior Git `Updater.install` method with synthetic two-port callbacks, **not observed on hardware**.

The returned device must now be a non-OTA keyboard of the target's family before it counts as a reboot. Recheck the returned identity immediately after discovery. A truly absent target returns `noreturn`; a genuinely rebooted same-family device continues to the existing product/version check. Do not remove existing local-file, backup, loader/OTA and physical-model confirmation protections.

## Executed checks (limited producer scope)

- Connected GitHub-backed V8 executed the expanded registered `web/test_install_nightly_ota_model.mjs` logic against the *exact candidate* `Updater.install` source with Node `import`, `fs` and `vm.Script` boundaries adapted for V8: **18/18 PASS**, including existing 13 cases and five new post-reboot cases. This is **not** native Node execution or complete browser validation. No OS exit code applies.
- Against consumed prior Git source, the new post-reboot case **failed as expected**: unrelated FM-1 selected first, `mismatch` observed instead of waiting for Pro. Native shell checkout was attempted: `git ls-remote https://github.com/gobbajogger/X0X37-for-Loopy-Pro.git refs/heads/nightly`, **exit 128**, DNS failure. Do not substitute this for native test evidence.
- **NOT RUN:** native full-tree `node web/test_install_nightly_ota_model.mjs`, `node web/test_install_slots.mjs`, generated-site Chromium 320/390/1280 EN/JA, backup/OTA device integration, per-model firmware/controller build, immutable HTTP bytes, physical-device operation.

**Disposition:** SOURCE-COMPLETE / PRODUCER-CHECKED *scoped*, ready for independent review only. Git push and final SHAs to be recorded in PR comment after remote verification. Independent Firmware QA owns whole-file native Node/browser/prewrite negatives, sole Firmware Safe Merge owns NIGHTLY integration after QA, separate NIGHTLY website source owner owns equivalent site source repair and reviewed route. Publisher alone qualifies lawful immutable model/controller/backup/source/QA/HTTP artifacts. No prior physical owner PASS is required for independent NIGHTLY owner testing; production remains separately hardware-proven and explicitly approved.
