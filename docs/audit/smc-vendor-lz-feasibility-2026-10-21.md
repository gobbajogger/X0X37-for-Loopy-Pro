# Optional vendor RAM-load compression: bounded study

The existing project codec compresses vendor initialized data well but saves
almost nothing on vendor RAM instructions. Including the measured load wrapper
and minimum metadata, compression can recover about 1.2 KiB of aggregate flash.
It does **not** release the additional contiguous application sector needed by
the current design. No runtime or startup path is changed by this study.

## Exact inputs and codec

Private input is `runtime-link-scopedcpu/resolved-NOT-FIRMWARE.elf`, SHA-256
`8096128a2f9c513c29a5e5f89ee0e41610295261139a7c1d1339213ab27b64c8`.
The recipe requires that exact input. Extract `.ramtext` and `.data` with the
pinned toolchain objcopy. They are contiguous in low RAM, so their concatenation
is also tested as one decoded output. No proprietary bytes are committed.

The host compressor includes the existing `firmware/src/app/lz.c` unchanged,
SHA-256 `05b40abb42ab2e34d08b48d0cb5c3b6214a2d0c1c3a7d02883e3cb70821092ee`.
This is the same codec used for projects, not a new Bluetooth compression scheme.

| Input | Raw | Packed | Gross saving | Minimum metadata |
| --- | ---: | ---: | ---: | ---: |
| RAM text | 3524 | 3442 | 82 | 16 |
| Initialized data | 1816 | 510 | 1306 | 16 |
| RAM text + data | 5340 | 3953 | 1387 | 16 |

The combined stream fits a 4096-byte storage extent with only 143 bytes spare,
or 127 after minimum metadata. This is storage arithmetic, not final placement.

## Full load-path cost and validation

The study wrapper requires fixed caller-admitted buffers and exact sizes,
rejects overlapping/wrapping source and destination, checks compressed CRC
before any output write, uses the existing bounded decoder, rejects trailing
compressed bytes, and checks decoded CRC before returning success. Failure may
leave partial output but never grants permission to execute it. A real caller
must not publish entrypoints, run IRQs, or start the controller until complete
manifest/source validation and decoded validation both pass.

Minimum metadata is 16 bytes: packed length, raw length and both CRCs. Format
version/flags, final manifest changes, load address ownership, boot sequencing,
cache synchronization and failure handling may require additional bytes. These
are not implemented. CRC is not cryptographic authentication. The descriptor
must be stable and admitted by the caller; no untrusted descriptor may select
arbitrary RAM destinations. No crt0 or application startup dependency changed.

A paired full-app target build retained an otherwise uncalled wrapper root:

- Before: 527460 bytes; after: 527636 bytes, **176 bytes incremental flash**.
- RAM `.data+.bss` 98100 and pool 335484 unchanged between the paired builds.
- All scoped target checks passed against the existing diagnostic app limit,
  not the proposed smaller app limit. Packaging stayed suppressed.
- Wrapper symbol 160 B, byte-source callback 24 B, decoder 144 B, CRC routine
  48 B, CRC table 64 B: 440 B of resulting load-path symbols, mostly shared.
- The baseline project codec was inlined; adding a second user outlines its
  decoder. Therefore the actual 176-byte linked delta includes the decoder and
  associated compiler sharing changes. Do not add another 144 bytes, or claim
  the standalone decoder symbol already existed in the baseline.
- Wrapper prologue saves 28 B and reserves 20 B; decoder saves 32 B and its
  source callback needs no frame: **80 B static nested path**. CRC's smaller
  frame is not simultaneous with decoding. Caller frames, IRQ nesting and
  target high-water verification remain additional obligations.
- No allocation or new 4 KiB compressor scratch is required on target. The
  compressor's 4 KiB hash table exists only in the host probe; output uses the
  already-declared vendor RAM destination, not a duplicate staging buffer.

The paired snapshot restored the known missing CORNER_OFF section annotation
in its private export before either build. It is not the earlier 527492-byte
snapshot or a claim about later parent edits. Before/after app SHA-256:
`8a036c7790d93a304b43033b56995cdf8f70dc70fb2a28d8add47b26152546bf`,
`3a548635398e76e0a1a781254ba2969e20293d0e606dcc2bc5aa6e867879b4fb`.

## Net bounds, not fit

Charging the full measured incremental wrapper once and minimum metadata:

- RAM text alone: 82 - 176 - 16 = **110 B larger**.
- Data alone: 1306 - 176 - 16 = **1114 B saved**.
- Combined: 1387 - 176 - 16 = **1195 B saved**.

Against the parent's then-stated 1072-byte aggregate storage deficit, combined
compression leaves only 123 bytes before the unimplemented costs above (data
alone leaves 42). Even moving all 1816 initialized-data bytes out of primary
leaves 91464 vendor primary bytes, **1352 above an 88 KiB primary**. Therefore
this compression does not release the required application sector. No feature,
sample, pool, stack or guard reduction is proposed to conceal that limitation.

## Reproduction and tests

```sh
python3 tools/studies/smc_vendor_lz/measure.py "$PRIVATE_ELF" "$JIELI_TOOLCHAIN" "$PRIVATE_OUT"
```

Outputs include proprietary bytes and must remain private, outside Git/bundles.
The script compiles the host probe from the same codec, extracts inputs, tests
all variants and records sizes/hashes in result.json. The committed wrapper
and `prepare_wrapper.py` reproduce the incremental target-root addition to an
existing non-installable app export; build that export before and after with
unchanged pinned environment and flags. It refuses a Git worktree and does not
change packaging or implement a callable startup path.

Round-trip bytes match for all variants. Native strict compilation passes;
ASan/UBSan passes on combined input with leak detection disabled for executor
compatibility. Tests reject every truncated length and every single-byte packed
mutation, appended data even with resealed source CRC, wrong decoded CRC,
overlap, initial invalid back-reference and oversized literals. Guard bytes
around malformed output remain intact. Hardware execution, manifest loader
integration and stack high-water tests remain pending.

Independent source review accepted this as non-installable sizing only. The
wrapper does not check whether its metadata pointer aliases the destination:
a real caller must pin or snapshot metadata outside the output range and bind
both CRCs to the admitted manifest/app version. The 123-byte residual uses the
older stated aggregate budget, not the runtime owner's later generation/layout
changes. No complete fit or executable activation was approved.
