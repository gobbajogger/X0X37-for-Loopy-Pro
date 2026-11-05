# Native evidence build of 43de100c (UNREVIEWED), not release qualification

Label: native evidence build of 43de100c (UNREVIEWED), not release qualification.

This file records one native build of source commit `43de100c83d845409bb801ecf29238a78f179aed` on a Linux x86-64 VM. It is an unreviewed evidence build. It is not release qualification, not a publish, and not a freeze. No firmware binary, package, toolchain file, or SDK file is in this commit.

## Commit and tree

| Item | Value |
| --- | --- |
| Candidate commit | `43de100c83d845409bb801ecf29238a78f179aed` |
| Candidate tree | `4993a90ae993da7d68ece0d352c88b27244b327e` |
| Source branch named by the request | `lane/dev-m7a-v4-fix-20261105` |
| Worktree at build time | clean, detached at that commit |
| Parent | v4 `35b729914792d947c675d29cb9bd80ac7d65d704` (tree `ccb0aa5b0ad2827f4a5176f12c032eba3db23c71`) |

`git rev-parse HEAD` and `git rev-parse HEAD^{tree}` matched those two values before any build. The parent was fetched. `git diff --stat 35b72991 HEAD` is only these eight test files:

```
 tests/host/part_off_file.c                        |  2 +-
 tests/host/part_off_store.c                       |  2 +-
 tests/host/part_off_test.c                        |  4 ++--
 tests/host/run_part_off.sh                        |  4 ++--
 tests/host/run_part_off_web_e2e.sh                |  4 ++--
 tests/host/run_part_off_web_e2e_node_gate_test.sh | 29 +++++++++++++++++++++++
 tests/host/smc_remainder_test.c                   | 10 ++++----
 tests/run_tests.sh                                |  7 +++---
 8 files changed, 46 insertions(+), 16 deletions(-)
```

`firmware` tree `8699e9f0ab39e4d804183eaaf86d8f51d900d404` and `ble` tree `0d4304546a60f22a06271b22b4881f6304ad439c` are the same trees as v4. `tests/run_tests.sh` is mode `644` and was run with `sh`.

## Inputs, expected against observed

All public inputs matched the expected pins. No 401 or 403 occurred.

| Input | Expected | Observed |
| --- | --- | --- |
| Toolchain URL | `https://pkgman.jieliapp.com/s/linux-toolchain` | HTTP 302, then HTTP 200 |
| Final archive URL | `https://jl-update.oss-cn-shenzhen.aliyuncs.com/jieli-linux-toolchains-20260730.1.tar.xz` | same |
| Archive size | 26027508 | 26027508 |
| Archive sha256 | `a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd` | same |
| clang | version 4.0.1 | `clang version 4.0.1` |
| clang binary sha256 | `c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f` | same (`pi32v2/bin/clang` and `common/bin/clang`) |
| AC79 SDK | shallow clone, tag `AC79NN_SDK_V1.2.1_2023-12-13` | peeled to `d179b4484759423312073f5fbb232501aa491047` |
| `uboot.boot` | `4e3b4c220dc96641cb5a723f41e68ce41d5261ae9434bb33fbd7f2c59976ded4` | same |
| `cfg_tool.bin` | `276579954f076886a6a7694f65dc71c034a63a2c204b76749065c0ac7b010d1b` | same |
| `cfg/eq_cfg_hw.bin` | `41167491bffed4651750719c973d2758adeb9021a5670d02d6a53c85ed80ea7d` | same |

The three SDK hashes are the pins in `tools/build.py`. The clone was not V1.2.0.

## Environment

Evidence firmware environment: `JIELI_TOOLCHAIN=/home/ubuntu/.jieli/toolchain`, `AC79_SDK=/home/ubuntu/fw-AC79_AIoT_SDK`, `JIELI_DOCKER=0`, `X0X_DEV_PADCAL=0`, `CTL_NO_CACHE=1`, `SDK_LOCAL=$AC79_SDK`, `CTL_NATIVE=1`, `CC=cc`, `CXX=c++`. Linux x86-64. These evidence builds did not pass `--nightly-version` or `--release`, so the package identity is `SMK-37 Elite_900` / `SMK-37 Pro_900` and the about text uses the default `DEV` version. Command times and exit codes are in `commands.txt`.

