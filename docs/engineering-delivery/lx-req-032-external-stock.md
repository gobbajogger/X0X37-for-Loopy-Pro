# LX-REQ-032: exact stock loops outside the firmware image

## Result and scope

The approved CRAMEN, TASTY and HOTPANT loops fit in the two existing SMK-37
sample slots as three zones. The delivery copies every original ADPCM byte and
keeps all samples at 44,100 Hz. It does not decode, encode, normalise, truncate,
resample or change the unused high nibble of an odd-length stream. It leaves
the main app image available for the compact Chicago effects.

This is an offline data export and replacement plan. It performs no device
access, network access, firmware packaging, sample replacement or publication.
The root integrator owns app/profile integration and the final native packages.

Source base: `6ae0928fe3264ee759c25c4f879eff4dc5dc3b20`.
Approved request: `LX-REQ-032`; explicit export profile: `lx-req-032-compact`.
Models: `elite` and `pro`, with separate labelled manifests and archives.

## Exact source and placement

The extracted source bank is 131,152 bytes, SHA-256
`cbfe682a9c69b51dac4ee96db7ab4605c8ecdb2839230efcc5445aa0a8fe199e`.
The tool requires that complete identity and an extraction receipt identifying
the same size and hash. The receipt is included unchanged in the distribution.

| Loop | Samples | Bars | ADPCM bytes | Physical slot / zone | LOOP menu with no built-ins |
| --- | ---: | ---: | ---: | --- | --- |
| CRAMEN | 70,589 | 1 | 35,295 | 1 / 0 | BR1, index 0 |
| TASTY | 105,827 | 1 | 52,914 | 2 / 0 | BR2, index 1 |
| HOTPANT | 85,725 | 1 | 42,863 | 2 / 1 | BR3, index 2 |

Zone and menu indices in this table are zero-based. Physical slot numbers are
one-based. Every stream starts with predictor 0 and step index 0 and uses the
low nibble first. The manifest records original names, bar counts, sample
counts, rate, byte offsets, hashes and decoder state.

| Slot | Flash offset | Capacity | Image including 512-byte header area | Remaining capacity |
| --- | --- | ---: | ---: | ---: |
| 1 | `0xC2000` | 81,920 bytes | 35,807 bytes | 46,113 bytes |
| 2 | `0xD6000` | 122,880 bytes | 96,289 bytes | 26,591 bytes |

The first image's SHA-256 is
`23c0958391ad386bd204e7238a2b5c07dc09d18d541369106b46c9fbf5fef5a2`.
The second image's SHA-256 is
`58b1f67d84a42f9daa3dae60f40c234c2f79c6317cf053cb29b4afa793ed82ec`.
These sample bytes and the raw slot images are identical for Elite and Pro;
the surrounding distribution and archive identities name the selected model.

## Firmware and format findings

[`slots.c`](../../firmware/src/app/slots.c) defines two slots, each with up to
16 zones. Its leading comment saying that X0X plays only zone 0 is stale.
[`plat_slot`](../../firmware/src/app/plat_fm1.c) accepts a zone index and returns
that zone's complete data pointer, sample count and rate.
[`scan_loops`](../../firmware/src/app/engine.c) enumerates every valid zone in
slot order, while `engine_brk_loops` selects the chosen slot and zone for both
A and B. Multi-zone playback does not require a firmware change.

The FSMP header has one eight-byte name for the whole slot. It has no per-zone
name or bar-count field. The two complete-set images therefore use slot names
`STOCK1` and `STOCK2`; the menu uses BR1, BR2 and BR3. The original loop names
remain in the bank, separate ADPCM filenames and manifest. The production
`loop_bars` calculation infers exactly one bar for every approved loop. The
exporter refuses a loop whose declared bars would change through that policy.

The header is 480 bytes: a 32-byte prefix plus 16 descriptors of 28 bytes.
Data starts at offset 512. The export uses the existing version-1 FSMP fields,
complete contiguous zones, initial state 0/0 and the existing zlib CRC-32.
It derives capacities from `fm1pkg_make.SLOTMAP["smk37"]` and shared format
constants, then fails closed if the reviewed map or header format changes.
Host static assertions compare them with the actual firmware definitions.

## Distribution and use

[`tools/stock_slots.py`](../../tools/stock_slots.py) has only `build`, `inspect`
and `plan` commands. It has no device write command. Each deterministic ZIP
contains:

- The original complete `stock-bank.x0xbrk` and extraction receipt.
- Each complete `loops/NAME.adpcm` stream.
- `slots/slot1.fsmp` and `slots/slot2.fsmp`, the full raw user-slot images.
- Matching `slots/slotN.hdr` and `slots/slotN.bin` files for the existing host
  `slotimg` and offline import format.
- An `individual/NAME.fsmp` image for each loop. Each individual image fits
  either physical slot, allowing a later explicit one-slot replacement.
- A model/profile-labelled native `.x0xslots` archive, its manifest and README.

`.fsmp` is the filename extension for raw FSMP images in this delivery. No
new `.brk` import format is claimed. The existing Python and browser archive
parsers both accept the native `.x0xslots` bytes. Older restore tools ignore
additional model/profile metadata; they do not acquire a new guard from this
export. This exporter validates those identities before inspecting or planning
and never invokes those restore tools.

