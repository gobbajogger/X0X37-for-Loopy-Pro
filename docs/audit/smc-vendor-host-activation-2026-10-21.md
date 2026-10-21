# Explicit SMC vendor DEV host path

This checkpoint adds software-only construction, validation and an opt-in adapter to the existing installer. It does not demonstrate USB hardware installation, cold SPL recovery, RF reset, live heap/stack safety or physical SMC controls. It is not release/full-gate approval.

## Interfaces and authorization

`tools/smc_vendor_bundle.py` exposes:

- `build_bundle(template, app, ui, vendor_regions, ota, *, descriptor_off, loader_binding)`
- `build_rollback(approved_baseline_fwsc, paired_ota, *, loader_binding)`
- `validate_bundle(raw, *, loader_binding)`
- `bind_head(raw, actual_head, *, loader_binding)`

`loader_binding` has exactly `app_size`, `app_sha256`, `app_crc32`, `descriptor_off`, `ota_sha256`. The build owner derives these from the sealed ELF-matched app and the loader actually compiled with that app pin. The module does not pretend to recover compiler constants from machine code.

Validation extracts and verifies structure and integrity, not source authorization. Schema3 DEV evidence must bind the exact package hash, complete returned metadata, retained controller ELF and all full-region identities. The installer compares the complete metadata to `record.smc_vendor`, using `record.loader_binding`. A structurally valid different vendor image is not authorized by CRC alone.

The template is the existing `fm1pkg_make.flash_image` result for the exact sealed app. Config, region descriptors and sample slot map remain intact. The explicit model is SMK-37 Elite, not a family wildcard. Three vendor regions and the UI region are full-sized, including padding. The loader remains the separate existing UFW type100 entry.

Per-device conversion validates the supplied readback head, copies all 16 KiB unchanged, and rekeys only the exact existing encrypted main/config extent (0x9796e bytes from physical0x4000). External UI/vendor bytes stay plain. Whole-main canonical and all component identities must remain identical. Only ciphertext/package and transaction CRCs change. No real device key is included in Git or the test evidence.

## Existing installer integration

The explicit path requires `--smc-vendor-dev --config dev --commit HASH --dev-record FILE --head-ref UNIT_READBACK --backup FILE --verify-slots --smc-receipt FILE --smc-cold-recovery`. `--ctl`, `--no-ctl`, `--force` and `--no-slots` are forbidden. It keeps the existing foreground check, hardware lock, gate/queue check, MIDI backend, Updater and SlotMove helpers. Defaults are unchanged.

The operator must physically cold-power the sole keyboard into recovery before any controller/audio startup. The new app gate rejects a touched controller session; the older firmware used for first migration cannot enforce that new gate. This is an operational prerequisite, not a proved RF shutdown acknowledgment or soft-reset guarantee. If recovery cannot supply the required sample backup, the adapter refuses; there is no sample-check bypass.

All package/record/head/backup/receipt paths must be distinct, including resolved symlinks and hardlinks. This is checked before opening the device. Backup data and its directory are flushed before a durable receipt is published.

Before UPGRADE, all sample slots are backed up and the archive read back using existing code. The exact slot map must remain unchanged. A durable local receipt binds source package, converted bytes, head hash, DEV record, loader/app identities and archive hash. Resume requires this same receipt and backup, including when the loader is already running. It does not claim to make a fresh sample backup from a loader. Port enumeration/model checks preserve existing single-keyboard constraints; the MIDI handshake has no unit serial.

The response source is immutable bytes. Wrong-phase finish, malformed flags/length and out-of-range reads are rejected before any success reply. The existing loader independently checks every staged loader byte against the source before payload writes. No automatic rollback occurs on interruption; retain the exact source/receipt/backup.

After forward install, ED39 reads and compares all primary94208/UI8192/vendorRO8192/RAMload4096 bytes, including padding, and checks region/offset/exact length echoes. All logical sample headers and used data are compared with the backup. This endpoint cannot read the whole app; app identity is enforced by the loader's compiled pin, not claimed as a new app readback.

## Logical rollback

Policy2 uses the exact approved816 package SHA `6f89a299deb2304a89e3094f25ba22dd70214a3ce935c770a6e2ccff39e9b65a`, the same paired bundle loader, and canonical main SHA `55e9e093d4adc7a2b303a807873cae536c6547013dd2c36af152ba92653ce446` (CRC53f84577). It validates the exact baseline app and ABI2 controller hashes and guards. ABI2 has a68-byte header; the loader withholds/publishes a128-byte commit prefix.

This intentionally clears only baseline-unused AB000..AEFFF and FE000..FEFFF to FF, preserving AF owner, samples, stock/calibration/key regions and head. It is logical rollback, not restoration of original unused bytes. The installer additionally requires `--smc-logical-rollback`; forward/policy mismatch is refused. Baseline supports ED37, not ED39: postboot verifies its entire64 KiB ABI2 block and sample slots. Cleared spare regions were verified by the RAM loader, not a fabricated postboot read.

Schema3 generation/verification and final source-to-device authorization remain the runtime/integration owner's responsibility. A rollback package needs its own exact checked package metadata, not a generic old FWSC exception.

## Reproduction and evidence

Source-only tests:

```
python3 tests/smc_vendor_bundle_test.py
python3 -O tests/smc_vendor_bundle_test.py
python3 tests/smc_vendor_install_test.py
python3 tests/install_test.py
python3 tests/installer_safety_test.py
```

Optional private actual-component check (constructs/rekeys only in memory; writes no package):

```
python3 tests/smc_vendor_bundle_test.py COMPONENTS_ROOT SDK_ROOT APPROVED_816_KIT_ZIP
```

The actual check used owner v2 components: app521348, descriptor393c4, sealed app SHA01cdfb8f..., paired ota10051 SHA b0821880..., and the reviewed full vendor regions. Both forward and exact baseline rollback round-tripped from explicit test key1234 to5678 with canonical identities unchanged. The actual source/loader replay is a separate required validation owned by integration; these host tests do not substitute for it.

The schema3 runner requires the exact approved baseline FWSC as
`--smc-rollback-fwsc`, records it among immutable input dependencies and constructs
both forward and logical-rollback packages with the same paired loader. Each
has its own record, artifact role, package SHA and complete component metadata.
Both records require native and sanitized actual-package replay of both policies.
The top-level controller and budgets describe the built central TEST profile;
`budget_scope` makes this explicit. The rollback's actual baseline ABI2 identity
is separately recorded in `smc_vendor`, never relabeled as ABI4. Logical rollback
restores the approved 816 main image and clears only the proven unused external
extents; it is not a byte-for-byte restoration of arbitrary prior spare bytes.
