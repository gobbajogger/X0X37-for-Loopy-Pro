# TEST bundle loader: forward and logical rollback checkpoint

**Resolved, source-only, non-installable.** This supersedes the paused forward
budget, not the hardware admission gates. Default runtime source remains unchanged
in this helper lane. No release/fullgate, installation, or external publication.

## Actual target budget

Disposable baseline export `ca6c21be5de21b990674dcf315be41262285dd10`, pinned
SDK/toolchain, Elite DEV build; packaging is disabled by the recipe **before**
validating its input pins. The final helper handles both policies:

| Item | Original loader | This helper | Change |
|---|---:|---:|---:|
| Resolved loader image | 8588 | 11776 | +3188 |
| Packed ota.bin | 7280 | 10051 | +2771 |
| Loader BSS | 11520 | 11520 | 0 |

`.text=0x2e00`, `.bss=0x2d00`, BSS ends `0x01c10300`. No new allocator.
Metadata96, saved commit128, decoded manifest128, and saved record112 reuse
1024-byte header scratch; two existing4096-byte sector buffers remain unchanged.
No stack high-water claim. Loader body is additionally bounded below record
sector `0xBF000`; retiring that sector cannot erase its staged executable body.

Private measured loader SHA256:
`d68487a8d31b92508f80fb67c18de214820ad7c14c3398f26e4668b4f7c71de8`.
Packed loader SHA256:
`492625579e42af2c5773912bed71cfb8989b5682dbc506b4c06b497a141e0937`.
The separately measured default app stays548480. Parent owns actual TEST app
integration; its current521156-byte image is the source of the loader's pins,
not the default app emitted by this isolated loader build. The loader is a
separate type100 UFW entry, not embedded in x0x.bin: its2771-byte packed growth
uses existing staging storage, **not app primary margin**. There is no app/loader
CRC self-reference. Final TEST app521156 retains2844 bytes below524000 before
any further app-side changes.

## Forward binding and lifecycle

`pin_app.py` derives descriptor physical `0x39324` from the actual final ELF,
rejects negative/out-of-image offsets, restores the unsealed20-byte descriptor,
and reuses exact `seal_fixture.py` to reconstruct/check all ELF load bytes before
accepting the sealed app. Current pins: plaintext app starts`0x4120`, length521156,
CRC32`b418c1ab`; ELF SHA256
`61cd9a1d4557afda36ef07722ebf01f7cc3c6f38a9e52efd31162cfa4b9ec62e`;
sealed app SHA256
`324435040ba2773e8f45edebd84ec5b2e1497746db23f90c9d9edee32aef15f6`.
Any subsequent app change requires regenerating these pins. Final app/UI pairing
is not interchangeable with the earlier521108-byte fixture.

Before writes: verify source model/ranges/aggregateCRC, full decrypted pinned
app, exact app-owned UI descriptor and XB4 manifest/payloads. Retain existing
chip-key check. Validate exact expected112-byte update record, exact staged
loader load address/capability/header and all staged bytes against the requested
package. Snapshot record before payload writes; compare again before cleanup.
All external fixed regions are transferred in full, not unverified short tails.

`update_gate.h` allows only explicit recovery with no controller attempt this
boot. Parent owns the actual app call sites. `ota_test_interlocks.patch` adds
TEST-only early session/stage rejection, mandatory bundle-loader capability,
refusal to overwrite any non-erased uncertain record, and non-destructive generic
unstage/bootcleanup. Default updater path is unchanged. The loader's HAL helper
uses exactly existing core-hold/audio-DMA-stop register operations, **not RF
shutdown**. Entry requires physical cold recovery before any controller attempt;
software cannot prove opaque RF inactivity after an arbitrary warm reset.
First migration from old firmware needs that same operational prerequisite:
old firmware cannot magically enforce the new gate.

## Explicit logical rollback, not original-byte restoration

Policy2 accepts only approved baseline816e0d3 whole-main canonical identity.
`rollback_fixture.py` verifies the approved installer ZIP SHA256
`c9ef5aa79d50189646703a046000c0b3e4e517c00f7df5ecaf4fc13f36a63907`, its embedded
combined FWSC SHA256
`6f89a299deb2304a89e3094f25ba22dd70214a3ce935c770a6e2ccff39e9b65a`, plaintext
app548480 SHA256`4b7c76e4bdeca3d8b1ce366e868406b8d554b0ae7d87beb282bd99254dcd0ec9`
and ctl65524 SHA256`9d6855e47791b3cab698af95d9625c35d2ea7b01d1fa7216e72ea622683a8245`.
The exact controller passes existing ABI2 header68/CRC/IRQ/RAM/entry validation.

