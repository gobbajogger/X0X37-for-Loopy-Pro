# Pinned vendor CPU initialization scope

This opt-in diagnostic uses `CTL_VENDOR_CPUPOOL_TEST=1` together with the host
and high-CPU test macros. It does not make the firmware installable or prove live
heap/stack headroom. Vendor objects and protocol implementations remain unchanged.

## Boundary proof

The exact SDK task module creates the host task after controller readiness and
its bootstrap semaphore wait. The host task begins by calling task initialization:
HCI init, memory init, then LE/profile init. The first project-owned frontend hook
is `ble_profile_init`. The allocator scope begins immediately before the scheduler
first enters that specific `btstack` task and closes at the first line of that
frontend hook. It is NOT enabled for subsequent host task execution.

With LE-only modules2 and the pinned central configuration, the intervening path is:

1. `btstack_hci_init` calls `ble_hci_init`: clears CPU HCI state, writes event masks
   through direct controller scalar-copy functions and reads ACL configuration.
   Those controller calls do not allocate, queue work, or wait.
2. `btstack_mem_init` creates the LE connection1240-byte and GATT128-byte CPU pools.
3. `btstack_bredr_le_init`: the two app-settings helpers return0; `l2cap_init`
   initializes CPU lists and registers a static callback. Its classic controller
   call is excluded by modules2. Command circular-buffer and run-loop registration
   store metadata/callbacks without invoking them. `att_global_info_init` clears64
   CPU bytes, then calls the frontend hook.

Source evidence is the exact archive members `btstack_task.c.o`, `hci_vendor.c.o`,
`btstack_main.c.o`, `btstack_memory.c.o`, `l2cap.c.o`, `run_loop.c.o`,
`att_dispatch.c.o`, controller `hci_ll.c.o`/`hci_device_manager.c.o`, and the pinned
system circular-buffer member. No successful-path scheduler or semaphore operation
occurs within this phase. Assertions/reset on failure are not successful returns.

The frontend then opens separate DB and SM initialization phases for56 and480
bytes. Earlier memory/security audit records their CPU ownership and the current
synchronous, nonpersistent syscfg implementation. GATT initialization does not
allocate another GATT pool. Every phase must retain the same opaque scheduler
owner; ISR allocation cannot consume the phase's allocation sequence.

The four expected allocations consume1936 bytes including target allocator
headers/alignment. A4096-byte CPU-only pool leaves2160 bytes of internal reserve,
rather than pretending1936 is a certified peak. The reserve is NOT available as
fallback for runtime/controller allocation. The linker must prove the pool plus
high BSS and metadata stay within7168 bytes; no low stack or heap shrinks.

## Failure and ownership rules

- Exact expected phase and allocation sequence is a pinned-SDK contract assertion,
  not a size-based selector for unrelated allocations.
- Scheduler entry, blocking switch, current-task deletion, or task return during
  an active phase fails closed. Nested/missing/out-of-order phases also fail.
- Interrupt allocations, `ram_malloc`, and lbuf packets stay in the low pool.
  Only CPU allocations in the matching non-IRQ phase enter the high pool.
- A critical phase OOM or unexpected request calls `ctl_fail(XB_E_HEAP)` before
  returning to a vendor caller that could dereference NULL. No low/high fallback.
- Free selects the owning pool by address range, then requires an exact live
  allocation boundary. It does not depend on the current phase. Invalid and
  double frees fail closed in this profile.
- `ctl_scope_diagnostic()` exposes only phase, ordinal and requested byte count.
  It contains no key, packet or user data. Integration may publish this bounded
  code separately; it must not mislabel the code as free heap bytes.

## Rejected interposition alternative

A transparent noinline `__wrap_btstack_mem_init` with a volatile entry/exit marker
was linked against the existing full diagnostic using `--wrap=btstack_mem_init`.
The link resolved, but LTO eliminated the wrapper and marker and inlined the real
initialization into the task. Thus a resolved link was NOT proof of interception.
Compiling the original task member without LTO restored unsupported classic/TWS/
UART reachability, failed resolution and exceeded the primary region by12612
bytes. This route was stopped; neither experiment changes retained vendor source.

## Validation and remaining gates

`sh tests/ble/run_ctl_vendor_scheduler.sh` exercises original scheduler/profile
behavior and the actual scope/heap source with native mocks. Scope tests cover
four allocation phases, IRQ and DMA-low routing, no-yield guard, wrong owner,
nested/missing phases, wrong allocation sequence, invalid/double free and critical
OOM without fallback. Native pointer/header sizes differ from target: these tests
prove ownership/control flow, not the1936-byte target charge or hardware fit.

Pinned target compilation with warnings as errors passes with the opt-in macro0
and1. The parent baseline heap shrink/overflow, CRC and timeout tests also pass
when linked with the parent's current CRC/timer adapters. Final assembled target
link, strict segment bounds, high-region initialization/admission, actual startup,
phase allocation counters and runtime heap/stack high-water remain required.

### Resolved target route check

The integration owner's scoped diagnostic ELF SHA-256
`8096128a2f9c513c29a5e5f89ee0e41610295261139a7c1d1339213ab27b64c8`
places high CPU BSS/pool at6440/7168 bytes. Target disassembly confirms:

- `task_trampoline` calls phase1 begin before the vendor function pointer.
- `btstack_task` first calls `btstack_task_init` at0x020885ba.
- In that initialization, malloc1240 at0x02086288 and malloc128 at0x020862ae
  both call the actual scoped allocator. The remaining pre-profile calls are
  direct mask/config access and CPU list/circular-buffer/ATT-state setup; no wait.
- The frontend at0x02081392 first closes phase1, then opens/closes DBphase2 and
  SMphase3. Their malloc56/480 calls are0x0208da18/0x0208dc92.
- `heap_alloc` checks phase, ISR status and owner before selecting `cpu_first`.
  It retains the four-size contract and no-fallback failure path.

Thus the1936-byte target relocation is supported by assembled call routes, not
merely assumed from C scope placement. It remains a diagnostic, not hardware
execution evidence. In this ELF the unused diagnostic accessor/storage was removed
by LTO: phase/ordinal/size telemetry is **not yet available** until integration
retains or publishes it. Do not claim telemetry from this image.
