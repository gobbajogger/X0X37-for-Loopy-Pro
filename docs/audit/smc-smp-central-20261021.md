# SMC central: security-manager receive boundary (2026-10-21)

This development-only repair is based on source commit
`ab1694ea6275826799de9912d8df0e8b4a2d05f2`, SMK-37 Elite.
The installed source is **not** replaced by this patch.

## Physical evidence supplied by the owner

- Installation completed, 65,536-byte controller block read back with matching CRC32
  `d84be7d4`; sample slots 1 and 2 matched their pre-install backups, slot 3
  remained empty.
- The 45-second `smc-connection.jsonl` watch contains 135 good diagnostics,
  one missing reply, one central connection, one successful subscription, and no
  recorded link failure or disconnect. Final state: `CONNECTED / READY`.
- The real `rx0` counters show 9 received ACL frames since initialization, zero
  MIDI notifications, zero decoded MIDI messages, and one rejected ACL frame.
  `rx1` shows zero enqueued/dispatched/dropped application SMC events. These
  values did not change in the sampled window. The owner confirmed actual
  fader/button gestures ("I did", transcript timestamp 2026-10-21 09:04:46).
  This was not an idle capture: the reported gestures had no observed MIDI
  ingress or application dispatch. The capture does not identify the cause.
- The retained rejected HCI ACL header is `50 20 06 00 02 00 06 00`, which
  identifies handle `0x0050`, L2CAP CID `0x0006` (Security Manager), payload
  length two. The original capture did **not** retain the two SMP payload bytes
  and does **not** establish whether it was a Security Request (`0x0B`), Pairing
  Failed (`0x05`), or another command. The frame may predate the watch.

## Verified software defect and scope of fix

The original central rejects all L2CAP CID 6 packets (`BLEC_RX_CID`) without
interpreting or responding. It implements no pairing or bond storage. Under
Bluetooth Core 6.3, Vol 3, Part H, Section 2.4.6, when pairing is unsupported,
a central receiving the Peripheral Security Request must reply with SMP Pairing
Failed, reason Pairing Not Supported (`0x05 0x05`). The general SMP rules also
require a non-pairing device to refuse other supported pairing commands; reserved
codes are ignored and received Pairing Failed is terminal.

Source: https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Core_v6.3/out/en/host/security-manager-specification.html

Changes:

- Accept bounded CID 6 packets in the central receive/fragment path; send SMP
  `0x05 0x05` through the existing four-entry, credit-aware L2CAP response FIFO.
- Preserve the existing no-pairing/no-bond/no-BTIF-write architecture, without
  new timers, buffers, memory allocations or radio-connection roles. Do not
  respond to peer Pairing Failed or reserved codes.
- Extend `smc_connection_watch.py` to report **actual** receiver/dispatcher
  counters and the retained rejected ACL channel; the previously exposed
  `smc_acl0` and `smc_acl1` are packet-header bytes, not receive counters.
  The monitor remains ED_DIAG-only, read-only, and cannot prove unseen input.

## Original protocol-patch host verification (historical)

- Negative control: compile new `ble_midi_smp_test.c` against original central
  source; expected assertion `sends == 1 && !c.rx_reject && !c.sig_count`
  **fails** (exit 134), showing the missing response to the simulated Security
  Request. It does not identify the original captured SMP command or prove
  that this defect caused the owner-reported missing input.
- Patched sanitizer-backed test covers the saved ACL header with simulated
  Security Request bytes (clearly not recovered from hardware), fragmentation,
  credit starvation, blocked HCI sends, signaling queue priority, reserved
  codes, peer Pairing Failed termination, unaffected MIDI notifications, and
  rejection of unrelated CID 7.
- Targeted host runner with `PYTHONDONTWRITEBYTECODE=1`, `X0X_TEST_JOBS=1`,
  `X0X_HOST_DEFS=-D_POSIX_C_SOURCE=200809L`, `ASAN_OPTIONS=detect_leaks=0`:
  `tests/run_tests.sh ble-midi-receive ble-capture-replay smc-protocol smc-queue
  smc-link smc-mapping smc-filter smc-feedback smc-connection-watch
  smc-host-ingress` — 10 passed, 0 failed.
- `git diff --check` clean.

**Original verification limit:** At the time of the source-package checks above,
no target firmware was built, installed, or tested. Those historical host checks
are not a current target-build or device result. The protocol repair does not
prove that SMC input reaches the intended controls.

## Counter-accounting and evidence correction

This section supersedes the original source package's idle-capture description
and reboot-detection claim. The original `eb39ea4` source tree is
`3dc1f145561fad897a54301d25cb229b4e5c9963`; its archived patch is unchanged.

- `firmware/src/ble.c` exports reset-cause registers as `reset_now`, not a unique
  boot identifier. The original monitor incorrectly treated equal reset causes
  as proof of continuity and counted a same-cause restart from 7 to 0 as 249
  events. The old regression changed the cause and missed this case.
- `alive_ms` is a uint32 last-Bluetooth-tick uptime retained across resets. The
  corrected monitor compares counters only when both snapshots report a live
  connection and advancing uptime, with ordered monotonic request windows.
  UTC request/end windows must agree with monotonic elapsed time within 10 ms
  for timestamp rounding and scheduling. UTC only vetoes continuity: macOS
  monotonic time can stop during sleep, and wall-clock jumps also need exclusion.
  The later uptime must exceed the larger UTC/monotonic host interval, allowing
  a conservative 2x clock margin plus 2 ms for rounding; its advance must fit
  that bound.
  This excludes a boot within the compared interval under that clock bound.
  It does not create or claim a firmware boot ID.
- Same-cause resets, changed causes, stalled or wrapped uptime, unbounded timing,
  host sleep or clock disagreement, non-live states, missing replies and malformed counters cannot produce
  modulo increments. These intervals remain indeterminate. Missing samples
  break comparison; they are not silently removed from the history.
- Counters remain modulo 256. Reported increments are minimums over comparable
  intervals, not exact traffic totals. Flat values cannot exclude complete
  wraps. Owner-reported gestures are retained separately, never inferred from
  the diagnostic counters.
- The earlier working-controls reference `53d5654c9bbaec10b64c55fb60d0e7d857ba8156`
  also ignored CID 6: `firmware/src/ble/ble_midi_central.c` lines 267–268 accept
  only CID 4 (blob `7440ed5f980f16f20f58b970b328586b566bdb20`). The
  [historical investigation](smc-normal-connection-20261021.md) records working
  controls on that reference. Missing SMP refusal is therefore a reproduced
  protocol defect, not an established explanation for the change in live input.
  The rejected SMP payload is still unknown.

The correction changes the host monitor, its tests and this audit only. No
firmware, controller, ABI, model, package or A/B behavior changes are included.
Run `python3 -B tests/smc_connection_watch_test.py` for counter/session, gesture
context, malformed-input and recorder regressions, and
`python3 -B tests/ble_central_diag_test.py` for the related diagnostic decoder.
These host checks do not require a firmware rebuild. The corrected capture report
is a reanalysis of retained data, not a new device test.
