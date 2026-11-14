# F1 #900: independent cross-engine DNPR import finding repaired

Source review: [QA reproduction](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/900#issuecomment-6866983211), verified on **`1f25de7a76e3af046c4ea32aecfa7815d221dc31`** with production `preset_record.c` blob `c73e19697d098222166c5a8c475bca037a310208`, `preset_record.h` `e0112e3d9a315759c68ff1a63f5c128ae2d809d0`, and `tools/preset_librarian.c` `d6181388a33e3134e471e10a6902d4478562cfb6`.

## Cause and scoped fix

On the previous commit, `preset_record_import()` could import `engine_id=Chicago` and Chicago's 22-byte schema into a Miami target because the metadata `original_engine=Miami` was used as the authorization check. This differs from `preset_record_decode(expected_engine)`.

- The native import now passes the actual `target_engine` to the existing `check_loaded` validator. The validator compares **`engine_id`**, not `original_engine`, before accepting any schema. It still decodes into scratch and assigns the destination only on success.
- The distinct `original_engine` provenance field is retained **byte-for-byte** when the actual engine matches, even when it differs from the actual engine; imports do not silently rewrite provenance or perform a cross-engine parameter conversion.
- The host-only librarian's no-`--target` default uses `engine_id` (header bytes 6–7), not provenance byte 21. Explicit `--target` is rejected unless it matches the actual engine; failed exports create no output file.
- Version, CRC, sample refs, source name, factory-derived export gate, storage-layout and UI/firmware UX code are unchanged.

## Focused regression actually executed

New isolated selector: `sh tests/host/preset_engine_identity_test.sh build/preset-engine-identity`.

On the authorized DnBOX cloud, with **`cc (Debian 14.2.0-19) 14.2.0`** and source staged from the exact PR parent:

| Configuration | New focused assertions |
| --- | --- |
| Factory export OFF | **60 PASS / 0 FAIL** |
| Factory export ON | **60 PASS / 0 FAIL** |
| ASan+UBSan (factory OFF) | **60 PASS / 0 FAIL** |
| **Total new** | **180 PASS / 0 FAIL**, exit **0** |

Covers both orientations: Chicago payload + Miami provenance offered to Miami and Miami payload + Chicago provenance offered to Chicago. For each, `decode`, native `import` and strict `import_file` refuse incompatible targets. Full occupied-slot and empty-slot byte snapshots remain unchanged. They accept the real engine with preserved provenance. Host `--target` wrong-engine rejection leaves no file, correct-engine export is byte-exact, and default inspect/export use `engine_id`, preserving provenance in output.

**Source Git blobs of test inputs (from DnBOX cloud `git hash-object`):**

| Path | Git blob SHA-1 |
| --- | --- |
| `firmware/src/app/preset_record.c` | `0075011b4bd9f8cb43982f8a0d53d106a14a5cfe` |
| `firmware/src/app/preset_record.h` | `36eb81f2789e2092c96ee5abdbcce0d3f2caa9a6` |
| `tools/preset_librarian.c` | `6b5e099d0e7a4f4385ee25c191ad577157cc0e57` |
| `tests/host/preset_engine_identity_test.c` | `22e36edfe9e9c3da32c234398b0b00c3e3b0487a` |
| `tests/host/preset_engine_identity_test.sh` | `d71a5824fbb718a7b717869454b344af47fdd84e` |
| `firmware/src/crc32_nibble.h` (unchanged) | `193ce7bcf580fcb69e59d66e610ce4ea3baaed60` |

Local focused log SHA-256 `1861f73dce03f37cb17cceb2ccdf855edf6c4f92b0bc7ca44d7ae3b3d1e00aa0`.

The previously verified **1,068 integrated and 407 focused assertions were reused, NOT rerun**. This is a deliberately scoped fresh test, not full-suite proof. **NOT_RUN** on this commit: complete integrated/full host suites, SDK V1.2.14 Elite/Pro builds, flash/pool headroom >=8,192 B, emu360, hardware, device writes, transport/model identity and native UI acceptance. #891 owns firmware UX design; W3 #901 owns the web module; #895 owns Juno DSP. No changes in their paths, no merge or publication.

**Next consumer:** independent QA should re-review this exact commit and repeat the focused cross-engine regression if required; firmware integration remains gated on approved native UX and resource evidence.
