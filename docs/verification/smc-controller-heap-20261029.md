# Two-peer SMC controller heap provisioning

## Established failure

Owner testing of the credit-parser successor still failed with one SMC mixer.
Read-only Pro diagnostics identified a controller allocation failure, not a host
startup timeout: mailbox FAILED, error RESET (5), host CONNECTING (4), CREATE
step (8), five available ACL credits, and zero controller-to-host ring drops.
The controller image was independently read back as SHA-256
`5068a95e96fb2481b07577f76a39cd9d1fe9211c72bee00b3c262cdeec461266`.

The diagnostic word usually named `ctl_took` was `0x02089FE6`. On RESET the
public shim deliberately replaces that counter with its caller return address.
The exact image's ELF maps this address to the instruction after the RESET call
in `btctler_nv_memory_malloc`'s NULL-allocation branch. This establishes the
physical failure class. The specific outer allocation caller is not recorded.
Raw diagnostics, device data, vendor IR and disassembly remain private.

## Why the single-peer controller works and B cannot

The pinned controller's mandatory initialization allocations are 2048, 48,
2560, 56, 512 and 4334 bytes. The public allocator adds an eight-byte header on
this 32-bit target and rounds to eight bytes: 9608 bytes are already occupied
in B's 10112-byte heap. Only 504 remain.

First initiation needs live 32-, 52- and 40-byte objects (152 bytes charged).
Its 416-byte connection object then needs another 424 bytes before initiation
can be released. Even ignoring retained scanner state and transient events,
10184 bytes are required. B cannot fit that lower bound. A's exact known-working
controller uses 3690 rather than 4334 bytes for radio storage, leaving enough
space for its one supported peer. This is an allocation difference, not evidence
that the compiler recipe itself is broken.

## Bounded correction

- Retain two hardware links and master multilink support.
- Retain five RX packets and 27-byte ACL packets.
- Provision three shared TX ACL credits rather than five. The host discovers the
  count from HCI and already applies per-peer accounting and fair backpressure.
  This reduces maximum outstanding TX packets, not peer count or MIDI semantics.
- Increase controller heap from `0x2780` to `0x2980` (+512 bytes), using existing
  unallocated controller RAM. Do not move the RAM/flash limits or either stack.
- Update the locked configuration declaration to the intended three-credit value;
  retain the checker and all other declarations.

With this configuration, the pinned radio allocation formula gives 3746 bytes
(3760 charged). Mandatory initialization is 9024 bytes, leaving 1600 in the
10624-byte heap. The replay retains the scanner, both connection objects,
per-link hardware state and initiation until link creation, plus a 512-byte
explicit transient reservation. The reservation is a conservative test budget,
not a claim that every possible controller event interleaving was emulated.

## Host proof and limits

`python3 tests/host/ble_controller_heap_test.py` executes the unchanged public
allocator and tests A first-peer capacity, original B first-peer failure, both
insufficient partial corrections, and 100 corrected two-peer allocation/free/
reconnect cycles with the transient reservation. It also replays complete HCI/ATT
discovery for two peers with three credits at handle bases 0, 1 and 64.

The allocator test uses LP64 host execution, compensating each request for the
host's 16-byte versus target's eight-byte block header. It never accesses the
simulated payload. The host's larger minimum split is conservative. Assertions
check free-space accounting and reuse. It is a lifetime replay, not vendor RF
emulation or target execution. A standalone ILP32 harness compiled, but this
execution environment cannot run 32-bit ELF; no ILP32 runtime pass is claimed.

The old configuration fails the same candidate acceptance. ASan/UBSan host
checks use `ASAN_OPTIONS=detect_leaks=0` because LeakSanitizer is unsupported under
this environment's ptrace. The source still requires exact native controller
size/allocation/map checks, independent review and owner hardware acceptance.
Source proof does not establish two connected physical mixers. The earlier
credit-only parser correction remains valid but did not solve the physical fault.

## Public projection boundary

This public-source projection adds only controller provisioning and its tests to
the previously reviewed credit-parser tree `5b281a84cfecaf06aaeb08675884aaae7bd391d4`.
The older public host still lacks the private handle-zero correction. Its
three-credit discovery replay therefore covers nonzero handle bases 1 and 64;
the 0/1/64 evidence above belongs to the private B successor only. This branch
is not a replacement owner package, nor closure of physical two-peer acceptance.
