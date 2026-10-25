# Storage staging consolidation (2026-10-25)

The firmware has only 300 bytes of static RAM and 68 bytes of app flash free
at reviewed baseline `e75fb2a584aa22978f352697a5e36fc9f886c3c5` (root tree
`6993010f1cd8630d6c911faeb7588ad5e4847d63`). This change lets native project I/O
borrow storage's existing full object buffer. It keeps separate bounded read/CRC
scratch so selecting an old A/B copy cannot overwrite an incoming save.

## Ownership and data safety

Storage and the project codec execute synchronously on the single main loop.
`st_put`/`st_get` callbacks finish inside their compression/decompression calls;
no object-buffer pointer survives to a later main-loop iteration. The other
storage caller, panel calibration, runs at boot outside project calls. Audio,
TIMER5 and core1 have no storage calls. Editor transfers retain their own header,
project stage and sample buffers across turns; they never retain project I/O
scratch. No previously independent operation acquires a new busy lock.

Both old-copy selection and final save readback use a separate static 256-byte
verification buffer. Full load staging remains: arbitrary external destinations
are written only after a complete valid body passes CRC. Internal shared scratch
can be invalid after a failed load and is discarded on error by project callers.
Independent and flash-resident save sources are copied into RAM before erasing.
There is no new large stack array, VLA, allocation, recursion or ISR scratch.

This proof depends on the current synchronous caller graph. Adding asynchronous
storage, nested UI/editor service inside a flash hook, an ISR/core1 storage caller,
or a retained object-buffer pointer requires a new ownership design. A lock only
inside storage would not protect a project compressor filling the buffer.

Header bytes, reserved fields, object IDs, CRC polynomial, format versions,
sequence ordering/ties, fallback, page program boundaries, header-last commit,
readback error propagation and flash allow-lists are unchanged. Project and
sample formats, recovery protocol, undo, controller ABI and all protected flash
regions remain unchanged. The full-size load read and native 256-byte IRQ-off
read windows are retained. Save verification now calls the read hook once per
256-byte chunk (up to 15 per full body); it adds bounded main-loop loop/park
handshakes. Physical timing and power-cut testing remain hardware qualification,
not a result of host mocks.

## Verification

`SAN_FLAGS=-fsanitize=address,undefined sh tests/host/run_storage_scratch.sh`
runs real storage code with shared incoming RAM, lengths 0/1/255/256/257/3839/3840,
all read/erase/page/header/readback hook faults, partial I/O, persistent failed
loads, corrupt newest bodies, malformed lengths, ties, flash sources and untouched
external failed-load destinations. It also extracts the exact baseline source
from Git and compares complete synthetic NOR bytes after 441 interleaved object
saves/loads, including panel. The two 1 MiB images are identical: SHA256
`04d9058a3d7c687cac97647c2effcdc84625fb3e5ab603749c08a0b7359a8b9a`.
Generated synthetic flash images stay in ignored build output.

`sh tests/host/run_project_stage.sh` compiles actual project and storage code with
the native shared buffer definition. It checks defaults, old fixtures, exports,
malformed data, capacity rejection, failed loads and protected sample metadata.
It also retains a transfer stage across complete independent storage operations
and verifies exact project-file header/CRC and staged bytes afterward.

The existing protocol and runtime fixtures cover cancellation, timeout, recovery
hold, project/undo preservation, queued save/input/audio behavior and SMC held
state. Their platform mocks complement the real-NOR/shared-buffer tests; they do
not emulate a physical NOR or multicore timing. Native section/frame results and
final check outcomes are recorded in the accompanying evidence receipt.

## Integration constraints

This is an isolated capacity patch against the reviewed installed source, which
was left immutable. It contains none of the separate current-repair (owner
01a172a2), SMC-navigation (01a172b7) or shared-fader-audit (01a172bb) work. Parent
must choose the combined reviewed integration head, preserve those fixes and
re-run affected storage/protocol/runtime tests and both native model budgets.
A clean cherry-pick alone does not establish concurrency or budget compatibility.
Changes to the caller graph or native unity include contract require re-review.

No new engines or presets fit merely because static RAM becomes available:
app flash and the pool reserve remain separate constraints. This patch does not
move allocations between regions, change the canvas, decompress break banks,
repartition flash, shrink sample slots, change controller topology, operate a
device, merge a branch or publish firmware. Hardware and integration acceptance
remain with their existing owners.

## Measured native result and acceptance

Both Elite and Pro DEV builds link at 552668 app bytes (4 bytes free), against
552604 baseline (68 free): this patch costs 64 bytes of app code. Static span
falls from 98004 to 94420 bytes, recovering **3584 bytes**, with 3884 bytes free.
The .bss reduction is 97620 to 94036 bytes; .data remains 368. Pool stays 335740
bytes, with 8324 free and only 132 bytes available above its required reserve.
This is a measured static saving; it does not create app or pool capacity.

Native assembly on both models shows st_body frames 12 to 32 bytes and st_current
88 to 92; the CRC implementation remains 8 and its wrapper tail-jumps with no
frame. Other affected callers/read callbacks retain their frames. Current-copy
selection chains grow 24 bytes; direct final-readback chains grow 20. No large
stack array was introduced. These are bounded changed-chain comparisons, not a
whole-program worst-case or measured device stack watermark.

Independent exact-candidate acceptance passed, reusing the unchanged sanitizer,
NOR byte, project-stage, protocol and Linux runtime evidence. Recovery read the
existing outputs and let the already-running Pro builds finish; it launched no
duplicate compile or unchanged host suite. Original final session exits were
unavailable after interruption; complete retained logs/products and a fresh
read-only build.check/mmio_check run on both candidate ELFs passed (exit 0).
See [native receipt](../../evidence/storage-capacity-20261025/receipt.json),
[section/frame metadata](../../evidence/storage-capacity-20261025/native-measurements.json)
and [independent acceptance](../../evidence/storage-capacity-20261025/independent-acceptance.md).
Full generated firmware, SDK/toolchain and synthetic flash images remain local.
