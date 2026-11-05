# UNPUBLISHED BUILD EVIDENCE for 8be16486 only; not a qualification; no publish/main/catalog/NIGHTLY claim
Native evidence build of 8be16486, not release qualification. Cloud box only. Nothing was pushed, committed, published, or flashed.
Date: 2026-11-05 19:08 (Asia/Vladivostok, UTC+10)

## Source
- Commit 8be16486e33f65b31c76710e1d825b6aa701c2c9 (build/0.0.24.5), checked out detached. Clone used --reference /workspace/repo.
- Tree f952ab4455723e4a1a68fdb3a8eea22d2da59710. `git status --porcelain` was empty after the build (logs/11-status.log). The commit/tree check and the pre-build status ran in the console but are NOT in commands.txt. A fresh worktree at the same SHA was logged later (logs/24-postcheck.log): HEAD 8be16486…, tree f952ab44…, 0 status lines.
- Preflight: `ps aux` showed no clang/build.py/build.sh running. This preflight was UNLOGGED, and GitHub Actions was not checked. A later ps check is logged in 24-postcheck.log (0 matches).

## Inputs
- Toolchain tar.xz sha256 a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd (matches). JIELI_TOOLCHAIN=/workspace/build-inputs/jieli/toolchain (has pi32v2/, common/). clang version 4.0.1.
- AC79 SDK HEAD d179b4484759423312073f5fbb232501aa491047, SDK working tree clean.
- SDK_SHA256 pins: uboot.boot 4e3b4c22…, cfg_tool.bin 27657995…, cfg/eq_cfg_hw.bin 41167491… (all 3 match).
- Env: JIELI_TOOLCHAIN, AC79_SDK (paths above), JIELI_DOCKER=0, X0X_DEV_PADCAL=0, CTL_NO_CACHE=1. These were NOT recorded in commands.txt at build time; they were logged afterwards in 24-postcheck.log. X0X_DEV_PADCAL=0 is the release configuration. BUILDING.md:5 and :383 say builds default to dev (PADCAL=1). PADCAL=0 was deliberate and requested by the coordinator (Grok Bot) under the owner's delegation. Pillow 13.0.0 and /usr/bin/cc were already installed.

## Steps (every exit code was 0)
| # | Command | rc | time |
|---|---|---|---|
|04| python3 tools/check_locked_settings.py | 0 PASS (20 settings ok) | 0s |
|05| MODEL=elite ./build.sh --board smk37 | 0 PASS | 6s |
|06| MODEL=pro ./build.sh --board smk37 | 0 PASS | 5s |
|07| SDK_LOCAL=$AC79_SDK CTL_NATIVE=1 sh ble/ctl/build_ctl.sh | 0 PASS | 3s |
|08| add_ctl.py build/x0x-smk37.fwsc ctl.img -o x0x-smk37-ble.fwsc | 0 PASS | 0s |
|09| add_ctl.py build/x0x-smk37-elite.fwsc ctl.img -o x0x-smk37-elite-ble.fwsc | 0 PASS | 1s |
|10| add_ctl.py ble/ctl/out/ctl.img --check | 0 PASS (ok) | 0s |
- Total build time was about 15s. The font, drums, and break bank were generated in the Elite run and reused from cache in the Pro run.
- NOT_RUN: `--git-sha`. build.py rejects it unless `--nightly-version` is also given, and no nightly flag was passed. The build used the default `git rev-parse HEAD`.
- NOT_RUN: `dev_validate.py`. It needs a `--plan` file that was not provided. Also NOT_RUN: tests/gate (outside this scope).
- No "SDK differs" warning, no "warning" lines, and no "FAIL" lines in any build log.

## Headroom (same for both models)
- App code area 552672 B, 40904 B free. Image 511768 B.
- RAM .data+.bss 97312 of 98304 B (992 B free). Pool 335736 of 344064 B (8328 B free).
- Loader ota.bin: Elite 7454 B, Pro 7450 B.
- ctl.img 61552 of 65536 B, crc32 564a8f55, 3984 B flash free. ctl RAM 26336 of 26368 B (32 B free). LTO undefined symbols: 0.

