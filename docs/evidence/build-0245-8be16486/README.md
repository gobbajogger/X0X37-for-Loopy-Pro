Native evidence build of 8be16486, not release qualification. Unpublished build evidence only. No publish, no main, no catalog, no NIGHTLY claim.

This folder is a text manifest for the native evidence build of build/0.0.24.5. The five build outputs, the dev_validate packages, the JieLi toolchain, and the AC79 SDK are not in git. They stay on the build box only.

Copied beside this file, unchanged: `REPORT.md`, `commands.txt`, `40-digests.txt`, and `dev-plan-8be16486.json`. Tables below are taken from `REPORT.md`. No size, hash, exit code, or verdict is changed. Where `REPORT.md` abbreviates a hash, this manifest keeps that abbreviation. The three SDK pin values are the full `SDK_SHA256` strings in `tools/build.py` at this commit; `REPORT.md` records the same pins by prefix and says all 3 match.

## Source

| Item | Value |
| --- | --- |
| Commit | 8be16486e33f65b31c76710e1d825b6aa701c2c9 (build/0.0.24.5), checked out detached |
| Tree | f952ab4455723e4a1a68fdb3a8eea22d2da59710 |
| Date | 2026-11-05 19:08 (Asia/Vladivostok, UTC+10) |
| Worktree after the build | `git status --porcelain` empty (`logs/11-status.log`) |
| Later fresh worktree | HEAD 8be16486…, tree f952ab44…, 0 status lines (`logs/24-postcheck.log`). The validation worktree recorded the full tree f952ab4455723e4a1a68fdb3a8eea22d2da59710 and 0 status lines |

`REPORT.md` says the commit/tree check and the pre-build status ran in the console but are not in `commands.txt`.

## Toolchain and SDK

| Item | Value |
| --- | --- |
| Toolchain | JieLi clang 4.0.1 for pi32v2. `REPORT.md` records clang version 4.0.1. `JIELI_TOOLCHAIN=/workspace/build-inputs/jieli/toolchain` (has `pi32v2/`, `common/`) |
| Toolchain source | `tools/get_toolchain.sh` |
| Toolchain tarball sha256 | a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd (matches). `commands.txt` hashes `/workspace/build-inputs/jieli-linux-toolchains-20260730.1.tar.xz` |
| SDK | AC79 V1.2.1, commit d179b4484759423312073f5fbb232501aa491047. `REPORT.md`: AC79 SDK HEAD that commit, SDK working tree clean |
| SDK pin `uboot.boot` | 4e3b4c220dc96641cb5a723f41e68ce41d5261ae9434bb33fbd7f2c59976ded4. `REPORT.md` writes 4e3b4c22… and says it matches |
| SDK pin `cfg_tool.bin` | 276579954f076886a6a7694f65dc71c034a63a2c204b76749065c0ac7b010d1b. `REPORT.md` writes 27657995… and says it matches |
| SDK pin `cfg/eq_cfg_hw.bin` | 41167491bffed4651750719c973d2758adeb9021a5670d02d6a53c85ed80ea7d. `REPORT.md` writes 41167491… and says it matches |

Build environment recorded in `REPORT.md`: `JIELI_TOOLCHAIN`, `AC79_SDK`, `JIELI_DOCKER=0`, `X0X_DEV_PADCAL=0`, `CTL_NO_CACHE=1`. Those variables were not in `commands.txt` at build time; they were logged afterwards in `24-postcheck.log`. The build used `BOARD=smk37`. `X0X_DEV_PADCAL=0` is the release configuration. `BUILDING.md:5` and `:383` say builds default to dev (`PADCAL=1`). `PADCAL=0` was deliberate and requested by the coordinator (Grok Bot) under the owner's delegation.

## Build outputs

Paths are under `/workspace/build-0245/out` on the build box. These bytes are not in this commit.

