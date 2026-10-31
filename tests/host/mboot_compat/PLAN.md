# m-bootloader adoption preparation

Status: offline compatibility preparation only. Root remains the sole firmware
integrator. This directory changes no firmware, default package generation,
installer guard, device backend, release, or catalog. Implementation/adoption and
device writes are separate decisions.

## Exact inputs

- Firmware nightly: `3eb42089e4cd2e8cb1a720246fb4526776433ed5`.
- Checked source tree: `025b3e64f43634ade0928af7b2315336cc391e0f`.
  Local commit `a71944d0b493e6d3c09ea61be99b6d3380bbd52b` and PR630
  `c9f01fd1042b3e6aa249baac57262fd1ffe08b56` have that same tree.
- Upstream: `jvitkauskas/m-bootloader` commit
  `64b74d009e6d21fe85ca60754b7c01d438214804`. The manifest pins exact Git
  blobs for the portable source and format helper. Copies in `.inputs/` are local
  test dependencies, not a bootloader distribution.
- Existing Pro package SHA-256:
  `c0ce49794cb5692c4c9ab65552a8193e445945f06a5488a24b769b9e9172484f`.
- Existing Elite package SHA-256:
  `67dcf8be40279e8b1e44224b3c1c9b76f1b425837b7ce634ae0ecd41d5c3f172`.
  Both are app-only DEV evidence, not installable/device-qualified artifacts.

## 1. Offline compatibility gate — prepared

`run_offline.py` compiles only upstream `core.c` and `contract.c` with the local
test adapter. It never compiles platform, GPIO, SPI, USB, installer execution, or
recovery code. All NOR callbacks target a synthetic 1 MiB memory array. A tagged
synthetic boot container is not executable firmware. Temporary flash buffers and
decoded OTA bytes are deleted when the run ends; logs contain no firmware bytes.

The checks cover:

- Exact input identities and clean firmware tree.
- Both existing package application directories, file CRCs, entrypoint and model
  identities; malformed header, payload CRC and key negatives.
- Exact existing OTA decompression by the upstream native decoder; flash request
  discovery and malformed request, record CRC, load address and size negatives.
  The firmware bytes are decoded and compared, never executed.
- Application/updater ABI and retained-request selection.
- Valid synthetic clock/configuration TLVs, truncation refusal, and rejection of
  the current generated package's minimal `[FELUCCA]` configuration.
- The unchanged Python installer head guard: equal head accepted, changed head
  refused. No backend is instantiated and the guard is not bypassed.
- Native and Python upstream boot-area transforms on synthetic supported layouts:
  all non-boot bytes remain equal, including configuration, application, samples,
  calibration and key/MAC allocations. Native bad-record-CRC and overlapping-config
  negatives are refused without modifying any fixture byte. The current generated SDK head's
  unsupported old allocation is also refused, rather than treated as a unit dump.
- Synthetic calibration backup, exact write boundaries, preservation of a valid
  recalibration, interrupted restoration at each page, safe retry, and refusal
  to repair invalid/partial backups destructively.

Run from this directory, with existing private inputs outside it:

```sh
python3 -B run_offline.py --repo /path/to/exact-clean-checkout \
  --packages /path/to/smk37-pad-diag-evidence --receipt out/offline.json
python3 -B run_offline.py --repo /path/to/exact-clean-checkout \
  --packages /path/to/smk37-pad-diag-evidence --sanitize \
  --receipt out/offline-sanitized.json
```

Source dependencies must already be available at `.inputs/upstream/`, or supplied
with `--upstream`. The runner performs no network retrieval. Preserve upstream
MIT `LICENSE` and `NOTICE.md` with the dependency. ASan/UBSan are enabled in the
second command. LeakSanitizer is explicitly disabled because this executor uses
ptrace; this is not a leak-check PASS.

Result at preparation: 17 grouped checks PASS normally and with ASan/UBSan.
These are host compatibility/preservation results, not target-build, installation,
physical power-loss, clock/audio or hardware acceptance.

## 2. Review a distinct adoption contract — next source step

Before changing a production code path, root should approve one small source PR
defining how adoption differs from an ordinary app update:

1. Keep the current app-only Felucca update path and its protected ranges intact.
2. Pin and identify the SMK m-bootloader separately from the application and OTA
   loader. Pro and Elite share the boot target but retain separate package models.
3. Define how an adoption candidate preserves the actual existing boot
   configuration and every other unit allocation. Do not substitute our synthetic
   SDK package head for a unit head or merely swap its `uboot.boot` member.
4. Define an explicit, exact-input bootloader-preserving head-validation policy
   for normal updates on an already-adopted unit. Do not remove the head guard,
   turn mismatches into unconditional success, or infer preservation from a name.
5. Keep the adoption entry separate: upstream's boot-installer package contains
   only `ota.bin`, while current X0X `ota_stage()` requires `flash.bin` too and
   recognizes only known official/Felucca loaders. That rejection remains intact
   in this preparation. No new boot-writing route is enabled.
6. Decide and test calibration-guard ownership explicitly: upstream uses
   `0xFF000..0xFF23B` and may restore `0xC1000`. Do not describe the guard as a
   complete factory-data backup or silently treat these as free application bytes.

The present fixtures become regressions for that contract. New behavior needs
new positive and negative tests, independent Sol review and exact source pins.

## 3. Build-only qualification — after the source contract

Use the documented upstream toolchain or qualify an alternative explicitly.
Its documented revision differs from the existing application toolchain; compiler
availability alone is not a compatible-build PASS. Check container identity,
CRC, fixed placement, private RAM, reproducibility and release/source provenance.
Retain MIT copyright/license notices; do not commit proprietary SDK/toolchain
files, user data or target binaries into the source PR.

No flash expansion is currently indicated. Upstream v1.2 metadata lists a
14,000-byte SMK boot container within the enforced 14,384-byte slot: 384 bytes
spare. Release source `22e2559906f9fc4e077f2d6aee20bf332300439e` differs from the
assessed pin only in documentation. This is release metadata, not a fresh build
receipt. A future exact build must pass the limit again.

Do not consume application growth margin, OTA staging at `0xBB000..0xBFFFF`,
settings, samples, calibration, resources or key/MAC space to make a build fit.
Any future layout change is a new reviewed design, not an automatic fallback.

## 4. Separate later hardware acceptance

Only a separately authorized, model-bound hardware stage can establish real
startup, recovery-key behavior, expected quad-XIP/audio timing, update lifecycle,
and same-unit preservation. No such stage is started or instructed here. A passing
offline gate does not authorize or claim bootloader replacement.

## Primary source anchors

- [Upstream boot/format code](https://github.com/jvitkauskas/m-bootloader/tree/64b74d009e6d21fe85ca60754b7c01d438214804).
- [Upstream v1.2 metadata](https://github.com/jvitkauskas/m-bootloader/releases/tag/v1.2).
- [Current firmware source](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/tree/3eb42089e4cd2e8cb1a720246fb4526776433ed5).

Flash occupancy by itself does not impose playback work. The relevant runtime
costs are active XIP reads/cache behavior and erase/program stalls; RAM headroom is
a separate budget. The supported bootloader already fits its own boot slot.
