# Native evidence build of ce278129 (UNREVIEWED)

This is a native evidence build of `ce27812951555ff7ed9694bf99d20a65dd5f410b`. It is not release qualification and it is not a publish. No package was sent to a catalog, a pages branch, or a device.

The player-facing firmware bytes match the `43de100c` packages. The two commits since that build change host tests only. Elite pool free is 8200 bytes and Pro pool free is 8200 bytes. Both are at or above the 8192 byte floor.

The host suite was still running when this note was written. Totals are not claimed here.

## Commit chain

| | value |
| --- | --- |
| commit | `ce27812951555ff7ed9694bf99d20a65dd5f410b` |
| tree | `a917fb6a2c738bb410601bd784278bf35c142f1c` |
| parent | `b9d97e100e70b0b14d713b3892328d17ec709211` |
| parent of that | `43de100c83d845409bb801ecf29238a78f179aed` (tree `4993a90ae993da7d68ece0d352c88b27244b327e`) |
| parent of that | `35b729914792d947c675d29cb9bd80ac7d65d704` (v4 candidate) |

`git diff --stat 43de100c83d845409bb801ecf29238a78f179aed HEAD` is 13 files, 308 insertions, 44 deletions. Every path is under `tests/host/`:

- `original_release_ui_test.c`
- `part_off_test.c`
- `part_off_web_e2e.c`
- `part_off_web_e2e.mjs`
- `run_part_off_web_e2e.sh`
- `smc_multi_peer_test.c`
- `smc_select_mode_test.c`
- `smc_solo_overflow_test.py`
- `sound_matrix_test.c`
- `test_builtin_break_output.py`
- `test_original_build_composition.py`
- `test_smc_boot_saved_roles.py`
- `ui_peer_names_test.c`

None of those paths are under `firmware/`, `ble/`, `web/`, or `tools/`. The last commit, `ce278129`, changes only the four part-off files. The parent commit, `b9d97e10`, changes the other nine. No firmware or BLE source differs from `43de100c`, so the package hashes were expected to stay the same. They did.

## Inputs, expected against observed

| input | expected | observed |
| --- | --- | --- |
| toolchain archive | 26027508 B, `a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd` | identical |
| clang 4.0.1 binary | `c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f` | identical (`jieli-linux-toolchains-20260730.1`) |
| AC79 SDK | `d179b4484759423312073f5fbb232501aa491047` | identical, shallow, tag `AC79NN_SDK_V1.2.1_2023-12-13` |
| `uboot.boot` | `4e3b4c22…ded4` | identical, 14536 B |
| `cfg_tool.bin` | `27657995…0d1b` | identical, 383 B |
| `cfg/eq_cfg_hw.bin` | `41167491…ea7d` | identical, 655 B |
| schwung-9W9 | `10cbe5c1e1c9e65b0818bb34644ab1a13092082c` | identical (`NINEW9_DIR`) |
| schwung-8W8 | `94aa2716d4c2e5851fc7c5dbb4c17adfd133c997` | identical (`EIGHTW8_DIR`) |
| schwung-303 | `7680578cdecd91e512cfddb9c9eb07b287fb64f4` | identical (`SCHWUNG303`) |
| schwung-tb3po | `f49d81d05d1e29ae37728a08e30f193892be3bc9` | identical (`TB3PO_REF` file present) |

Build environment: `JIELI_TOOLCHAIN=/home/ubuntu/.jieli/toolchain`, `AC79_SDK=/home/ubuntu/fw-AC79_AIoT_SDK`, `JIELI_DOCKER=0`, `X0X_DEV_PADCAL=0`, `CTL_NO_CACHE=1`, `CC=cc`, `CXX=c++`. The controller build also set `SDK_LOCAL` to that SDK and `CTL_NATIVE=1`.

`python3 -c "import PIL"` succeeds. The loaded module is Pillow 13.0.0 from the user site. `apt` also installed `python3-pil` 10.2.0. `lua5.1`, `libclang-rt-18-dev`, and `libstdc++-14-dev` are installed. Node is v22.23.4. Playwright and Chromium were not installed.

`df -i /` before the work: 278679 inodes used, 10534761 free. After the builds, with the host suite still running: 299647 used, 10513793 free.

## Outputs against 43de100c

| file | size | sha256 | versus 43de100c |
| --- | --- | --- | --- |
| `x0x-smk37-elite.fwsc` | 651570 | `be4d608eb98b1408ffaed1e254b0e38f1c154b8c7865ae9a46f8ac184452bf85` | identical |
| `x0x-smk37.fwsc` | 651566 | `0773c61a48a7e1c84585a40d3b52f0e8c2cc49759b3c8693b3ac0ed983a67489` | identical |
| `x0x-smk37-elite-ble.fwsc` | 651570 | `4ca8b7b3f3c10ee5ca46639635cca4da7f13f49791a3b8a9d5d5cf59df2c8deb` | identical |
| `x0x-smk37-ble.fwsc` | 651566 | `cbb090311d0d9f9df92bfcf99c70c9354dc135a340d7438053d1c20a66b7acf7` | identical |
| `ctl.img` | 61552 | `a334208b51f62bc7231ebb76a6de0dfa260033d7f7316e5b92053c540bc1d984` | identical |

`tools/add_ctl.py ble/ctl/out/ctl.img --check` printed `ble/ctl/out/ctl.img: ok`.

## Headroom

The numbers come from the build log lines in `logs/build-elite.txt`, `logs/build-pro.txt`, and `logs/build-ctl.txt`.

Pool free is cap minus used: 344064 − 335864 = 8200. The floor check is `pool_free >= 8192`.

