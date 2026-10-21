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
  values did not change in the sampled window. No control movement is recorded,
  so these observations alone cannot establish whether button/fader input works.
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

## Host verification

- Negative control: compile new `ble_midi_smp_test.c` against original central
  source; expected assertion `sends == 1 && !c.rx_reject && !c.sig_count`
  **fails** (exit 134), showing the old CID 6 defect.
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

**Limit:** The proprietary AC79 SDK and target compiler are not present in
this analysis container. No new Elite firmware was built, installed, or tested
on hardware. The change is a host-verified protocol repair, not proof that
SMC control inputs now reach 909/808/sequencer controls. The original 45-second
watch did not capture active input traffic, so the cause of missing MIDI input
remains unconfirmed.
