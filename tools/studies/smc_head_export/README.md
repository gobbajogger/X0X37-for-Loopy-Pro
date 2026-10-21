# Elite DEV head acquisition

This dedicated RAM loader exports exactly physical flash `0..0x4000`. It does
not install firmware or know/guess the chip key. Existing app OTA staging writes
only BB000..BFFFF. The exporter itself can erase only BF000..BFFFF, and only the
exact admitted persistent record after the host has durably saved the head.
No main, head, sample, owner, calibration or controller writes are exposed.

## Operator and launcher contract

Run only from the supported 53d/816 normal editor with Bluetooth saved OFF,
followed by a physical power cycle. The current vendor TEST app deliberately
accepts only its bundle loader and is NOT an exporter entry path. Keep the
acquired head locally for all later installs and rollback. Only one keyboard
may be attached. This is not an FM1 rescue path.

The launcher pins source/ota hashes from the separately reviewed exporter
inventory; the main firmware DEV record does not authorize arbitrary exporters.
Call `acquire.py --dev-head-export --source FILE --source-sha256 HASH
--ota-sha256 HASH --output PRIVATE/unit-head.bin --receipt
PRIVATE/head-export-transaction.json`. It requires the parent's reviewed
`smc_vendor_install.check_cold` parser, existing fm1_install hardware lock,
foreground guard, transport/backend and sole-keyboard binding. The CLI presents
an action-time confirmation; there is no yes bypass. Disconnect headphones and
external amplified outputs. Onboard audio may make a transient: do not promise
silence or an undocumented speaker mute.

The first UPGRADE is existing app staging. Before the second UPGRADE the host
requires exporter HELLO with exact protocol/model/nonce. A legacy loader cannot
receive a second UPGRADE or payload response through this route. Second-phase
FINISH_CHECK only acknowledges full staged-source/record ownership admission;
it is never FINISH_WRITE. Fixed-range read responses bind nonce, whole-head CRC,
offset and length. Integrity is not cryptographic remote attestation.

Head and metadata publish atomically without overwrite, mode0600, followed by
file and directory fsync/readback BEFORE FINISH. A receipt binds exact source,
loader and output path before staging; resume cannot substitute another source.
Partial transfer publishes no partial head. Failed/lost cleanup is reported
uncertain with the saved head retained. No foreign record is erased. Cleanup
may already have completed despite a lost response; never claim every failure
retains a flash record. SPL cold/warm selection and actual USB/reset timing need
hardware verification. A new capture after reset is a new staged transaction,
not an assertion that the former record survived.

## Transition evidence and limits

Exact53d holds CPU1 before SPL and starts with IRQ disabled. Its live audio
buffer overlaps loader RAM. Independent source/disassembly review of WL82
SDK V1.2.0 channel direction maps the selected clear CON1 bit15 to TX/read DMA,
not a RAM-writing receiver. That is a different SDK archive than the old app;
it bounds a suspected corruption mechanism, not a physical transition PASS.
The new loader's first C action stops ALNK TX before BSS zeroing. It cannot stop
an earlier SPL-time transient. No hardware action was performed.

## Reproduction (private artifacts remain outside Git)

Use pinned SDK/toolchain environment and an empty disposable `EXPORT` directory:

```sh
set -e
git archive 816e0d3987bcb69be972cff2cd4df4469969f196 | tar -x -C "$EXPORT"
python3 tools/studies/smc_head_export/prepare.py "$EXPORT"
grep -q 'HEAD EXPORT SOURCE STUDY: NO PACKAGE' "$EXPORT/tools/build.py"
(cd "$EXPORT"; python3 tools/build.py --board smk37 --model elite)
python3 tools/studies/smc_head_export/build_source.py \
  --baseline-kit "$APPROVED_816_KIT" \
  --ota "$EXPORT/build/smk37-elite/loader/ota.bin" --output "$PRIVATE_SOURCE"
python3 tools/studies/smc_head_export/test_host.py
python3 tools/studies/smc_head_export/test_acquire.py
cc -std=c11 -Wall -Wextra -Werror tools/studies/smc_head_export/test.c -o "$CORE_TEST"
"$CORE_TEST"
```

For the actual wire replay, decode PRIVATE_SOURCE using existing
`add_ctl.logical_of` into a private logical file. Compile `wire_test.c` with
`HX_OTA_SOURCE` set to the absolute quoted generated `firmware/src/ota.c` and
`HX_CORE_SOURCE` to generated `firmware/loader/loader_core.h`, flags
`-std=c11 -Wall -Wextra -Wno-unused-function -Werror`; run with the logical file
as sole argument. Repeat with `-fsanitize=address,undefined`.

## Measured checkpoint

* Exact816 baseline disposable source; default app remains548480 bytes.
* Exporter raw9584 bytes, BSS end01C0DFD0; compressed ota8228 bytes.
* Raw SHA256 `a58532690119b027abce7f12c93b03c8e1b39b9d5b0ae9dbf22bf1f136c602ac`.
* OTA SHA256 `e19ea1f7ebf3f00ac45b600f04c9a2a55159307b027ae06222d82d895f4784b4`.
* Export-source SHA256 `dcea0622e2c26f45cf6ef21b8ca361b929ef1149e1acd1dacc245bc75dd88718`.
* Core native/ASan+UBSan, five host persistence test groups, host ordering group
  with five scenarios, and actual ota_stage/exporter wire native/ASan+UBSan PASS.
