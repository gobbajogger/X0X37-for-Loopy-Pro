# Native evidence build of 35b72991 (UNREVIEWED), not release qualification

This record is an unreviewed native evidence build of commit `35b729914792d947c675d29cb9bd80ac7d65d704`. It is not release qualification, not a publish, and not a freeze. QA had already returned this candidate as needs changes. The numbers below are the baseline for that commit. No firmware binary, package, toolchain file, or SDK file is in this commit.

## Commit and tree

| Item | Value |
| --- | --- |
| Commit | `35b729914792d947c675d29cb9bd80ac7d65d704` |
| Tree | `ccb0aa5b0ad2827f4a5176f12c032eba3db23c71` |
| Source branch named by the request | `lane/dev-m7a-v4-24.5-carry-20261105` |
| Worktree at build time | clean, detached at that commit |
| Relationship | v3 `490e656c` plus the 10 following commits on this tip |

`git rev-parse HEAD` and `git rev-parse HEAD^{tree}` matched the expected values before any build. `tests/run_tests.sh` is mode `644` and was run with `sh`.

## Inputs, expected against observed

All public inputs matched the expected pins. No 401 or 403 occurred.

| Input | Expected | Observed |
| --- | --- | --- |
| Toolchain URL | `https://pkgman.jieliapp.com/s/linux-toolchain` | HTTP 200, one redirect |
| Final archive URL | `https://jl-update.oss-cn-shenzhen.aliyuncs.com/jieli-linux-toolchains-20260730.1.tar.xz` | same |
| Archive size | 26027508 | 26027508 |
| Archive sha256 | `a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd` | same |
| clang | version 4.0.1 | `clang version 4.0.1` |
| clang binary sha256 | `c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f` | same |
| AC79 SDK | shallow clone, tag `AC79NN_SDK_V1.2.1_2023-12-13` | peeled to `d179b4484759423312073f5fbb232501aa491047` |
| `uboot.boot` | `4e3b4c220dc96641cb5a723f41e68ce41d5261ae9434bb33fbd7f2c59976ded4` | same |
| `cfg_tool.bin` | `276579954f076886a6a7694f65dc71c034a63a2c204b76749065c0ac7b010d1b` | same |
| `cfg/eq_cfg_hw.bin` | `41167491bffed4651750719c973d2758adeb9021a5670d02d6a53c85ed80ea7d` | same |

The three SDK hashes are the pins in `tools/build.py`. The clone was not V1.2.0.

Inodes on `/`: 10544358 free before the downloads, 10522456 free after the builds and tests. Both readings are 3% used.

## Environment for the evidence firmware

`JIELI_TOOLCHAIN=/home/ubuntu/.jieli/toolchain`, `AC79_SDK=/home/ubuntu/fw-AC79_AIoT_SDK`, `JIELI_DOCKER=0`, `X0X_DEV_PADCAL=0`, `CTL_NO_CACHE=1`, `SDK_LOCAL=$AC79_SDK`, `CTL_NATIVE=1`, `CC=cc`, `CXX=c++`. Linux x86-64. These builds did not pass `--nightly-version` or `--release`, so the package identity is `SMK-37 Elite_900` / `SMK-37 Pro_900` and the about text uses the default `DEV` version. Command times and exit codes are in `commands.txt`.

## Evidence outputs

These hashes are the padcal-off builds above, before the later nightly-label rebuild used only for packaging.

| File | Size | sha256 |
| --- | --- | --- |
| `build/x0x-smk37-elite.fwsc` | 651570 | `be4d608eb98b1408ffaed1e254b0e38f1c154b8c7865ae9a46f8ac184452bf85` |
| `build/x0x-smk37.fwsc` (Pro) | 651566 | `0773c61a48a7e1c84585a40d3b52f0e8c2cc49759b3c8693b3ac0ed983a67489` |
| `build/x0x-smk37-elite-ble.fwsc` | 651570 | `4ca8b7b3f3c10ee5ca46639635cca4da7f13f49791a3b8a9d5d5cf59df2c8deb` |
| `build/x0x-smk37-ble.fwsc` | 651566 | `cbb090311d0d9f9df92bfcf99c70c9354dc135a340d7438053d1c20a66b7acf7` |
| `ble/ctl/out/ctl.img` | 61552 | `a334208b51f62bc7231ebb76a6de0dfa260033d7f7316e5b92053c540bc1d984` |

`add_ctl --check` printed `ble/ctl/out/ctl.img: ok`. The `ble/` tree is `0d4304546a60f22a06271b22b4881f6304ad439c` on both this commit and `8be16486`, so the controller sources are unchanged. The controller sha256 matches the full hash recorded on `8be16486` (`docs/evidence/build-0.0.24.1/source_and_inputs.json` and the LCD evidence note there). Size is 61552 bytes, the same size cited for that build. The short form `a334208b920` is not a prefix of this hash. The hash does contain `920` later in the digest.