## Outputs (/workspace/build-0245/out, box only)
| file | bytes | sha256 |
|---|---|---|
| x0x-smk37-elite.fwsc (SMK-37 Elite_900) | 651570 | 22af1dec2ec846f2df93680e79447f85ad908698b9f644639f7b932223c30585 |
| x0x-smk37.fwsc (SMK-37 Pro_900) | 651566 | 22412cdd67d852d116fb8fc4a4ea025d931da18555da7be89844e7d07804b2da |
| x0x-smk37-elite-ble.fwsc | 651570 | 66c6a09874ab7e444c6cc957c121a9a232bd3944cd9f4eb49f0a22694e9caae9 |
| x0x-smk37-ble.fwsc | 651566 | 2e9334a1693b5473e7936469f98f49b7171fdb8a80ea26a2a23ffa23424f51d8 |
| ctl.img | 61552 | a334208b51f62bc7231ebb76a6de0dfa260033d7f7316e5b92053c540bc1d984 |
- The -ble packages are the same size as the base packages. The repo has no reference digests, so these cannot be compared.

## Housekeeping
- The source clone was deleted afterwards. out/, logs/ and REPORT.md were kept.
- Inodes on / are at 60% (840368 free).
- Logs are in logs/ (commands.txt plus one log per step).

## Follow-up (2026-11-05 19:12 Asia/Vladivostok, UTC+10): is BLE per model or shared?
**Answer: shared.** One ctl.img is built per board, and the same image goes into both the Elite and the Pro package. What is model-specific is the package that carries it.

Evidence from the code at 8be16486:
- `ble/ctl/build_ctl.sh` reads no MODEL or BOARD variable. The only X0X_ variable it reads is X0X_JOBS, which sets the number of parallel compile jobs (build_ctl.sh:15, :30). `grep -rn "MODEL|BOARD" ble/ctl/` finds nothing.
- `BUILDING.md:469-471` gives one `build_ctl.sh` command and one `ctl.img`, then runs `add_ctl.py <package> ctl.img`.
- `tools/add_ctl.py:3-16` says it adds a controller image to an SMK-37 package. Its only board constant is the SMK layout CODE_MAX at add_ctl.py:32; nothing in it depends on the model.
- `docs/bundled-nightly-packages.md:1-23` and `tools/package_nightly.py:3, :27` describe a "model-specific NIGHTLY package with its matching controller". There the model check (`--model`, the `SMK-37 Elite_900`/`Pro_900` identity) applies to the package. The controller is pinned only by an explicit SHA256 and size, with no model field. In this code, "matching" means the reviewed controller hash, not a per-model controller.

Tests (box only):
- I rebuilt ctl.img from a temporary detached worktree at the same SHA (tree f952ab44…), first with MODEL=elite and then with MODEL=pro. Both runs exited 0 and both gave sha256 a334208b51f62bc7231ebb76a6de0dfa260033d7f7316e5b92053c540bc1d984, identical to the original. So the build output does not depend on MODEL and is reproducible.
- Embedding round trip, for both -ble packages: `add_ctl.py X-ble.fwsc --remove` gave back a package byte-identical to the base .fwsc. Adding out/ctl.img to that gave back a package byte-identical to X-ble.fwsc. All steps exited 0. So both -ble packages embed exactly ctl.img a334208b…, and nothing else differs. The image does not appear as plain bytes inside the .fwsc, because the package format transforms it.
- FAIL (rc=1): `add_ctl.py <pkg>-ble.fwsc --check`. I used the tool wrongly: `--check` only takes a ctl.img and printed "larger than the 64 KiB Bluetooth block". The image check `add_ctl.py ctl.img --check` passed earlier (rc=0, "ok").
- NOT_RUN: `package_nightly.py <pkg> --check`. It exits 2 because it requires `--firmware-version` from the NIGHTLY ledger, and this build is unversioned and not a NIGHTLY, so I did not supply one.
- No variant was missing, so no new output files. The output list is unchanged (5 files, listed above).
- The temporary worktree was removed and pruned. Inodes on / were at 44% before and after.

## Validation run (2026-11-05 19:19–19:40 Asia/Vladivostok, UTC+10). Native evidence build of 8be16486, not release qualification.
- Preflight was logged this time (logs/30-preflight.log). `ps aux` found 0 native build/validate processes. Fresh worktree at HEAD 8be16486…, tree f952ab4455723e4a1a68fdb3a8eea22d2da59710, 0 status lines. Inodes on / were at 48% before and 44% after.
- Env recorded in each log: JIELI_TOOLCHAIN, AC79_SDK (paths above), JIELI_DOCKER=0, CTL_NO_CACHE=1. X0X_GATE_LOCK pointed to a scratch file under build-0245, which has since been deleted.

