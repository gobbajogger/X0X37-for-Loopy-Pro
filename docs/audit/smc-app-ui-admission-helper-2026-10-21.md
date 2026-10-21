# TEST app-data admission helper

This checkpoint adds conditional app-owned UI-data admission and store write
exclusions. It is not an installable profile or a completed update protocol.
`X0X_SMC_VENDOR_TEST` defaults to zero. Default dual-mode behavior stays selected.

## App ownership and recovery

`x0x_app_ui_data.h` defines a 20-byte descriptor with magic, version, physical
address `0xAB000`, exact length 8192 and CRC-32. The descriptor is embedded in
primary app read-only storage. It must be sealed from the final linked external
bytes, including relocated font pointers. Source magic is deliberately zero:
unsealed builds fail closed. CRC detects corruption/mismatched data, not an
adversarial replacement. No external descriptor can select its own read range.

The profile hook in `fm1_main` runs after plain-XIP setup and the retained M/S
boot splash, before engine initialization and normal UI. Missing flash, wrong
version/address/length, corrupt data or an unsealed descriptor sets safe mode.
`smc_app_ui_admitted()` reports the decision independently of optional BLE.
Recovery uses retained FONT_S for its heading and reports `UI DATA INVALID`.
USB safe-mode and bootloader entry remain existing code; no hardware execution
of these paths is claimed. A future build/installer must preserve the descriptor
and data as an app-version-bound pair. A new image with an old/mismatched data
payload cannot proceed into normal UI.

Actual source hooks keep owner migration below `0xAB000` while preserving its
`0xAF000` owner tag. The profile app store and update-loader record allowlists
exclude all `[0xAB000,0xAF000)` immutable app/vendor sectors, even if BLE is off
or its manifest is bad. Sample slots, boot/model protections and PC/IRQ/stack
limits are unchanged. Old firmware does not have these exclusions: paired
rollback remains mandatory. No new write command or alternate unchecked range
is provided.

## Segmented controller contract revision

The unused segmented helper now uses explicit contract `0x534D4302`:

- Primary executable: physical `[0x84000,0x9B000)`, 92 KiB.
- Secondary load-only: `[0xAD000,0xAF000)`, 8 KiB.
- Tertiary load-only: `[0xFE000,0xFF000)`, 4 KiB.
- External app data `[0xAB000,0xAD000)` is never a controller segment.

Secondary `exec_size` must be zero; executable-pointer admission only accepts
primary code. Old revision-1 manifests are rejected. Existing reserved IRQ,
low task-stack and high data-only RAM checks remain. Default ABI-2 is untouched.
This changes an unused candidate helper, not an installed layout.

## Update contract: model only

Native tests model, but do not implement:

1. Park audio, UI, radio and both cores under loader ownership before any
   external data rewrite; an ordinary active app cannot replace its own fonts.
2. Stage and verify new data.
3. Install and admit the compatible smaller app.
4. Install and verify expanded controller pieces.
5. Commit controller manifest last.

Every modeled transition requires verified predecessor and loader quiescence.
Interruption, recovery, active profile-to-profile updates and paired rollback
still require real loader/installer integration tests. Model success is not
proof of those implementations. Updating app data in place temporarily
invalidates the old app's CRC; a safe loader/recovery path is required throughout.

## Checks and measured cost

Native strict test `tests/app_ui_admission_test.c` exercises the real validator
and `storage.c` owner handling, every descriptor-byte and payload-byte corruption,
missing/truncated data, foreign/erased owner preservation, object save/load,
app/loader range exclusions, unchanged sample slot capacity and transaction
model ordering. PASS with `-O2 -Wall -Wextra -Werror`, and ASan/UBSan with
`ASAN_OPTIONS=detect_leaks=0` (executor does not support LeakSanitizer).
Updated `tests/ble_segment_manifest_test.c` passes, including rejection of
old contract, app-data segment alias and executable secondaries. Existing
default `smk_flash_test.c` passes with its normal non-Werror build; an initial
-Werror attempt rejected pre-existing fixture unused/type-limit warnings.

The disposable full-feature target app with relocation and helper measures
540452 primary bytes versus relocation-only 540288: **164 bytes net**. Main RAM
97972 and pool 335740 are unchanged. The profile update loader is 7307 bytes
versus baseline 7280 (+27) from its narrowed allowlist; this is not the device
bootloader. All scoped target checks pass. This still contains the old BLE
hosts; the runtime owner separately measures their replacement. The unused
public admission getter can add code when the future bridge calls it.

Reproduce by preparing the baseline export as in the relocation report, then:

```sh
python3 tools/studies/smc_app_data/prepare_admission.py "$EXPORT"
(cd "$EXPORT" && X0X_DEV_PADCAL=1 CC=cc CXX=c++ python3 tools/build.py --board smk37 --model elite)
```

Use the established pinned SDK/toolchain environment. The study suppresses
packaging. No generated image, vendor binary or private SDK source is committed.
Descriptor sealing, controller startup, actual updater quiescence and hardware
SMC control remain pending.

Fresh default-off target regression also passes: image 548480 bytes, original
4192-byte slack, RAM/pool unchanged. Default app SHA-256:
`4b7c76e4bdeca3d8b1ce366e868406b8d554b0ae7d87beb282bd99254dcd0ec9`;
default loader SHA-256 matches the earlier baseline
`32a62f83dd17468aed3d08629b97d50499846efa916d53970a8b44f6043fe33e`.
The default app SHA also matches the runtime owner's retained baseline in
`docs/audit/smc-reference-fit-2026-10-21.json`: default output is byte-identical,
not merely equal in size.