## Headroom and floors

Caps: RAM 98304, pool 344064, pool free at least 8192, BLE flash 65536, BLE RAM 26368.

| Measure | Elite | Pro | Floor |
| --- | --- | --- | --- |
| App code free | 38664 | 38680 | area 552672, no overflow |
| RAM used | 97328 | 97328 | cap 98304 |
| RAM free | 976 | 976 | used is under the cap |
| Pool used | 335864 | 335864 | cap 344064 |
| Pool free | 8200 | 8200 | need >= 8192 |
| BLE flash | 61552 / 65536 | same image | under the cap |
| BLE RAM | 26336 / 26368 | same image | under the cap |

Floor verdict: pass. Pool free is 8200 bytes. The floor is 8192, so the margin is 8 bytes. The sibling build of `490e656c` reported the same 8200 bytes free. Against the `8be16486` reference (app 40904 free, RAM 97312 used and 992 free, pool 335736 used and 8328 free, BLE flash 61552, BLE RAM 26336), this tip uses more app code and 128 more pool bytes. BLE flash and BLE RAM match that reference.

## Host tests

`timeout 20m sh tests/run_tests.sh` started `2026-11-05T10:08:47Z` and the log closed `2026-11-05T10:27:20Z` (1113 seconds). Exit code 1. The last line is `TESTS FAILED`. The 20 minute cap did not fire.

Totals: ok 375, fail 28, not-run 6, counted lines 409.

Not-run: `ble-midi-receive` (ASan runtime), `ble-capture-replay` (ASan runtime), `drum909` (missing `../schwung-9W9`), `drum808` (missing `../schwung-8W8`), `ble-role-switch` (ASan runtime), `ui-303-entry` (ASan).

Every name on the frozen `8be16486` list failed: `original-release-ui`, `original-build`, `ui-theme`, `smc-select-mode`, `smc-solo-overflow`, `smc-boot-saved-roles`, `smc-multi-peer`, `ui-peer-names`, `smc-ui`, `smc-remainder`, `builtin-break-output`, `offline-sequencer-browser`, `offline-touchosc-lua`, `ui-feedback`, `sound-matrix`, `ui-glyph-polish`, `scenario-plock`.

`smc-remainder` and `ui-feedback` are in that frozen list. They failed here as well. They are not new relative to that list.

Eleven FAIL names are new relative to that list. All eleven are from this machine, not from a new product assertion in the suite summary:

- Missing UBSan runtime (`libclang_rt.ubsan_standalone-x86_64.a`): `fastmath-ubsan`, `freeplay-rate-pads`, `smc-ui-bounds`, `original-stale-release`.
- Missing ASan runtime (`libclang_rt.asan-x86_64.a`): `chicago-fx2-inserts`, `original-sound-cell`, `engine-808-scratch-alias`, `ble-device-setting-flow`, `ble-toggle-persistence-gate`, `original-release-integration`.
- `part-off`: `PART_OFF_REQUIRE_BASE` and base `4b240bd33a0eb7008b44e12bc270aba4d32b82c1` is not in this clone, so `tests/host/run_part_off.sh` exits 1. That is the script's fail-closed path, not a silent skip. It is not a missing-archive failure.

The primary totals above stay the raw full-suite result. They are not revised by the later re-run.

### Missing compiler-rt archives

The full-suite log contains `cannot find .../libclang_rt.asan-x86_64.a`, `libclang_rt.asan_static-x86_64.a`, and `libclang_rt.ubsan_standalone-x86_64.a`. These tests were FAIL or NOT_RUN only because those archives were missing:

FAIL at link time: `fastmath-ubsan`, `freeplay-rate-pads`, `smc-ui-bounds`, `original-stale-release` (UBSan), and `chicago-fx2-inserts`, `original-sound-cell`, `engine-808-scratch-alias`, `ble-device-setting-flow`, `ble-toggle-persistence-gate`, `original-release-integration` (ASan). `original-release-integration` left an empty suite log; the linker line is in `build/ui-theme/original-integration/sanitized.compile.log`.

NOT_RUN from `asan_links` in `tests/run_tests.sh` once that same link failed: `ble-midi-receive`, `ble-capture-replay`, `ble-role-switch`, `ui-303-entry`.

`part-off` is not in this set. `drum909` and `drum808` are not-run because the reference trees are absent.

