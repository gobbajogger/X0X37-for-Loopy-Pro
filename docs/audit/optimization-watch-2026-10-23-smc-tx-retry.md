# Optimization Watch — direct-SMC transmit backpressure and retry

**Date:** 2026-10-23 · **Scope:** documentation-only research; no firmware, web, site or physical-device changes.

**Baseline:** [`main@c9718ae`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/c9718ae833eb64a5191c350922593df8a36017c9), **not** the entire historical offline checkout. This is new verification of the *central outbound* path; it does not duplicate the previously documented audio command, direct-SMC ingress, dual-SMC ingress, or Loopy peripheral mirror overflow probes.

## Practical finding

**SOURCE CONFIRMED + FORCED HOST CHECK PASS:** when the connected direct-SMC **32-message outbound** queue is full, `blec_midi_send()` refuses another feedback message and increments `c->drops`. Unlike a lost inbound Solo release, the calling `smc_frame()` does **not** mark that rejected LED feedback as known/sent, nor clear a rejected fader's pending bit. A later UI frame can retry the *current desired state* when transmit queue room becomes available. In a second forced case, a rejected LED-*off* was retried correctly.

This means **central-TX queue-full is not by itself proof of a permanently stale SMC LED**. The `drops` counter counts rejected enqueue attempts, including states that may later be retried; interpreting every increment as an irrecoverably lost update would overestimate end-user impact. Backpressure can still delay feedback or affect end-to-end behavior if queue capacity/credits never recover. Real BLE progress and latency remain unmeasured.

| Executed source-slice stress step | Verified result |
| --- | --- |
| Set 43 LED states unknown and 8 fader values pending; call `smc_frame()` with no simulated central drain | 32 TX entries queued; attempted 33rd refused; unaccepted LED not marked sent; all 8 faders still pending |
| Simulate consumer capacity returning, then invoke `smc_frame()` again | Remaining 11 LEDs and 8 fader values queued; pending mask becomes 0 |
| Fill TX queue again, request LED 0 change from on to off, then restore capacity and invoke next frame | First off enqueue refused while full, previous lit state retained; next invocation queued `90 00 00` and updated stored LED state |

## Exact source identity and replay

- [`ble_midi_central.c` `blec_midi_send()`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/ble/ble_midi_central.c#L594-L604): live main Git blob `9afc57c6cf19e42f3e66ff7c1aa9201c89e1939a`; source function length **352** bytes/chars and FNV-1a32 **`9be5ddf0`**.
- [`smc_mixer.c` `smc_frame()`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/app/smc_mixer.c#L698-L725): live main Git blob `b89e5c12062a3dc3459eeb660d205f607273aab6`; function length **966** and FNV-1a32 **`656e63f2`**.
- [`ui_frame()`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/app/ui.c#L4795-L4807) invokes `smc_frame()` on SMK37 when not in a project transaction; this provides the subsequent retry opportunity.
- The historical offline Git source tree was **not** mistaken for current `main`: these *two extracted functions only* were independently compared with the exact current GitHub source text and passed the lengths/hashes above. No whole-main-tree equivalence is implied.

**Runner:** native ChatGPT Linux x86-64 container, GCC 14.2.0, `-std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer`, with minimal stubs for the transport and UI state. The local harness extracted the unmodified production functions by their signatures, asserted both FNV hashes, then tested the three forced cases. Final C harness SHA-256 `0dbf4699befada0e36b35e50b9597c6de02121f17b16d68b894cf2f24f92220d`. **Compile exit 0, run exit 0**, all three assertions passed, no sanitizer diagnostics. An initial harness compile failed due to the fixture's missing forward declaration; the fixture was corrected and successfully recompiled; firmware source was never changed. Consumer drain was simulated by advancing `qr`; no actual ATT/HCI packets were sent. **GitHub Actions was not used as an execution runner.**

## Effect on optimization priorities

1. **Keep critical receive-side event losses ahead of speculative transmit LED queue repair.** [The pre-existing full SMC ingress ring can discard a Solo Note Off](optimization-watch-2026-10-23-queue-saturation.md); the outbound feedback path tested here retries unsent desired LED/fader states instead.
2. For performance analysis, measure actual **central TX occupancy, queue-full attempts, credit starvation, time-to-consistency and repeat attempts**. Do not equate `c->drops` with permanent user-facing errors. No radio throughput, two-device occupancy or 99th-percentile feedback latency was measured.
3. If timing requires change, verify the existing redraw/state-refresh contract before attempting batching or priority lanes; protect MIDI order and model/transfer safety. A separately owned firmware patch would still need Elite/Pro target builds and independent regression/device validation.

**Boundaries:** SOURCE-SLICE PASS only; forced saturation is not a normal-workload benchmark. No evidence of real BLE/SMC link latency, audio XRUN, two-SMC fairness, memory/flash margin, model package validation, owner sample readback or device PASS. No firmware code, public installer, `main` or `gh-pages` changed by this report.
