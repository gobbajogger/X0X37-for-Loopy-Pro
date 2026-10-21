# Opt-in CPU-only high BSS diagnostic

`CTL_VENDOR_HIGHCPU_TEST=1` requires the vendor-host test macro; its default is0.
This is source preparation for a **non-installable diagnostic**, not ABI admission,
new heap routing, or a safe firmware package. The default macro emits no section
attributes and no initialization code.

## Ownership

`.ctl_highcpu_bss` contains only these original project-owned objects:

- `ctl_os.c`: `tasks` (576 bytes in the prior target map), `cur`, `idle_ctx`,
  `bootstrap_deadline`, `bootstrap_active`, `bootstrap_running`. These are CPU
  queue/context records and scalar bookkeeping. Stack pointers still refer to the
  original low task stacks; the actual stack contents are not moved.
- `ctl_timer.c`: `timers` (320 bytes), software timer records, not hardware DMA.
- `ctl_vendor_profile.c`: `att_storage` (828) and `search_storage` (512), CPU ATT
  send/discovery storage; `request` (13), MIDI/CCCD handles and frontend flags.
  The vendor consumes the request through its CPU command path; it is not an RF
  descriptor. The initialized `connection=ABSENT` remains in low initialized data.

The first four arrays total2236 bytes before additional scalars and alignment.
The previous mapped frontend BSS was49 bytes and scheduler bookkeeping20 bytes;
exact post-placement section size must be obtained from the final link because
explicit four-byte alignment and LTO representation can change padding. This
adds slack beyond the current2280-byte low-window overage without reducing any
stack, RF pool, heap, app guard or sample space. No size/fit claim replaces a map.

## Required integration ordering

The parent must add a separate `NOLOAD` output section whose start symbol
`__ctl_highcpu_start` is exactly `0x01c7d400` and whose aligned end symbol
`__ctl_highcpu_end` is nonempty and at most `0x01c7f000`. Keep the low BSS symbols
separate. Add `ctl_highcpu.c` to the diagnostic build. After clearing low BSS,
call `ctl_highcpu_init()` exactly once, **before any timer, scheduler, frontend or
vendor startup access**, and fail closed on nonzero status. Do not insert the
call late inside `ctl_vendor_startup`: it reads frontend state first.

The helper validates those bounds then zeros the full7168-byte admitted region,
not just the currently occupied bytes. Its one-time latch deliberately remains
low BSS so a repeated call cannot erase live queues or state. Future ABI boot
admission must prove app/loader lifetime exclusion for the whole region and retain
the protected top page. The helper cannot establish those conditions itself.

## Validation

Native actual-source scheduler, event and profile regression tests pass. The new
Linux mmap test validates wrong start, empty/unaligned/overflowing end rejection,
zeroing of the entire allowed region, untouched sentinel bytes below/above it,
and rejection of a repeated clear. These tests do not prove hardware addressability.
Pinned pi32v2 compilation with warnings as errors passes for all four affected
translation units with high-CPU macro0 and1. Vendor-only fixed pools, actual
stacks, run/ISR stacks and RF/DMA storage are unchanged. No vendor library or
private representation is edited.