Host compiler: Ubuntu clang 18.1.3 (`/usr/bin/cc`). Python 3.12.3. Pillow 13.0.0 was already installed.

### Environment additions

Installed on this VM before the host suite, from public Ubuntu packages. Nothing from this list is in git.

| Package | Version |
| --- | --- |
| `libclang-rt-18-dev` | `1:18.1.3-1ubuntu1` |
| `lua5.1` | `5.1.5-9build2` |
| `libc6-i386` | `2.39-0ubuntu8.9` (dependency of compiler-rt) |
| `lib32gcc-s1` | `14.2.0-4ubuntu2~24.04.1` |
| `lib32stdc++6` | `14.2.0-4ubuntu2~24.04.1` |

`find / -name 'libclang_rt.asan*'` found `libclang_rt.asan-x86_64.a` and `libclang_rt.asan_static-x86_64.a` under `/usr/lib/llvm-18/lib/clang/18/lib/linux/`. `libclang_rt.ubsan_standalone-x86_64.a` is in the same directory. A probe `cc -fsanitize=address,undefined` linked after the install.

`node` was already on `PATH` at `/exec-daemon/node`, version `v22.23.4`. Apt `nodejs` was not installed.

`playwright` 1.64.1 was installed with `python3 -m pip install --user playwright`. No Playwright browser download was run. Google Chrome `156.0.8078.47` (`google-chrome-stable 156.0.8078.47-1`) was already installed. The suite was given `DNBOX_CHROMIUM_EXECUTABLE=/usr/bin/google-chrome`.

The `490e656c` and `35b72991` evidence runs did not have this compiler-rt install. Their ASan and UBSan results are environment differences, listed separately under host tests.

## Evidence outputs, byte for byte against v4

These hashes are the padcal-off builds above, before the later nightly-label rebuild used only for packaging. Each one matches the v4 `35b72991` evidence manifest (`docs/handoff/build/manifest-35b72991.json` on `evidence/build-v4-35b72991-20261105`).

| File | Size | sha256 | v4 |
| --- | --- | --- | --- |
| `build/x0x-smk37-elite.fwsc` | 651570 | `be4d608eb98b1408ffaed1e254b0e38f1c154b8c7865ae9a46f8ac184452bf85` | match |
| `build/x0x-smk37.fwsc` (Pro) | 651566 | `0773c61a48a7e1c84585a40d3b52f0e8c2cc49759b3c8693b3ac0ed983a67489` | match |
| `build/x0x-smk37-elite-ble.fwsc` | 651570 | `4ca8b7b3f3c10ee5ca46639635cca4da7f13f49791a3b8a9d5d5cf59df2c8deb` | match |
| `build/x0x-smk37-ble.fwsc` | 651566 | `cbb090311d0d9f9df92bfcf99c70c9354dc135a340d7438053d1c20a66b7acf7` | match |
| `ble/ctl/out/ctl.img` | 61552 | `a334208b51f62bc7231ebb76a6de0dfa260033d7f7316e5b92053c540bc1d984` | match |

Nothing in that set differs. The controller sha256 is the same value recorded for `8be16486` and `490e656c`, size 61552 bytes. `add_ctl --check` printed `ble/ctl/out/ctl.img: ok`.

## Headroom and floors

Caps: RAM 98304, pool 344064, pool free at least 8192, BLE flash 65536, BLE RAM 26368.

| Measure | Elite | Pro | v4 | Floor |
| --- | --- | --- | --- | --- |
| App code free | 38664 | 38680 | match | area 552672, no overflow |
| RAM used | 97328 | 97328 | match | cap 98304 |
| RAM free | 976 | 976 | match | used is under the cap |
| Pool used | 335864 | 335864 | match | cap 344064 |
| Pool free | 8200 | 8200 | match | need >= 8192 |
| BLE flash | 61552 / 65536 | same image | match | under the cap |
| BLE RAM | 26336 / 26368 | same image | match | under the cap |

