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
