# Bounded security-dependent SMC subscription recovery

## Result

The isolated controller now retries the discovered CCCD once after an owned,
validated encryption success. Failed security recovery retires the connection
instead of leaving a permanent silent subscription wait. Independent source
review and application callback tests pass. The exact controller-only target
link fits, with **8 bytes of primary flash remaining**. This is an extremely
small static margin, not permission to add another firmware change without
relinking. No installable package or hardware result is produced here.

Base: `12ea396ba4902331b6f058e99d3175e8ed72e552` (terminal cleanup above owned
retirement). Final `ble/ctl/src/ctl_vendor_profile.c` SHA-256:
`f7e8f9ff41467acdc24951faa34f49daba67228f63482a721098d46daf2af794`.
The original preferred-v2 patch SHA-256 is
`08161c096490fab49a110a75a17b293879f81e70ed5326d85f415a6928b817cd`;
its unmodified combined source is
`bea226ae6b9c6a1494d3624e71a75264abd7088c716c4c03c52f740c24904b31`.
Both frozen originals and failed size probes remain outside this checkout.

## Behavior and ownership

- ATT statuses `05`, `08`, `0c`, `0f` enter security wait, without forcing pairing
  or changing authentication/key-size requirements. Existing SDK pairing owns it.
- An exact six-byte encryption-change success for the current pending peer
  schedules, but does not synchronously send, the same discovered CCCD `{1,0}`
  acknowledged write. Only one retry may be accepted per connection.
- BUSY `-100` and BUFFER_FULL `-97` do not spend that accepted-retry budget.
  The budget is reserved before submission so synchronous completion is safe;
  a synchronous retirement cannot be revived by an enqueue refusal.
- Owned encryption failure/disable, validated local/remote Pairing Failed, SM
  timeout, and a second security rejection retire the connection. Unknown flags,
  malformed callbacks and unrelated peers do not authorize recovery.
- Security wait is bounded at 35 seconds, allowing the SDK's 30-second timeout
  plus grace. Retry-enqueue contention is bounded at five seconds. Duplicate
  encryption success does not extend either deadline. Deadline handling remains
  active under old LINK-down backpressure, while writes retain publication order.
- Desired OFF is applied before polling. The combined mailbox/profile test also
  verifies authoritative ABI4 OFF precedes queued retry, MIDI and stale queued ON.
- CCCD ACK is not MIDI input. Real early notifications remain deliverable under
  the same ownership/copy rules. No read handshake, wake packet or fake input was
  added. Actual-down/new-owned-connection/startup resets the retry budget.

The independent reviewer rechecked the exact pinned SDK's static event shapes,
SM tag encoding, 30-second timer units, callback ordering and original-write
completion. The SDK queues the pairing request and does not switch tasks before
its A0 error completion, so this request's pairing success cannot race before
its own error under the reviewed cooperative dispatch. An unrelated prior
successful encryption is deliberately not cached as proof that a later
security/key-size failure has been resolved.

## Size repair without reducing behavior

Unmodified preferred-v2 resolves all imports but overflows primary by 392 bytes.
The final source saves 400 bytes from it through private boolean packing, shared
unchanged ownership predicates and cold actions, measured inlining, and an
identical-result public UUID comparison. Retirement/subscription/reason codes
and startup state remain full bytes. No externally visible structure changed.

Only eight genuine 0/1 fields share a private word. They have no escaping
addresses, ISR writers or second-core writers on the inspected cooperative
route. All reads/writes remain in the application profile; no synchronization
assumption was broadened. UUID equality still accepts exactly canonical or
reversed 16-byte values, using existing `memcmp` then a reversed comparison.
It is not a secret comparison. The inlining annotations affect code generation,
not checked conditions or deadlines.

Independent deterministic comparison produced **102,000 identical state
snapshots** between frozen v2 and the final compact source; trace SHA-256:
`e6bf5053763ece4db5d3e24ac5143d4643fdde6fdc10d504c14b1b0f58858363`.
The 32,768-case UUID matrix covers both orientations, every changed-byte value,
and zero/palindromic examples. No security, queue, stack, heap, sample, feature,
flash-capacity or RAM-capacity guard was reduced. No executable code was moved
into secondary data or CPU-init storage.

