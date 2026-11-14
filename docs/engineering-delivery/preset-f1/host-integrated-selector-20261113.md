# DnBOX F1: integrated native preset-record host selector, 2026-11-13

## Exact scope and ownership

Draft [#900](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/900), existing `lane/fmb-presets-format`. Source and fixtures pinned to **`622eaf59c673045667432a92510d3934e7c68d6b`**. This evidence commit changes only the POSIX host runner and this receipt; all firmware, codec, DSP, user-preset schemas, and fixtures remain byte-identical to that pinned revision. No extra tracker; ROADMAP #887 26.8 remains the owner source of truth.

The shell runner originally used `test_binary | tee log` under `set -eu`. POSIX `sh` reports `tee`'s success even if the binary fails, allowing false PASS. The runner now redirects the binary output to a log and only prints it after successful exit. It preserves OFF / pinned OFF / ON / sanitizer variants, factory pinning, log comparisons, and host-only size reporting.

## Reproduction

Run from this exact source tree with a POSIX shell, host C compiler, and binary fixtures below:

```sh
sh tests/host/preset_record_test.sh build/preset-record
```

**Execution:** authorized DnBOX cloud via Remote Desktop Commander, isolated `/workspace/scratch/dnpr-f1-927156b`. GitHub connector retrieved all text inputs at `622eaf59c673045667432a92510d3934e7c68d6b`; the two binary fixtures were decoded from GitHub base64. Local `git hash-object` (no Git networking) matched **28/28** pinned Git blobs before the runner changed. Compiler: **cc (Debian 14.2.0-19) 14.2.0**.

| Host check | Exact result |
| --- | --- |
| Factory export OFF | **271 assertions PASS, 0 FAIL** |
| Factory export OFF with pin | **271 assertions PASS, 0 FAIL** |
| Factory export ON | **255 assertions PASS, 0 FAIL** |
| Sanitized factory export OFF (ASan + UBSan) | **271 assertions PASS, 0 FAIL** |
| **Total executed** | **1,068 assertions PASS, 0 FAIL**, script exit **0** |
| Factory table CRC | `0xd644a5f5` on each run; pin matches |
| Host `preset_record.o` `size` | `.text`/const/unwind included total 5,421 bytes, `.data=0`, `.bss=0`; **HOST ONLY, not device flash** |

The test includes structural validation, byte-exact golden encoding, real Chicago/Miami/Juno/Drone factory inventories, imported and malformed DNPR, model-neutral engine rejection, project and fixture CRC pins, sample references, commit-marker undo, and bounds/canary checks added at `927156b0`.

**Negative control:** a host-only temporary substitute compiler emitted an executable that exits with code **23**. The hardened runner returned **23**, stopped at the first OFF test and did **not** emit `PASS preset-record`. This proves the previous `tee` status masking is fixed. The negative control was not committed as a binary or as a firmware artifact.

**Scratch receipts** (not committed, because logs contain local paths):
- Hardened full selector log SHA-256: `68d4dd9a088aa7646de69cac26016cad117a09af43b6730f4304265db1ca0062`.
- Negative-control log SHA-256: `ab414db20ac0f717e6dbba01a8ce076faaf0be76dfc4e819fa084bcbe219e88b`.
- Hardened runner local Git blob: `1a8f17754f648d3e6d0df547947b24b23ba6e967`.

The **407 previously completed focused assertions** (preset file-boundary and host librarian runners) were **not rerun**. They are earlier, separate PASS evidence in #900; do not add them to the 1,068 as if a single suite ran.

## Missing / blocked gates

**NOT_RUN on this new head:** native SDK V1.2.14 Elite or Pro build, code-flash/pool floors ≥8,192 B, emu360, playback/dropouts, device/transport/model handshake, hardware, interrupted flash writes, full host suite or website W3. No SDK refusal was bypassed. This was a host-only integration test and does not change native resource headroom. Neither factory-plugin compatibility nor native on-device save/load is claimed.

## Exact input closure (Git blob identities at `622eaf59c673045667432a92510d3934e7c68d6b`)

`chicago_program_private.h` is an essential transitive **macro** include (`#include ORIGINAL_CHICAGO_PROGRAM_METADATA`), not caught by naive literal-include scans. `project.c` and the two store binaries are required by pinned fixture CRC checks even though they are not compiled in this test. All local file hashes were checked after download and before the runner was modified.

| Path | Git blob SHA-1 |
| --- | --- |
| `tests/host/preset_record_test.c` | `73beb1cc8c82c8d822d9c828465cd0bd74229edd` |
| `tests/host/preset_record_test.sh` | `88aa410d57ebf2454bdb420eb567064dda579af9` |
| `firmware/src/app/preset_record.c` | `c73e19697d098222166c5a8c475bca037a310208` |
| `firmware/src/app/preset_record.h` | `e0112e3d9a315759c68ff1a63f5c128ae2d809d0` |
| `firmware/src/crc32_nibble.h` | `193ce7bcf580fcb69e59d66e610ce4ea3baaed60` |
| `firmware/src/app/original_sound.c` | `6268b4191c9f348d7d78354a92d95c87b6341e63` |
| `firmware/src/app/original_sound.h` | `c715ca3ad06228158a725128332ba680ae5a930c` |
| `firmware/src/app/factory_banks.h` | `f617cacefe5fd3f4ec97454dd148e913d2f4fb30` |
| `firmware/src/app/factory_banks.c` | `340d87fc51e53ddcce3c9484b2e1b1529aeabf14` |
| `firmware/src/app/bass_pattern_preset.h` | `0396235c0da545fdc3590e42ed8c97c2b84df8eb` |
| `firmware/src/app/project.c` | `8d69214ce9840865a1a007e7fc9d6033a09961d0` |
| `firmware/src/dsp/x0x_param.h` | `79d0f38d30190eceeda1226f58e631921e29d06f` |
| `experiments/original_synth/miami_program.c` | `3c0133d81c7049946faf2e751fb5a4f11ec5e80f` |
| `experiments/original_synth/chicago_program.c` | `a6fd3ae523069b8357ac6a6fb30d11cf23723740` |
| `experiments/original_synth/fused_reference.c` | `c1f9883f6d0331b3d3d68f4df95e74ad5f08d386` |
| `experiments/original_synth/miami_program.h` | `5b55afcab4be454731b1371e90e3c291dfc7ae3f` |
| `experiments/original_synth/chicago_program.h` | `6e5021eb2cb8b6495fbdfc6201f58a575c95c419` |
| `experiments/original_synth/fused_reference.h` | `81ce58008e8ac4eed5d20155e6a99cdb5a1990ff` |
| `experiments/original_synth/miami_program_private.h` | `0e6589f19b24afee30ea2220f0dca0baee79534d` |
| `experiments/original_synth/chicago_program_private.h` | `4f386aa0be5aee61744964bfa0c7214940b0b655` |
| `experiments/original_synth/binary32_math.h` | `37ffc4c5ae4a38d273808d36ff127d6d74c70f8c` |
| `experiments/original_synth/binary32_classify.h` | `ea30a78de9aed93f5820792fef18c3b5c6e855a2` |
| `tests/fixtures/project-v3-sound10.json` | `f9457d95bbc1dee232ef810e006f503afc2a963d` |
| `tests/fixtures/project-v6-sound10.json` | `4a2862c86deb196d3ae02b187a6cc134b2ec4dca` |
| `tests/host/fixtures/bass_identity/project-v3-sound10.json` | `f9457d95bbc1dee232ef810e006f503afc2a963d` |
| `tests/host/fixtures/bass_identity/project-v6-sound10.json` | `4a2862c86deb196d3ae02b187a6cc134b2ec4dca` |
| `tests/fixtures/v0102_store.bin` | `79041003d15533ca11ca6847944f3da5ef96bebf` |
| `tests/fixtures/v1_store.bin` | `9eec4232c45af845b559411290aa6f6968103013` |
