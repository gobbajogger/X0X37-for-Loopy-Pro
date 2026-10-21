# Whole-bundle loader budget — paused checkpoint

The resolved **forward-only** prototype adds 1812 bytes to the RAM loader, not
the running app. Work stopped at the parent's request before completing legacy
loader rejection, actual peripheral quiescence, or paired ABI-2 rollback. This
is a sizing prototype, not an admitted updater or a completed protocol.

## Concrete reuse and ownership plan

- Keep `ota_stage` / `ota_session` for verified persistent RAM-loader staging
  and the flash/RAM UPDATA_PARM record. They do not write app UI or controller
  sectors while the musical app runs.
- Reuse `ldr_session` chip-key validation, 1 KiB header scratch, 4 KiB target
  sector, 4 KiB current sector, USB `ota_read`, watchdog polling and two-attempt
  erase/program/readback loop. Bulk checks execute from the existing RAM loader.
- Add an app-owned capability check to `ota_stage` so the active TEST profile
  refuses old standalone loaders; the distinct marker experiment is retained
  but not validated. New metadata is not silently legacy-compatible.
- A fixed 96-byte metadata suffix to the logical flash entry binds exact model,
  policy, image id, app descriptor location, four range lengths/CRCs and a
  whole-bundle CRC. This is only a prototype format, not a published package.
- Exact write domains: whole main `[0x4000,0x9D000)`, UI `[0xAB000,0xAD000)`,
  vendor RO `[0xAD000,0xAF000)`, tertiary `[0xFE000,0xFF000)`. The commit header
  stays at `0x84000`, 128 bytes; no shared-sector or layout change is proposed.
- Before writes, validate all raw package CRCs, decrypt and cross-check the
  app's UI descriptor, validate the existing XB4 structural contract and stream
  its decrypted primary/plain external payload CRCs. The structural prefix is
  generated exactly from the existing validator, not a permissive replacement.
- Write external data first, then the complete main range while leaving its
  commit header erased. Read all final payloads back and compute their expected
  complete CRCs using the withheld header bytes. Only then program/read back
  the header and require the exact eight-byte `success` response.
- Only after helper success may the caller inspect and erase the one known
  staging-record sector and clear the RAM record. No data/sample-sector record
  scan. Retain the RAM record at loader entry rather than clearing it before
  an install attempt. Real SPL warm-reset behavior still needs verification.

Runtime owner retains admission/loader activation and app bridge ownership;
packaging owner must implement the versioned bundle and host response binding.
The helper lane owns only this disposable prototype and tests. Independent
review was requested but was not complete when the parent paused the task.

## Existing finish bug

Current `ldr_core.c:ldr_session` accepts any eight-byte finish response, then
clears records even after four failed finish reads. It also clears the RAM
record before scanning persistent records, and a record-read failure merely
breaks that scan. The prototype returns an error before cleanup on finish
failure, checks the literal response, and reads the exact staging record before
cleanup. Default source behavior is untouched by this study.

Record erasure itself is an irreversible commit operation: once all payloads
are admitted and the record erase succeeds, a later communication/read failure
cannot restore a previous persistent record. Real commit/readback/retry semantics
must state that explicitly rather than promise that every possible failure
leaves the old record present. Current tests do not establish this final route.

## Measured forward-only budget

Source baseline: `ca6c21be5de21b990674dcf315be41262285dd10`.
Pinned Elite DEV target build with packaging suppressed:

| Item | Baseline | Prototype | Delta |
| --- | ---: | ---: | ---: |
| RAM loader text/data image | 8588 | 10400 | +1812 |
| Packed ota.bin | 7280 | 9020 | +1740 |
| Loader BSS | 11520 | 11520 | 0 |
| Running default app | 548480 | 548480 | 0 |

The metadata (96 bytes), saved encrypted header (128) and decoded XB4 header
(128) reuse the 1024-byte scratch. They are not extra BSS. The bundle metadata
is transmitted, not installed into an app/sample sector. Existing 20 KiB
staging leaves room for this 9020-byte loader plus its headers/record, but this
is **not the final complete updater cost**. Stacks remain at their existing
loader locations; no stack high-water test occurred. CRC loops poll between
chunks. The running app's existing 20-byte UI descriptor and earlier admission
cost are not removed or counted as free.

Private output SHA-256:

- loader.bin: `d6fdbe39121326eb72759bb2e1d592737165b3412d334f09efb4484f88d74960`
- ota.bin: `6ad54bc6763ca6cd1c2083a365d789c0448bae214f4b56d2f890d2bce8bafdb2`

Reproduce the resolved forward variant in a disposable archive export:

```sh
git archive ca6c21be5de21b990674dcf315be41262285dd10 | tar -x -C "$EXPORT"
python3 tools/studies/smc_bundle_loader/prepare.py "$EXPORT"
(cd "$EXPORT" && X0X_DEV_PADCAL=1 CC=cc CXX=c++ python3 tools/build.py --board smk37 --model elite)
cc -O2 -std=c11 -Wall -Wextra -Werror -I"$EXPORT/firmware/loader" -I"$EXPORT/firmware/hal" tools/studies/smc_bundle_loader/test.c -o "$PRIVATE_OUT/test"
"$PRIVATE_OUT/test"
```

The code fixture passed forward programming, untouched-range comparison,
preflight read/CRC failure with no erase, source changes between preflight and
write (header remains erased), and finish timeout with staging record intact.
These are native hook tests, not USB/SPL/power-loss tests.

## Paused unresolved work

`quiescence_attempt.py` preserves an unsuccessful later experiment: importing
all of `fm1_cpu1.h` into the loader retains `.ram_text` functions referencing
`.noinit` data discarded by loader.ld, so the target link fails. It also adds a
loader capability marker and an app-stage marker gate. No linked budget or
acceptance is claimed for that variant. A small HAL-only hold/audio-stop helper
could avoid those unrelated imports, but was not implemented before the pause.
Halting core 1 alone does not prove RF DMA stopped; a real app-to-loader radio
shutdown/acknowledgment path and reset-state evidence are still required.

Paired ABI-2 rollback remains unimplemented. The forward helper accepts only
policy 1 and rejects old controller manifests. A separate explicit rollback
policy must bind a full compatible app/controller backup, validate old ABI-2
header/payload and chip/model guards, restore all pieces under persistent loader
ownership, and preserve recovery through interruption. Do not represent the
forward prototype as supporting rollback or use it to erase anything on a unit.

Further gates: marker-only legacy rejection tests; record read/erase failures;
interruption at every phase; profile-to-profile update while readers quiesced;
actual bundle identity across host reconnect; final app metadata binding;
bootloader/SPL resume semantics; final complete target size and stack checks.
No runtime adoption, package, hardware installation, or hardware result exists.
