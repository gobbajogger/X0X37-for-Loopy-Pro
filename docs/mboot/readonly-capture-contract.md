# LX-REQ-039: read-only binding and consistent-capture dependency

Status: **proposed semantic contract + synthetic executable proof**, not wire
protocol, implemented firmware capability, acquisition permission or a boot writer.
Parent PR670's adoption/update separation and all preservation guards remain in
force. This is its next read-only dependency, not a replacement review of PR640 or
PR670. The normal NIGHTLY outcome remains OPEN.

## Source feasibility, not presumed capability

`readonly-source-closure.json` pins the inspected files. At the parent firmware:

- `fm1_flash.h:fl_read_ram` performs physical SPI command 0x0b with XIP disabled;
  `fl_read` parks core 1 and masks interrupts around the operation. This is a
  candidate raw-byte primitive, not an exposed full-snapshot service. It has no
  complete snapshot range/owner/epoch protocol. Encrypted XIP reads, CTL_READ's
  decrypted controller and logical sample APIs cannot substitute for it.
- `fm1_cpu1_park` uses a new sequence each call. A timeout holds/resets core 1 and
  records a fault; the wrapper discards its return. That is not evidence of unsafe
  flash execution, but it is not a successful nondisruptive capture either. A new
  capture wrapper must report this fault and invalidate its lease, never silently
  advertise normal controller continuity. Interrupt-save semantics and main-loop
  call context must be qualified; never invoke a raw read from an ISR.
- `storage.c:store_stage_lock` protects a shared staging buffer. It is not a global
  flash mutation barrier. A capture lease must cover settings/autosave, project,
  sample import, startup picture/text, OTA, controller loading and every raw erase/
  program caller, including deferred queues and other-core work. Audit all writers
  and gate them at a common admission boundary before claiming consistent capture.
- `fl_jedec_ram` identifies flash type, not a physical unit. Existing product/model,
  configuration key and MAC fields are not qualified unit-identity sources.
- Pinned MIT MBOOT upstream `platform.c:flash_uid` implements command 0x4b and an
  eight-byte UID. This establishes source feasibility only. Actual installed chip
  command support, uniqueness/stability, replacement semantics and invalid values
  remain unqualified. No upstream source is copied here; preserve MIT notices if
  reused later. No fallback to JEDEC/product/key is permitted.
- No firmware SHA-256 implementation was identified in the inspected tree. A
  target streaming digest implementation, known-answer tests, RAM/code-size and
  latency bounds are a concrete source prerequisite, not supplied by Python hashes.

## Proposed semantic exchange (no opcodes allocated)

Capabilities must explicitly advertise revision, exact model, firmware provenance,
raw physical address representation, flash capacity, qualified identity scheme,
maximum chunk, digest algorithm and maintenance-lease support. Unknown revision,
unsupported identity, wrong capacity/model or absent capability yields UNSUPPORTED,
not an attempted composition of old APIs. Current capability remains false.

A future private owner session supplies a fresh 256-bit challenge and random
owner-local binding salt. A qualified immutable UID may produce a domain-separated
SHA-256 pseudonym over fixed-length salt and UID, retained under that owner's local
salt across reconnect. The salt is not authentication and the digest is not hardware
attestation. Firmware must bind its real UID, not echo a host unit label. Qualification
must reject unsupported/all-zero/all-FF UID responses and address collision/flash
replacement risk. This option remains disabled until target identity is qualified;
no fabricated stable UUID or public MAC/key export is an alternative. A new session
challenge after reboot prevents replay; an unpredictable target session identifier
(or proven unique boot generation) remains an implementation prerequisite. A volatile
counter that restarts at zero is insufficient across reboot.

All raw bytes, unit pseudonyms, per-unit head/full hashes, salts and receipts remain
private/local to the owner. Public tests use deterministic synthetic bytes only;
public telemetry, logs, PR attachments and browser analytics must never contain a
real capture or identity material. Hashes of low-entropy/private content are not
assumed anonymous. Full raw capture necessarily includes protected configuration;
future export requires explicit private-channel/retention UX and separately allowed
device acquisition, not inferred permission from this specification.

1. **Acquire maintenance lease:** reject active mutation/import/update/audio-critical
   work; drain admitted writes and stop autonomous writers before issuing a lease.
   Bind owner session, challenge, physical pseudonym, exact model, source revision,
   transaction/package/controller/boot selection digest and mutation epoch. The
   device owns the lock, monotonic deadline and epoch. An epoch is not a substitute
   for exclusion. No lease exists in today's implementation.