| file | bytes | sha256 |
| --- | --- | --- |
| x0x-smk37-elite.fwsc (SMK-37 Elite_900) | 651570 | 22af1dec2ec846f2df93680e79447f85ad908698b9f644639f7b932223c30585 |
| x0x-smk37.fwsc (SMK-37 Pro_900) | 651566 | 22412cdd67d852d116fb8fc4a4ea025d931da18555da7be89844e7d07804b2da |
| x0x-smk37-elite-ble.fwsc | 651570 | 66c6a09874ab7e444c6cc957c121a9a232bd3944cd9f4eb49f0a22694e9caae9 |
| x0x-smk37-ble.fwsc | 651566 | 2e9334a1693b5473e7936469f98f49b7171fdb8a80ea26a2a23ffa23424f51d8 |
| ctl.img | 61552 | a334208b51f62bc7231ebb76a6de0dfa260033d7f7316e5b92053c540bc1d984 |

`REPORT.md`: the -ble packages are the same size as the base packages. The repo has no reference digests, so these cannot be compared. No variant was missing, and the output list stayed these 5 files.

## Headroom

Same for both models on the `PADCAL=0` build:

- App code area 552672 B, 40904 B free. Image 511768 B.
- RAM .data+.bss 97312 of 98304 B (992 B free). Pool 335736 of 344064 B (8328 B free).
- Loader ota.bin: Elite 7454 B, Pro 7450 B.
- ctl.img 61552 of 65536 B, crc32 564a8f55, 3984 B flash free. ctl RAM 26336 of 26368 B (32 B free). LTO undefined symbols: 0.

Verdicts against documented limits. `REPORT.md` heading: earlier release build `PADCAL=0`; dev numbers in brackets.

| Item | Ours | Limit (source) | Verdict |
| --- | --- | --- | --- |
| App code area | 511768 of 552672 B, 40904 free [dev 39000] | image <= APP_CODE_MAX (build.py:518-519; gate.sh:440 free >= 0) | within |
| RAM .data+.bss | 97312 of 98304, 992 free [dev 560] | error if bss > 96*1024 = 98304 (build.py:536); gate.sh:441 free >= 0; dev_validate.py:59 | within, no margin minimum documented |
| Pool | 335736 of 344064, 8328 free | headroom >= 8192 (build.py:538; gate.sh:442; dev_validate.py:59) | within, by 136 B |
| BLE ctl.img flash | 61552 of 65536, 3984 free | <= BLE_SIZE 0x10000 (add_ctl.py:33, :49) | within |
| BLE RAM | 0x1c01800..0x1c07ee0, 26336 of 26368, 32 free | inside 0x01C01800..0x01C07F00 (add_ctl.py:37, :66; mkimage.py:15) | within, 32 B |
| ota.bin loader | 7454 / 7450 B | none found in build.py (it only prints, :336) | no documented limit |

## Step results

`REPORT.md` section "Steps (every exit code was 0)":

| # | Command | rc | time |
| --- | --- | --- | --- |
| 04 | python3 tools/check_locked_settings.py | 0 PASS (20 settings ok) | 0s |
| 05 | MODEL=elite ./build.sh --board smk37 | 0 PASS | 6s |
| 06 | MODEL=pro ./build.sh --board smk37 | 0 PASS | 5s |
| 07 | SDK_LOCAL=$AC79_SDK CTL_NATIVE=1 sh ble/ctl/build_ctl.sh | 0 PASS | 3s |
| 08 | add_ctl.py build/x0x-smk37.fwsc ctl.img -o x0x-smk37-ble.fwsc | 0 PASS | 0s |
| 09 | add_ctl.py build/x0x-smk37-elite.fwsc ctl.img -o x0x-smk37-elite-ble.fwsc | 0 PASS | 1s |
| 10 | add_ctl.py ble/ctl/out/ctl.img --check | 0 PASS (ok) | 0s |

Locked settings, both model builds, and add_ctl PASS. Total build time was about 15s. The font, drums, and break bank were generated in the Elite run and reused from cache in the Pro run. No "SDK differs" warning, no "warning" lines, and no "FAIL" lines in any build log.