* Wire replay includes source-bound admission, full head bytes, foreign-record
  refusal, selected torn erase retry and outside-record NOR preservation.
  It is not exhaustive byte-tear, hardware/SPL, or physical USB proof.

No generated binaries, private heads or proprietary inputs belong in Git.

## Physical head versus generated template contract

Generated templates and source bundles retain strict whole-SPL and whole-isd
payload CRC checks. The explicit native per-unit route instead validates the
flash/entry header CRCs, existing bounded ranges, native entry traversal/name,
and the independent 34-byte key blob. It reports both unused payload CRCs as
non-secret diagnostic metadata. It never repairs or writes any physical-head
byte. This is not a claim that the SPL is healthy or that the key was independently
attested against eFuse or previous application encryption.

`bind_head` first validates the strict source package and paired loader binding.
The converted result must preserve the exact head and every canonical component;
`validate_bound_bundle` requires those original strict source bytes explicitly.
Ordinary `validate_bundle` never accepts the physical exception. The SMC installer
uses the already-validated fixed slot map, not a whole-image magic search that
could accidentally read SPL/isd data as host metadata. Generic installer behavior
is unchanged when no checked map is explicitly supplied.

### Continuing an already staged exporter

`resume_physical.py` is a **host-only, resume-only overlay**. Its record must have
an independently reviewed exact inventory of all local host dependencies, the
original exporter source/OTA hashes above, and a review evidence digest. The
record's externally checked SHA256 is supplied separately. A proposal marked
`review-required` is deliberately refused before MIDI is opened. The delivery
owner must provide an independently checked isolated launcher: clear
`PYTHONHOME`/`PYTHONPATH`, use Python `-I` (no user-site or current-directory
imports), and insert only the verified overlay directories into `sys.path`
before importing host code. Post-import file checks are not a substitute for
this launch boundary. Keep the original checked-source inventory separate from
the revised overlay inventory.

The operator must retain the original exporter package, original private receipt,
and original head-output path. Do not copy transaction state into a new kit, stage
another exporter, or power-cycle merely to apply the host overlay. Continuation
requires the original pending exporter to be currently detected. If the device
is already off, this tool neither starts nor restores that exporter and gives
no guarantee about the mode after power-up. Do not power-cycle again during a
continuation attempt. The receipt is checked unchanged; no migration or receipt editing occurs. The receipt never
stored a nonce: continuation uses a fresh nonce, consistent within that complete
read session, as before. A normal application instead of the pending exporter is
refused before any staging request.

After independent review, pass these arguments through the verified isolated
launcher supplied with the overlay (do not invoke the script directly):

```sh
REVIEWED_ISOLATED_RESUME_LAUNCHER \
  --resume-original-physical-head \
  --source ORIGINAL_EXPORTER --source-sha256 ORIGINAL_SOURCE_SHA \
  --ota-sha256 ORIGINAL_OTA_SHA --receipt ORIGINAL_PRIVATE_RECEIPT \
  --output ORIGINAL_PRIVATE_OUTPUT \
  --overlay-record REVIEWED_OVERLAY_RECORD --overlay-sha256 REVIEWED_RECORD_SHA
```

A separate `OUTPUT.validation.json` binds saved head identity to the reviewed host
record, contract, and original exporter hashes. It is atomically saved and read
back **after** the complete head and its existing metadata, **before** FINISH.
Failure to persist any of these sends no FINISH. Existing files are never replaced;
partial completion can be retried with the same identities. Lost FINISH remains
an explicitly uncertain cleanup result. Original stage-receipt bytes and existing
head-metadata schema remain unchanged.

This record authorizes neither a firmware install nor reuse of an old `DEV CHECKED`
record as evidence for changed host code. Standard `checked_tools` still refuses
an old source commit with revised invoking tools. Firmware installation needs its
own matching reviewed tool/source provenance; do not disable that guard.

Source-only regressions:

```sh
python3 tests/smc_physical_head_test.py
python3 tools/studies/smc_head_export/test_resume_physical.py
```

### Matching firmware kit after the continuation

The integration owner must include this host repair in the reviewed firmware
candidate **before** generating the next DEV package and validation records.
The new kit's `COMMIT`, installation-tool subset, forward/rollback artifacts and
records must all refer to that integrated source. Its normal `checked_tools`
verification is unchanged and must pass. A resume-only overlay is not a complete
replacement firmware kit.

After the original export finishes with confirmed cleanup, the new kit uses
`--head-ref ORIGINAL_PRIVATE_OUTPUT`; this intentionally skips its acquisition
path. Use the original `--state-dir` in place to retain pending-transaction checks,
and start a new commit-specific firmware transaction only when none is pending.
Do not resume a firmware transaction under a different commit. Do not run the old
8145957 installer, copy its state, invent an acquisition completion marker, or
rewrite its receipts. Perform the normal explicit cold power-cycle/audio steps
only after exporter continuation has ended; no automatic install follows capture.
