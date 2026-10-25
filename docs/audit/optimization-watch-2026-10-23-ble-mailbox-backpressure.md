# Optimization Watch — BLE HCI mailbox rejection is treated as a successful send

**Date:** 2026-10-23 · **Scope:** documentation-only, source-backed transport audit.

**Reviewed live source:** [`main@c9718ae8`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/c9718ae833eb64a5191c350922593df8a36017c9).
**Finding status:** SOURCE CONFIRMED / FORCED HOST REPRODUCED. Hardware incidence and performance impact UNTESTED.

## Executive summary

The peripheral BLE-MIDI host can consume queued MIDI while its underlying HCI mailbox rejects the actual packet. A host test using the unchanged production BLE-MIDI sender, the real fixed-size mailbox implementation and a full-ring failure callback reproduced loss of a Note Off and of a short SysEx message. The same lower-layer failure leaves an HCI startup command marked as pending even though it never entered the mailbox.

This is distinct from the previously reported 64-message peripheral BLE mirror overflow: the new failure occurs **even with spare MIDI queue space and nonzero controller ACL credits**, when the separate core-to-controller mailbox is full. Real Loopy Pro use has **not** been observed to exhaust the mailbox.

| Priority | Source-confirmed condition | Forced host observation | Practical significance, if reachable |
| --- | --- | --- | --- |
| P1 investigation | `l2cap_send()` ignores the integer return from `h->send()`; `bleh_poll()` removes MIDI from the queue before calling it. | With full mailbox, rejected Note Off disappeared from queue; no retry after space returned. | Note releases or clip controls could be missing over Bluetooth even if upstream queues have room. |
| P2 investigation | `sysex_packet()` changes its send position/state before `l2cap_send()` checks transport acceptance. | A short, complete SysEx message was lost on send rejection, without retry. | Device handshake/replies could be incomplete under actual mailbox backpressure. |
| P2 startup guard | `cmd()` sets `cmd_wait=1` regardless of failed transport submission. | Rejected Reset command left STARTING/`cmd_wait=1` after 100 subsequent polls. | Bluetooth initialization can stall if the startup mailbox ever becomes full. |

## Immutable source evidence

