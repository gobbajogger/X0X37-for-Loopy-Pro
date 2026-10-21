# Copied vendor-notification ingress helper

**The helper and host tests pass; no runtime startup is wired.** The new GPL
`firmware/src/ble/smc_host_ingress.h` parses a bounded copied BLE-MIDI notification
without including the custom HCI/GATT host. It is intended for the separately
selected central-only test profile, not a change to default Bluetooth modes.

## Interface and ownership

- `smc_host_ingress_t` is 32 B. Zero-initialize it once.
- `smc_hi_link(state, up)` resets running status and partial SysEx on every link
  boundary, while retaining diagnostic counters. Process link-up before its
  first notification; disconnected notifications are rejected.
- `smc_hi_notification` consumes up to 128 stable copied bytes synchronously. The
  donor MTU 128 allows 125-byte ATT values, so the parser bound is sufficient.
  It stores no packet pointer. No private DMA pointer may cross into this API.
- Its callback is exclusively the dedicated SMC input queue. The callback must
  copy message bytes before returning and return 1 only for an actual enqueue,
  or 0 for rejection/full queue. `messages` counts accepted enqueues; `dropped`
  counts callback refusals. `rejected` counts malformed/disconnected notifications.
- The existing `smc_ble_midi` returns void. The tested adapter compares its real
  `smc_enqueued` counter before/after the call; it does not invent acceptance.
  Calls and that comparison require the existing single-producer serialization.
- Parent integration owns mailbox generation/session checks. Resetting parser
  state does not by itself reject an old queued payload arriving after reconnect.
  Link and notification ordering, whole-record copy, queue-full handling and
  reset-before-new-session consumption must be enforced at the mailbox boundary.

## Parsing scope

The payload grammar comes from `blec_midi_packet` at baseline 816e0d3: timestamp
handling, running status, channel messages, realtime messages and fragmented
six-byte SysEx used by the MMC mapping. No HCI, service discovery or transport
state is retained. Deliberate differences are strict rejection of incomplete or
high-bit channel data and unsupported system-common messages, realtime handling
between channel-data bytes, and a syntax/state preflight before any callbacks.
Thus a malformed notification cannot dispatch its otherwise valid prefix.

This is **not a generic BLE-MIDI conformance claim**. The inherited SysEx rule
cannot always distinguish continuation timestamp-low 0xF8–0xFF from realtime
messages. Both baseline and this helper emit those bytes as realtime in that
ambiguous position. The exact preserved Mac corpus contains channel messages,
not SysEx, so it does not resolve that limitation. Synthetic fragmentation/MMC,
timestamp-low 127 and realtime tests cover only their explicit patterns.

## Reproducible checks

From the checkout root:

```sh
cc -std=c11 -Wall -Wextra -Werror -O2 \
  tests/host/smc_vendor_ingress_test.c -o /tmp/smc_vendor_ingress_test
/tmp/smc_vendor_ingress_test
cc -std=c11 -Wall -Wextra -Werror -O1 -fsanitize=address,undefined \
  -fno-omit-frame-pointer tests/host/smc_vendor_ingress_test.c \
  -o /tmp/smc_vendor_ingress_sanitized
ASAN_OPTIONS=detect_leaks=0 /tmp/smc_vendor_ingress_sanitized
python3 tests/host/smc_vendor_capture_replay_test.py
sh tests/host/smc_vendor_dispatch_test.sh
```

All pass. LeakSanitizer is disabled because it cannot run under this executor's
ptrace environment; address and undefined-behavior checks remain enabled.

The parser test covers running status, two-byte messages, timestamp boundaries,
fragmented six-byte MMC, realtime, truncated/high-bit data, packet preflight,
reconnect reset, caller-buffer overwrite after return, bounded queue refusal,
a 128-byte payload and 10,000 deterministic malformed-input trials.

The capture replay verifies all **1271 exact Mac notification payloads → 2711
matching messages, zero rejects/drops** against an independent corpus-specific
oracle. Maximum payload is 67 B. Capture SHA-256:
`a92f72f54c6aba7004e2c3a29759fa38818bb72a976486fb272b38c61d9d245a`.
The input is the existing owner-operated Mac capture in
`cloud-handoff/2026-10-20/smc_filter_fix/evidence/smc-raw-ble-retry.json`, **not a
Hortator donor capture**. No HCI framing is invented for this test.

The dispatch test links the production project, engine, UI, `smc_mixer.c` and
SMC queue using the existing host fixture. Exact new-parser output reaches
SELECT, knob editing, fader pickup and User-preset MMC controls; reconnect and
actual queue saturation are exercised. The peripheral/USB SMC queue write index
remains unchanged. Hardware I/O and clip-field storage remain host substitutes;
the test does not prove real radio/controller, APC feedback or USB hardware.
Existing host-fixture signedness warnings are unchanged; the focused parser test
compiles with warnings as errors.

## Measured footprint and remaining work

A standalone target wrapper around `smc_hi_notification`, built with pinned
JieLi clang 4.0.1 (`-target pi32v2 -integrated-as -mcpu=r3 -mfprev1 -Oz
-ffreestanding`), measures 578 B `.text`, 32 B state and one `memcpy` import. The
application already supplies `memcpy`, but the final linked delta must still be
measured. This excludes mailbox storage, link/notification adapters, output
feedback and startup code; it is not a fit certificate.

Independent narrow review accepted the helper's ownership, preflight and
counter behavior, with the SysEx caveat above. Parent runtime owner must wire
and test vendor callback → copied mailbox → this parser → SMC dispatch and
feedback. Real vendor transport, complete memory budgets, profile packaging,
installation and physical SMC controls remain unverified.
