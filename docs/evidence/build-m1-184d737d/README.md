# Native evidence build of M1 184d737d (UNREVIEWED)

native evidence build of M1 184d737d (UNREVIEWED), size estimate for M1 on its own base 69351a26, not after a port onto v4-fix, not release qualification.

This record is a size estimate for the panic-completion tree on its own base. It is an unreviewed evidence build. It is not release qualification, not a publish, not a freeze, not the 24.5 candidate, and not M1 after a port onto v4-fix `43de100c`. That rebase does not exist. No firmware, package, controller, toolchain, or SDK bytes are in this commit.

## Source

| Item | Value |
| --- | --- |
| Commit | `184d737d40749742980c07c30838f5b222f9551d` |
| Tree | `bfeefddce643d3591ffe7b2cea0c4e034b2b0719` |
| Parent | `b446995665bae681eba7b53913f47b962382e0b7` |
| Base | `69351a26a756bec914b81633636793f3df3ea827` (reachable) |
| Lane name | `lane/dev-m1-panic-complete-20261105` |
| Working tree before the evidence commit | clean |

`git diff --stat 69351a26 HEAD` at the code tip is 19 files, 1067 insertions, 3 deletions. The firmware edits are `engine.c`, `smc_mixer.c`, `ui.c`, `drum808`, `drum909`, and `sequencer`, plus host panic tests.

## Inputs

Public downloads only. No authentication challenge.

| Input | Expected | Observed |
| --- | --- | --- |
| Toolchain URL | `https://pkgman.jieliapp.com/s/linux-toolchain` | HTTP 200, final URL `https://jl-update.oss-cn-shenzhen.aliyuncs.com/jieli-linux-toolchains-20260730.1.tar.xz` |
| Toolchain archive sha256 | `a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd` | same |
| Toolchain archive size | 26027508 | 26027508 |
| `pi32v2/bin/clang` | clang version 4.0.1 | `clang version 4.0.1 () ()` |
| clang binary sha256 | `c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f` | same |
| AC79 SDK | `AC79NN_SDK_V1.2.1_2023-12-13` peeled to `d179b4484759423312073f5fbb232501aa491047` | same, `git clone --depth 1` |
| `uboot.boot` | `4e3b4c220dc96641cb5a723f41e68ce41d5261ae9434bb33fbd7f2c59976ded4` | same, 14536 B |
| `cfg_tool.bin` | `276579954f076886a6a7694f65dc71c034a63a2c204b76749065c0ac7b010d1b` | same, 383 B |
| `cfg/eq_cfg_hw.bin` | `41167491bffed4651750719c973d2758adeb9021a5670d02d6a53c85ed80ea7d` | same, 655 B |

The three SDK pins in `tools/build.py` match those hashes. Host packages installed before the build: `libclang-rt-18-dev` 1:18.1.3-1ubuntu1 and `lua5.1` 5.1.5-9build2. Pillow 13.0.0 and Node 22.23.4 were already present.

Inodes on `/` before the downloads: 277609 used, 10535831 free. After the builds and host tests: 303672 used, 10509768 free.

## Environment

`JIELI_TOOLCHAIN=$HOME/.jieli/toolchain`, `AC79_SDK=$HOME/fw-AC79_AIoT_SDK`, `JIELI_DOCKER=0`, `X0X_DEV_PADCAL=0`, `CTL_NO_CACHE=1`. Controller build also set `SDK_LOCAL=$AC79_SDK` and `CTL_NATIVE=1`. `X0X_JOBS` was unset, so the build used 4. `check_locked_settings.py` reported 20 settings ok (rc 0).

Command log with the environment captured at execution: `commands.txt`.

## Outputs

Sizes and sha256 of the unversioned `X0X_DEV_PADCAL=0` packages. These are the size-estimate artifacts. They stayed on the build machine.

| File | Size | sha256 |
| --- | --- | --- |
| `build/x0x-smk37-elite.fwsc` | 651419 | `4fed26faa4b97461218715544b8f819d17cc57ed7da6d90c55b822283c6aaae4` |
| `build/x0x-smk37.fwsc` | 651419 | `c0d286f95fa2083dfbacf3bb3d020431c8fdc959696873293da77515b8b20a1b` |
| `build/x0x-smk37-elite-ble.fwsc` | 651419 | `a6afd211766ba7be09186d03342e92f08bcf3fc3786fd899bd19c20278314fa0` |
| `build/x0x-smk37-ble.fwsc` | 651419 | `de59d67203cdaa68bf66f1284803620e170926c66adf68efc42d9107106500d6` |
| `build/smk37-elite/x0x.bin` | 336628 | `d5716baef60db4587f2da525e6ff4e925fa960f33499c16426b87abb5d66c0c1` |
| `build/smk37/x0x.bin` | 336628 | `9ed696e7482ac55ab2b67126c0263a168b1d6914647adc2ad0dc6e8f4b71240a` |
| `ble/ctl/out/ctl.img` | 65492 | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