Validation run (2026-11-05 19:19–19:40 Asia/Vladivostok, UTC+10):

| # | Command | rc | time |
| --- | --- | --- | --- |
| 31 | tests/run_tests.sh | 126 FAIL | 0s |
| 32 | sh tests/run_tests.sh | 1 FAIL ("TESTS FAILED") | 1000s |
| 33 | dev_validate.py run --model elite | 0 PASS ("DEV CHECKED") | 14s |
| 33 | dev_validate.py run --model pro | 0 PASS ("DEV CHECKED") | 13s |
| 34 | tools/gate.sh | NOT_RUN (not invoked) | |

### Unit tests

Step 31 FAIL rc=126: harness issue. `tests/run_tests.sh` is tracked as mode 100644, not executable, so it cannot be run directly.

Step 32 FAIL rc=1: **365 ok, 17 FAIL, 2 not-run**. The 2 not-run are drum909 and drum808, because the ../schwung-9W9 and ../schwung-8W8 reference trees are absent.

Environment issues (2):

- offline-sequencer-browser: the Python module `playwright` is not installed.
- offline-touchosc-lua: `lua5.1` was not found.

Likely real defects, where test and source disagree at 8be16486 (13):

- Compile errors because function signatures or struct members changed:
  - smc-solo-overflow: struct has no member `kind`/`action`.
  - smc-remainder: too few arguments to `smc_button_symbol`.
  - sound-matrix: `adsr_inline_page` is not declared, and `adsr_values` gets too few arguments.
  - ui-glyph-polish: `draw_adsr_inline`/`adsr_values` get too few arguments.
- Assertion failures:
  - smc-select-mode: smc_select_mode_test.c:61.
  - smc-multi-peer: smc_multi_peer_test.c:145.
  - ui-peer-names: ui_peer_names_test.c:15.
  - smc-ui: smc_ui_test.c:285.
  - ui-feedback: ui_feedback_test.c:98.
  - scenario-plock: expect lanes==1, got 2.
  - original-build: test_rate_pad_port_preserves_574f…
  - builtin-break-output: test_current_factory_set_is_preserved (web/factory_break_pack.js).
  - smc-boot-saved-roles: ValueError "substring not found" at test_smc_boot_saved_roles.py:37.

Cause not captured (2): original-release-ui and ui-theme wrote empty logs and need a rerun with xtrace to diagnose.

`REPORT.md` says the per-test logs were in the scratch worktree and were deleted with it. The causes above were taken from them before deletion. The summary is in `logs/32-run_tests-sh.log`. Suggested fixes in `REPORT.md` were not applied.

### dev_validate

Plan: `plan/dev-plan-8be16486.json`, sha256 d813efe2eff4cd7d7acf8d0f0ff80a525a2868293014541282d92253d79d6868. The copy in this folder is `dev-plan-8be16486.json` and has that sha256. Authored from `BUILDING.md:95-105` (format and the example check) and the schema in `tools/dev_validate.py:66-79`. base = the commit itself. One check: `sh tests/host/test_install_gate_guard.sh`.

`dev_validate.py:150` forces `X0X_DEV_PADCAL=1` (dev config), so the requested `PADCAL=0` is not supported by this tool. These results are dev config, unlike the earlier release-config build. It does not flash: `dev_validate.py:15` says "never writes device state". The install step (`fm1_install.py`) was NOT_RUN by policy.

- 33 elite: PASS rc=0, 14s, "DEV CHECKED". Budgets: code_free 39000, ram_free 560, pool_free 8328.
- 33 pro: PASS rc=0, 13s, "DEV CHECKED". Same budgets.

`DEV CHECKED` is local dev evidence, not a release PASS (`BUILDING.md`).

Outputs in `out/dev-validate/` (box only, not in git):

