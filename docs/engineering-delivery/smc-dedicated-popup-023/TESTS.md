# SMC and mixer results

Current integrated run: **47 PASS / 0 FAIL / 0 selected NOT_RUN** on the frozen
test inputs recorded in `verification.json`. All nine reported failures are
resolved. The runner's unselected package footer is outside these selectors.
Full repository host suite and physical hardware checks remain NOT_RUN.

PLAY and MMC Play/Stop open no popup. RW/FF show NEXT, the reused HOME pattern
glyph, arrow and number in the existing 1000 ms header toast; existing popups stay
in place. Popup integration passes 4958 checks. smc_connected is bounded by
SMC_PEERS; 22 link cases pass UBSan/bounds in default one-peer and optional
two-peer builds. No keyboard navigation writes, new renderer or persistent state.

The fixes repair stale test fixtures and expectations:

| Previously failing selector | Cause and repair |
|---|---|
| smc-action-source | Extract current FX navigation helper/threshold and fixture dependencies; retain input ownership and add transport/hold boundary assertions. |
| smc-boot-saved-roles | Extract Bluetooth default from its current settings helper; retain all saved/default role scenarios, normal and UBSan. |
| smc-clip-visibility | Replace obsolete keyboard clip-edit expectations with full production queue/engine/popup coverage and unchanged keyboard fields. |
| smc-fx-preset-preserve | Correct signed ternary in strict fixture compilation; preserve all effect assertions. |
| smc-lock | Service real audio cancellation before acquiring project; verify independent SMC reset/navigation. |
| smc-multi-peer | Explicit test-only two-peer topology, independent controller contexts and real FX assignment gesture; retain exact arbitration checks. |
| smc-select-mode | Assert current four-row ROOM SELECT reverb assignment and real held/released effect. |
| smc-solo-overflow | Refresh extracted FX release dependencies; retain all 24 queue overflow/release cases. |
| smc-ui | Assert ownership BUSY before audio acknowledgement, then acquire after a real audio block; preserve popup/keyboard invariance. |

Clip coverage: 4860 checks through three queues, five keyboard clip/fallback
modes and visible/menu/help/confirmation states. UI 8196, lock 900 and SELECT
5303 checks pass. All fourteen existing UI production mutations are rejected.

Fresh Elite/Pro native builds PASS: SDK V1.2.1, X0X_DEV_PADCAL=0, 2432 B free RAM
and 8536 B pool free each. Source/test hashes are recorded in verification.json.
No firmware binaries, flashing, protected-path edits or hardware claims.

| Selector | Result |
|---|---|
| smc-action-source | PASS |
| smc-audit-contract | PASS |
| smc-audit-core | PASS |
| smc-ble-payload | PASS |
| smc-boot-saved-roles | PASS |
| smc-buttons | PASS |
| smc-clip-visibility | PASS |
| smc-drum-mutes | PASS |
| smc-drum-rs | PASS |
| smc-fader | PASS |
| smc-feedback | PASS |
| smc-filter | PASS |
| smc-fx | PASS |
| smc-fx-caption | PASS |
| smc-fx-page-port | PASS |
| smc-fx-peer-view | PASS |
| smc-fx-preset-preserve | PASS |
| smc-fx-two-page-strip | PASS |
| smc-host-data7 | PASS |
| smc-host-overflow-data7 | PASS |
| smc-interaction-negative | PASS |
| smc-link | PASS |
| smc-lock | PASS |
| smc-lock-beat | PASS |
| smc-mapping | PASS |
| smc-mixer-edges | PASS |
| smc-mixer-page | PASS |
| smc-mode-page | PASS |
| smc-multi-peer | PASS |
| smc-multi-peer-negative | PASS |
| smc-part-select | PASS |
| smc-pause-loss | PASS |
| smc-performance-abort | PASS |
| smc-pickup-acceptance | PASS |
| smc-popup-engine | PASS |
| smc-protocol | PASS |
| smc-queue | PASS |
| smc-reachability | PASS |
| smc-reconnect-epoch | PASS |
| smc-record-feedback | PASS |
| smc-select-mode | PASS |
| smc-sender-isolation | PASS |
| smc-solo-overflow | PASS |
| smc-ui | PASS |
| smc-ui-bounds | PASS |
| smc-ui-negative | PASS |
| smc-ui-popup | PASS |