Floor verdict: pass. Pool free is 8200, which is 8 bytes above 8192. That is inside the floor, and it is close. The same 8200-byte pool free is what v4 and v3 `490e656c` recorded. The `8be16486` reference had app 40904 free, RAM 992 free, and pool 8328 free. This tip matches v4, so Elite app free is 2240 bytes below that reference, RAM free is 16 bytes below it, and pool free is 128 bytes below it. No floor is violated.

The later packaging rebuild with `--nightly-version 0.0.25` is a different image. Its app free is Elite 38600 and Pro 38616. RAM and pool stay 97328 and 335864. Those packages are not the v4 comparison set.

## Host tests

`timeout 2400 sh tests/run_tests.sh` started `2026-11-05T10:33:05Z` and ran 1238 seconds. Exit code 1. The last line is `TESTS FAILED`. The 40 minute cap did not fire.

Totals: ok 380, fail 15, not-run 2, counted lines 397.

Suite not-run lines:

- `drum909` (`../schwung-9W9/src/dsp/er99_engine.c` absent)
- `drum808` (`../schwung-8W8/src/dsp/sc808_engine.cpp` absent)

`rg NOT_RUN build/host/*.log` also hits phrases inside tests whose suite line is ok: `audio-stress.log` says `target NOT_RUN`, and `ui-page-wrap.log` says `full UI NOT_RUN`. `update-names.log` contains `quick check SKIPPED` inside a passing test. Those are not suite not-run lines. The suite did not exit 0, and it did not print `all tests passed`.

### Explicit verdicts

`smc-remainder`: ok. The log ends `smc-remainder: 205861 checks PASS`. The suite line is `ok   smc-remainder`.

`part-off-web-e2e`: ok. The log ends `part_off_web_e2e: ok (0 failing cases)` after five PASS lines. The suite line is `ok   part-off-web-e2e`.

`part-off-web-e2e-node-gate`: ok. `tests/host/run_part_off_web_e2e_node_gate_test.sh` ran the web script with `node` removed from `PATH`. The log is:

```
  FAIL part_off_web_e2e (node not found)
  ok   part_off_web_e2e_node_gate (missing node => exit 1)
```

The suite line is `ok   part-off-web-e2e-node-gate`. Missing node is a non-zero FAIL, not a NOT_RUN exit 0.

`part-off`: ok. The suite sets the base in `tests/run_tests.sh`:

```
run part-off env PART_OFF_REQUIRE_BASE=1 PART_OFF_BASE_REF=c32c54ac8ec68aaca53adccf7af66f155590f2dc sh tests/host/run_part_off.sh
```

`tests/host/run_part_off.sh` says the base must be the published pre-M7 commit `c32c54ac` (the RC1 tree). That object is in this clone (`c32c54ac Preserve the exact 0.0.25-rc1 source tree for independent QA`). The script archives it and the log shows:

```
    crc base 22d26473 here 22d26473
  PASS all_on_bit_exact_vs_base
part_off: ok (0 failing cases)
part_off_store: ok (0 failing cases; dumps and sha256 in build/host/part-off/store)
```

No fetch of `4b240bd3` was required. That object is not the base this suite uses.

### Every FAIL

| Name | Class | In v4 28-FAIL list | One-line cause |
| --- | --- | --- | --- |
| `original-release-ui` | REAL | yes | undeclared `header_transport` at `tests/host/original_release_ui_test.c:377` |
| `original-build` | REAL | yes | `AssertionError`: the expected `if (vel && pad < 8u && smk_div_owner && !smk_pad_note[pad])` snippet is not in the SMK input header |
| `ui-theme` | REAL | yes | same undeclared `header_transport`, compiled through `tests/host/ui_theme_test.c` |
| `smc-select-mode` | REAL | yes | `FAIL tests/host/smc_select_mode_test.c:61: smc_page_now() == (strip == 5 ? 0 : strip == 6 ? 1 : 3)` |
| `smc-solo-overflow` | REAL | yes | first compile error: no member named `kind` in the combined `smc_ui` struct |
| `smc-boot-saved-roles` | REAL | yes | `ValueError: substring not found` for `int bad = 0, rc = load_settings(&set);` |
| `smc-multi-peer` | REAL | yes | `FAIL tests/host/smc_multi_peer_test.c:145: smc_states[0].fx_effect[0] != smc_states[1].fx_effect[0]` |
| `ui-peer-names` | REAL | yes | `FAIL tests/host/ui_peer_names_test.c:15: name_row` |
| `smc-ui` | REAL | yes | `FAIL tests/host/smc_ui_test.c:285: engine_project_notes()` |
| `builtin-break-output` | REAL | yes | `AssertionError: '909 GR' not found` in the break-pack header comment |
| `offline-sequencer-browser` | ENV-ONLY | yes | Chrome `FATAL`: `Socket path too long` for `SingletonSocket` under the test `TMPDIR` |
| `ui-feedback` | REAL | yes | `assert(help_with_pending_readout == help_only)` failed at `tests/host/ui_feedback_test.c:98` |
| `sound-matrix` | REAL | yes | undeclared `adsr_inline_page` at `tests/host/sound_matrix_test.c:143` |
| `ui-glyph-polish` | REAL | yes | `draw_adsr_inline` expected 5 arguments, the test passes 3 (`ui_glyph_polish_test.c:80`) |
| `scenario-plock` | REAL | yes | `FAIL expect lanes == 1, got 2` |