| file | sha256 as written in REPORT.md |
| --- | --- |
| dv-elite/x0x-smk37-elite.fwsc | bed4ead88039d6e52fe01d7e09d02a619e8704f7beedc4186984ee18202deffb |
| dv-pro/x0x-smk37.fwsc | ef38ad44e8659be3b475bac584df2c7fc3edcb4d8c9b5832ff8a3bef899b8760 |
| ctl.img | a334208b… (same image for both) |
| record.json elite | 8b03e3f1… |
| record.json pro | e031ddbd… |

The records' "tree" field is 0a6296c2… (the project's ls-tree key, not the git tree).

### tools/gate.sh

NOT_RUN. `commands.txt` line 34: `tools/gate.sh 8be16486... not invoked`.

- `BUILDING.md:95` and `:278` say "Only after an explicit user request for a release build may the full gate run".
- The gate requires `--purpose release-install|release-package|owner-validation` with a reason (`gate.sh:3-7`). No such purpose was authorized.
- The commit would need `--trial`, since it is not on `X0X_INTEGRATION`.
- The pinned reference folders (`/workspace/x0x37-refs`, `X0X_REF_PINS`, `gate.sh:24`, `:65`) are absent.
- The gate itself touches no device and needs no secrets. The blocker is authorization plus the missing reference inputs.

## Shared BLE ctl.img

One `ctl.img` is built per board, and the same image goes into both the Elite and the Pro package. What is model-specific is the package that carries it.

A rebuild from a temporary detached worktree at the same SHA (tree f952ab44…), first with `MODEL=elite` and then with `MODEL=pro`, both exited 0. Both gave sha256 a334208b51f62bc7231ebb76a6de0dfa260033d7f7316e5b92053c540bc1d984, identical to the original. The build output does not depend on `MODEL` and is reproducible.

Embedding round trip, for both -ble packages: `add_ctl.py X-ble.fwsc --remove` gave back a package byte-identical to the base `.fwsc`. Adding `out/ctl.img` to that gave back a package byte-identical to `X-ble.fwsc`. All steps exited 0. Both -ble packages embed exactly `ctl.img` a334208b…, and nothing else differs. The image does not appear as plain bytes inside the `.fwsc`, because the package format transforms it.

FAIL (rc=1): `add_ctl.py <pkg>-ble.fwsc --check`. `REPORT.md` says the tool was used wrongly: `--check` only takes a `ctl.img` and printed "larger than the 64 KiB Bluetooth block". The image check `add_ctl.py ctl.img --check` passed earlier (rc=0, "ok").

## NOT_RUN

| Item | As written in REPORT.md |
| --- | --- |
| `--git-sha` | NOT_RUN. `build.py` rejects it unless `--nightly-version` is also given, and no nightly flag was passed. The build used the default `git rev-parse HEAD`. |
| `dev_validate.py` on the first pass | NOT_RUN, because it needs a `--plan` file that was not provided. The validation run later supplied the plan. Elite and Pro then PASS, as recorded above. This item did not stay NOT_RUN. |
| tests/gate on the first pass | NOT_RUN (outside this scope). The validation section then records `tools/gate.sh` as NOT_RUN for the reasons in the gate section. |
| `package_nightly.py <pkg> --check` | NOT_RUN. It exits 2 because it requires `--firmware-version` from the NIGHTLY ledger, and this build is unversioned and not a NIGHTLY, so one was not supplied. `commands.txt` records three such invocations at rc=2. |
| `fm1_install.py` | NOT_RUN by policy. `dev_validate.py` does not flash. |
| `tools/gate.sh` | NOT_RUN. Not invoked. Reasons are in the gate section. |
| drum909 and drum808 | 2 not-run inside the unit-test run, because the ../schwung-9W9 and ../schwung-8W8 reference trees are absent. |
| GitHub Actions | Not checked. The first preflight was UNLOGGED. |

`40-digests.txt` is the attached digest list and is copied as written. It names six files: `logs/30-preflight.log`, `logs/31-run_tests.log`, `logs/32-run_tests-sh.log`, `logs/33-dev_validate-elite.log`, `logs/33-dev_validate-pro.log`, and `plan/dev-plan-8be16486.json`.
