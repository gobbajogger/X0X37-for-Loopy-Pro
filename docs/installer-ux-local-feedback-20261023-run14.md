# Installer UX: immediate local firmware validation feedback

2026-10-23 | owner: hourly X0X37 Installer Improvements | source-only draft PR #83.

## User-visible before and after
Before: with no public firmware available, users who select an invalid local `.fwsc` from the Step 02 picker see the validation failure only in the later installation status area. On narrow mobile layouts the file chooser itself provides no visible result.

After: Step 02 displays an inline "Checking the selected firmware file…" message followed by an actionable localized English/Japanese error when invalid. This status is adjacent to the local-file button and announced via `role=status`/`aria-live=polite`. On successful validation the existing unchecked model confirmation is focused. Retry clears obsolete feedback; stale file-read completion does not overwrite current state.

## Scope
`web/x0x_installer.html` visual/status presentation and `web/test_install_local_entry.mjs` mock regression. No changes to firmware bytes, public catalog eligibility, sample backup/write/restore, Web MIDI updater, install consent or download logic. The browser acceptance test is `web/test_install_local_feedback_browser.py`; Python Playwright and /usr/bin/chromium are local test dependencies (not shipping installer assets).

Parent draft: #77 @ bbe325d48c5d4d2de2d48823bcaec69e45a524ec, dependencies #67/#59/#52; merge destination via Independent QA then sole Safe Merge into firmware repository `nightly`; **not production main**. Current public gh-pages remains unchanged.

## Actual validation
- Baseline PR #77: `python3 test_local_feedback_browser.py source-pr77.html before` => exit 1, expected missing adjacent inline validation feedback, before.log SHA256 4c9183a229e6d1a47a210650f385bee3a638dcea8a11744480d274858e3a07cc.
- Candidate: `python3 test_install_local_feedback_browser.py candidate.html after` => exit 0 at 320/390/1280 px. Test checks no horizontal overflow, invalid-file feedback, Japanese/English, retry/pending, stale-file result, confirmation still unchecked. browser_final.log SHA256 7e16aa5077bc92a287bf871bf82099bc355e29f46d7f0b3c6324540323066b60.
- `node test_install_local_entry.updated.mjs candidate.html` => exit 0, 5 scenarios, SHA256 868c394779897d1ff585823212684d53fce79806dad23323116eb2eff17cb7bc.
- `node test_install_public_eligibility.mjs candidate.html` => exit 0, 20 eligibility cases, SHA256 bf9e698b4fcfb81f8ba414fa1e38bebd933e7f2d31bafd501f8b905a6c9dbb1b.
- `node test_install_connection_retry.mjs candidate.html` => exit 0, existing connection cases, SHA256 f6b5751df59e1237eac2ceda901b21a0ccfafef3db215fa01d7a84c597eeac93.
- Extracted installer inline JavaScript `node --check inline-module-final.mjs` => exit 0.
- **Not run:** complete combined `web/test_install_slots.mjs` (full exact source tree not available in this container); Elite/Pro firmware builds (unnecessary for browser-only change); real device, real HTTP Pages installation. Independent QA still needed.

Tested code commit 860eb0aef6b6fbb4b0e23fee0490a1ba914d4576, tested tree fcf06a6199a0426bf788442830e8536eff5b741a; HTML blob c0e7d2c0695cfe3e6cd6b0638bb1d469a7d3663f, Node test blob 8d262287093bd020f94dd9d6be960b0092e24275.
