# Optimization Watch — BLE peripheral outbound capacity and packing

**Date:** 2026-10-23  ·  **Scope:** documented research only; no firmware, hardware, or Pages changes.

**Baseline:** [`main@c9718ae`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/c9718ae833eb64a5191c350922593df8a36017c9), before the open SMC/installer changes. **Targets:** Pro / Elite shared BLE implementation.

## Executive summary

Two independent host experiments confirm that the peripheral BLE output path can discard a MIDI Note Off when its 64-message mirror is deliberately saturated, while an existing optional BLE-MIDI packing mode reduces notification count for a repeated-note burst. Neither experiment measures live Loopy Pro latency, ordinary-usage saturation probability, or iOS interoperability. A code patch is **not** part of this research lane.

| Priority | Source-backed evidence | Practical effect if exercised | Status |
| --- | --- | --- | --- |
| P1 investigation | The outgoing peripheral BLE mirror silently discards events above 64 queued messages. | A Note Off, clock or clip gesture might not reach Loopy Pro during sustained downstream backpressure. | **SOURCE CONFIRMED / FORCED HOST REPRO**; device incidence UNTESTED. |
| P2 optimization | Running-status packing is implemented and host-tested but not enabled by the firmware. In a 12-note test, it reduced packets from 3 to 2. | Potentially less BLE transport overhead for repeated MIDI statuses; only if compatible with Loopy/iOS and beneficial under measured load. | **SOURCE CONFIRMED / SYNTHETIC MEASUREMENT**; physical compatibility UNTESTED. |

## 1. Unobserved overflow can discard a critical outbound MIDI event