## Fresh checks

```sh
sh tests/ble/run_ctl_vendor_scheduler.sh
sh tests/ble/run_ctl_vendor_security.sh --sanitized
python3 tests/ble/check_segment_elf.py PRIVATE/controller-final/resolved-NOT-FIRMWARE.elf
```

The scheduler runner executes 11 native test binaries, including the existing
scheduler/event/profile/terminal/high-CPU/pool checks and the five new security,
edge, mailbox and UUID tests. The security runner executes seven relevant
binaries with ASan/UBSan. All pass. LeakSanitizer is explicitly disabled under
this traced executor; fixtures allocate no heap. These are application tests
with ordinary stub callbacks, not vendor execution, producer replay or radio.

The integrated retry test against exact terminal base fails 26 assertions
(exit 1), while the candidate passes. Independent reviewer testing also passed
all four frozen suites and three extra suites, each native and sanitized, plus
the sanitized deterministic differential trace. `git diff --check` passes.

## Fresh exact target link

The unchanged retained recipe is central role, AVCTP extras 0,
`data-only-92k-ramload`, high-CPU BSS, CPU-init pool, no Classic key/sniff and no
Classic test modes. The compiler/linker, seven libraries, system cbuf member,
RF table and four recipe-file hashes all match the terminal base. The only
changed compiled source is `ctl_vendor_profile.c`. Every import resolves.

| Region | Terminal base | Frozen v2 | Final | Capacity | Final free |
| --- | ---: | ---: | ---: | ---: | ---: |
| Primary flash | 94,072 | 94,600 | 94,200 | 94,208 | **8** |
| Read-only flash | 7,272 | 7,272 | 7,272 | 8,192 | 920 |
| RAM-code load flash | 3,524 | 3,524 | 3,524 | 4,096 | 572 |
| Low controller RAM | 26,352 | 26,352 | 26,352 | 26,368 | 16 |
| High-CPU RAM | 6,456 | 6,464 | 6,436 | 7,168 | 732 |

All figures are bytes. Final loaded size is 104,996. Frozen-v2 figures come from
the failed link's completed map, not a successful ELF. Final ELF extraction
passes malformed/truncated/type/protected-VMA rejection and external-record-tag
checks. The diagnostic retains its manifest placeholder and is **NON-INSTALLABLE**.
Static fit does not prove live stack/heap high-water or radio timing.

Reproduce with the exact pinned assets, using a new outside-repository output:

```sh
python3 tools/studies/smc_reference/link_runtime.py \
  --repo . --toolchain "$TOOLCHAIN" --libs "$LIBRARIES" \
  --rf-tables "$RF_TABLES" --system-lib "$SYSTEM_ARCHIVE" --out "$NEW_OUTPUT" \
  --controller-roles central --avctp-extra-commands 0 \
  --placement data-only-92k-ramload --high-cpu-bss --cpu-init-pool \
  --no-classic-key-sniff --no-classic-test-modes
```

## Handoff and limits

Private evidence is under `security-recovery-review/`: `controller-final/`,
`controller-security-v2/`, final native/sanitizer/ELF logs and the 26-failure
negative control. Independent review is under
`security-recovery-independent-review/`. Adjacent JSON contains portable source,
recipe, tool/input and artifact hashes. No proprietary binaries or IR are added
to source control.

The old production kit, controller pins, original installer transaction,
physical-head checks and receipts remain unchanged. The existing DEV builder
still rejects this new ELF until a separate combined owner reviews and updates
its exact ELF/result/region identities. Fresh app/paired-loader validation and
package admission are separate work; source approval is not package approval.
Session contract 3 is not included in this lane.

No claim is made about fresh hardware reception, the historical custom-host
raw-ACL stall, or universal SMC compatibility. Identically reused callback
handles have no epoch in the SDK ABI. Accepted retries depend on the vendor ATT
completion/timeout contract; time bounds require polling progress. A stopped
scheduler and persistent peer authentication/key-size mismatch remain limits.
