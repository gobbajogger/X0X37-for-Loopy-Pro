# Optimization Watch — BLE outgoing timestamps under queueing

**Date:** 2026-10-23. **Status:** P2 investigation, SOURCE CONFIRMED + FORCED HOST REPRODUCTION, not firmware changes or physical-device performance evidence.

## Executive summary and relevance

The peripheral BLE output mirror queues only MIDI packet words, without preserving when the events happened. On transferring those messages to the BLE-MIDI host queue, `ble_service()` timestamps them with the **current** `fm1_ms`, which can differ from the original event time. A source-identity-checked Linux test confirmed that deliberately delaying a Note On/Off pair can collapse their encoded 40-ms timing gap to zero. Both messages remained ordered and were accepted into the host ring. Whether this ever happens in normal Loopy Pro use, whether Loopy interprets these timestamps and whether audible timing changes occur **are not measured**.

This is a new finding distinct from the already reported 64-message peripheral mirror loss when full, the SMC receive queue overflow and the direct-SMC central TX retry.

## Exact source

Live `main` at `c9718ae833eb64a5191c350922593df8a36017c9`. The checked `firmware/src/ble.c` blob `941f91b2960ee5c40b3f5c661c3b012adf160467` stores only `uint32_t pkt` in `ble_out()` (lines 342–350); its `ble_service()` mirror drain (lines 460–471) passes `(uint16_t)(fm1_ms & 0x1FFFu)` to `bleh_midi_send()`. The checked `firmware/src/ble/ble_midi_host.c` blob `ece7f0cb3c2ba878dc4a351fcb864fcb2da5b8f7` stores the timestamp in `bleh_midi_send()` and serializes it in `bleh_poll()` (lines 488–502, 551–589). Only the **peripheral Loopy BLE role** uses this mirror; direct-SMC central mode is separate.

The local historical source files were matched to live GitHub blobs by `git hash-object`, not assumed to be an identical full checkout. Other matched blobs: `ble_midi_host.h` `488977fab5e6300e8e2f7ad96cb834fe62c490f1`, `ble_glue.h` `d30343e367b0c4ad08b2a1a59c8e976b3d8c2bb2`, `x0x_ble_abi.h` `8716949b32a1bb11fb3bf5f338f0265dffae9de9`, `tests/ble_test.c` `768d194b66afc400b65913d4d046d45faa5c5196`.

## Executed host checks and measured synthetic effects

Linux x86-64, GCC 14.2.0. Harness SHA-256 `d71d17b9d1b725f02ed39fc1a66ab88e97586512c859206decd51c05713c07d5`. It compiled unmodified `ble_midi_host.c` and real BLE glue plus exact extracted `ble_out()` and the exact production mirror-drain loop; hardware was stubbed. Compilation and run both **exit 0**, no ASan/UBSan diagnostics, with flags `-std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined -fno-omit-frame-pointer -Ifirmware/hal -Ifirmware/src/ble`.

- **Control:** Note On at simulated 100 ms and Note Off at 140 ms, drained promptly: encoded timestamps **100, 140**; the host generated **2 ACL notifications** (different timestamp-high groups).
- **Forced queue delay:** Same events held until simulated 300 ms: encoded timestamps **300, 300**, artificial separation **0 ms** instead of 40; the host generated **1 ACL notification**. Both events still ordered.
- **Full-host negative case:** With all 32 host entries occupied, a Note Off at 100 ms stayed pending in the mirror. Freeing capacity at 300 ms admitted it stamped **300 ms** (forced age 200 ms).

These times and packet counts are simulated test outcomes, **not measured wireless latency, real-time performance or ordinary XRUNs**. Independent matched-source `tests/ble_test.c` compiled with `cc -O2 -Wall -Ifirmware/hal -Ifirmware/src/ble tests/ble_test.c -lpthread`; compile and run **exit 0; 5,153 checks, 0 failed**. Recent separate failing GitHub Actions runs are **not** claimed as test PASS.

## Proposed fix investigation, limits and ownership

First instrument event age between production and mirror drain, both queue peaks, mailbox/credit stalls, actual BLE packet timing, Loopy/iOS timestamp behavior, phase and XRUNs in real simultaneous clip/clock/SMC workflows. Only if meaningful real-world behavior is reproduced, consider preserving an event-time field through the mirror with correct 13-bit wrapping, ordering, reconnect, overflow and Note Off behavior. A separate 16-bit timestamp for each of 64 entries costs a nominal **128 bytes** of extra pool space, before padding: future patches require real **Elite and Pro** DEV target builds, flash/static RAM/pool checks and at least **8,192 bytes spare pool**.

Physical-only open risks: whether backpressure occurs routinely, iOS timestamp handling, audio timing, BLE radio latency, two-SMC effects and installed-device compatibility. No code changes, controller builds, sample/project writes, flash, `main` or `gh-pages` update, public installer change or stable release were made in this documentation audit.

**User-visible improvement:** none yet; a reproducible performance-risk hypothesis is now documented for prioritization.