Elite and Pro app images are the same length and different bytes. Elite and Pro packages are the same length and different bytes. `add_ctl.py --check` on `ctl.img` returned rc 0 (`ble/ctl/out/ctl.img: ok`). Controller CRC32 is `77b7a496`.

Compared with the v4 package prefixes supplied for this estimate: Elite `4fed26fa…` differs from `be4d608e…`. Pro `c0d286f9…` differs from `0773c61a…`. `ctl.img` `956e3e21…` (65492 B) differs from `a334208b…` (61552 B).

## Headroom

Unversioned size build. App code area is 552672 B. RAM cap is 98304 B. Pool cap is 344064 B, and pool free must be at least 8192 B. BLE flash cap is 65536 B. BLE RAM cap is 26368 B. The controller image is one build shared by both models. Delta is this build minus the v4 reference.

| Metric | Elite | Pro | v4 Elite | v4 Pro | Delta Elite | Delta Pro |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| App code free | 216044 | 216044 | 38664 | 38680 | +177380 | +177364 |
| RAM used | 95576 | 95576 | 97328 | 97328 | -1752 | -1752 |
| RAM free | 2728 | 2728 | 976 | 976 | +1752 | +1752 |
| Pool used | 323000 | 323000 | 335864 | 335864 | -12864 | -12864 |
| Pool free | 21064 | 21064 | 8200 | 8200 | +12864 | +12864 |
| BLE flash used | 65492 | 65492 | 61552 | 61552 | +3940 | +3940 |
| BLE RAM used | 25824 | 25824 | 26336 | 26336 | -512 | -512 |

Floor verdict: no floor is violated. Pool free 21064 is above 8192. RAM used 95576 is within 98304 (2728 free). App code free is 216044. BLE flash has 44 B free (65492/65536). BLE RAM has 544 B free (25824/26368).

The large app-free delta against v4 is the point of this estimate. The number is M1 on base `69351a26`, where the linked image is 336628 B. It is not a measurement of this source after a port onto v4-fix.

Elite link rc 0 in 4 s. Pro link rc 0 in 5 s. Controller link rc 0 in 1 s, 0 undefined symbols. Build lines are in `elite-build.log`, `pro-build.log`, and `ctl-build.log`.

`symbols-elite.txt` and `symbols-pro.txt` list the top 25 pool symbols and the top 25 RAM symbols, then the pool and RAM symbols from `ui.c`, `smc_mixer.c`, `engine.c`, `drum909.c`, `drum808.c`, and `sequencer.c`. Elite and Pro symbol sizes in those lists match. The largest pool objects are `dly_buf` 158400, `cv_px` 66240, and `project_stage` 36396. `d909` is 7612 and `d808` is 3628, both in `app_engine.o`. `sequencer.c` has no pool or RAM object of its own. Panic data in the lists: `kbd_panic` 2 B in `ui.c`, `smc_panic` 2 B, `smc_panic_held` 4 B, and `smc_panic_mode_seen` 4 B in `smc_mixer.c`.

## Host tests

`sh tests/run_tests.sh` (the script is mode 755; it was invoked with `sh`). Wall time 508 s. Exit 1. The 40 minute cap was not reached.

321 result lines: 286 ok, 30 FAIL, 5 not-run.

Panic tests on this tree:

| Test | Added on this M1 diff | Status |
| --- | --- | --- |
| panic-chord | already on the base | ok |
| panic-notes | yes | ok |
| panic-clear | yes | ok |
| panic-route | yes | ok |
| panic-mutants | yes | ok |

The five not-run lines are missing reference trees, so they are skips rather than FAILs: `drum909` (no schwung-9W9), `drum808` (no schwung-8W8), `bass303` (no schwung-303), `chicago-au-compare` (`CHICAGO_AU_REFS`), `miami-au-compare` (`MIAMI_AU_REFS`).

`offline-sequencer-browser` is not a test name on this tree, so Playwright was not required. `4b240bd3` is not an object in this clone. No test log asked for that commit. The `part_at` compile failures below are from host fragments that call a function defined `static` in `ui.c` on this tree.

### FAIL list

Every FAIL below is REAL. None is an environment-only miss of a compiler, sanitizer, browser, or reference checkout.

