# MBOOT source adoption stage

This implements an **offline in-memory adoption transform**, not installation.
It also builds the real SMK m-bootloader from pinned source. It does not add MBOOT
to an application/controller package, change the default updater, or authorize a
device operation. PR631's compatibility fixtures remain unchanged.

## Implemented behavior

`adoption.c` uses the portable upstream core, not a device backend. Its caller
supplies separate frozen reviewed inputs and current input bytes. It refuses:

- unknown Pro/Elite model identifiers, incomplete images and stale full-image bytes;
- a different container, a non-SMK identity, bad CRCs, or unsupported boot geometry;
- missing, corrupt, overlapping, duplicate or unsupported retained configuration;
- missing calibration marker or any occupied byte of the future calibration guard.

On success it constructs a full memory-only candidate. Only boot directory bytes
`0x20..0x3f` and the fixed boot slot `0xa0..0x38cf` may change. Configuration,
application, projects, samples, calibration and key/MAC allocations remain byte
identical. Output is not modified on refusal. Buffers must be disjoint as stated
in the API. Frozen inputs are a caller responsibility: identical buffers do not
prove that a snapshot was authorized or came from any particular device.

The calibration guard belongs to the pinned upstream bootloader at first boot:
`0xff000..0xff23b` is reserved; upstream may restore calibration at `0xc1000`.
This stage neither initializes nor restores calibration and refuses an occupied
reservation rather than overwriting it. This is not a complete factory backup.

`x0x_mboot_same_head` enforces exact **all-16-KiB** head equality for a later
ordinary app update. It is deliberately stricter than a bootloader-name match.
Every one-byte mismatch position is tested for both model identifiers. It does
not replace existing model, package, application-range or installer guards.

## Reproduce source and native proof

Use a clean checkout of `jvitkauskas/m-bootloader` at
`64b74d009e6d21fe85ca60754b7c01d438214804`, tree
`105c8347f2b8a1f1f4e3993bec31735fde1e5828`. Keep its MIT `LICENSE` and
`NOTICE.md`. No upstream source, proprietary SDK/toolchain or binaries are
vendored in this change. Its `toolchain` directory (or symlink) must contain the
local JieLi tools. The upstream documented revision is **20250324.1**.

```sh
python3 -B tools/mboot/check.py --upstream /path/to/pinned/m-bootloader \
  --toolchain-id 20260730.1 --qualify-alternative-toolchain \
  --receipt /path/to/private-evidence/qualification.json
```

The explicitly selected alternative is the already available 20260730.1 compiler.
The check records executable hashes, runs two forced native builds, checks byte
reproducibility, entry/startup instructions, CRC/container identity, boot-slot and
private-RAM bounds, upstream synthetic core/ABI/boot checks, the adoption tests
normally and under ASan/UBSan, and a pi32v2 compile of the new API. It builds no
installer or firmware package and serializes no unit-image output. Private
upstream fixtures are SKIP when absent; LeakSanitizer is NOT_RUN in the ptrace
executor. Structural qualification of an alternative compiler is not hardware
qualification or proof of equivalence to the documented compiler.

## Offline caller and ordinary-package adaptation

`prepare.py` is a usable, separate offline caller. It verifies exact SHA-256 pins
before running the portable C transform. It requires a complete embedded
controller, exact Pro/Elite package model, valid package/file CRCs and an existing
Felucca app-only loader. The retained configuration must be at directory record
64, as the real pinned MBOOT boot path requires. The package and snapshot key
prefixes must have valid CRCs and decode to the same key; this conservative test
is not a measurement of the unit's runtime efuse key.

The adapter replaces only the ordinary package's `flash.bin` head and recomputes
outer file/list/header CRCs. Flash capacity/alignment and complete application
directory/banking records must match the reviewed package; no app relocation or
bank migration is supported. Application, complete embedded controller and OTA
loader bytes remain identical. It checks the unchanged actual Python installer
head guard against the resulting adopted head. It never relaxes that guard,
changes default packaging, or enables installer-only packages in `ota_stage()`.

The pins JSON has exactly five fields: `model` (`pro` or `elite`),
`snapshot_sha256`, `package_sha256`, `container_sha256`, and `controller_sha256`.
The controller pin identifies the embedded image including its header, without
its erased 64-KiB-block padding. Pins must come from already reviewed inputs;
hashes by themselves do not establish ownership, model or snapshot provenance.

```sh
python3 -B tools/mboot/prepare.py --snapshot /private/reviewed-unit.bin \
  --package /private/reviewed-complete.fwsc --container /private/uboot.boot \
  --pins /private/reviewed-pins.json --upstream /path/to/pinned/m-bootloader
```

The default emits a nonsecret hash/status report only. Optional
`--candidate-dir /private/new-directory` creates a new mode-0700 directory with
mode-0600 `adoption-head.PRIVATE-candidate`,
`app-package.NOT-INSTALLABLE-candidate` and `report.json`. An existing directory
is refused. The head contains private unit configuration; never upload this
folder to GitHub, Library delivery or hosting as a normal firmware package.
No full snapshot is serialized. These outputs are source-review candidates,
not qualified installation artifacts; no install command is provided.

`tests/host/mboot_adoption/test_prepare.py` exercises the caller and CLI using
only a synthetic snapshot plus a pinned existing complete firmware/controller
package. Synthetic heads and derived packages are deleted with the temporary
test directory. Tests cover stale pins, wrong model/key, missing controller,
damaged package, short snapshot, CRC-valid moved app base/banking metadata,
unchanged head guard, report-only behavior,
private output permissions, overwrite refusal and unchanged original inputs.

## Remaining gate

The source caller and normal-package adaptation are implemented, but an actual
owner-reviewed complete unit snapshot and exact model identity have **not** been
supplied to this stage. No installable per-unit adoption candidate can be claimed
from synthetic fixtures. Snapshot acquisition is not authorized here. A later
device transaction still needs separate approval, backup verification and its
own reviewed writer; none is included or automatically started by this tool.

The technical MBOOT artifact is separate from a complete application deliverable,
which still needs its matching controller. Hardware startup, key behavior, real
interruption safety, calibration restoration, clock/audio timing and same-unit
preservation remain untested. Root owns subsequent integration and independent
review. Existing firmware/controller deliverables are not changed by this stage.