The first `sudo apt-get install -y libclang-rt-18-dev` exited 100: `E: Package 'libclang-rt-18-dev' has no installation candidate`. The local package lists were stale. `sudo apt-get update` then the same install exited 0 and installed `1:18.1.3-1ubuntu1` from `http://archive.ubuntu.com/ubuntu noble-updates/universe`. `find / -name 'libclang_rt.asan*'` then listed `/usr/lib/llvm-18/lib/clang/18/lib/linux/libclang_rt.asan-x86_64.a` and `libclang_rt.asan_static-x86_64.a`. The UBSan archive `libclang_rt.ubsan_standalone-x86_64.a` is in that same directory. Nothing from apt was committed.

`tests/run_tests.sh` accepts names on the command line and through `X0X_TESTS` (lines 8 and 47). The re-run used only the 14 names above:

`sh tests/run_tests.sh fastmath-ubsan original-release-integration chicago-fx2-inserts original-sound-cell freeplay-rate-pads engine-808-scratch-alias ble-device-setting-flow ble-toggle-persistence-gate smc-ui-bounds original-stale-release ble-midi-receive ble-capture-replay ble-role-switch ui-303-entry`

It started after the install, ran 128 seconds, and exited 0. The last line is `all tests passed`.

### sanitizer re-run (compiler-rt installed)

| Test | First full suite | Sanitizer re-run |
| --- | --- | --- |
| `fastmath-ubsan` | FAIL | ok |
| `original-release-integration` | FAIL | ok |
| `chicago-fx2-inserts` | FAIL | ok |
| `original-sound-cell` | FAIL | ok |
| `freeplay-rate-pads` | FAIL | ok |
| `engine-808-scratch-alias` | FAIL | ok |
| `ble-device-setting-flow` | FAIL | ok |
| `ble-toggle-persistence-gate` | FAIL | ok |
| `smc-ui-bounds` | FAIL | ok |
| `original-stale-release` | FAIL | ok |
| `ble-midi-receive` | NOT_RUN (ASan runtime) | ok |
| `ble-capture-replay` | NOT_RUN (ASan runtime) | ok |
| `ble-role-switch` | NOT_RUN (ASan runtime) | ok |
| `ui-303-entry` | NOT_RUN (ASan) | ok |

Sanitizer re-run totals, this subset only: ok 14, fail 0, not-run 0. These numbers are not added to the full-suite totals of ok 375, fail 28, not-run 6.

## part-off web e2e: false-green check

Node is installed: `node --version` is `v22.23.4` at `/exec-daemon/node`.

`tests/run_tests.sh` calls node for these tests only when `command -v node` succeeds, so a missing node omits them instead of marking them ok: `installer-f02-bind-web`, `ota-request-guards-web`, `installer-six-stage-web`, `break-pack-web`, `breaks-web`, `breaks-page`, `project-adoption-identities`, `projects-web`, `installer-project-handoff`, `projects-page`, `install-slots-web`, `installer-backup-selection` (`tests/run_tests.sh:531` and the same guard through `:619`). `offline-sequencer` is started as `node` with no guard (`tests/run_tests.sh:610`). `tests/host/bass_tagged_storage_test.sh:26` also calls node from inside a host test. `part-off-web-e2e` is always started (`tests/run_tests.sh:446`).

The web script does not start a browser. It compiles a host exporter and runs `node tests/host/part_off_web_e2e.mjs`.

False green when node is missing: yes. `tests/host/run_part_off_web_e2e.sh:5` says "No node: NOT_RUN (exit 0)." Line 8 is:

`command -v node >/dev/null 2>&1 || { echo "  NOT_RUN part_off_web_e2e (node not found)"; exit 0; }`

Observed with node removed from `PATH`: stdout `  NOT_RUN part_off_web_e2e (node not found)`, exit 0. The same line and exit 0 occur if `PART_OFF_BASE_REF=4b240bd33a0eb7008b44e12bc270aba4d32b82c1` is also set, because the node check returns first and the script never reads that variable.

`tests/run_tests.sh` counts any exit 0 as ok. Serial path `tests/run_tests.sh:239-241` prints `ok` when the command returns true. Parallel path `tests/run_tests.sh:119-120` writes `ok` the same way. Observed: `env PATH=<no node> X0X_TEST_JOBS=1 sh tests/run_tests.sh part-off-web-e2e` printed `ok   part-off-web-e2e` and `all tests passed`, exit 0, while `build/host/part-off-web-e2e.log` contained the NOT_RUN line.

With node present, the same script on this checkout exited 0 in 12 seconds and printed five PASS lines and `part_off_web_e2e: ok (0 failing cases)`. It does not print NOT_RUN in that case.

