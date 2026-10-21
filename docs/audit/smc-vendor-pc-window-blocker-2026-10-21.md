# Segmented vendor-host execution: PC guard blocker

The proposed 84 + 16 + 4 KiB layout does not yet support a guard-preserving
vendor-host startup. This is a source and linked-section finding, not a hardware
result. No application adapter, layout constant, packaging path, or default
runtime was changed for this finding.

## Existing hardware admission

`firmware/hal/fm1_guard.h` defines two PC-limit register pairs (offsets
`0x380/0x384` and `0x388/0x38C` in the debug register block). Its existing
`fm1_guard_enable(FM1_GUARD_PC)` uses:

1. One continuous XIP interval from `0x02000120` through the application text,
   extended to the controller primary block when BLE starts.
2. One continuous RAM interval for application and controller RAM text.

The source states these limits also apply to core 1. Both intervals are already
needed. The inspected local sources do not establish a third independent PC
window. This is not a claim that no undocumented hardware facility exists.

Executing controller instructions from the proposed second segment
`[0xAB000,0xAF000)` or third segment `[0xFE000,0xFF000)` would require widening
the existing XIP interval across unrelated store/sample regions, or replacing a
needed RAM execution interval. Neither preserves the current guard contract.
CRC and entry-pointer validation alone do not constrain subsequent instruction
fetches. Temporarily changing windows around callbacks also does not establish
safety for concurrent core execution and vendor interrupts.

## Concrete linked-section bound

The runtime owner's non-installable `runtime-link-central-roles` measurement
reports a resolved link, no undefined symbols, 104,536 bytes total flash extent,
and 28,664 bytes static RAM extent. Its `.text` section is `0x16618` = 91,672
bytes. The candidate primary is 84 KiB = 86,016 bytes.

- Text alone exceeds primary by **5,656 bytes**.
- Text plus the proposed 128-byte manifest exceeds primary by **5,784 bytes**.
- This lower bound needs at least **92 KiB** of sector-aligned primary capacity.
- Moving only read-only data and initialized RAM load bytes to secondary pieces
  cannot meet this executable-code bound.

The measurement has not been relinked to that layout. Alignment, adapters and
new admission code can add costs; these numbers are necessary bounds, not fit
proof. Existing application RAM code and controller `.ramtext` are additional
to the `.text` quantity above.

Evidence digests from the owner's measurement:

- `sections.txt`: `008ec4ba7f28814dc0790e809d1e9a7a588e71fb6b64fbf86f6c9b6b86fe97f2`
- `resolved-NOT-FIRMWARE.elf`: `82f80bbb3518571beb8623e080a9e7cded4fe65e2874fbe9d6b7f3bbbf87c409`

The measurement inputs and runner belong to the runtime owner's study; no
vendor or generated binary is included here. The arithmetic is reproducible
from the `.text` row of the runner's `objdump -h` output.

## Required next decision

Before enabling an app-side startup path, demonstrate one of:

- Enough contiguous executable primary capacity while retaining a fully
  functional app, all sample capacity and all guards.
- A measured executable-code reduction of at least the deficit above.
- A safe relocation of enough code into an already admitted executable RAM
  interval, with complete RAM/stack/heap ownership and lifetime proof.
- A documented, independently reviewed additional hardware isolation mechanism.

Data-only secondary pieces remain structurally possible. High RAM previously
proposed as CPU-only data must not silently become executable, and the PC RAM
interval must not broaden across application data to reach it. The current
fixed 80 KiB manifest helper is unchanged; neither it nor a proposed 84 KiB
variant solves this execution-window constraint.

No app bridge target build, installable package, hardware test, or physical SMC
control result is claimed by this note.

Independent source review confirmed the two-window constraint and found no
source-backed third window. It identified low-RAM execution with secondary load
storage as a structural possibility only, not a demonstrated fit; high-RAM code
would broaden the RAM interval across unrelated application memory.
