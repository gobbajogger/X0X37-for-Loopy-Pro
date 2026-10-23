# Installer UX — visible current step (run15, 2026-10-23)

Source-only change on an isolated lane stacked after draft PR #83 (ed3fc80a084a4ab39de3335075e31be505e84750). Browser firmware, controller and sample-writing gates unchanged.

## Before and after

Before: all three guide labels (Connect keyboard / Select firmware / Review and install) appeared identical, even when connecting, selecting a local firmware while disconnected, reconnecting or rejecting an invalid replacement. Users could not tell which action remained.

After: exactly one numbered step has a lime current-state treatment and `aria-current="step"`. Completed connection/selection are indicated in green. If a local file is selected before connecting, step 02 is complete but step 01 remains current. Failed validation removes selection completion. While firmware is writing, Review/install stays current; no claimed completion. English and Japanese text remains as before.

No new flash-enabling conditions, device access, writes, catalog changes, model acceptance, package trust, backup or OTA logic.

## Actual native checks on exact source

Tested source base Git tree `56defe7f92a7cbde2ccf15125efa82c5730a7641`, generated source Git blob `0f81dd4bf0c5cac624b38209d349f554e8b161b3`.

- `node test_install_journey.mjs candidate.html` — exit 1 (EXPECTED RED baseline; no status projection).
- `node test_install_journey.mjs candidate_journey.html` — exit 0, seven state cases and exactly one aria-current.
- `python3 test_install_journey_browser.py candidate.html before` — exit 1 (EXPECTED RED baseline).
- `python3 test_install_journey_browser.py candidate_journey.html after` — exit 0, Chromium at 320/390/1280 pixels: keyboard/retry, connected/disconnected, local firmware selected/invalid, JA, no horizontal overflow, guarded install.
- `python3 test_install_local_feedback_browser.py candidate_journey.html after` — exit 0, previous invalid/stale/local-file scenarios at three widths.
- `node test_install_local_entry.mjs candidate_journey.html` — exit 0, five local file scenarios.
- `node test_install_connection_retry.mjs candidate_journey.html` — exit 0; its isolated MIDI harness was given a no-op `refreshJourney` stub; production MIDI scan logic unchanged.
- `node test_install_public_eligibility.mjs candidate_journey.html` — exit 0, 20 forged model/proof cases.
- `node --check candidate_journey.html.js` — exit 0.

Logs and desktop/mobile screenshots were captured privately. Full exact-tree `node web/test_install_slots.mjs` including sample/OTA was NOT_RUN: shell git fetch failed DNS, only exact installer source and scoped dependencies reconstructed. Elite/Pro native builds NOT_RUN (browser-source-only). DEVICE_NOT_TESTED. No `main`, firmware `nightly`, `gh-pages` or website deployment occurred.

Independent QA should review this display-state projection and its connection/selection interactions, execute full combined slot/OTA suite against integrated exact source, then sole Safe Merge may integrate reviewed stacked source to firmware `nightly`. No production promotion without same-byte owner hardware PASS and approval.