Base ref `4b240bd33a0eb7008b44e12bc270aba4d32b82c1`: `git cat-file` in this clone fails. An unauthenticated GitHub API GET returned HTTP 422 and `No commit found for SHA`. `run_part_off_web_e2e.sh` does not consult `PART_OFF_BASE_REF`. With node present and that variable set, the script still ran the real checks and exited 0. It did not error and it did not skip. The variable is ignored, so a missing base is not a failure of this script.

The sibling `tests/host/run_part_off.sh` does consult the ref. `tests/run_tests.sh:445` sets `PART_OFF_REQUIRE_BASE=1` and that base. `tests/host/run_part_off.sh:29-33` then prints FAIL and exits 1. Without `PART_OFF_REQUIRE_BASE`, lines 35-36 print NOT_RUN and clear the variable, and line 73 can print another NOT_RUN while the script still exits with the test's own status, which can be 0. That path is not how the suite calls it. The suite's `part-off` result on this VM was FAIL, exit path 1, because the ref cannot be resolved.

`prints_not_run_with_exit_0` is true for `run_part_off_web_e2e.sh` when node is absent.

## Packaging and --check

`releases/openbeta/ledger.json` has an empty `allocations` list. It was not edited. `release_alloc.py` was not run. Release records were not touched.

`tools/build.py` accepts `--nightly-version`. `tests/host/test_nightly_version.py` shows that flag keeps the `model_900` identity and sets the about version string. Elite and Pro were rebuilt with:

`./build.sh --board smk37 --nightly-version 0.0.25 --git-sha 35b729914792d947c675d29cb9bd80ac7d65d704`

and `X0X_DEV_PADCAL=0`. Identities stayed `SMK-37 Elite_900` and `SMK-37 Pro_900`. Those rebuilds are packaging inputs. They are not the evidence output hashes in the table above.

`tools/package_nightly.py` has no `--nightly-version` flag. The exact error is `unrecognized arguments: --nightly-version 0.0.25` (exit 2). It requires `--firmware-version`. Create and `--check` were run with `--firmware-version 0.0.25`, the controller sha256 and size of `ctl.img`, and outputs under `/tmp/x0x-evidence-35b72991/nightly-packages` outside the repo. Because the product identity stays `*_900`, `tools/package_nightly.py` treats the package version as absent and returns before the ledger allocation check (`_embedded_version` and `_require_allocated_version`). No ledger number was demanded, and none of these steps is NOT_RUN for lack of a ledger number.

| Step | Flag | rc | Result |
| --- | --- | --- | --- |
| Elite and Pro `./build.sh` for packaging | `--nightly-version 0.0.25` | 0 | identity `*_900` |
| `package_nightly.py` create, elite and pro | `--firmware-version 0.0.25` (no `--nightly-version`) | 0 | local packages |
| `package_nightly.py --check`, elite and pro | `--firmware-version 0.0.25` | 0 | same receipts |

Local package bytes, not committed:

| Package | Size | sha256 |
| --- | --- | --- |
| elite `0.0.25` complete | 651570 | `22368e6f2ab5e6ab0c086e2fdac7fb9292671f59b8416f3409a5a57cbfad2dee` |
| pro `0.0.25` complete | 651566 | `3f352a0e8a48b0b105c98f1da7dfa63abbaa864dde41ca090c1351b3cf99f5e3` |

The receipt field `release_display` is `0.0.25_openbeta`. That string is the tool's display suffix. It is not a ledger allocation and not a publish.

## NOT_RUN

| Item | Reason |
| --- | --- |
| `tools/gate.sh` | This grant is an unreviewed evidence build. The gate was not requested. |
| `tools/dev_validate.py` | The tool forces `X0X_DEV_PADCAL=1` and writes DEV CHECKED. This grant does not include that validation claim. QA already marked the candidate needs changes. Not run. The tool has no separate authorization prompt. |
| `tools/publish_nightly.py` | Requires Elite and Pro DEV CHECKED records, which were not produced. No `--push` and no `--remote`. |
| `tools/release_alloc.py` and `releases/openbeta/ledger.json` | The coordinator said there is no open-beta allocation. The ledger was not edited. |
| Reference DSP trees `schwung-9W9` and `schwung-8W8` | Not present. Dependent host tests are the not-run lines above. |
| ASan and UBSan compiler-rt during the full suite | The archives were absent for that run. The four ASan-gated tests are not-run in the primary totals, and ten other tests failed at link time. A later public install of `libclang-rt-18-dev` is a separate re-run, not a change to those totals. |

No package or check step was skipped for lack of a ledger number.

## What this commit contains

Docs only: this README, `commands.txt`, trimmed logs, and `docs/handoff/build/manifest-35b72991.json`. `git show --stat` is the check that no binary was committed.