2. **Stream:** exact raw `[0,0x100000)`; unsigned bounded offset; payload
   `1..max_chunk`, with `length <= capacity - offset` checked before addition or
   read. One outstanding request, ascending exact coverage; each response binds
   lease/session/challenge/epoch and requested offset/length. Reject unexpected,
   duplicate, gapped, overlapping, out-of-order, truncated or overrun responses.
   Transport lost response invalidates this first version; restart a new capture,
   no implicit partial resume. Target hashes the same raw bytes supplied to the
   host, in ascending order. The host independently hashes them and retains an
   immutable private backup in a separate future component.
3. **Finish:** under the still-held lease, device returns exact count, full/head
   SHA-256 and fixed epoch plus current lease binding. Host requires exact 1 MiB
   coverage, matching full/head digests, fresh response and durable local backup
   before supplying input to PR640. The synthetic model intentionally supplies no
   backup facility and cannot produce an installable artifact. COMPLETE_HELD is
   only an observation, not an adoption result.
4. **Revalidate:** a fresh never-reused challenge, same unit/session/selection,
   current epoch, same held lease and matching raw full/head digest are mandatory.
   Recompute on target under exclusion unless an implementation proves the captured
   digest is still valid through a complete writer barrier. A host-selected cached
   digest is not fresh target evidence. Package selection changes invalidate.
5. **Use boundary:** immediately before any downstream use, atomically validate
   live lease/deadline/binding and exclusion in the same device-side critical
   section. Never release and reacquire the lock between validation and use. The
   future writer, if separately designed and approved, must consume/transfer the
   same lease atomically; a host boolean or completed revalidation response is not
   a write capability. This stage implements no transfer or writer. Synthetic
   `eligibility_only` can only report current read-only evidence.
6. **Invalidate/release:** expiry (at deadline, not after), disconnect, reboot,
   owner cancel, package reselection, core/flash fault or attempted mutation revoke
   all capture/revalidation receipts. A mutator must first revoke, then wait for
   the in-flight raw read to finish before admission; no write may overlap a read.
   Cancellation by another owner cannot unlock this lease. Reconnect starts a new
   challenge/capture; identity similarity never resumes a stale lease. Releasing a
   read-only lease is clean only because this protocol has no flash mutation. It
   does not change PR670's OUTCOME_UNKNOWN rule for a future possible boot write.

## Bounded implementation sequence and qualification gates

Candidate payload bound **224 bytes** matches an existing editor read's data
ceiling, but metadata consumes framing space: choose the final payload only after
encoding and transport tests establish total frame limits. At 224 bytes, one MiB
requires 4,682 chunks (last 32 bytes). Target memory should be O(chunk + SHA state),
not a one-MiB allocation. Hash outside XIP-off/IRQ-off intervals over the same RAM
bytes; keep SPI code, needed constants and destination in verified RAM. No proposed
millisecond timeout is a measured guarantee. Establish SPI worst-case duration,
core-park timeout/fault behavior, watchdog servicing, USB/MIDI responsiveness and
maintenance-only playback/controller semantics before enabling capture. Firmware
must impose its own maximum lease duration; host keepalive cannot extend forever.

Next source work can implement and host-test a bounded pure collector/digest and
lease admission core with all platform support disabled, then qualify the actual
writer closure and target UID/digest/RAM primitives. Such source does not authorize
device probing. Firmware feature advertisement remains off until all of those
facts are reviewed; no unsupported capability bit for the sake of advancing UI.
A production host adapter additionally needs private immutable backup lifecycle,
strict encoded-message parsing and tests; this model is not that adapter.

The independent boot-write problem is still unresolved: read-only capture cannot
make single-slot boot erasure interruption-safe, create a persistent journal or
prove ROM fallback. No writable transaction follows automatically from this review.
Ordinary NIGHTLY update remains its existing separate path with unchanged 16-KiB
head comparison and known-loader guards.

## Executable proof and non-proof

Run `python3 -B tests/host/mboot_readonly_contract/test_capture.py`.
The model hashes one MiB of synthetic bytes in bounded chunks (including the chunk
crossing the 16-KiB head boundary), rejects coverage/digest/binding/replay/expiry
failures and invalidation between revalidate and use, and isolates owner cancel.
It has no file/device/wire backend, write method or real UID, and retains no raw
capture. Its binding/target digests are synthetic assertions. It does not prove
cryptography, transport integrity, physical identity, a global writer barrier,
private backup persistence, target timing or hardware functionality.

Source-pin reproduction:
`python3 -B tests/host/mboot_readonly_contract/check_source_closure.py --firmware . --upstream /path/to/pinned/mboot`.
No unchanged PR640/PR670 build/review budget is reset by this scoped continuation.