- [`BLE_OQ=64` and the ring storage](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/ble.c#L22-L35) are independent of the separate 32-message BLE host queue.
- [`ble_out()`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/ble.c#L342-L350) enqueues only when the role is peripheral, notifications are enabled and the ring has free space. It returns no failure and does not increment an outbound-drop counter if full.
- [`ble_service()`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/ble.c#L461-L471) drains into `bleh_midi_send()`, waiting for downstream room when it returns -2. That backpressure can keep mirror messages pending; [the BLE host's second 32-entry queue](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/ble/ble_midi_host.c#L488-L501) is a separate capacity limit.
- [The clip/APC MIDI output path](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/app/main_fm1.c#L202-L225) mirrors UI MIDI packets to BLE. An outbound lost packet could therefore affect performance controls; the experiment does **not** establish that a human can normally produce 65 packets before a drain.

### Host boundary reproduction

The test extracted **the actual `ble_out()` function** from `firmware/src/ble.c` and compiled it with a minimal ring/notification stub under GCC 14.2.0 with `-fsanitize=undefined`, `-Wall -Wextra -Werror` (exit 0, no sanitizer finding). Test source SHA-256: `e5247ab000eafb04b7a60e5b1a240db027d7bf26e90bc6ea9ea2dc16f8f25dde`.

| Input while connected/notifying | Observed | Exit |
| --- | --- | --- |
| 63 queued MIDI messages, then Note Off | Note Off retained at occupancy 64 | 0 |
| 64 queued messages, then Note Off | Note Off not retained, occupancy unchanged at 64 | 0 (expected overflow reproduced) |
| Free one queue entry, then retry Note Off | Note Off accepted at write index 65 | 0 |

**Reproduction outline:** From a scratch checkout of the pinned source, extract bytes from `static void ble_out(uint32_t pkt)` up to `static void ble_fail(void)`. Compile that unchanged function with `BLE_OQ=64`, the `ble_r.ow/orr` counters, a 64-word ring, `RING_PUBLISH()` as a compiler memory barrier, and a notifying BLE stub. Send the three input sequences above and assert retained-message presence and queue indices. This reproduces the producer's fail-silent policy only, not a real BLE controller, audio IRQ, Loopy connection, or packet delivery.

**Interpretation:** Outbound queue saturation is a **conditional loss mechanism**, not a proven routine live-performance defect. Before changing policy, capture actual peak “queued, committed, sent, dropped” counts and BLE credit stalls. Preserve time/order of Note On, Note Off, transport and clock; avoid blindly retrying stale clips after reconnect.

## 2. Measurable unused outbound compression option

- [The host definition includes `tx_running`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/ble/ble_midi_host.h#L47-L53) and [the packetizer uses it to suppress repeated MIDI status bytes](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/ble/ble_midi_host.c#L551-L589).
- In the production-matching source, `bleh_init()` zeroes the state and no runtime writer sets `tx_running=1`; [the host test explicitly exercises it](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/tests/ble_test.c#L248-L265). This is an **unselected optional behavior**, not a defect by itself.

**Repeatable isolated experiment:** An unchanged `ble_midi_host.c`/`.h` host implementation was linked to a send callback that counts outgoing HCI ACL packets. It queued twelve consecutive channel-1 Note Ons at timestamp 1024 ms, all with the same MIDI status, and called `bleh_poll()` until the queue emptied with ample simulated credits.

| Packetizer setting | ACL notifications | Total HCI ACL bytes | Largest ACL frame |
| --- | ---: | ---: | ---: |
| `tx_running=0` (production default) | 3 | 84 | 28 |
| `tx_running=1` (test-only) | 2 | 62 | 31 |

**Measured only for this synthetic burst:** one fewer notification (33.3%) and 22 fewer host/controller bytes (26.2%). This is **not** a measured 33% reduction in hardware latency, CPU load, MIDI jitter or audio XRUNs. Test harness SHA-256: `81bcf4c3bb4c55aedf17678b68305da227f76da12073e655dc3599dfea446e1a`.

Run the independent existing host protocol regression on the pinned matching blobs:

```sh
gcc -O2 -Wall -Ifirmware/hal -Ifirmware/src/ble tests/ble_test.c -lpthread -o /tmp/x0x_ble_test
/tmp/x0x_ble_test
```

**Executed result:** `ble_test: 5153 checks, 0 failed`; exit 0. Input Git blobs confirmed against live `main`: `ble.c` `941f91b2960ee5c40b3f5c661c3b012adf160467`, `ble_midi_host.c` `ece7f0cb3c2ba878dc4a351fcb864fcb2da5b8f7`, `ble_midi_host.h` `488977fab5e6300e8e2f7ad96cb834fe62c490f1`, `tests/ble_test.c` `768d194b66afc400b65913d4d046d45faa5c5196`, plus matching BLE glue and mailbox headers. Runner: Linux x86-64, GCC 14.2.0. The source worktree was a synthetic historical checkout; **only the listed byte-identical files, not the full current `main`, are validated by this claim**.

## Safety disposition and ownership

- **BLE/audio owner:** add telemetry for outbound mirror/host queue high-water and lost critical events, then use actual Loopy Pro BLE capture and dense MIDI feedback/clock stress to determine if the overflow is reachable. Keep an explicit source/port identifier so SMC and Loopy input paths cannot cross.
- **MIDI compatibility reviewer:** before proposing `tx_running=1`, verify the same Note On/Off, real-time clock, SysEx and status ordering against Loopy on iOS and the transport parser. Compare actual packet counts, worst delivery latency, queue drops and XRUNs; revert compression if an interoperability issue is detected.
- **Firmware integrator:** any future implementation belongs in an independently owned source lane with Elite **and** Pro target builds, flash/RAM/pool guard checks (minimum 8192 bytes free pool), affected MIDI/host regressions and independent pre-hardware review; do not merge a report as if it fixed anything.
- **Installer/sample safety:** no writes or reads of physical sample slots occurred here. Previously verified mocked backup/restore tests remain separate; no new sample safety PASS is inferred.
- Active firmware owner work: [dual-SMC PR #18](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/18), [SMC repair PR #19](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/19), [sanitizer repair PR #28](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/28). Do not edit their branches or duplicate those fixes.

**No user-visible firmware change or physical-device improvement was delivered in this research cycle.** No firmware build, installer publication, flash or release performed; `main` and `gh-pages` are untouched.
