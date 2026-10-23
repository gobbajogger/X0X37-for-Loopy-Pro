# SMC BLE link-epoch input isolation — source-only finding

**2026-10-23 / SMC Transport / extracted host checked / device not tested / builds pending**

Production firmware `nightly@c9718ae833eb64a5191c350922593df8a36017c9`, source blob `b89e5c12062a3dc3459eeb660d205f607273aab6`.
The central TIMER5 producer publishes link changes but `smc_poll` previously retained
the central event ring across a disconnect and reconnect coalesced before one main-loop
poll. A queued Solo, Note Off, START, STOP or fader message from the old connection
could run as input from the new connection.

A single 32-bit link publication now packs an 8-bit queue cutoff alongside the
link-up bit and a 23-bit generation. The poll skips only pre-edge central packets;
post-connect packets and independent forwarded USB/User-mode queues remain intact.
It never rewinds an already consumed cursor. No sequencer, flash, picker or
peripheral BLE host source was modified. Under a completely saturated prior
session, brand-new input can still be dropped until the poll drains the ring.

## Executed local Linux x86_64 checks

Baseline: `SMC_SOURCE=.../smc_source_before.c CC=gcc python3 tests/host/smc_ble_reconnect_epoch_test.py`
EXIT **1**, **24/48 failures** reproduced (old-link event replay).

Patched extracted production windows, same test (48 scenarios each):
- GCC native EXIT **0**, 48/48.
- GCC UBSan EXIT **0**, 48/48.
- GCC ASan+UBSan EXIT **0**, 48/48.
- Clang ASan+UBSan EXIT **0**, 48/48.

Window fixture transcribed from the exact GitHub source regions for
`smc_ble_midi`, `smc_ble_link`, `smc_poll` and state, not a complete source checkout.
Original blob was checked via GitHub before edit. Regression reads production
source when run from an actual repo checkout; **exact remote-tree execution is
NOT RUN**, so no full firmware, real ISR timing or device claim.

Host cases: queue wrap 0/240/254/255, same-link delivery, coalesced reconnect,
stale presses/releases, old/full queue, live new data retention, cursor already
advanced by consumer or project drop, independent host queue and busy gating.
No HCI mailbox, hardware, Elite/Pro build, controller, CRC, backup or flash tested.

## Next consumers

Independent QA: run regression with actual checked-out branch source and native
sanitizers, inspect 32-bit publisher atomicity/TIMER5 publication relative to BLE
packet producer, and verify combined SMC #40/#43/#50/#58/#63/#71/#73 compatibility.
Safe Merge: integrate via an isolated firmware source lane only after QA;
Hourly Build: qualify exact integrated Elite/Pro DEV separately.
This draft targets firmware NIGHTLY; it does **not** alter NIGHTLY HEAD,
production main or either Pages repo.
