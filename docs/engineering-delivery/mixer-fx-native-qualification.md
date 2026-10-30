# PR #736 real framebuffer and SDK V1.2.1 verification

## Final status

**Source and scope:** the original #688 UI was compared against `7b2d1726f3515b319d5edcab34529007d33ab3a8`. Both knob-bank captions and colored destination fader travel occur in that original source. They remain in #736. The physical fader pickup marker and current peer-ownership integration are preserved. No unrelated UI has been introduced and the subsequent evidence-only commit changed no production firmware code.

**Draft, not merge-ready:** the full host suite has failures and skips. #734 is frozen. No device install, firmware flash, GitHub merge, public release, or installer publication occurred.

## Reproducible source and environment

- Repo: `gobbajogger/X0X37-for-Loopy-Pro`; PR #736, branch `chat/mixer-fx-page-port`.
- GitHub source at baseline `integration/all-in-one@a73882485f867d3ffeaea2c872f312fe4319295a`; exact C blob `4b79cfc3f022f15b6ab165c8e046cac470c22d06`.
- GitHub source for this verification `2c8fb7d5f975958bd0ef8c41b4ff44e39d5b0c6b`, tree `f234e5d5d0faba5992ccbd16b59ad464d5e7d1eb`; C blob `da2d1388b3b4f1ac98cc67490ddc72cf3fe7abb2`.
- Evidence commit `dc4109b7dfcddda41168016d0a8c33143606c4dc` adds only documentation, real paired PNG screenshots and the native host capture fixture. It does not alter code compiled into firmware. Therefore firmware source used in the builds is identical to that at the evidence commit.
- The complete GitHub source was downloaded via the GitHub authenticated REST API on its exact commit (tree and blob SHAs verified locally); no `git clone`, `fetch`, `pull`, or `push` network commands.
- Full AC79 SDK checkout verified to be **`AC79NN_SDK_V1.2.1_2023-12-13`**, HEAD `d179b4484759423312073f5fbb232501aa491047`. JieLi toolchain `jieli-linux-toolchains-20260730.1`, `pi32v2/bin/clang --version` output **4.0.1**.
- The firmware build runner was an isolated Linux **x86_64** Colima QEMU guest with native JieLi compiler, Python 3.12 and Pillow 10.2. The user's existing default Colima profile was left untouched.

## Authentic LCD framebuffer evidence

Both screen pairs use the production `smc_ui_draw()` in the actual host framebuffer, as compiled by `tests/host/smc_ui_test.sh`, with the same fixture on integration baseline and #736. Each screenshot preserves the raw native **240 × 240** LCD pixels; only comparison labels are outside the frames.

- `mixer-fx-home-framebuffer-before-after.png`: **1,290 / 57,600 changed pixels** (2.24%), paired PNG SHA256 `949d60331b793ed79521b90da9fce942a9e633aedf258a1f29e5cc9471a4184d`.
- `mixer-fx-page-framebuffer-before-after.png`: **1,358 / 57,600 changed pixels** (2.36%), paired PNG SHA256 `923b4cb1229cbe6ebad6be519ad668d510b76dcd728c7e68568c7dd5ca2c9507`.
- Production capture harness: `tests/host/smc_fx_framebuffer_capture.c` (uses `smc_ui_test.c`, not a hand-illustrated screenshot). Old SVG files were deleted.

## Host test results

The following checks passed on the exact candidate firmware source (macOS host, Python 3.12.15 and Pillow 13.0):

| Check | Measured result |
| --- | --- |
| `sh tests/host/smc_fx_page_port_test.sh` | **PASS**, 67 assertions |
| `python3 tests/host/smc_fx_peer_view_test.py` | **PASS**, 17 exact-source checks |
| `sh tests/host/smc_ui_test.sh` | **PASS**, 8,967 assertions, native firmware framebuffer |
| `sh tests/run_tests.sh` (`X0X_TEST_JOBS=6`) | **FAIL (exit 1)**: 191 PASS, 123 FAIL, 3 NOT_RUN |

