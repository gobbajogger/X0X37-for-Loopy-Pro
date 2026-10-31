# SMC and mixer results

Current integrated run: **47 PASS / 0 FAIL / 0 selected NOT_RUN** on the frozen
test inputs recorded in `verification.json`. All nine reported failures are
resolved. The runner's unselected package footer is outside these selectors.
Full repository host suite and physical hardware checks remain NOT_RUN.

The firmware is unchanged from `ee89337b`: PLAY and MMC Play/Stop open no popup;
Rewind/Forward reuse the existing kind-3 popup with the cued pattern. Popup
integration passes 4183 checks. No keyboard navigation writes or new UI added.

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

Elite/Pro native build evidence is reused for identical firmware, tools,
configuration and dependencies: SDK V1.2.1, X0X_DEV_PADCAL=0, 2432 B free RAM
and 8536 B pool free each. Firmware source SHA-256:
`8edbb7fc6e1b6233898b93542ff11f020b360c7ea511d49173941a87ef054ae2`.
No binaries, flashing, protected-path edits or hardware claims.

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
