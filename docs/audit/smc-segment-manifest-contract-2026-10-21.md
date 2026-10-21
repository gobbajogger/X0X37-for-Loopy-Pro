# Segmented SMC test-profile admission helper

**This is an unused, source-only helper, not new firmware.** Default ABI-2 code,
all features, flash/RAM maps, samples and guards remain unchanged. The runtime
owner may adopt it only in the explicitly approved temporary central-only test
profile (SMC over Bluetooth, Loopy over USB). Both Bluetooth modes remain the
final goal. Hardware, actual fit and complete updater admission are pending.

## Format and checked ranges

`firmware/hal/x0x_ble_segment.h` is a new MPL-2.0 shared format/validator. It does
not include GPL application code or proprietary vendor code. ABI 4 avoids the
old, discarded ABI-3 experiments. A 128-byte aligned manifest contains exact
model/contract/profile identification, entrypoints, memory bounds, three fixed
segment descriptors and independent manifest integrity.

| Segment | Flash [start, end) | XIP start | Capacity |
|---|---|---|---:|
| Primary | `[0x87000,0x9B000)` | `0x02083000` | 81920 B |
| Secondary | `[0xAB000,0xAF000)` | `0x020A7000` | 16384 B |
| Tertiary | `[0xFE000,0xFF000)` | `0x020FA000` | 4096 B |

XIP follows the existing `fm1_xip.h` rule: `0x02000000 + flash_offset - 0x4000`.
The first 128 bytes belong to the manifest. Segment 0's payload CRC excludes
those bytes, avoiding self-reference; every other segment CRC covers all its
stored bytes. The separate manifest CRC covers the entire manifest with only
its own CRC field treated as zero. Thus it protects all descriptors and their
payload checksums. CRC detects corruption, not adversarial authenticity.

Each descriptor declares stored size and leading executable extent. All sizes
are four-byte aligned and bounded by fixed capacities. Init/poll must be two-byte instruction aligned
and lie after the manifest inside the primary executable extent. An additional
executable-range predicate can guard future IRQ registration; data-only regions
are never executable. Model identity is supplied by the compiled application,
not a new hard-coded model numbering scheme. The C test uses a synthetic model.

Low memory remains `[0x01C01800,0x01C07F00)`, with ISR stacks inside it. The
potential high region is data-only `[0x01C7D400,0x01C7F000)`. These are admission
limits, **not** proof that high memory is available, DMA-accessible, zeroed or
preserved across loader transitions. All task stacks and unproved RF DMA pools
must remain low. Pool headroom must remain at least 8192 B.

## Required integration touchpoints

The helper does not change or bypass any of these consumers:

1. **Profile/link/package limits:** `tools/build.py`, `tools/fm1pkg_make.py`,
   application linker source, controller linker/mkimage, locked settings and
   `gate_flashmap.py` must agree on the explicit test profile. Default limits
   remain unchanged. Account for manifest, alignment, all adapter code and
   data-load bytes in a fully resolved link before packaging.
2. **Storage owner migration:** `storage.c:st_owner_check` currently erases
   `[0x9D000,0xAF000)` on foreign/unknown owner. In the new profile its erase
   range must be actual objects `[0x9D000,0xAB000)` plus owner tag 0xAF000 only.
   `xb4_owner_erase_ok` expresses that proposed allowlist. Ordinary
   `fm1_flash_map.h:FL_STORE_OK` also currently permits the whole store and must
   exclude the new secondary region. Narrow controller permissions are separate;
   never broadly enlarge `FL_DATA` or protected ranges.
3. **Boot:** read a naturally aligned manifest snapshot, configure plain XIP
   before secondaries, validate model/contract/ranges/CRCs before any vendor
   entrypoint/IRQ, and keep the radio/update exclusion across validation and
   startup. A failed check leaves BLE off, not app/USB/updater disabled. Caller
   must enforce update exclusion; CRC validation alone cannot prevent races.