`ui-feedback` is still FAIL. This commit does not change firmware. The first failing assertion is `help_with_pending_readout == help_only` in `readout_does_not_cover_help`. The blit path still calls `cv_lcd_maybe_input()` from `cv_lcd_emit` and `cv_lcd_full` in `firmware/src/app/ui.c`, and that function calls `input()` through `cv_lcd_input()` once per millisecond. That is the `179ab384` path. The hash of the help screen changes when a readout is pending.

ENV-ONLY FAIL group: `offline-sequencer-browser` only. Playwright and system Chrome were installed. Chrome aborted before any page assertion because the socket path was too long.

REAL FAIL group: the other 14 names in the table. Each one is a test or source mismatch, and each one is also in the v4 28-FAIL list. No current FAIL name is new relative to that list.

### Versus the v4 28-FAIL list

All 15 current FAIL names are in that list. Thirteen names from that list are ok now:

Source fix on this commit:

- `smc-remainder` (ok, 205861 checks PASS; it was FAIL on v4)
- `part-off` (ok; v4 failed because `4b240bd33a0eb7008b44e12bc270aba4d32b82c1` was not in the clone)

Environment, compiler-rt installed before this suite and absent on the v4 run:

- `fastmath-ubsan`
- `original-release-integration`
- `chicago-fx2-inserts`
- `original-sound-cell`
- `freeplay-rate-pads`
- `engine-808-scratch-alias`
- `ble-device-setting-flow`
- `ble-toggle-persistence-gate`
- `smc-ui-bounds`
- `original-stale-release`

Environment, `lua5.1` installed before this suite:

- `offline-touchosc-lua` (log: `TouchOSC root controller: PASS`; the suite invokes `lua5.1` with no skip guard, so a missing interpreter is a FAIL)

v4 not-run that are ok here because compiler-rt is present: `ble-midi-receive`, `ble-capture-replay`, `ble-role-switch`, `ui-303-entry`. `drum909` and `drum808` stay not-run. The published v4 host log records 28 FAIL names. The two names in that list and not in the `490e656c` 26 are `smc-remainder` and `ui-feedback`. This run makes `smc-remainder` ok and leaves `ui-feedback` FAIL.

### Versus the 490e656c RC1 26-FAIL list

FAIL now but not in that list: `ui-feedback` only. It is REAL, and it is the v4 regression this test-only commit does not touch.

In that list and ok now, source fix: `part-off`.

In that list and ok now, environment only (compiler-rt or lua, which those baseline runs did not install): `fastmath-ubsan`, `original-release-integration`, `chicago-fx2-inserts`, `original-sound-cell`, `freeplay-rate-pads`, `engine-808-scratch-alias`, `ble-device-setting-flow`, `ble-toggle-persistence-gate`, `smc-ui-bounds`, `original-stale-release`, `offline-touchosc-lua`.

Still FAIL and in that list: `original-release-ui`, `original-build`, `ui-theme`, `smc-select-mode`, `smc-solo-overflow`, `smc-boot-saved-roles`, `smc-multi-peer`, `ui-peer-names`, `smc-ui`, `builtin-break-output`, `offline-sequencer-browser`, `sound-matrix`, `ui-glyph-polish`, `scenario-plock`.