| | Elite | Pro |
| --- | --- | --- |
| app code free | 38664 (same as 43de100c) | 38680 (same as 43de100c) |
| RAM used / free / cap | 97328 / 976 / 98304 | 97328 / 976 / 98304 |
| pool used / free / cap | 335864 / 8200 / 344064 | 335864 / 8200 / 344064 |
| `pool_free >= 8192` | holds (8200) | holds (8200) |

BLE flash is 61552 of 65536. BLE RAM is 26336 of 26368. Both match 43de100c. No floor is violated.

## Version agreement

| source | value | where |
| --- | --- | --- |
| firmware ABOUT string | `DEV` | `firmware/src/app/version.h` line 7, `#define X0X_VERSION "DEV"`. `firmware/src/app/ui.c` builds `DnBOX ` + that macro + `  MEM `. Both app images contain `DnBOX DEV  MEM ` and `X0X DEV`. |
| manifest | `DEV` | `docs/handoff/build/manifest-ce278129.json` field `firmware_version_string` |
| catalog row `display_version` | `DEV` | `publish-ready-ce278129.md` and `versions-would-be.json` |
| would-be `versions.json` | `DEV` | `versions-would-be.json` → `versions[0].display_version` |
| expected display version | `0.0.24.5` | coordinator plan. The live entry `nightly-8be16486-dev` stores that string, and its description says the embedded firmware reports 0.0.24 and that 0.0.24.5 is a catalog label only. This tree does not contain `0.0.24.5` as `X0X_VERSION`. |
| label / version key | `nightly-ce278129-dev` | manifest `catalog_version`, catalog row `version`, `versions-would-be.json` `latest` and `versions[0].version` |
| build id | `ce278129` | prefix of commit `ce27812951555ff7ed9694bf99d20a65dd5f410b` |

The firmware string, the manifest, the catalog row, and the would-be `versions.json` entry agree with each other on `DEV`. They disagree with the expected display version `0.0.24.5`. Nothing was edited to close that gap.

`build.py --nightly-version` accepts only canonical `X.Y.Z`. It rejects `DEV`, `0.0.24.5`, and `nightly-ce278129-dev` with `firmware version must be canonical X.Y.Z (components 0..99)` and does not compile. `package_nightly.py` has `--firmware-version` and does not have `--nightly-version`. The same canonical-shape error is what `--check` prints for `DEV` and for `0.0.24.5`. A diagnostic `--firmware-version 0.0.24` (not used as the identity) then prints `NIGHTLY image contains DnBOX DEV`. The allocation line was not reached. No ledger file was edited.

## package_check against outputs

`package_nightly --check` produced no sha256. The comparison with `outputs[]` is **NOT_RUN**. There is no package_check digest that can be equal or different. The exact line is `package_nightly: firmware version must be canonical X.Y.Z (components 0..99)`.

The `-ble` files in `outputs[]` come from `tools/add_ctl.py`, not from `package_nightly.py`. Their hashes are the table above. They match 43de100c. They are a different byte string from the plain `.fwsc` files because the controller block is filled in. The size stays 651570 for Elite and 651566 for Pro.

## Host suite

Status: **running**. Command: `timeout 2400 sh tests/run_tests.sh`, started 2026-11-05T12:02:37Z, with `NINEW9_DIR`, `EIGHTW8_DIR`, `SCHWUNG303`, and `TB3PO_REF` set to the pinned checkouts. Parsed part-off and part-off web status lines are not available yet. A NOT_RUN, SKIP, or missing-node line will be reported as NOT_RUN even if the process exit code is 0. Playwright was not installed, so `offline-sequencer-browser` is expected to stay an environment gap. Its exact message will be copied from the log.

## Catalog dry run

Live file: `https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/versions.json`, HTTP 200, 66132 bytes, sha256 `3585a36cdd5da576be04834704f8ff98e8b59a03cef3d186cae336b89c19dd90`. The `install/versions.json` URL returned HTTP 404. No authentication challenge occurred. Nothing was pushed and the gh-pages branch was not cloned into a remote write.

The live `versions` array has 28 entries. One of them is `nightly-8be16486-dev`. There are 27 other entries, not 28 others. `latest` on the live file is `nightly-8be16486-dev`. `history` is an empty list.

`versions-would-be.json` is that file plus one new entry, and `latest` points at `nightly-ce278129-dev`. `versions-json.diff` is `diff -u` of the live file against that draft. `catalog-entry-sha256.txt` is the canonical JSON sha256 of each of the 28 pre-existing entries before and after the addition. All 28 lines say `equal`, including `nightly-8be16486-dev`. Differing count is 0.

`publish_nightly.py` was not run. The command lines are in `publish-ready-ce278129.md`. That file is a draft waiting for the owner's go-ahead. It is not a publish.

## Copying the bytes

`SHA256SUMS` lists every produced artifact: name, size, sha256. The files are in `/home/ubuntu/x0x-scratch-ce278129/pkg/` and also in the gitignored `build/` and `ble/ctl/out/` directories of this checkout. They are not in git. This VM disk is not a release archive. The bytes remain only while this machine keeps that scratch directory.

No binary, package, toolchain, SDK, or reference tree is in this commit.

## NOT_RUN

- `package_nightly.py` create and `--check`: `package_nightly: firmware version must be canonical X.Y.Z (components 0..99)`. No installer package file was produced. Allocation text was not reached.
- `publish_nightly.py`: not executed. It needs DEV CHECKED records this unreviewed build does not have. `--push` was not passed.
- Public `pkg` URL for the new catalog row: not published. The draft stores the local scratch path.
- Ledger number and `release_alloc.py`: not used.
- Host totals, FAIL names, NOT_RUN names, and the parsed part-off lines: the suite was still running.
- `tools/gate.sh`: not run.