**Independent Linux x86_64 GCC rerun:** In the isolated V1.2.1 firmware build environment, `sh tests/host/smc_fx_page_port_test.sh` passed all **67** checks, `python3 tests/host/smc_fx_peer_view_test.py` passed all **17** checks, and `sh tests/host/smc_ui_test.sh` passed all **8,967** true-renderer assertions. Combined command **exit 0** on the exact production source. Compiler warnings were present but non-fatal. Linux QA log SHA-256 `d28c6e9a800b316da10a9304736d22b43e557341e60e0cbf780c20bd498214cb`. This focused Linux pass is **not** a substitute for the failing full-suite gate.

This is the complete host suite, not a passing subset. The three NOT_RUN cases were `ble-controller-heap` (retired assumption), `drum909` (missing external 9W9), and `drum808` (missing external 8W8). On the same macOS runtime, many failures involve unavailable Darwin `rusage_info_v4` types, source-derived test harness assumptions or missing Git metadata in an API ZIP snapshot. **Do not treat all 123 failures as environment-only.** Example source-level failures include `smc-lock`, `smc-multi-peer`, `smc-select-mode`, `smc-ui-negative`, `smc-mapping`.

A separate **baseline rerun on integration@`a73882485`** used the same runtime with selectors `smc-ui-negative smc-lock smc-select-mode smc-multi-peer smc-mapping`; **all five selectors failed on baseline as well** (exit 1), so these five are not newly introduced solely by the screenshot/documentation correction. This does not prove that no other regressions are present.

Full-suite verification gate remains **RED**: not 0-fail / 0-skip. Do not merge into #734.

## Native Elite and Pro SDK firmware builds

Executed **both** `BOARD=smk37 MODEL=elite` and `BOARD=smk37 MODEL=pro` with `sh build.sh` in the isolated real Linux x86_64 guest using the exact full AC79 SDK V1.2.1, JieLi compiler and complete GitHub API source. Each target was built from its own clean source snapshot.

| Check | Elite | Pro |
| --- | --- | --- |
| Actual build exit | **0, PASS** | **0, PASS** |
| Package name | `x0x-smk37-elite.fwsc` | `x0x-smk37.fwsc` |
| Verified identity | `SMK-37 Elite_900` | `SMK-37 Pro_900` |
| Package size | 651,457 bytes | 651,457 bytes |
| App image | 452,224 bytes | 452,224 bytes |
| OTA loader | 7,341 bytes | 7,341 bytes |
| Free code bytes | 100,448 | 100,448 |
| RAM use | 94,960 / 98,304 bytes | 94,960 / 98,304 bytes |
| Free pool | **10,292 bytes (≥ 8,192 requirement)** | **10,292 bytes (≥ 8,192 requirement)** |
| Package SHA-256 | `2435dfc57cc01221f868a4d1440a62dbdb22979a9b7d0a3cfff721a0653608c0` | `09585d5d0d8180f73cc5e70d2a0b5d61a935d1d494f5d840fbc149156dc00c00` |
| Build log SHA-256 | `6fcb0c96a76ecc60f8bad7c34e89930ad4f75a1e3e0765caa53b88b516191ca0` | `233dfeeef6a286f12dc536059a1452d9af5c442c5b4a2bbc0eecc978ad8cbeb8` |

Both package files independently passed the **read-only** `tools/firmware_build_labels.py` inspection: exact SMK-37 model identity, required update loader marker, payload deinterleaving, and `package_damage` integrity verification. Inspections returned exit **0** and `status: "package-inspected"`. Both packages are **unversioned development outputs**, **not installable/DEV CHECKED** records and **not device tested**. A single non-fatal initializer-braces compiler warning appeared in `miami_decimator_adapter.c` for each model.

No firmware packages, compiler binaries, raw device captures, proprietary SDK or private headers were uploaded to the PR. Packages and logs are retained only in the local isolated build workspace.

## Next required gate

Work in #736 is complete for the requested #688 UI trim and true framebuffer evidence. **Keep draft/freeze**. The integration owner must obtain the required fixed-head 0-fail/0-skip host-suite result (and corresponding accepted Elite/Pro evidence) before any integration or merge. This work provides no permission for installer publication or hardware flashing.