| Test | Class | Cause and first failing line |
| --- | --- | --- |
| project-save-preflight | REAL | `project_save()` did not return -7. `memory-full save changed 0 objects` |
| editor-ota-dispatch | REAL | undeclared `ED_MAINTENANCE_HELD` |
| project-stage | REAL | `project_stage_test.c:394: CRC-valid unsafe values rejected before commit: 0` |
| smc-buttons | REAL | undeclared `part_at` |
| ble-central-first-notification | REAL | undeclared `smc_ble_role_get` |
| smc-reachability | REAL | `smc_reachability_test.c:306: Assertion led_vel[2] == 127 failed` |
| smc-ui-negative | REAL | `RuntimeError: main-mix-ignores-channel: expected one production anchor, found 0` |
| smc-mapping | REAL | undeclared `part_at` |
| smc-drum-mutes | REAL | `smc_drum_mutes_test.c:146: Assertion seq.mute == MUTE_MASK[strip] failed` |
| smc-drum-rs | REAL | `FAIL pattern cue during held p-lock restores the original drum sound line 446` |
| smc-filter | REAL | undeclared `part_at` |
| smc-feedback | REAL | undeclared `part_at` |
| smk-clip-chord-display | REAL | undeclared `ui_boot_flash_busy` |
| padcal-flag | REAL | `FAIL tools/build.py: X0X_DEV_PADCAL=0 or 1 from the environment goes to the compiler` |
| smk-clip-knob | REAL | undeclared `ui_boot_flash_busy` |
| rec-held-duration | REAL | undeclared `unlink` under `-Werror` |
| rec-live | REAL | `rec_live_test.c:260: 303 A preview ended on part change` |
| rec-live-smk | REAL | same first line as rec-live |
| scenario-getting_started | REAL | `FAIL expect sound2.0.0 == 58, got 1` |
| scenario-patterns | REAL | `FAIL expect hit0.1.4 == 1, got 0` |
| scenario-perf | REAL | `FAIL expect sound2.0.0 == 48, got 0` |
| scenario-plock | REAL | `FAIL expect view == 1, got 8` |
| scenario-song | REAL | `FAIL expect sound2.0.0 == 48, got 0` |
| scenario-sound_matrix_chicago | REAL | `FAIL expect page == 6, got 0` |
| scenario-sound_matrix_chicago_idle | REAL | `FAIL expect page == 6, got 0` |
| scenario-sound_matrix_lock | REAL | `FAIL expect sound2.0.19 == 51, got 0` |
| scenario-sound_matrix_miami | REAL | `FAIL expect page == 6, got 0` |
| scenario-steps64 | REAL | `FAIL expect hit0.0.2 == 1, got 0` |
| scenario-undo | REAL | `FAIL expect sound0.0.0 == 39, got 34` |
| perf-scratch | REAL | `FAIL expect sound2.0.0 == 48, got 0` |

### Difference from the v4-fix 43de100c list of 15 FAILs

This base is `69351a26`, so a different fail set is expected.

Still FAIL: `scenario-plock`.

Pass on this tree (those names are not failures here): `smc-select-mode`, `smc-solo-overflow`, `smc-ui`, `ui-feedback`, `sound-matrix`.

Not in this suite, so they are not failures and not passes here: `original-release-ui`, `original-build`, `ui-theme`, `smc-boot-saved-roles`, `smc-multi-peer`, `ui-peer-names`, `builtin-break-output`, `offline-sequencer-browser`, `ui-glyph-polish`.

New FAIL names relative to that list of 15: the 29 names in the table above other than `scenario-plock`.

## Package check

Host-only, scratch worktree `/tmp/m1-pkgbuild`, same commit and tree. Packages were not committed.

`./build.sh --board smk37 --nightly-version 0.0.25 --git-sha 184d737d…` cannot run on this tree. `tools/build.py` accepts `--release`, `--board`, `--model`, and `--sdk`. rc 2: `unrecognized arguments: --nightly-version 0.0.25 --git-sha …`.

The version stamp this tree does accept was run in the scratch worktree only: `MODEL=elite ./build.sh --board smk37 --release 0.0.25` and the same for Pro. Both rc 0. Identities `SMK-37 Elite_9000025` and `SMK-37 Pro_9000025`. Those packages are a version string on top of the same configuration. They are not the size-estimate hashes above. Elite versioned package sha256 `de086428fc47c874fdded61a50c1b7b175602330cc952a16867a29a09da92c1f`, 651419 B, app free 215916. Pro versioned package sha256 `4c41087c9d977ec3889f6fb7f5cd38afbf5ba2995a0a154ac76b62d0e1f033b5`, 651419 B, app free 215932. RAM and pool match the unversioned build.

`python3 tools/package_nightly.py` create and `--check` were run with `--firmware-version 0.0.25` and the controller sha256/size from `ctl.img`. All four commands returned rc 1: `package_nightly: firmware version is not allocated`. `releases/openbeta/ledger.json` has an empty `allocations` array. No bundled package was written. `release_alloc.py` was not run. The ledger was not edited.

## Not run

| Item | Reason |
| --- | --- |
| `./build.sh --nightly-version --git-sha` | `build.py` has no such arguments (rc 2). The scratch `--release 0.0.25` rebuild is recorded above. |
| Bundled nightly package | `package_nightly.py` refused 0.0.25 because the version is not allocated. |
| `publish_nightly.py` | Not run. No `--push` and no `--remote`. |
| `tools/gate.sh`, `tools/dev_validate.py` | Not run. |
| `tools/release_alloc.py` and ledger edits | Not run. |
| NIGHTLY catalog, gh-pages, `artifacts/nightly-dev` | Not touched. |
| Reference-engine compares | not-run, trees absent (see host tests). |
| `offline-sequencer-browser` | Test name is absent on this tree. |
| Device, publish, merge, force-push, `main` | Not done. |

## What this commit contains

Documentation only: this file, `commands.txt`, the symbol lists, and the trimmed logs. `docs/handoff/build/manifest-m1-184d737d.json` is the same facts in JSON. No binaries are committed.
