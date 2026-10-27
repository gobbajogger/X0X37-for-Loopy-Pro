# Two-SMC integration: source and host qualification

Base: `fb9eb07c5d8eba1fcc931ee060f9ad9c502276b4` (the delivered PR639 source).
This is the missing LX-REQ-007 integration, not a repeat of LX-REQ-005/006.
The base already contains the spatial mixer, captured context, 900 ms unlock
and clock-following lock feedback. Its controller and application are single-link.
PR18 and PR174 were merged into historical lanes, not this delivered tree.

## Changes

The application now has two separately addressed ingress rings, reconnect epochs,
release-loss generations, control contexts, pickup histories and feedback caches.
Only the main loop selects a control context. Interrupt producers use an explicit
peer index. Hosted User MIDI/MMC remains associated with peer zero, as before.
Solo publishes the union of both peers. Held FX have separate owners; peer zero
has priority for competing effects on one bus until release. The temporary
screen renders the last touched peer without changing the underlying UI.

The transport demultiplexes two links, serializes HCI commands, retains command
credits, accounts for ACL packets per handle and returns outstanding credits on
disconnect. A failed or reconnecting second link must not reset the first. Role
replacement waits for both links, pending commands and controller command credit.
Controller settings explicitly change hardware links from 1 to 2 and master
multilink from 0 to 1; the locked-settings record and BLE attempt row match.

## Executed proof

- Production application integration: 5,768 checks, including 512 two-peer
  queue cursor/generation-wrap, overflow and reconnect cases, PASS with ASan/UBSan.
- Five compiling application ownership mutants are rejected by runtime assertions.
- Dual HCI/controller model: 167 checks, PASS with ASan/UBSan; five compiling
  transport mutants rejected. First-notification checks: 95 PASS.
- Existing lock/clock 5,141, lock interaction 714, Pause-loss 414, full SMC UI
  6,510, renderer bounds 315, feedback 1,920, queue 42 and reconnect 48 checks PASS.
- Solo overflow, host data validation/sender isolation, all five reachability
  routes, drum mutes, real recording and performance-abort regressions PASS.
- Legacy BLE central, signaling, stop/deadline, abort and role-switch checks PASS.
- Two drum p-lock assertions in the 124,992-check drum R/S suite FAIL identically
  with the candidate and the exact delivered base app source. They remain FAIL,
  not a passing regression gate. The source-selectable runner retains both cases.
- Several retained extraction fixtures assumed numeric part IDs were physical
  strip positions. They now use the already-delivered 909/808/BREAK/303B/303A
  order. No production part mapping was changed by this package.

LeakSanitizer is unavailable under this runner's ptrace setup. The successful
ASan/UBSan runs use `ASAN_OPTIONS=detect_leaks=0`; no leak-proof claim is made.
Private execution logs and input hashes are under the integrator's
`smc-current-evidence-20261027/` directory.

## Delivery gate

The first actual two-link controller link exceeded the unchanged 65,536-byte
Bluetooth block by 1,144 bytes. The controller recipe now emits the resolved LLVM
module, applies an explicit ordinary vendor `-O2` optimization pass with the
existing zero inline threshold, and lowers it with ordinary vendor `-Oz`.
The final native link uses the original linker script and all flash/RAM guards.
No function-merge/address-identity relaxation, feature cut, diagnostic removal,
RF change, heap/stack reduction or flash-map change is used.

The full native controller build passes: 61,088 bytes, 4,448 bytes of block space
free; RAM 25,824 of 26,368 bytes (544 free); ABI 2. Ten orchestration tests pass,
including failed emit/optimization/codegen/final-link stages, malformed IR and
interruption cleanup with descendant reaping. Failed stages retain no stale
successful image. Exact-head Elite/Pro app and package qualification follows
this source checkpoint; the earlier app-only diagnostic is not a deliverable.

A production red/green check also covers project reset: all peer runtime owners
are cleared before any FX initialization can publish, so peer one's old-project
effect cannot be transiently re-armed while peer zero resets.

Independent exact-source review and exact-head full matching controller/app packages
are still required. Physical dual-device radio, timing, lamps and installation
remain a separately authorized hardware stage. Existing single-SMC owner
acceptance is retained; it is not evidence for changed two-link controller bytes.
No device operation, external Git write, merge, release or publication occurred.
The root integrator owns source delivery and appoints the independent reviewer.