4. **Updater and readback:** baseline 816 exposes `ED_CTL_READ=37` for its 64 KiB
   block, **not a controller-write command**. Its installation path uses the
   package/loader. New segmented support therefore needs an explicit bounded
   write mechanism and bounded readback for each piece, with model/contract
   identity and matching backup metadata. `xb4_range_ok`, `xb4_erase_ok` and
   `xb4_payload_write_ok` are proposed bounds predicates, not write authority.
5. **Transaction:** park/quiesce the radio; write secondary pieces and read them
   back; erase/write primary payload while leaving its manifest erased; verify
   every payload; publish the fully sealed manifest last; read back and validate
   the whole set. Any interruption must leave either an unchanged valid old set
   or BLE unavailable. Do not publish a valid manifest before all payloads match.
6. **Legacy rejection/rollback:** the old app reads its ABI-2 header at 0x8B000,
   while this primary starts 0x87000. The actual newly packaged bytes at the
   legacy location must fail the old validator; do not assume new manifest magic
   alone proves that. Rollback restores a matching old app and 64 KiB controller.
   Old owner migration may erase the secondary, so it must never be needed after
   rollback. New app plus stale/erased secondary must reject the set.
7. **Memory and IRQ ownership:** map/zero/guard high CPU-only data explicitly;
   enforce low task stacks; park radio before loader transitions. Apply the
   executable predicate only after manifest admission and retain existing IRQ
   ownership checks. All ABI-2 reserved IRQs remain forbidden: 1, 11, 16, 20, 24, 37, 62, 63.
   No manifest field grants permission to take an app interrupt.

No sample region, stock rollback resources, OTA staging, calibration, keys,
bootloader policy or pool guard is sacrificed by this design.

## Tests and measured code cost

Reproduce native checks from the repository root:

```sh
cc -std=c11 -Wall -Wextra -Werror -O2 \
  tests/ble_segment_manifest_test.c -o /tmp/ble_segment_manifest_test
/tmp/ble_segment_manifest_test
cc -std=c11 -Wall -Wextra -Werror -O1 -fsanitize=address,undefined \
  -fno-omit-frame-pointer tests/ble_segment_manifest_test.c \
  -o /tmp/ble_segment_manifest_sanitized
ASAN_OPTIONS=detect_leaks=0 /tmp/ble_segment_manifest_sanitized
```

Both pass. LeakSanitizer itself fails under the executor's ptrace environment;
its leak detection was disabled only for the second command. AddressSanitizer
and UndefinedBehaviorSanitizer remain enabled; the fixture has no dynamic
allocations.

Coverage includes all short-header lengths, incompatible identity/ABI/flags,
manifest-byte flips, protected/overlapping/wrapping sizes, invalid entry/data/IRQ
bounds, every short segment length, every payload-byte corruption, mixed builds,
all partial-manifest publication prefixes, interruption after each changed
secondary/primary payload byte, foreign-owner erase preservation and rejection
after legacy rollback erases the secondary. These are tests of the helper and a
flash-array transaction model, **not the real loader/USB/flash driver**.

A standalone wrapper calling `xb4_validate` compiled with pinned JieLi clang
4.0.1, `-target pi32v2 -integrated-as -mcpu=r3 -mfprev1 -Oz -ffreestanding`,
produces 632 B `.text` and no global RAM. Clang SHA-256:
`c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f`.
This is not the final linked delta, and excludes the 128-byte stored manifest,
caller state, reader adapter, storage changes and update protocol.

The wrapper is reproducible without vendor libraries:

```c
#include "x0x_ble_segment.h"
int segment_check_probe(const xb4_manifest_t *h, uint32_t n, uint32_t model,
                        xb4_read_fn read, void *ctx)
{ return xb4_validate(h, n, model, read, ctx); }
```

Compile with `-I firmware/hal -c` and inspect `.text` using the matching target
`objdump -h`. Target compilation is not a firmware build or hardware PASS.
