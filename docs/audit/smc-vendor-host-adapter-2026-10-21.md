# Vendor host adapter checkpoint

This is a **non-admitted helper checkpoint**, not an SMC firmware fix. Runtime
integration owner: `reintegrate_reference_smc_transport`. Its parent owns final
assembly, layout, package and hardware admission. Default ABI-2 code paths remain
selected unless `CTL_VENDOR_HOST_TEST=1` is explicitly supplied.

## Proven lifecycle to retain

Exact donor SDK `64afb30b805af5f5d3c2e6471e970c3fd0ba40ad` host library:

1. `btstack_init()` starts the controller with its vendor private HCI transport.
   It waits for controller-ready, then creates the named `btstack` task.
   Do not call `btctrler_task_init()` separately.
2. The actual vendor task calls `btstack_task_init()`: HCI initialization, stack
   memory initialization, then `btstack_bredr_le_init()`.
3. The last initializer binds L2CAP to its real private pool and dispatcher,
   initializes ATT globals and invokes the application `ble_profile_init()`.
   Do not replace this with a no-argument `l2cap_init()` call: its actual API takes
   a stack-pool pointer and a five-argument dispatcher.
4. Application profile initialization registers DB/SM/GATT and HCI/L2CAP event
   callbacks after vendor HCI initialization. The tested donor selects central
   role, NoInputNoOutput, no proactive pairing, key size 7–16, GATT and MTU 128.
   An SM success/pairing requirement must not be inferred from transport counts.
5. The actual host task dispatches inline HCI events, private ACL pointer/length,
   credits and profile-loop work. Keep that vendor implementation rather than
   reconstructing it to fit a misleading direct-root size.

The controller's private ACL pointer belongs to the vendor host on core 1. Its
ATT callback payload must be copied into a bounded message before that callback
returns. No DMA pointer or borrowed event pointer can cross to the application.

## Implemented glue

`ctl_os.c` adds an opt-in two-task scheduler configuration, exact named queue
lookup, queue argument bounds, returned-task stack cleanup and idle-blocking
failure. Task stacks remain 512 target words; these changes do not justify their
size or reduce any guard. `ctl_os_bootstrap_begin(timeout_ms)` admits a window of
at most 5000 ms around the **single** vendor `btstack_init()` call on the already
owned init/run stack. Its idle delay and semaphore waits cooperatively run ready tasks until
the condition or bootstrap deadline. Exact controller-ready posts message
`0x40000C` and waits on an 80-byte SDK semaphore; this path is covered. Nested timer blocking and expired deadline use
existing `ctl_fail`; no third startup-task stack is allocated. The caller must
always close the window after successful return. `ctl_fail` retains the existing
stop-core failure behavior, not a successful return or a new recovery mechanism.

`ctl_vendor_events.c` adapts the public 12-byte `bt_event` ABI to a required
`ctl_vendor_event_copy(from, bytes, length)` callback. The integration owner must
implement that callback as a bounded copy and return 0 only on acceptance. There
is deliberately no weak empty/success implementation. Startup readiness is set
only for accepted source 3/event 3 and means neither connection nor physical SMC
control. Reset this state before initializing the stack.

## Remaining integration obligations

`ctl_vendor_profile.c` now supplies the application frontend using actual vendor
`btstack_init`, `user_client_init`, profile-search and ATT command helpers. It
selects only the exact advertised SMC-Mixer name, matches the BLE-MIDI service and
characteristic, and enables the vendor's descriptor discovery callback to locate
CCCD 2902 within that characteristic's range. It does not assume value-handle+1.
The public SDK report structures have target layout assertions. No HCI/L2CAP/
SM/GATT engine is rewritten.

The required `ctl_vendor_address` callback supplies the app's per-device address;
there is no tick-derived identity. `ctl_vendor_command` accepts only radio 0/1 and
one-to-three-byte MIDI feedback. Exact SDK BUSY−100 and BUFFER_FULL−97 normalize
to positive retry; stale/disconnected/off feedback is terminally rejected. Radio
off returns accepted immediately and retries its stop independently. The caller
must run `ctl_vendor_poll` regularly to retry pending stop/scan/link messages.
The bridge must preserve link-reset ordering before notifications and count
copy congestion. New linkup is generated on a real notification, not CCCD command
acceptance; that still is not proof of physical control dispatch.

These files do not implement the application mailbox bridge, flash segments,
timers, allocator, package admission or complete host library linkage.
The current timer `_to_task` functions ignore task names. Before admission,
confirm retained-library call paths or implement proper task-context callbacks;
the successful reference consumes queued callbacks inside `os_taskq_pend`.

A real vendor startup root probe exceeds the optimistic direct-core probe. Keep
fully resolved image/RAM measurements authoritative; do not drop required
initialization or introduce private structure guesses to meet a limit.

## Checks

- `sh tests/ble/run_ctl_vendor_scheduler.sh`: actual scheduler source executed
  with native `ucontext`: independent named queues, wrap/full, invalid arguments,
  semaphore/time wake, returned stacks, idle blocking, bootstrap limit/expiry;
  event adapter copy ownership, startup status and congestion. The separate
  frontend mock test covers actual adapter source startup/profile ordering,
  SMC selection, handle-zero discovery, nonadjacent CCCD, notification copying,
  feedback, exact busy statuses and radio-off cancellation.
- JieLi target compilation of `ctl_os.c` and `ctl_vendor_events.c` with profile 0
  and 1, warnings as errors, is separate from a full target link.
- Native contexts use larger libc stacks. These tests do **not** measure target
  stack high-water, vendor DMA lifetime, radio behavior or hardware controls.

All changes are newly written MPL glue around the existing MPL shim/public SDK
ABI facts. No Hortator GPL implementation or proprietary generated code is
copied into this controller source.
