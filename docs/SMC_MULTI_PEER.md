# SMC multi-peer recovery: ingress foundation only

LX-REQ-007 remains incomplete. This branch supports **one direct BLE SMC**.
It does not enable two physical controllers, bind Pause, change STOP or restore
PR18's UI. No hardware, target build, flash, merge or binary publication occurred.

## Reconciled inputs

- Request tracker `ce8bfd4a40179590485ecf35e6bb0be2d652e602`,
  `coordination/requests/requests.json`, LX-REQ-007: per-controller locks,
  pickup, Solo and feedback; full completion requires combined overload/link
  tests, Elite/Pro and matching-controller builds, and multi-controller hardware proof.
- Transport PR122 `b037edac22f914bd14c6ca7aaa2988a530abd13d` is this branch's
  base. Its release-loss, preset-at-ingress, host cutoff and midpoll link checks
  are retained. Current `nightly` observed at
  `58e2c5205a02836df678083eaa8db9994b6192e8` has the same BLE/controller topology,
  but a different mixer. This branch is not a combined NIGHTLY candidate.
- Historical draft PR18 `d29f365df8b60436749b6712ca9d605d2f912708` was inspected,
  not merged. It shares one 32-event queue between peers with no release-loss
  recovery, clears producer link flags from the consumer, lacks session cutoffs,
  aliases invalid slots to zero, and changes STOP/Pause/UI behavior.
- UI/lock PR168 `b8794f9f10b0be68bf21579d43d4a6fef6cd4d72` owns separate work.
  Its neutral lock API and mapping must be reconciled by its owner at integration;
  no files or behavior from that branch are changed here. Physical Pause remains
  unproven. Refresh all moving heads before integration.

## What changed and why

`smc_ble_input_t` owns one peer's bounded event ring, read/write cursors, link
publication, lost-release generation and link release watermark. The existing
one-peer path uses one explicit instance. A future peer must own another instance:
there is no shared capacity or shared release-loss counter in this primitive.
No memory is allocated for a second peer in production yet.

The writer snapshots the preset in each event. On full capacity, a recognized
Solo release increments that input's loss generation without changing the other
input or overwriting unread messages. Recognition remains the production
caller's role/link/preset/channel policy. Invalid envelopes are rejected first.
The link helper captures that input's loss watermark and queue cutoff before
publishing a new generation, including coalesced down/up edges.

The real consumer is unchanged: copy before releasing a slot, acknowledge only
snapshotted losses, detect midpoll link changes, retire pre-edge events, reject
stale preset input and avoid rewinding acknowledgments after project reset.
The existing host loss/cutoff stays inside the same publication seqlock: current
single-peer disconnect intentionally clears host Solo as well. That coupling
must be redesigned explicitly with the application records, not duplicated for
peer 1. These are core-0 ISR/main-loop rules, not general multithreaded atomics.

The expected benefit is a reusable ownership boundary that prevents the old
shared-queue design from being reintroduced. It does not prove end-to-end
peer isolation for mapping, locks, pickup, Solo output or feedback.

## Native evidence and limitations

The registered `smc-ble-input` native test compiles the production header. Its
512 scenarios exercise both object identities and every byte cursor offset:
peer A saturation and lost release leave B's queued release and epoch unchanged;
A reconnect leaves B unchanged; copy-before-ack survives producer refill; a
post-snapshot loss remains pending; late-session loss survives the link watermark;
invalid lengths, preset snapshots and generation wrap are checked.

Existing production-C extraction suites check the actual single-peer consumer
with mocked UI/ISR callbacks. Three stale fixture behaviors were reconciled:
preset transitions release old Solo; unrelated-host reconnect tests use SELECT
24 rather than Solo 11; link/feedback fixtures include current STOP-drain and
epoch dependencies. The preset tests additionally assert unchanged loss counters
and that a rejected channel-16 release in stable DAW mode leaves channel-1 Solo
held. No failing production behavior was hidden or removed. The old fixtures
also fail against exact PR122 before those fixture corrections.

The native receipt records exact commands, compiler, exits and log hashes.
Independent review includes compiling runtime mutations for shared input,
erased release loss, incorrect reconnect cutoff and stale link watermark.
Counts and these mocks do not qualify the full firmware, target memory budget,
HCI concurrency, controller image or physical devices. Apple `gcc`/`cc` are
Clang aliases here; they are not independent GCC validation.

## Next bounded integration stages

1. **Application/transport owner, with PR168 owner:** choose one combined root
   containing the accepted current transport and UI; port this foundation there
   without older UI/Pause changes. Give each physical peer its own ingress,
   consumer acknowledgments, session identity, mapping/neutral lock snapshot,
   fader pickup/crossing history, encoder acceleration, held Solo target and
   feedback cache. Keep hosted USB/peripheral MIDI identity explicitly separate;
   it currently cannot identify physical SMC senders. Publish the union of held
   targets; releasing/disconnecting A must retain B's same/different target.
   Reject invalid peer slots. Project reset clears all; per-peer reconnect clears
   only that peer. Test blocked input, mapping/preset changes, page/bank changes,
   callback project reset and same-key holds across peers.
2. **BLE/controller owner:** introduce explicit peer-aware RX/TX and handle/session
   routing. Keep link publication before the first new notification. One shared
   HCI command arbiter must serialize controller-global setup, scanning, creation,
   cancellation and disconnect commands and route completions to their owner,
   including identical opcodes. Do not start a second `blec_start`: it sends HCI
   Reset and would invalidate the first connection. Maintain per-handle ACL
   outstanding accounting and one conserved credit pool across completed packets,
   disconnect, handle reuse and rejected mailbox sends. PR18's shared pointer
   alone neither recovers disconnected credits nor resolves command collisions.
   Test first-link survival while second scans/connects/fails and both disconnect
   together; preserve peripheral mode and clock routing.
3. **Controller/build owner:** current `ble/ctl/src/config.c` has `hw_nums=1` and
   `master_multilink=0`. A reviewed two-link configuration and matching rebuilt
   controller are required; any installed one-link image remains one-link.
   Update locked settings and BLE attempt records only in that explicit stage.
   Recalculate actual current `sizeof(blec_t)`, union/BLERAM/linker budget,
   code/static RAM/pool for both Elite and Pro. PR18's historical 340-byte central
   and spare-union fit calculation is not current evidence. Source-only stage
   has not acquired or published SDK/toolchain/controller binaries.
4. **Independent QA then sole firmware integrator:** pin the complete combined
   tree and test two-peer overflow (including A flood while B releases),
   coalesced reconnect, wrap, stale and fresh sessions, pickup/locks and feedback,
   HCI command and credit faults. Require genuine matching controller + Elite/Pro
   DEV builds and reviewed budgets. Normal source integration remains owned by
   the sole integrator; this branch requests no merge. Hardware testing and any
   installation/publication are separate, explicitly authorized stages. Only
   exact-device two-peer proof may change the physical capability claim.

Do not mark LX-REQ-007 complete from this branch. The tracker writer can record
this reviewed ingress foundation and retain the application/topology/build/QA/
hardware work as open. No scheduler or plugin maintenance is part of recovery.
