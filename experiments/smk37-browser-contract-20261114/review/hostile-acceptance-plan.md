# Independent hostile acceptance probes

These probes target lifecycle and transmission invariants rather than repeat the source allowlist. They will run against the final importable transport and fake endpoint, once its API exists.

1. A locally held note retains its original channel; changing channel emits its release before exposing the new disarmed route. A duplicate down does not add an unmatched note count.
2. Invalid arguments reject before bytes or state mutation. Unsupported raw controls, SysEx, pressure, program changes, arbitrary CC, CC120/123 and F2 positioning cannot reach output.
3. A send that records bytes and then throws is treated as uncertain; cleanup attempts an original-channel release, but status remains unconfirmed. A send that throws before recording is also uncertain because the transport cannot distinguish them.
4. One release failure must not skip other note/control/Stop cleanup. Reconnection never replays old notes or old cleanup on a replacement endpoint.
5. Old-route incoming, close and timer callbacks cannot disarm or update the new endpoint. Incoming note/CC/bend/clock is observation-only and sends no reply.
6. Feedback rejects wrong channels and malformed values, is bounded, and clears at disarm/channel/disconnect.
7. Timeout covers notes, pedal, bend and locally requested play, without depending on a note being held. Hidden/blur/Escape/cancel routes perform the same disarm contract.
8. Connection status distinguishes connected fake/disarmed/armed/disconnected/fault or uncertainty. A successful browser API call does not assert firmware receipt, identified model or readback.

The check packet will retain real command/exits and hashes. No endpoint probing, permissions, firmware build, hardware qualification or production UI assessment follows from these tests.
