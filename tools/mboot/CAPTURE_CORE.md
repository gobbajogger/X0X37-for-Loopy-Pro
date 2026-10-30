# Platform-disabled read-only capture core

This is executable C source implementing PR671's bounded **pure collector, SHA-256
and lease-admission bookkeeping**, not another capability declaration. It is not
linked into firmware or imported by the installer. `mb_capture_platform_supported()`
always returns zero. Normal NIGHTLY MBOOT installation is still not implemented.

## API and ownership

Use `capture_core.h` with zero-initialized, single-owner `struct mb_capture`.
A separately reviewed caller must establish all four `MB_CAPTURE_REQUIRED` facts:
raw byte representation, qualified physical identity, actual global exclusion and
fresh session binding. Passing these bits is a caller assertion, not validation of
hardware or authorization to acquire bytes. Production must not synthesize them.
There is currently no qualified production caller.

1. `mb_capture_start`: validate fixed-width binding, Pro/Elite model, deadline and
   qualification facts. Reject an active context without clearing its owner.
2. `mb_capture_chunk`: consume an ordered, bounded raw chunk (maximum 224 B), updating
   full and first 16 KiB streaming SHA-256. Reject gaps/overlap, null/empty/oversized
   inputs and integer-overflow-shaped offsets. No heap or complete capture buffer.
3. `mb_capture_finish`: require exactly 1 MiB, compare independent target full/head
   digests; return digests only on success. Failed output remains unchanged.
4. `mb_capture_revalidate`: require new nonzero challenge, same still-held binding
   and digests. One successful revalidation per capture, so replay/repeated use of
   this transition cannot issue another receipt. The caller must obtain a fresh
   target response; this core does not authenticate a transport message.
5. `mb_capture_evidence_current`: recheck state, binding, epoch, selection and
   monotonic time immediately at use. This is instantaneous evidence, **not a
   writer token**. The actual device must serialize/exclude mutation continuously;
   no external lease transfer, locking or boot writer is implemented here.
6. Invalidate on faults, mutation attempts, disconnect or selection/session changes.
   Same-owner cancel clears bookkeeping; foreign cancel cannot release it. Clearing
   this context does not release a platform lock. Caller must perform its separately
   reviewed safe release before reusing a terminal context. No partial resume.

Every time value must come from the same trusted monotonic clock; the core rejects
backwards time and expiry at the deadline. The platform must choose a bounded lease
duration and detect clock rollover/reboot. Thread-safety, concurrent mutation and
physical freshness are not established by a C structure. The interface has no
callback, wire parser, I/O, private backup or writer. Non-null buffers must be valid
and disjoint from the context and output; callers must not edit internal fields.

All binding/selection/digests and raw input are private per-unit material. The core
has no logging/export. Invalid/cancelled state clears stored bookkeeping with ordinary
byte stores; this is not a cryptographic secure-erasure guarantee. The eventual host
backup adapter must add a private immutable storage lifecycle; this core cannot
produce a retained backup or installable artifact.

## Implementation and reproduction

Original GPL-3.0-only portable C; no third-party crypto implementation copied. SHA-256
is implemented here, not a crypto-library attestation or signing scheme. Hashes
provide byte consistency, not authenticity. The target core has no SDK/libc dependency;
bounded local byte helpers avoid hosted headers and compiler-emitted struct memcpy.

```
python3 -B tests/host/mboot_capture_core/check.py --output /tmp/mboot-core-proof
# Optional exact previously qualified compiler object check:
python3 -B tests/host/mboot_capture_core/check.py --output /tmp/mboot-core-target-proof \
  --target-compiler /path/to/jieli-linux-toolchains-20260730.1/common/bin/clang
```

The optional compiler must match the previously recorded exact hash before execution.
The script compiles with C11/Wall/Wextra/Werror/pedantic, runs 14 C test groups normally
and under ASan/UBSan, and compares 267 deterministic SHA-256 cases against Python
hashlib. Known-answer checks include empty, abc, multi-block and one-million-a;
streaming exercises both models, strides 1/63/64/223/224 and the head-boundary split.
Negative checks cover every qualification bit, identity fields, active-context
replacement, changed unit/model/session/epoch/selection, coverage, output preservation,
bad hashes, expiry at each boundary, backwards time, replay, cancellation, invalidation
between revalidation/use and wrong-state calls. All input bytes are synthetic.
LeakSanitizer is disabled for the existing ptrace environment; core uses no allocation.

Initial target builds found missing hosted string.h and a 32-bit size-comparison
warning; source corrections removed those dependencies. Final pinned pi32v2/r3
freestanding object compile passes strict flags. Symbol inspection finds no named
undefined dependency. Host context is 456 B (hash context 112 B); target object compilation
is **not** final target RAM/stack/link/timing qualification. Compression uses a 64-word
stack schedule and constants; hashing must run with XIP enabled over RAM chunks.
No firmware or upstream loader rebuild is required for this unlinked source stage.

Next: reviewed host private-backup/transport-adapter boundary using this usable core,
then actual target identity/session, complete writer barrier, RAM/time and strict wire
qualification before platform enablement. Read-only success still cannot solve the
independently blocked interruption-safe single-slot boot-write design.
