# Retire terminal CCCD failures without canceling SDK pairing

## Result and scope

The vendor adapter now handles the exact SDK's asynchronous CCCD completion.
A terminal rejection or SDK timeout retires the owned connection and uses the
reviewed BUSY/BUFFER_FULL disconnect retry. A successful acknowledgment alone
never declares MIDI reception. Genuine notifications may arrive before it.

Security-related ATT statuses `05`, `08`, `0c` and `0f` remain non-retiring in
an explicit unresolved state. The SDK's pairing and current JustWorks handling
are preserved. This repair **does not implement security-dependent CCCD retry**
and must not be presented as complete subscription recovery.

Base: retirement commit `75c0872`, whose parent is `4cf8c2f`. The supplied
terminal patch SHA-256 is
`9d308cb0267ea049d96384368d2db1bb4d066ee6c012825eaeaf7e9cf7e8a1d1`.
Applying it independently to `4cf8c2f` reproduces candidate source SHA-256
`054dc46e4439a41ef421ca28d123327894e59523b5f91ea853a61509a611f673`.
The combined source preserves the retirement repair's real-down state reset.

## Exact callback contract reviewed

The pinned `btstack.a` SHA-256 is
`d4c02f42e1be97a5288ecbcb5d42e6d0727e2ca4ffa2e0e2e6bb2ae61fede4bc`.
The `att_send.c.o`, `gatt_client.c.o` and `client_user.c.o` members were checked
against their local analysis objects byte for byte. GATT/client-user IR was
freshly regenerated with the pinned compiler and matched retained IR except
for the module path. No vendor code was executed in a native test.

The private static evidence establishes:

- `att_send.c.o.ll:1073-1102`: operation 5 submits an acknowledged write with
  the user callback; operation 6 is write without response.
- `gatt_client.ll:2531-2618,701-713`: the write stores its connection/attribute,
  enters state 24 and completes on Write Response.
- `gatt_client.ll:2121-2156` and `client_user.ll:798-844`: completion kind `A0`
  supplies the owned connection, attribute, offset and one status byte.
- `gatt_client.ll:3599-3629,4288-4318`: errors and timeout `7f` end the
  transaction and emit completion, not automatic connection retirement.
- `gatt_client.ll:894-902,958-970`: status `05` starts SDK pairing before
  reporting the failed write. Indiscriminate disconnect would cancel it.
- `gatt_client.ll:238-257`: notifications are processed independently of the
  pending write transaction, so early real input must not be dropped.

The adapter sets pending before submission. Only a one-byte, zero-offset `A0`
for that pending current connection and discovered CCCD can complete it. Wrong
peer/attribute, malformed, duplicate, read and pre-submission reports do not.
There is no CCCD read owner in this adapter. Zero offset is specific to its
current discovery/MTU/ordinary-write path; adding long/prepared operations
requires another review.

GATT event callbacks are synchronous (`gatt_client.ll:4222-4238`), and the
reported object is stack-local. Disconnection reports pending failure, removes
and frees the context (`:147-157`); completion removes its timer. The current
shim's timer deletion clears the slot without a deferred callback queue.
These facts support normal retirement before handle reuse, not an invented
callback epoch. An out-of-contract delayed old event with the same reused
connection and CCCD cannot be distinguished by the callback ABI.

## Additional defect found during independent combination

`down_pending` means an old link-down has not yet been copied to the app queue.
The original terminal patch guards discovery and completion against that flag,
but the old adapter could start scanning and connect a new peer before the copy
succeeded. Its one-shot discovery/A0 callbacks could then be discarded. The
existing JustWorks guard had the same ordering risk.

A callback-boundary regression reproduced the fresh-link stall: four assertions
failed and reception did not recover when congestion cleared. The final repair
blocks **scan enable and connect** until the old down has been copied. Scan
disable/OFF still works; `scan_pending` is retained and normal poll resumes
scanning after the queue accepts down. No queue ABI, capacity or event format
changes. The original ownership guards remain intact.

The final test checks congestion, no premature scan/connect, repeated polling,
OFF, OFF then ON, eventual down delivery, new peer JustWorks, discovery, CCCD ACK
and real notification. The original terminal+retirement source fails 15 of
these ordering assertions; the final source passes.

## Fresh checks

- The supplied terminal callback tests fail with 16 assertions against the
  retirement-only source and pass unchanged against the final source.
- `sh tests/ble/run_ctl_vendor_scheduler.sh` passes scheduler, event-copy,
  existing profile/retirement, new subscription, high-CPU and CPU-init suites.
- New cases combine terminal CCCD failure with BUSY and BUFFER_FULL, paced
  retry across millisecond wrap, actual-down ownership reset, synchronous
  completion and synchronous down during cleanup. Security statuses preserve
  JustWorks and early notification. Wrong/stale/malformed callbacks and late
  discovery after OFF or retirement remain covered.
- Profile and subscription suites pass AddressSanitizer and UndefinedBehaviorSanitizer
  with `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1`. LeakSanitizer
  is excluded because this executor is traced. These are source-adapter tests,
  not vendor-stack or hardware memory-safety evidence.
- `git diff --check` passes.

Run the subscription test alone from the checkout root:

```sh
cc -std=c11 -O1 -g -Wall -Wextra -Werror \
  tests/ble/ctl_vendor_subscription_test.c -o /tmp/ctl_vendor_subscription_test
/tmp/ctl_vendor_subscription_test
```

## Fresh controller-only link and fit

The exact retirement recipe is reused: central role, AVCTP extras 0,
`data-only-92k-ramload`, high-CPU BSS and CPU-init pool, no Classic key/sniff
and no Classic test modes. All tool/library/RF/recipe hashes match the base.
The only changed compiled input is `ctl_vendor_profile.c`; all imports resolve.

| Region | Retirement base | Combined final | Capacity | Remaining |
|---|---:|---:|---:|---:|
| Primary flash | 93,744 | 94,072 | 94,208 | 136 |
| Read-only flash | 7,272 | 7,272 | 8,192 | 920 |
| RAM-code load flash | 3,524 | 3,524 | 4,096 | 572 |
| Low controller RAM | 26,352 | 26,352 | 26,368 | 16 |
| High-CPU RAM | 6,452 | 6,456 | 7,168 | 712 |

All figures are bytes. Total loaded size is 104,868. The first combined source
without reconnect ordering used 94,040 primary bytes; the tested ordering
repair adds 32. No safety checks, features, audio quality or samples were cut.
Static fit, especially 16 bytes of low-RAM margin, is not runtime stack/heap
high-water proof. The linked diagnostic has a manifest placeholder and is
**NON-INSTALLABLE**.

## Boundaries and evidence

No public application/controller ABI changed. No application build, package
regeneration, installer transaction/head-check edit, device operation or HCI
replay was performed. The old active kit is not repinned to this source. A
future combined controller requires its own exact build and package admission.
Neither this work nor its native tests establish the historical failure's
cause or fresh hardware reception.

Private evidence is under `subscription-cleanup-review/`: combined native and
sanitizer logs, both negative controls, `sdk-member-check.json`,
`sdk-ir-check.json`, and `controller-final/result.json`. The adjacent audit JSON
records portable hashes and fit evidence. Proprietary SDK objects, toolchain,
IR and ELF remain outside source commits and distribution.
