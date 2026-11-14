# Independent acceptance review

**Accepted for the bounded fake-endpoint prototype stage.** The final transport contract and plain engineering fixture have no remaining blocking finding in the exercised scope. This is independent review by `/root/contract_review`, not Sol, formal GitHub approval, firmware qualification or device acceptance.

## Input consumption and scope

First read `PLAN.md`, then actual pinned firmware `engine.c:1635–1795` at `94197fc839b00aff47b98ca02f3c6396e4a6eabf`. Checked `sequencer.c:1171–1183` and `1187–1228` to distinguish real-time clock/start/continue/stop from F2 clip-position observation. The design receipt is `design-review.md`. Reviewer owns only this `review/` directory; implementation remained with the parent. No firmware, shared branch, historical protocol, DeviceSync or physical MIDI operation was performed.

The transport admits only sealed fake instances, uses a conservative channel/message allowlist, keeps original-channel local cleanup obligations, and does not dispatch received observations into send methods. Cleanup attempts all local obligations independently, retires them before route replacement, and retains uncertain-delivery status across reconnection. Its ten-second arm lease cannot be extended by ordinary traffic or a repeated arm while still live. Fresh sends also enforce expiration before admission. Lifecycle bindings continue after connection replacement and explicit detach makes them inert.

The fixture accurately says FAKE ENDPOINT ONLY and does not claim firmware ACK, identity, audio or confirmed state. Pointer cancellation performs full disarm, including notes, sustain, bend and locally requested transport. The fixture is an executable diagnostic page rather than a production interface qualification.

## Findings resolved

1. Source review found that an ordinary object lookup allowed inherited names such as `toString` to reach real-time transmission. The implementation now uses a `Map`; independent hostile names reject without output.
2. Source review found that subclass or patched fake methods could escape the declared fake-only dispatch boundary. Fake construction now rejects subclasses and freezes instances/prototype. Independent duck-type, prototype-copy, subclass and patched-method probes pass.
3. Executable review reproduced expired-lease fresh-send admission and lifecycle binding invalidation after replacement. `second-run.tap` and `second-run.json` retain **20 PASS / 2 FAIL / 0 skip, exit 1** on stable transport SHA-256 `70fa13f2c2c6d18347e39a2458613b53538a77662f7ed10eed84c833c845baa9`. Both are fixed in the final source.
4. Fixture source review found pointer cancellation released only MIDI60 while leaving the route armed and other controls active. The fixed handler performs full disarm. The independent focused Chromium check verifies all four cleanup packet classes. No independent browser RED is claimed: the parent fixed the handler before this reviewer's browser run.

`first-run.tap` also preserves the earlier independent 21-probe observation, 20 PASS / 1 FAIL. That initial shell invocation did not preserve the child process exit in a structured receipt; the later RED and GREEN runs do.

## Executed evidence

`node --test --test-reporter=tap review/hostile.test.mjs`: **22 tests, 22 PASS, 0 FAIL, 0 skip; actual exit 0**. Node v24.19.0. `final-run.json` records equal before/after hashes; `final-run.tap` retains raw output.

| Final reviewed input | SHA-256 |
|---|---|
| transport.mjs | `6bed0640913053805aba5518ab236609b517c6e21812b4120618aeb57ed1e489` |
| review/hostile.test.mjs | `50a205e0cb51ae8646080ab15ec45839cbd215fb1ffa52605d057ac882da2159` |
| fixture.mjs | `0a19c297683fe94deafd3745a49b774249f74712e82b92144a96acc8ab62c5cd` |
| index.html | `09d79c1ca93443634beffeb976e4e251bfa87aa876b5c652076007e72e9083bb` |
| review/browser-cancel.py | `d4a5792c28c085fd197ec1d7ba628753702643c0f7535d6a2d8665f55b8ed70e` |

`python3 review/browser-cancel.py`: **PASS, actual exit 0** in system Chromium 154.0.8037.57 on Debian GNU/Linux 13. It exercised actual page pointer handlers using a real browser mouse-down and dispatched cancellation, after enabling local sustain/bend/start. Observed outgoing cleanup was Note Off `[129,60,0]`, pedal reset `[177,64,0]`, bend centre `[225,0,64]`, Stop `[252]`; status became `connected-disarmed` with no held notes. It recorded **zero MIDI permission requests**. `browser-cancel-run.json` preserves exact before/after source hashes, observations and the PASS result. Its loopback server and browser were stopped by this reviewer.

The 22 independent probes cover original-channel release, duplicate suppression, validation before mutation, unsupported controls, fake-only admission, before/after-enqueue failures, sticky uncertainty, continued cleanup after an exception, disappearance/replacement, no input echo, bounded defensive feedback, four absolute-lease obligation types, clock rollback, unpolled expiration, binding replacement/detach, Escape and pagehide.

## Limits and smallest next stage

This proves JavaScript contract behavior and a focused fake-only browser cancellation flow. It does not prove firmware effect, current mode/channel assignment, hardware model identity, real MIDI endpoint matching, browser/OS unplug behaviour, latency, no-drop delivery, mobile input, native resources, sound or multiclient isolation. JS suspension can delay periodic cleanup; after link loss a successful release cannot be guaranteed. The sticky warning remains necessary. The fake-only boundary is an engineering scope guard, not a security sandbox against arbitrary page-code modification.

Keep this exact packet with the creator. The smallest next implementation stage is one owner implementing a permission-free injected Web MIDI adapter seam plus synthetic port lifecycle tests, while keeping the fake fixture runnable and physical permission/request paths disabled until separately authorised. That owner must first consume the exact matrix and packet, confirm no equivalent active remote-controller writer, and preserve existing firmware/protocol/#52/#104/#900/#901/TouchOSC/UI ownership. No live protocol or firmware integration is assigned by this review.