Runtime canonical mainCRC`53f84577` decrypts only `[0x4000,0x4000+0x9796e)`
(including encrypted cfg), leaving the trailing plaintext slotmap/padding as-is.
This is device-key independent, verified by re-encryption with another key.
The whole-main pin includes the app, controller, configuration and slot map.
Policy2 explicitly clears **only** unused`AB000..AF000` and`FE000..FF000` toFF;
source and installed bytes must all beFF. It does not claim these were their
original per-device values. Store data/tag, samples, calibration, head and key
regions are untouched. Exact816 source confirms these spare extents are unused.
Publishing the first128 bytes at`0x8B000` last is a commit **prefix**, containing
the real68-byte ABI2 header plus60 code bytes, not a fictional128-byte header.
This is an explicit policy, not an automatic fallback or arbitrary ABI2 loader.

## Commit and interruption semantics

Both directions write external regions then main, withholding the selected
128-byte prefix; verify all installed piece/aggregate CRCs with that saved
prefix substituted; publish/read back prefix; require exact8-byte `success`;
recheck saved record; erase only its owned sector; verify erased record; clear
RAM record last. Finish/read failure before erasure retains records. If erasure
succeeds but its readback fails, the outcome is **uncertain committed**, not a
promise that the persistent record survived. Complete payloads were verified
before that point; cold SPL may boot the final app while RAM record persists.
No test pretends to execute the vendor SPL.

Native results:
- Strict forward checks; full operation-boundary sweep5374 cuts/resumes passed
  before policy2 addition and again on the final paired helper source: both
  terminal runs passed all5374 cuts/resumes.
- Forward representative torn-prefix sweep317 cuts passed: each sector half
  erase/first page half program and final prefix half program.
- Actual approved baseline rollback317 equivalent torn cuts/resumes passed;
  alternate chip key passed; changed app/controller, policy mismatch and non-FF
  spare negatives pass (latest added negatives rerun separately).
- Record identity/foreign validCRC/source-loader mismatch/capability/lifecycle,
  cleanup before/after erase, partial erasure entering the record CRC (loader body intact), and read-failure cases pass; ASan/UBSan pass with
  unsupported LeakSanitizer disabled.
- Actual disposable-export ota.c: denied session makes no request/commit/write;
  bootcleanup, rejected old loader, and uncertain record preserve staging NOR.
  Positive actual ota_session stages the resolved10051-byte bundle-capable
  loader, verifies its bytes and creates matching valid flash/RAM records. The
  test adjusts UFW entry framing only in memory; no package file is emitted and
  staged loader execution/payload writing is tested separately.
  Reused baseline test harness has existing signedness/uninitialized warnings.
- ELF/sealed-image provenance accepts exact pair and rejects changed app byte,
  below-image descriptor symbol and truncation.

These are model/source results, not exhaustive every-byte torn-flash behavior,
physical SMC controls, RF reset proof, live-memory ownership or actual cold/warm
SPL recovery. Host bundle preparation/response binding, narrow actual writer
activation, final fixture/pin coherence and hardware review remain required.

## Replay (private output only)

Use `set -e`; do not run build if preparation fails. Archive the baseline into a
new disposable export (not a worktree), then:

```
python3 tools/studies/smc_bundle_loader/prepare.py "$EXPORT" "$FINAL_ELF" "$SEALED_APP"
grep -q 'NON-INSTALLABLE bundle-loader budget; no package' "$EXPORT/tools/build.py"
(cd "$EXPORT" && X0X_DEV_PADCAL=1 CC=cc CXX=c++ python3 tools/build.py --board smk37 --model elite)
```

Source pinned SDK/toolchain environment first. Compile native `test.c`,
`record_test.c`, `rollback_test.c` with `-O3 -std=c11 -Wall -Wextra -Werror` and
include directories `$EXPORT/firmware/loader` and`$EXPORT/firmware/hal`.
`test.c` default runs the full boundary sweep; `BP_TORN=1` selects the317 cuts;
`BP_FAST=1` runs only fast checks and reports **zero** sweep cuts honestly.
Generate private rollback native input with`rollback_fixture.py APPROVED_ZIP OUT`
and pass OUT to rollback_test. `test_pin.py FINAL_ELF SEALED_APP` tests provenance.
For stage_test define `BP_BASE_OTA_TEST` to the export's `tests/ota_test.c`, compile
without `-Werror` for its inherited warnings, and pass the approved old FWSC
followed by the newly resolved private `loader/ota.bin`.
No generated binaries, vendor bytes or device key are committed.

