# Native evidence build of ce278129 (UNREVIEWED)

This is a native evidence build of `ce27812951555ff7ed9694bf99d20a65dd5f410b`. It is not release qualification and it is not a publish. No package was sent to a catalog, a pages branch, or a device.

The player-facing firmware bytes match the `43de100c` packages. The two commits since that build change host tests only. Elite pool free is 8200 bytes and Pro pool free is 8200 bytes. Both are at or above the 8192 byte floor.

The host suite finished with 391 ok, 6 FAIL, and 0 NOT_RUN. That differs from the reference figure of 364 ok / 19 FAIL / 6 NOT_RUN.

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

Build environment: `JIELI_TOOLCHAIN=$JIELI_TOOLCHAIN`, `AC79_SDK=$AC79_SDK`, `JIELI_DOCKER=0`, `X0X_DEV_PADCAL=0`, `CTL_NO_CACHE=1`, `CC=cc`, `CXX=c++`. The controller build also set `SDK_LOCAL` to that SDK and `CTL_NATIVE=1`.

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

The catalog description is: UNREVIEWED · DEVICE NOT TESTED. Host run measured 391 ok, 6 FAIL, 0 NOT_RUN. FAIL: original-release-ui, smc-ui, offline-sequencer-browser, ui-feedback, ui-glyph-polish, scenario-plock. NOT_RUN: none. package_nightly --check was NOT_RUN: there is no canonical X.Y.Z version for this build, and there is no installer and no package_check digest. The firmware images show "DnBOX DEV". display_version 0.0.24.5 is a carried catalog label only. It is not the firmware's own string.

`display_version` stays `0.0.24.5` in the manifest, the catalog row, and `versions-would-be.json`. That value is carried from gh-pages `7c7baecb1be17b860b13e8832882a399679231c1`, entry `nightly-8be16486-dev`. The firmware images show "DnBOX DEV".

The label is exactly `UNREVIEWED · DEVICE NOT TESTED` (U+00B7). `hardware_tested` is false. The version key is `nightly-ce278129-dev`. The build id `ce278129` is the prefix of `ce27812951555ff7ed9694bf99d20a65dd5f410b`.

## package_check against outputs

`package_nightly --check` produced no sha256. The comparison with `outputs[]` is **NOT_RUN**. There is no package_check digest that can be equal or different. The exact line is `package_nightly: firmware version must be canonical X.Y.Z (components 0..99)`.

The `-ble` files in `outputs[]` come from `tools/add_ctl.py`, not from `package_nightly.py`. Their hashes are the table above. They match 43de100c. They are a different byte string from the plain `.fwsc` files because the controller block is filled in. The size stays 651570 for Elite and 651566 for Pro.

## Host suite

`timeout 2400 sh tests/run_tests.sh` started 2026-11-05T12:02:37Z and ended 2026-11-05T12:23:46Z. Exit code 1. Elapsed 1269 seconds. The reference checkouts were set, and `drum909`, `drum808`, and `sequencer` each printed ok.

Measured: **391 ok, 6 FAIL, 0 NOT_RUN**. The reference figure 364 ok / 19 FAIL / 6 NOT_RUN differs: this run has 27 more ok, 13 fewer FAIL, and 6 fewer NOT_RUN.

FAIL names, with the first failing line:

- `original-release-ui`: the captured log is empty. The only recorded line is `FAIL original-release-ui`.
- `smc-ui`: `FAIL tests/host/smc_ui_test.c:285: engine_project_notes()`
- `offline-sequencer-browser`: `ModuleNotFoundError: No module named 'playwright'`. The runner printed FAIL, not a not-run line.
- `ui-feedback`: `Assertion help_with_pending_readout == help_only failed` in `tests/host/ui_feedback_test.c:98`
- `ui-glyph-polish`: `error: too few arguments to function call, expected 5, have 3` at `draw_adsr_inline`
- `scenario-plock`: `FAIL expect lanes == 1, got 2`

NOT_RUN names: none.

Compared with the 8be16486 FAIL names, none of these six are new. Compared with the 43de100c FAIL names, these now print ok: `original-build`, `ui-theme`, `smc-select-mode`, `smc-solo-overflow`, `smc-boot-saved-roles`, `smc-multi-peer`, `ui-peer-names`, `builtin-break-output`, `sound-matrix`, `smc-remainder`, `offline-touchosc-lua`.

`part-off` parsed status: `part_off: ok (0 failing cases)`. `part-off-web-e2e` parsed status: `part_off_web_e2e: ok (0 failing cases)`. The runner lines are `ok` as well. Neither is NOT_RUN.