### 1. Unit tests: tests/run_tests.sh
- 31: `tests/run_tests.sh` FAIL rc=126, 0s. Harness issue: the file is tracked as mode 100644, not executable, so it cannot be run directly.
- 32: `sh tests/run_tests.sh` FAIL rc=1, 1000s ("TESTS FAILED"). Results: **365 ok, 17 FAIL, 2 not-run**. The 2 not-run are drum909 and drum808, because the ../schwung-9W9 and ../schwung-8W8 reference trees are absent.
- Environment issues (2):
  - offline-sequencer-browser: the Python module `playwright` is not installed.
  - offline-touchosc-lua: `lua5.1` was not found.
- Likely real defects, where test and source disagree at 8be16486 (13):
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
- Cause not captured (2): original-release-ui and ui-theme wrote empty logs and need a rerun with xtrace to diagnose.
- The per-test logs were in the scratch worktree and were deleted with it. The causes above were taken from them before deletion, and the summary is in logs/32-run_tests-sh.log.
- Suggested fixes (not applied):
  - Mark run_tests.sh executable, or document `sh tests/run_tests.sh`.
  - Install playwright and lua5.1 for full coverage.
  - Update the 13 drifted tests or source to match.
  - Rerun original-release-ui and ui-theme with xtrace.

### 2. dev_validate.py (plan authored by Grok Bot executor)
- Plan: plan/dev-plan-8be16486.json, sha256 d813efe2eff4cd7d7acf8d0f0ff80a525a2868293014541282d92253d79d6868.
  - Authored from BUILDING.md:95-105 (format and the example check) and the schema in tools/dev_validate.py:66-79.
  - base = the commit itself. One check: `sh tests/host/test_install_gate_guard.sh`.
- PADCAL: dev_validate.py:150 forces X0X_DEV_PADCAL=1 (dev config), so the requested PADCAL=0 is not supported by this tool. These results are dev config, unlike the earlier release-config build.
- It does not flash: dev_validate.py:15 says "never writes device state". The install step (fm1_install.py) was NOT_RUN by policy.
- 33 elite: PASS rc=0, 14s, "DEV CHECKED". Budgets: code_free 39000, ram_free 560, pool_free 8328.
- 33 pro: PASS rc=0, 13s, "DEV CHECKED". Same budgets.
- Outputs are in out/dev-validate/:
  - dv-elite/x0x-smk37-elite.fwsc bed4ead88039d6e52fe01d7e09d02a619e8704f7beedc4186984ee18202deffb
  - dv-pro/x0x-smk37.fwsc ef38ad44e8659be3b475bac584df2c7fc3edcb4d8c9b5832ff8a3bef899b8760
  - ctl.img a334208b… (same image for both)
  - record.json: elite 8b03e3f1…, pro e031ddbd…
  - The records' "tree" field is 0a6296c2… (the project's ls-tree key, not the git tree).
- DEV CHECKED is local dev evidence, not a release PASS (BUILDING.md).

### 3. tools/gate.sh: NOT_RUN
- BUILDING.md:95 and :278 say "Only after an explicit user request for a release build may the full gate run".
- The gate requires `--purpose release-install|release-package|owner-validation` with a reason (gate.sh:3-7). No such purpose was authorized.
- Further blockers:
  - The commit would need `--trial`, since it is not on X0X_INTEGRATION.
  - The pinned reference folders (/workspace/x0x37-refs, X0X_REF_PINS, gate.sh:24, :65) are absent.
- The gate itself touches no device and needs no secrets. The blocker is authorization plus the missing reference inputs.

### 4. Headroom against documented limits (earlier release build PADCAL=0; dev numbers in brackets)
| Item | Ours | Limit (source) | Verdict |
|---|---|---|---|
| App code area | 511768 of 552672 B, 40904 free [dev 39000] | image <= APP_CODE_MAX (build.py:518-519; gate.sh:440 free >= 0) | within |
| RAM .data+.bss | 97312 of 98304, 992 free [dev 560] | error if bss > 96*1024 = 98304 (build.py:536); gate.sh:441 free >= 0; dev_validate.py:59 | within, no margin minimum documented |
| Pool | 335736 of 344064, 8328 free | headroom >= 8192 (build.py:538; gate.sh:442; dev_validate.py:59) | within, by 136 B |
| BLE ctl.img flash | 61552 of 65536, 3984 free | <= BLE_SIZE 0x10000 (add_ctl.py:33, :49) | within |
| BLE RAM | 0x1c01800..0x1c07ee0, 26336 of 26368, 32 free | inside 0x01C01800..0x01C07F00 (add_ctl.py:37, :66; mkimage.py:15) | within, 32 B |
| ota.bin loader | 7454 / 7450 B | none found in build.py (it only prints, :336) | no documented limit |
