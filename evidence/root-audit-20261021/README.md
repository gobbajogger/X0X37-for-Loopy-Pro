# Controller, local controls, ingress and runner audit repairs

Input: `7b0bce55c5446a709fc3eecdafb993181654dcab`, including the completed SMC
connection retry and diagnostic repair. This is host and build-preflight
evidence, not a physical-device PASS or final development validation record.

## Repairs

- Publish a controller task's wait under the same IRQ lock as its condition
  check. Queue and semaphore posts at IRQ restore or immediately before the
  context switch cannot be overwritten by a later wait flag. Finite deadlines
  that wrap to zero remain timed. Restore IRQ state before yielding.
- Mark changed persistent SCALE/CHORD/ARP fields dirty at the shared setter.
  Local knobs and arrows now reach the existing autosave path. Runtime ON/LATCH
  toggles and unchanged values do not create persistent changes.
- Apply the existing opt-in SMC User/channel-16 predicate before APC ingress
  consumes a message, on USB and peripheral BLE, with notifications on or off.
  Direct SMC Bluetooth retains its separate connection and queue. This is an
  explicit message reservation on forwarded routes, not recovered sender
  identity. The routing documentation now states the APC-animation and MMC
  limits of a shared route.
- Reject unknown/unavailable or empty explicit test selections before running
  tests. Count admitted executions and reject a zero-test result. Inventory
  collection has no test-side effects. Update the admission fixture to retain
  inventory validation and account for a rejected shell redirection.
- Correct the additional negative signed shift exposed by the integrated DSP
  sanitizer. Unsigned exponent-bit adjustment preserves the numerical result.

## Evidence

The controller regression executes production scheduler code with real host
context switches and injected interrupt posts. All 19 cases pass, including
queue/semaphore/mutex wakeups, timeouts, zero-wrapping deadlines and a spurious
wakeup. UBSan passes; no ASan claim is made for the ucontext harness.

The new controls tests exercise extracted production controls, USB ingress and
autosave logic. The ingress test checks every recognized forwarded CC/note in
both clip states and APC handshake states, with BLE notifications on/off.
Ordinary APC feedback and User-mode-off behavior are checked separately.

The selected runner invocation executed 13 tests and returned zero:

```sh
X0X_TEST_JOBS=4 sh tests/run_tests.sh controller-wait smk-clip-settings smc-host-ingress smk-clip-knob smk-clip-wait smk-clip-dark smk-clip-usb-clear smk-clip-chord-display smk-arrows-owner smc-protocol runner-selections runner-admission gen-order
```

Per-test logs are alongside this file. The earlier overlapping exploratory
runner invocation had stale result interference; the isolated rerun above
completed with all 13 tests passing. It supplied the retained evidence.

The unchanged input fails the new controller wait assertion, settings and
ingress assertions, and the existing numerical suite under strict UBSan.
`checks.json` records the commands and expected nonzero baseline results.
The repaired numerical suite passes its documented libm error bounds under
strict UBSan. After integrating the sequencer/project repairs, the production
UI/engine suite passes 54 checks under ASan and strict UBSan with no sanitizer
diagnostics. Its two existing signedness compiler warnings are retained in the
log and are not sanitizer failures.

The controller preflight rebuilt from pinned local SDK inputs with cache writes
disabled. It produces 65,524 of 65,536 image bytes and uses 25,824 of 26,368 RAM
bytes. No device was opened or written. Final Elite application budgets and
exact artifacts belong to the subsequent `tools/dev_validate.py` record.

The installer/sample and sequencer/project evidence is recorded in their
sibling evidence directories. All original commits are integrated, not copied
as untracked replacements.
