# Installer prewrite sample-slot fail-closed — 2026-10-24

Source producer: X0X37 Installer Improvements; existing firmware PR #102.
This change is firmware-repository *browser* source, not a native firmware compile or device install.

## Before (exact remote source)
Firmware NIGHTLY `nightly@58b5b45a2dd90581de7a674c331ca5cbe8a67c7e`;
producer PR #102 consumed `9c57637c6d8d4d681d9e46084ea0c5b4e7a2b3f0`; baseline helper Git blob
`8489f4ca358cf78e3f80181c9d7984bebb4cdeef`; installer blob `8f36ca0f4d70f92ed6835443fcb3437bada5fa32`.
The old prewrite `readSlotLayout(...).catch(() => null)` treated malformed
X0X SMP_INFO, a timeout, and a legitimate unsupported stock firmware the same.
Truncated declared extension bytes could parse as legacy without exception.
Synthetic malformed replies caused `up.install` to be called without backup.

## Repair / semantic boundary
- Validate count, size, every bounded zero-terminated sample name/used size,
  complete versioned X0X extension, and positive per-slot sizes; reject
  undeclared/truncated trailing information.
- If a recognized current/legacy X0X cannot return trustworthy slot layout,
  fail **before** invoking the updater, leaving samples and firmware unchanged.
  Return distinct `slotlayout` error in EN/JA, not "success".
- Preserve the one preexisting *synthetically exercised* genuine stock
  `SMK-37 Pro_016` timeout (no SMP_INFO) without treating a corrupt
  response or lost USB as a license to bypass the backup gate. Other unknown
  stock versions stay NO-GO until per-protocol proof/review broadens support.
- No changes to actual firmware/, SDK, controller, OTA loader, flash layout,
  model picker, approved remote hashes/allowlist or Pages publishing.
- Browser parser is also shared by the breaks importer: include its affected
  format tests in Independent QA.

## Check scope / evidence
Before repair: GitHub-connected V8 executing the exact remote helper reproduced
old X0X truncated, empty, declared-truncated and timeout responses reaching
`up.install` (four unsafe positives).
After repair: checked exact source via V8 and direct focused synthetic tests
`web/test_install_nightly_slot_prewrite.mjs` with boundary cases, no hardware.
Pending independent: native Node `node web/test_install_nightly_slot_prewrite.mjs`,
full `node web/test_install_slots.mjs`, `node web/test_breaks.mjs`, real
Chromium EN/JA user flow at 320/390/1280, safe backup/OTA negative-path review.
Firmware compilation is not a prerequisite for this browser-only repair.

Disposition: SOURCE CHECKED / READY FOR INDEPENDENT QA, NOT MERGED,
NOT BUILD/DEVICE/HTTP PASS; Elite and Pro public NIGHTLY installs remain
unavailable unless all separately required gates pass. Sole Safe Merge owns
integration and separate NIGHTLY website source/publisher must implement equivalent
prewrite controls and verify complete source/generated/HTTP independently.