Example offline build, using the already extracted approved inputs:

```sh
python3 -B tools/stock_slots.py build \
  --model elite --profile lx-req-032-compact \
  --bank /path/to/stock-bank.bin \
  --receipt /path/to/stock-extraction.json \
  --out /path/to/stock-elite-lx-req-032-compact.zip
```

Repeat with `--model pro` and a new output filename for the Pro distribution.
Existing output files, including symlinks, are never replaced.

Inspect the exact distribution and plan both replacements:

```sh
python3 -B tools/stock_slots.py inspect \
  --model elite --profile lx-req-032-compact \
  --bundle /path/to/stock-elite-lx-req-032-compact.zip
python3 -B tools/stock_slots.py plan \
  --model elite --profile lx-req-032-compact \
  --bundle /path/to/stock-elite-lx-req-032-compact.zip \
  --replace-slots 1,2
```

To plan a single loop, give `--loop CRAMEN --replace-slots 2`, for example.
The plan leaves slot 1 outside the selected replacement. An optional
`--current-backup FILE.x0xslots` reads a local backup and includes the prior
slot hashes, names and zone counts. It does not establish current device
state or verify the physical model. Without a backup, the current state is
explicitly unknown.

A later slot replacement replaces all zones in that slot. It needs a retained,
verified backup and separate authority for the exact device operation. After
any replacement, check saved patterns' LOOP A/B selections: the absolute menu
indices follow all currently valid zones and the number of built-in loops.
The offline plan includes those requirements and no executable write command.

## No-stock boot and package integration

No new slot or engine integration is required for this layout. The compact
build must use `X0X_NBREAKS == 0`; ensure `X0X_BREAK_BANK` is absent and
`X0X_FACTORY_BREAKS` does not select the compiled factory bank. Do not append
the external stock bank to the compact app during packaging or installation.
The root build owner must verify the generated bank header and final app
bytes, including any generated-table cache input.

With no compiled bank, `load_pack` still checks the app code area for an optional
tail pack. For SMK-37 it examines 16-byte-aligned candidates in the first
`0x86EE0` app bytes. The actual [`brk_pack.h`](../../firmware/src/app/brk_pack.h)
parser requires the complete `X0XBRKPK` signature, version 1, bounded count,
supported rate, complete per-loop lengths/names/bars, body CRC and an all-`0xff`
tail through the code limit. A signature alone or a valid pack embedded amid
other code is refused. A complete valid tail pack would still be accepted by
this general loader; final compact package inspection must establish its
absence. This task does not change or claim to disable that loader.

With no pack and empty user slots, the actual engine exposes zero loops and
selects empty loop descriptors; the UI's existing `draw_break` branch shows
`A none` and `B none`. The source-path test checks the empty engine selection,
not a target display render. With both exported stock slots present, the same
existing runtime exposes all three loops and both A/B controls can select each.

The maintained SMK firmware package's flash payload ends at `0x9D000`. The
user sample range `0xC2000..0xF4000` is outside that payload, rather than a
region of `0xff` bytes within it. Final package and loader-route verification
belong to the native/package QA owner. No claim of physical sample preservation
or successful installation follows from this offline export.

## Validation

Run the focused checks against the approved local bank:

```sh
python3 -B tests/stock_slots_test.py \
  --bank /path/to/stock-bank.bin \
  --receipt /path/to/stock-extraction.json \
  --evidence-dir build/stock-assets/validation
```

All 12 tests passed on the approved inputs. The suite checks full lengths,
all original bytes including odd-sample tail nibbles, slot capacities,
deterministic output, model/profile identity, original metadata, damaged
archives and checksums, malformed headers, zone bounds, explicit replacement
selection, no device helper calls and preservation of existing output files.
Encoder and audio-processing functions are patched to fail if invoked.

The C probe includes the actual slot scanner, optional pack parser and IMA
decoder, and extracts the unchanged `plat_slot`, `loop_bars`, `load_pack`,
`builtin_count`, `scan_loops` and `engine_brk_loops` functions. It tests all
three zone/menu mappings, every A/B selection, no-stock fallback, invalid
indices, corrupted data and malformed zone metadata. Address, undefined and
bounds sanitizers remain enabled. Leak scanning is disabled because the
managed environment denies its `/proc` task enumeration; the probe uses
static buffers and does not allocate sample data on the heap.

The independent maintained browser implementation serializes both FSMP images
to identical bytes, parses the native archive and compares all 262,141 decoded
samples with the production C decoder. This decoder comparison runs only in
the test; exporting never decodes audio. The decoded little-endian PCM hashes
are:

| Loop | PCM SHA-256 |
| --- | --- |
| CRAMEN | `d4bef52783131f0b163a2ae540cafe03b2360e36de2d4452ca899ac60ec41ef3` |
| TASTY | `78b916a324468a1fb8069ddb5dcd7113149a8bf35c92fb61adee44bd1623665b` |
| HOTPANT | `746dbc3f76af353543a33011d1ec7af55dc8588656f31f2add30fb6266388a82` |

The receipt records source file hashes, projected C source hash, compiler,
commands and exit codes. Coverage is simulated flash and loop selection at
the `breaks_set_loop` boundary. It is not target execution, DSP render timing,
audio-unit equality, device playback or installation proof. The private input
bank and generated audio/data artifacts are not committed with the tools.