`490e656c` not-run that are ok here for the same compiler-rt reason: `ble-midi-receive`, `ble-capture-replay`, `ble-role-switch`, `ui-303-entry`.

### Versus the frozen 8be16486 17-FAIL list

Still FAIL: `original-release-ui`, `original-build`, `ui-theme`, `smc-select-mode`, `smc-solo-overflow`, `smc-boot-saved-roles`, `smc-multi-peer`, `ui-peer-names`, `smc-ui`, `builtin-break-output`, `offline-sequencer-browser`, `ui-feedback`, `sound-matrix`, `ui-glyph-polish`, `scenario-plock`.

Now ok: `smc-remainder` (source fix) and `offline-touchosc-lua` (lua5.1 installed).

No FAIL name is new relative to that 17.

## Packaging and --check

`releases/openbeta/ledger.json` was not edited. Its sha256 is still `5ea26647dac1fba48fc90053882474ac8da084c7cdf59a259822a1dd948bbde1`. `allocations` is empty. `release_alloc.py` was not run.

`tools/package_nightly.py` has no `--nightly-version` flag. The exact error is exit 2, `the following arguments are required: --model, --controller-sha256, --controller-size, --firmware-version`.

Create against the evidence `*_900` DEV image, with `--firmware-version 0.0.25`, exited 1: `package_nightly: NIGHTLY image contains DnBOX DEV`. No package was written. That is not a missing ledger number. `_900` skips the allocation check, and the refusal happens earlier, in `_app_version`.

`tools/build.py` accepts `--nightly-version`. Elite and Pro were rebuilt only for this local check, after the evidence hashes were copied aside:

`MODEL=elite ./build.sh --board smk37 --nightly-version 0.0.25 --git-sha 43de100c83d845409bb801ecf29238a78f179aed`

and the same command with `MODEL=pro`. Identity stayed `SMK-37 Elite_900` / `SMK-37 Pro_900`. Create and `--check` then used `--firmware-version 0.0.25`, the controller sha256 and size, and outputs under `/tmp/x0x-pkg-43de100c/nightly` outside the repo. Both `--check` runs exited 0. The receipt field `release_display` is `0.0.25_openbeta`. That string is the tool's display suffix. It is not a ledger allocation and not a publish.

| Package | Size | sha256 | check |
| --- | --- | --- | --- |
| Elite nightly local package | 651570 | `efde47c9da2f51363b3021ca8178dce73f3fb112df3047576cb886e768e2ee0f` | rc 0 |
| Pro nightly local package | 651566 | `5657ec2ef5220837ef490fd97f9c922b9bedd48383f26b58c554e24119e94ac9` | rc 0 |

These hashes are not the evidence outputs and not the v4 packaging hashes. The short git sha in the image is `43de100c`. No ledger number was demanded. No package or check step is NOT_RUN for lack of a ledger number. The packages were not committed.

## NOT_RUN

| Item | Why |
| --- | --- |
| `tools/gate.sh` | Not invoked. This build is not a release gate. |
| `python3 tools/dev_validate.py run` | Not invoked. `run` forces `X0X_DEV_PADCAL=1`, rebuilds, and writes a DEV CHECKED record. This grant is an unreviewed evidence build, not that validation claim. |
| `tools/publish_nightly.py` | Not run. No `--push` and no `--remote`. |
| `tools/release_alloc.py` and `releases/openbeta/ledger.json` | No ledger allocation. The ledger was not edited. |
| Reference trees `schwung-9W9` and `schwung-8W8` | Absent, so `drum909` and `drum808` are not-run. |
| Device install, NIGHTLY catalog, `gh-pages` | Not touched. |

## Inodes

`df -i /` on this VM, all 3% used:

| When | IUsed | IFree |
| --- | --- | --- |
| Before the toolchain and SDK downloads | 269083 | 10544357 |
| After the SDK clone and toolchain extract | 287679 | 10525761 |
| After the host suite | 298115 | 10517443 |
| After the packaging rebuild | 298126 | 10515314 |

## What is not in git

No binaries are committed. The `.fwsc` files, `ctl.img`, the toolchain, the AC79 SDK, and the scratch nightly packages stay on this VM.