- [Send callback contract](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/ble/ble_midi_host.h#L30-L36) specifies return `0` when sent; the firmware's [actual callback](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/ble.c#L101-L106) returns `xb_frame_put()`.
- [Mailbox producer](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/hal/x0x_ble_abi.h#L85-L141) is finite (1,024-byte ring), with `xb_frame_put()` returning `-1` when there is insufficient space.
- [L2CAP send](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/ble/ble_midi_host.c#L111-L125) decrements `h->credits` before calling `h->send()` and discards the return value.
- [MIDI notification queue drain](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/ble/ble_midi_host.c#L551-L589) increments `h->q_r` for messages added to a pending notification **before** sending the assembled frame.
- [SysEx packet assembler](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/ble/ble_midi_host.c#L520-L538) advances its stream position and completion state ahead of the transport call.
- [HCI command helper](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/ble/ble_midi_host.c#L52-L62) marks `cmd_wait` and does not inspect a failed send.
- [Original host regression](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/tests/ble_test.c#L103-L109) uses a send callback that always returns success, so this negative condition is outside that test's documented coverage.

**Exact source Git blobs independently compared with the working source files** (not a claim of complete-main checkout equivalence):

| Input | Git blob SHA-1 |
| --- | --- |
| `firmware/src/ble/ble_midi_host.c` | `ece7f0cb3c2ba878dc4a351fcb864fcb2da5b8f7` |
| `firmware/src/ble/ble_midi_host.h` | `488977fab5e6300e8e2f7ad96cb834fe62c490f1` |
| `firmware/src/ble.c` | `941f91b2960ee5c40b3f5c661c3b012adf160467` |
| `firmware/hal/x0x_ble_abi.h` | `8716949b32a1bb11fb3bf5f338f0265dffae9de9` |
| `tests/ble_test.c` | `768d194b66afc400b65913d4d046d45faa5c5196` |

## Reproduction and checks actually executed

**Runner:** local ChatGPT Linux x86-64, GCC 14.2.0 and Python 3.13.5; no GitHub Actions runner.

**Mailbox-pressure setup:** Fill the *real* `xb_ring_t` mailbox with 255 one-byte HCI frames (`xb_ring_free() == 3`). Use `xb_frame_put()` itself as the host `send` callback. The BLE-MIDI host has a live connection, notifications enabled, three controller credits, and one queued Note Off. The mailbox rejects new HCI frames while credits are still nonzero. Drain the mailbox, call `bleh_poll()` again, and count successful physical-mailbox enqueues. Repeat for a short `F0 7D 22 F7` SysEx and separately for HCI Reset at startup.

```sh
gcc -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
  -fno-omit-frame-pointer -Ifirmware/src/ble -Ifirmware/hal \
  bleh_actual_mailbox_backpressure_test.c firmware/src/ble/ble_midi_host.c \
  -o bleh_actual_mailbox_backpressure_test
ASAN_OPTIONS=detect_leaks=1 ./bleh_actual_mailbox_backpressure_test
gcc -O2 -Wall -Ifirmware/hal -Ifirmware/src/ble tests/ble_test.c \
  -lpthread -o ble_test
./ble_test
```

| Check on verified-matching blobs | Actual result | Exit status |
| --- | --- | --- |
| GCC ASan+UBSan negative test: Note Off with full HCI mailbox | `queued=1`, `accepted=0`, `pending_after_failure=0`, one credit consumed; freeing mailbox generated no retry | 0, expected failure mechanism reproduced |
| Same negative test: short complete SysEx | Rejected by mailbox; outgoing SysEx marked complete; no later retry | 0, expected failure mechanism reproduced |
| Same negative test: startup Reset | Command rejected; remains STARTING with `cmd_wait=1` after 100 polls | 0, expected failure mechanism reproduced |
| Original host regression, unmodified `tests/ble_test.c` | `5153 checks, 0 failed` | 0 (existing protocol tests PASS, **not** negative-test coverage) |

**Fixture provenance:** standalone scratch harness SHA-256 `f25e5d936b1ee3b6216c2c6a278167b8c0c5cec3909d9391034ec30508a81e05`; final test stdout SHA-256 `5271e7a225d2b1c14f894d52ef5fc0e86cb82046dc7cd7a00c3ce1d7774bcc28`; baseline output SHA-256 `e1451de0d2f3a62d7a59db0dca219c09513df5a3417c02c8c7f711381033fbd1`. These are local scratch artifacts, **not committed or published binaries**. A temporary fixture assertion was corrected for a new third test case and all final checks then passed; no production source was changed.

## Interpretation and next safe action

1. **BLE transport owner / sole integrator:** Add a negative regression where the *real mailbox* returns full while the BLE host has credits. Define a retry/acknowledgment contract: dequeue MIDI and consume controller credit only after a successful mailbox write, and preserve SysEx position on rejection. Avoid blocking the audio ISR or replaying obsolete MIDI after a disconnect.
2. **Bluetooth lifecycle reviewer:** Ensure rejected startup/control HCI commands cannot leave the state machine indefinitely waiting for an event that can never arrive. Test failed mailbox, delayed recovery, timeout and reset/reconnect conditions separately.
3. **Timing reviewer:** Instrument real mailbox high-water mark, failed `xb_frame_put` attempts, credit availability, BLE notification delivery delay and audio XRUNs. A saturated mailbox is a forced stress condition here; there is **no measurement** of ordinary iOS/Loopy traffic, percentiles or hardware dropouts.
4. **Test/build owner:** If a separate firmware patch is authorized, run affected BLE/HCI/SMC/clip tests, real Elite and Pro target DEV builds with code/RAM/pool headroom (pool at least 8192 B), controller/package identity and installer backup/restore checks. Physical device evidence remains separate.

**Ownership and safety:** Existing SMC work in PRs #18/#19/#28, installer work in #20/#24/#27/#31, and Pages publication in #25/#30 remain untouched. No firmware edit, merge, build, installation, sample write or Pages publication was performed for this research-only report. Do not treat the test's exit 0 as a product fix or a hardware PASS.