## Catalog dry run

Live file: `https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/versions.json`, HTTP 200, 66132 bytes, sha256 `3585a36cdd5da576be04834704f8ff98e8b59a03cef3d186cae336b89c19dd90`. The `install/versions.json` URL returned HTTP 404. No authentication challenge occurred. Nothing was pushed and the gh-pages branch was not cloned into a remote write.

The gh-pages tip read for this dry run is `7c7baecb1be17b860b13e8832882a399679231c1`. `git ls-remote` showed that same SHA as `refs/heads/gh-pages` of `gobbajogger/x0x37-for-Loopy-Pro_nightly`. The tip had not moved. The file `versions.json` at that commit is byte-identical to the HTTP 200 pages file above.

The live `versions` array has 28 entries. One of them is `nightly-8be16486-dev`. There are 27 other entries. `latest` on the live file is `nightly-8be16486-dev`. `history` is an empty list. That commit's tree has no `.fwsc` files. `nightly-8be16486-dev` points `pkg` at raw GitHub URLs in the main repository.

`versions-would-be.json` adds one entry and points `latest` at `nightly-ce278129-dev`. `versions-json.diff` is `diff -u` against that gh-pages file. The only removed line is the old `latest` value. The 28 pre-existing entries are unchanged. `catalog-entry-sha256.txt` says `equal` for all 28, including `nightly-8be16486-dev`.

## Push A

This is a listing. It was not executed. No file was written to gh-pages, and no `.fwsc` is in git.

Paths relative to the gh-pages root, matching a new `artifacts/nightly-dev/` directory. Sizes and sha256 are the bundled packages from this build.

| path | size | sha256 |
| --- | --- | --- |
| `artifacts/nightly-dev/4ca8b7b3f3c10ee5ca46639635cca4da7f13f49791a3b8a9d5d5cf59df2c8deb/x0x37-elite-dev.fwsc` | 651570 | `4ca8b7b3f3c10ee5ca46639635cca4da7f13f49791a3b8a9d5d5cf59df2c8deb` |
| `artifacts/nightly-dev/cbb090311d0d9f9df92bfcf99c70c9354dc135a340d7438053d1c20a66b7acf7/x0x37-pro-dev.fwsc` | 651566 | `cbb090311d0d9f9df92bfcf99c70c9354dc135a340d7438053d1c20a66b7acf7` |

`versions.json` would be modified, not added. The `nightly-8be16486-dev` layout stores the controller as `controller_sha256` and `controller_size` on the model. It does not add a separate controller file. `ctl.img` is 61552 bytes, sha256 `a334208b51f62bc7231ebb76a6de0dfa260033d7f7316e5b92053c540bc1d984`. `package_nightly.py` produced no installer file.

The Pages URLs in the new catalog row are:

- `https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/artifacts/nightly-dev/4ca8b7b3f3c10ee5ca46639635cca4da7f13f49791a3b8a9d5d5cf59df2c8deb/x0x37-elite-dev.fwsc`
- `https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/artifacts/nightly-dev/cbb090311d0d9f9df92bfcf99c70c9354dc135a340d7438053d1c20a66b7acf7/x0x37-pro-dev.fwsc`

Would-be `git diff --stat --no-renames` if those paths were added and only `versions.json` were edited:

```
 artifacts/nightly-dev/4ca8b7b3f3c10ee5ca46639635cca4da7f13f49791a3b8a9d5d5cf59df2c8deb/x0x37-elite-dev.fwsc | Bin 0 -> 651570 bytes
 artifacts/nightly-dev/cbb090311d0d9f9df92bfcf99c70c9354dc135a340d7438053d1c20a66b7acf7/x0x37-pro-dev.fwsc | Bin 0 -> 651566 bytes
 versions.json | 34 +++++-
 3 files changed, 34 insertions(+), 1 deletion(-)
```

The committed `versions-json.diff` does not contain the binary lines. It shows only the new catalog entry and the `latest` pointer.

## Copying the bytes

`SHA256SUMS` lists name, size, and sha256. The bytes are not in git. No installer package was produced.

## NOT_RUN

- `package_nightly.py` create and `--check`: `package_nightly: firmware version must be canonical X.Y.Z (components 0..99)`. No installer file. Allocation text was not reached.
- `publish_nightly.py`: not executed. No `--push` and no `--remote`.
- Ledger number and `release_alloc.py`: not used.
- `tools/gate.sh`: not run.
- Host NOT_RUN names: none were printed.
