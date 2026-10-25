# Optimization Watch: ATT control responses still drop under HCI mailbox pressure

**2026-10-23 · documentation-only independent host audit · P2 conditional reliability risk · DEVICE NOT TESTED**

## Executive summary

The SMC Transport worker's [PR #50](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/50) correctly guards *queued BLE-MIDI Note Off*, SysEx and HCI command state against a rejected mailbox submission, but its synchronous GATT Attribute Protocol (ATT) response handlers still discard the result of the send. This report **independently reproduces two specific instances** of the residual ATT-response problem already acknowledged generally in that PR: a lost **MTU Exchange Response** and a lost **CCCD Write Response**. The latter updates the local notification-enabled state even though the central never receives confirmation. Neither response is regenerated in 100 later `bleh_poll()` calls after mailbox capacity is restored.

**User effect if the forced condition occurs during iOS/Loopy Pro connection:** GATT discovery or subscription may wait for a reply that X0X37 no longer has, potentially delaying/failing MIDI connection setup. No ordinary-use incidence, over-the-air delay, device failure or XRUN was measured; saturation was deliberate.

## Source identity and ownership

- Checked production base: [`main@c9718ae833eb64a5191c350922593df8a36017c9`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/c9718ae833eb64a5191c350922593df8a36017c9), relevant original `firmware/src/ble/ble_midi_host.c` blob `ece7f0cb3c2ba878dc4a351fcb864fcb2da5b8f7`.
- **Exact patched candidate tested:** [PR #50 HEAD `4aa93f62b5f08eb5df9663747ada84fef590a014`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/4aa93f62b5f08eb5df9663747ada84fef590a014), `ble_midi_host.c` Git blob **`f4415750533c0e43f1a6c45dcd3a54dac1af6605`**. The actual PR file patch was applied to a local file whose starting Git blob matched live main; `git hash-object` of the resulting *complete patched C file* matched the GitHub-reported blob exactly. This is **not** a claim to have reconstructed or tested the full PR tree.
- Verified current-main linked headers: `ble_midi_host.h` blob `488977fab5e6300e8e2f7ad96cb834fe62c490f1`, `x0x_ble_abi.h` blob `8716949b32a1bb11fb3bf5f338f0265dffae9de9`. The existing `tests/ble_test.c` blob was `768d194b66afc400b65913d4d046d45faa5c5196`.
- Relevant unchanged [ATT request dispatch](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/4aa93f62b5f08eb5df9663747ada84fef590a014/firmware/src/ble/ble_midi_host.c#L263-L367): `A_MTU` writes `mtu_req` and calls `att_send` without retaining failed response; `A_WRITE` on `H_CCCD` updates `cccd[0]` then calls `att_send` without checking return. [PR #50's new `att_send` return](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/4aa93f62b5f08eb5df9663747ada84fef590a014/firmware/src/ble/ble_midi_host.c#L116-L139) is checked by queued MIDI/SysEx logic but not those synchronous request handlers.
- Ownership: **SMC Transport** owns any firmware fix and dedicated regression; **Independent QA** owns independent review. Optimization Watch changes **documentation only**, not their source/test branches.

## Executed controlled Linux negative test

**Runner:** local Linux x86-64, GCC and Clang with AddressSanitizer + UndefinedBehaviorSanitizer. The host regression uses the production-matched `xb_ring_t` mailbox, sends real synthetic H4 ACL/L2CAP/ATT requests to the **unaltered functions of the exact PR #50 patched C file**, and measures physical-mailbox acceptance via `xb_frame_get`. No actual radio, BLE central, keyboard, firmware target build or user data involved.

Positive controls on an empty mailbox: MTU request obtains an **11-byte H4 ACL frame** with ATT `A_MTU_R=3`, and CCCD Write obtains a **9-byte frame** with `A_WRITE_R=19`. Both commands set expected local state.

Negative controls: fill the 1,024-byte mailbox with 255 one-byte frames (leaving **3 free bytes**) while retaining 4 controller credits, then inject the requests; remove mailbox occupants and poll 100 times.

| Case on exact PR #50 file | Actual host observation | Interpretation |
| --- | --- | --- |
| MTU Exchange while mailbox full | `mtu_req=23` recorded; **0 accepted response frames**, **0 retries accepted** after 100 polls; controller credits remain 4 | Requested MTU tracked, but ATT reply discarded |
| CCCD notification-enable Write while full | `bleh_notifying()==1` **despite 0 accepted Write Response frames** and **0 retries accepted** after 100 polls; credits remain 4 | Peripheral and central confirmation can diverge |
| Same requests with free mailbox | MTU response 11 bytes; CCCD response 9 bytes; both accepted normally | Distinguishes saturation from an unconditional protocol failure |

**Actual command outcome:** GCC `-std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer`: compile **exit 0**, test **exit 0**, **528 harness assertions / 0 failures**. Identical Clang sanitizer compile and test: **exit 0 / 0**, **528 / 0**. With the patched source explicitly included (verified by `gcc -M`), the preexisting `tests/ble_test.c` regression: compile **exit 0**, execution **exit 0**, **5,153 checks / 0 failures**. These harness assertions intentionally confirm the loss mechanism; their exit 0 **does not certify correct product behavior**.

Private scratch evidence: `att_reject_repro.c` SHA-256 `844a38ed0b5f26985b39c0a74cdf08524b78e2c83b51cefbd3372470c4871f91`; both compiler runs' output SHA-256 `4429c7b48c85982f80ce2e90f2f56670d8ec262d5bfd95740ea100b98831f919`. The source projection was constructed **solely to test this PR #50 C file**; no combined PR integrations or final firmware binaries were built. There are no wall-clock latency measurements: “100 polls” means 100 host calls, not 100 ms.

## Proposed separately owned follow-up / acceptance

1. **SMC Transport:** define and test an accepted-or-pending contract for synchronous ATT replies (MTU, characteristic read/discovery, CCCD Write, error responses), including bounded retry, ordering, link teardown and duplicate request handling. Do not block the audio callback or displace release-critical MIDI; avoid counting unsent ACL frames against credits.
2. **Independent QA:** add *expected-success* negative tests that **fail** against the current PR #50 source when the mailbox rejects an ATT reply, then pass after the specialist implements its own repair; exercise GATT discovery/subscription, mailbox recovery and BLE reconnect. Review whether response bytes and CCCD state should be staged until acceptance, accounting for ATT transaction semantics.
3. **Performance instrumentation on hardware (owner stage only):** log actual mailbox occupancy/rejections, GATT retries/timeouts and audio XRUNs during iOS pairing/reconnect. **Ordinary use may never trigger this forced stress case.** Do not infer latency or device failure from these host polls.

**Stage:** exact patched-source scoped **HOST NEGATIVE REPRODUCED** / PR #50 ATT control-plane **NO-GO under deliberate saturation** / independent fix+QA pending / exact target build pending / DEVICE NOT TESTED. Production `main`, NIGHTLY, `gh-pages`, firmware, `ctl.img`, private toolchain/SDK/DSP, and samples are untouched.

X0X37-Role: LoopyX0X37 Optimization Watch  
X0X37-Stage: DOCS-ONLY / EXACT-BLOB HOST REPRODUCED / DEVICE NOT TESTED  
X0X37-Consumer: LoopyX0X37 SMC Transport; Independent QA  
X0X37-Source: production `main@c9718ae833eb64a5191c350922593df8a36017c9`; PR #50 `4aa93f62b5f08eb5df9663747ada84fef590a014`  
X0X37-Next: dedicated ATT-send rejection regression and separately owned repair; no integration/publication inferred.
