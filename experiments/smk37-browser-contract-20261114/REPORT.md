# Isolated stage acceptance

The source-backed MIDI matrix and fake-only browser transport are complete and independently accepted within this stage. No current remote-browser controller owner was recovered through the available safe source/PR/history reads. This is an ownership-visibility limit, not an assertion that every other thread is idle or absent.

The exact baseline is `94197fc839b00aff47b98ca02f3c6396e4a6eabf`, also the current observed `lane/026-combined-20261112` head. Production `main` is separately `035026fe70059af35e25c7c121b40f318b732fe3`; maintained `nightly` is `66073c68dd80d51a7053432c2c161a8b720a1341`. The archive ZIP integrity check passed and all 5,345 blobs match the complete, untruncated GitHub tree. Current ROADMAP consumed at `32b0cacd17c89f23bb2f16915732750d158b33e7`.

First substantive source consumption was the historical-editor/DeviceSync warning in `web/EDITOR_PROTOCOL.md` and `web/offline-sequencer/SYNC.md`. The independent investigator consumed the real dispatcher at `firmware/src/app/engine.c:1635`; the reviewer consumed that dispatcher and PLAN.md before reviewing the contract. `review/design-review.md` records initial design consumption; `review/final-run.json` and `review/browser-cancel-run.json` establish actual executable artifact consumption, beyond acknowledgement.

## Implemented boundary

The transport accepts only sealed instances of its own fake endpoint. It emits standard notes, CC1, CC64, pitch bend and F8/FA/FB/FC through named methods after explicit arming. Human channels1–15 are allowed; channel16 is conservatively excluded because SMC User mode intercepts controls there. Changing the outgoing channel does not change or discover device assignments. It preserves original-channel releases, clears observations on lifecycle changes, bounds logs, suppresses duplicate local presses, rejects unsupported commands and never echoes received MIDI. It latches delivery uncertainty and attempts every outstanding cleanup even if one send fails. Locally started/continued transport gets a best-effort Stop on cleanup. CC1 is an engine-dependent sound change and is not automatically reset; cleanup does not restore a saved sound snapshot.

The firmware matrix distinguishes normal musical ingress from SMC/APC controller modes. Notes can record into the actual instrument when its recording mode is active. Channel collisions, clip/handshake interception, mute state, part assignment, physical-wheel priority and clock ownership all affect response. No incoming stream observation is a parameter readback, identity handshake or ACK. Song Position adjusts APC phase, not sequencer seek. CC120/123 have no normal ingress panic implementation. Program change, pressure, arbitrary CCs and historical Felucca get/set/watch commands are rejected.

## Executed proof

| Check | Result | Evidence |
| --- | --- | --- |
| Producer transport suite | PASS 16/16, exit 0,0 skip | `evidence/producer.stdout`, `evidence/execution.json` |
| Independent hostile contract suite | PASS 22/22, exit 0,0 skip | `review/final-run.tap`, `review/final-run.json` |
| Actual Chromium acceptance | PASS 25 cases, exit 0; zero MIDI permission requests | `evidence/browser.stdout`, `evidence/execution.json` |
| Independent actual Chromium pointer cancellation | PASS, exit 0; notes/pedal/bend/transport cleanup, zero requests | `review/browser-cancel-run.json` |
| Exact archive/Git source closure | PASS 5,345/5,345 blobs; no tree truncation | workspace `source/verified-files.json`, packet `evidence/source-closure.json` |
| Shared checkouts | Clean before/after | no branch, source or UI writes |

Reviewed/executed transport SHA-256: `6bed0640913053805aba5518ab236609b517c6e21812b4120618aeb57ed1e489`.
Browser fixture SHA-256: `0a19c297683fe94deafd3745a49b774249f74712e82b92144a96acc8ab62c5cd`.
All check receipts pin the bytes that ran. Final code was unchanged during the recorded passing runs.

Resolved findings are retained: realtime prototype-name admission; mutable fake endpoint dispatch; a lease that depended on periodic polling; lifecycle cleanup invalidated by reconnect; and pointercancel releasing only C4 rather than disarming all local obligations. Stable independent RED captures 22 tests/20 pass/2 fail for expired-lease and reconnect behavior; final 22/22 is GREEN. The first independent run is retained without a stable-source or structured-exit claim. The first broad browser run failed cancellation and is documented in `evidence/initial-browser-failure.md`. The reviewer’s cancellation run occurred after repair, so it is not claimed as RED evidence. No expected result was weakened to obtain a pass.

## Reuse, ownership and limitations

Proposal `fde38845e7474fffae24ae835907d7ca9197b589` was consumed through its exact HANDOFF/PROTOCOL. Its prior owner/CAS model tests are reused as synthetic proposal evidence; they were not duplicated or rerun. Its listed results (18 focused/29 hostile methods and bounded explorers) remain prior producer evidence, not this stage’s passes. DeviceSync stays disabled. Its firmware/protocol and pattern owners, #900/#901, TouchOSC, emulator, UI and integration owners remain unchanged. The proposal’s recorded denial of a remote browser-tab metadata lookup is preserved in the copied handoff; that lookup was not attempted here.

Real Web MIDI/OS endpoints, permissions, firmware effect/audio, native firmware builds, timing/CPU/resource budgets, exact physical Elite/Pro compatibility, ACK/state protocol, hardware, deployment and publication are NOT_RUN/outside scope. A disconnected route cannot deliver cleanup. A successful fake enqueue cannot establish delivery. JavaScript suspension can delay cleanup beyond the arm lease; new sends check expiry synchronously, but this is not a realtime or guaranteed-silence system. Cleanup messages lack ownership/ACK on the device and cannot isolate concurrent clients sharing one physical ingress. This fake-only result grants no physical adapter permission.

State: COMPLETE for the assigned isolated prototype. Branch: NOT PUSHED. PR: none. No tool exposing `thread_management.create_thread` or a supported successor reader exists in this thread’s registry. No successor was created and no successor consumption is claimed. The exact next packet returns to the creator. No in-scope implementation blocker remains; ownership allocation and a supported creator are pending for the next stage.