## Local workflow incident

One stale-ELF pin attempt correctly failed, but a caller shell without `set -e`
continued a baseline build and generated a private baseline FWSC. It was never
used, installed or transferred; only that generated package was removed and the
export quarantined. Its hash was not captured before removal, so remains unknown.
The incident log is private; it is not candidate evidence. The recipe now turns
packaging off **before** pin validation, preventing that failure mode even when a
caller neglects `set -e`. Correct paired replay above resolved without packaging.

## Actual-byte host-response replay (follow-up)

`real_fixture.py` now binds the verified baseline kit, final sealed app/UI and
complete materialized vendor regions into one **private response-data fixture**.
It reuses baseline SFC/JLFS machinery, preserves the canonical main configuration
and slot map, updates their CRCs for changed app/controller bytes, and uses only
explicit test key`0x1234`. The simulated flash head's isd key record is rebuilt
with that fixture key; no real device key or backup is used. It emits no UFW,
FWSC or uploader. Current source fixture length1048672 and SHA256:
`4fc83a935b198f409701ed1c8debcd7db97c3848c087e524a3be551c90351273`.

Actual input identities:
- app/UI hashes are the exact interlocked pair documented above; UI SHA256
  `b3f4184cdb6c7bd52c50a7df644bdf7f813a090a0a5b0f2a7607901210ab48a9`.
- full primary94208 SHA256
  `f932189d1ef6d61b60693347b5fb54d7a0dc7001c2c83e573d98836139b64414`;
- full vendor RO8192 SHA256
  `3c3fa1f88b5a080788fce673738c9110bd9c41a78170ca1e6d1d078f6ad63b10`;
- full tertiary4096 SHA256
  `0905950ad661147aa5de2cc1a16ae39f252814af8a48a7065c8d3915f8e5f0ed`.

`wire_replay.c` compiles the actual prepared `ota.c` and `ldr_core.c`. Its host
answers real packed SysEx read requests from immutable bounded source maps;
entry framing exists only in RAM. Calling **ldr_session**, not just bp_install,
exercises key discovery, source parsing, exact staged-loader/112-byte record
ownership, app/UI/vendor admission, all writes/readbacks, exact finish response,
persistent-record retirement and RAM-record clear. Success: **3779 requests,
159 erases**, exact installed four payload ranges and all outside bytes unchanged
except the explicitly owned final record sector.

Strict `-Werror` and ASan/UBSan replay pass. Negative cases reject a changed staged
loader, foreign valid-CRC record, wrong echoed response address/length, source
chunk substitution after preflight, overflowing source offset and wrong finish
response. Substitution leaves the controller prefix unpublished; finish failure
retains records. The host map explicitly bounds both storage capacity and declared
length. Source swap here means one changed returned UI chunk, not a claim of
exhaustively exploring every possible coherent hostile source replacement.
No actual USB transport, RF, SPL, firmware execution or live-memory proof is
implied. The staged capability marker is not cryptographic authentication.

Reproduce with the same privately retained actual inputs:

```sh
set -e
python3 tools/studies/smc_bundle_loader/real_fixture.py \
  "$BASELINE_KIT" "$FINAL_ELF" "$APP_FIXTURE_DIR" "$VENDOR_FIXTURE_DIR" "$NEW_PRIVATE_OUT"
cc -O2 -std=c11 -Wall -Wextra -Werror -Wno-unused-function \
  -DBP_OTA_SOURCE=\""$EXPORT/firmware/src/ota.c"\" \
  -DBP_LDR_SOURCE=\""$EXPORT/firmware/loader/ldr_core.c"\" \
  -I"$EXPORT/firmware/hal" \
  tools/studies/smc_bundle_loader/wire_replay.c -o "$PRIVATE_TEST"
"$PRIVATE_TEST" "$NEW_PRIVATE_OUT/source-NOT-INSTALLABLE.fixture" \
  "$EXPORT/build/smk37-elite/loader/ota.bin"
```

EXPORT must be the prepared paired-loader export from the preceding recipe, with
pins for the same final app. The copied sealer is updated identically to owner's
`fa22c3e` record-slot checks; the builder also rejects secondary sector-end tags
that could resemble persistent update records. All recipe inputs/hashes remain
source-verifiable; generated/vendor bytes remain private.
