# MBOOT no-USRTRIM variant/configuration investigation

## Result
**No qualifying existing SMK-37 variant or configuration was established. PLAN_ONLY HOLD remains.** This is a bounded source investigation, not proof that no private/unindexed variant exists. No production source was changed, no device was accessed, and no image was installed or published. This is a sanitized planning contribution; no upstream source files or host executables are publication assets.

The upstream current main and maintained project pin both resolve to `jvitkauskas/m-bootloader@64b74d009e6d21fe85ca60754b7c01d438214804`. API branch enumeration returned only main. The retrieved complete public source tree provided 48 text files, verified by Git object SHA before compiling. MIT license and NOTICE retained locally.

## Exact blocking behavior
- `src/boot.c@e896c9d6e4e3ee7229873b2c50bd66b7b2a73c73`: SMK build sets `SMK=true` via `BOARD_SMK` and startup unconditionally calls `core_guard(&nor, SMK, work)`.
- `src/core.c@7e473df3bb828876ba50701f86977ccaf654df6b`, lines788–825: valid stored TRIMCOPY plus invalid current marker reaches a **4096-byte erase at0xC1000**, then programs0x22c bytes. Normal first boot may create backup at0xFF000; a later corrupt-trim/error boot may restore USRTRIM. Pre-install byte equality cannot prove never-write behavior over the lifecycle.
- `Makefile@6fab54c9f0e034dd0ccdff0c9063276447438d18` has SMK versus FM-1 board selection, not a guard-disable flag. The actual config parser only recognizes SPI/clock/reset/SDRAM keys and exposes no USRTRIM-write-disable setting.
- Setting `SMK=false` is **not** a qualifying configuration: it selects the foreign FM-1 board behavior and changes protected wipe semantics as well as keys/display. Do not use that workaround. Corrupting/filling the backup guard is likewise not a supported configuration or owner-authorized write.

## Existing alternatives inspected
Upstream initial1.0 commit `93f7613b0c1f03b1d5bcc8fa0e274648467736d6` already has the same USRTRIM erase path. A public repository-name search identified the directly relevant fork `kmplngj/m-bootloader`; its only branch is main at `df15966a4bcf49b8a2d95ffd9c8395186de14314`. Exact fork `src/core.c@87c33ed4dc65a1b9851d052b81c144ab518b10bd` still restores USRTRIM and its boot source is identical to upstream current boot. Neither is a valid no-write selection. Historical installation commit claims of calibration preservation do not override the reachable restoration code.

The `/tags?per_page=100` API read returned unsupported-endpoint HTTP400 INVALID_ARGUMENT. This is a capability restriction, not a transient failure; not retried or routed around. Tags were not exhaustively inspected. Repository search was bounded and is not an exhaustive worldwide fork audit.

## Executed cloud tests
All commands ran in the isolated `upstream` text-source directory, using local host compilers only. No `make all`, `make test` (which depends on firmware artifacts), Git network, CI trigger, installer invocation or device operation ran.

1. `make build/core-test build/boot-test-smk37` — exit0.
2. `./build/core-test` — exit0, **6 synthetic groups passed**, including guard/NOR fault logic. Four private firmware fixture groups SKIPPED;0private images checked. The original guard regression explicitly asserts restoration and erase at0xC1000, so this passing test is evidence of incompatibility with the owner's no-write contract, not preservation compliance.
3. `./build/boot-test-smk37` — exit0, **3 scenarios/15checks/0failures**. Stock/Snoop37/BaudGirl fixture groups SKIPPED. Full boot lifecycle/guard fixture coverage is NOT_RUN without those inputs.
4. `cc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Iinclude src/core.c tests/core_test.c -o build/core-test-sanitize`; `ASAN_OPTIONS=detect_leaks=0 ./build/core-test-sanitize` — exits0, same6synthetic groups and skips.
5. `cc -std=c11 -g -Wall -Wextra -Werror -DPLATFORM_SIM -DBOOT_SIM -no-pie -Iinclude -O1 -fsanitize=address,undefined -DBOARD_SMK src/core.c src/contract.c src/platform.c src/board.c src/display.c tests/boot_sim.c tests/boot_nor.c tests/boot_test.c -o build/boot-test-smk37-sanitize`; `ASAN_OPTIONS=detect_leaks=0 ./build/boot-test-smk37-sanitize` — exits0, same3scenarios/15checks and skips. Leak detection disabled; no leak claim.
6. `PYTHONDONTWRITEBYTECODE=1 python3 check_usrtrim_contract.py` — exit0: exact source identity/static checks PASS; owner untouched-USRTRIM contract explicitly **FAIL_FOR_PINNED_SMK_VARIANT**, selection PLAN_ONLY_HOLD.

## Install-plan implication and precise next action
Do not name an installable no-USRTRIM payload/configuration: none is verified. Keep source selection unresolved and preserve existing full private backup, same-unit detection, authentic payload/hash/CRC, interruption-safe commit and recovery gates. No supported install/recovery command is established.

The existing MBOOT producer must supply either an authentic already-reviewed SMK variant with immutable source/build pins, or a separately admitted bounded source change that removes every USRTRIM program/erase path without changing SMK identity, BTIF, efuse, key_mac or wipe protections. That is **future implementation**, not performed or newly claimed here. Required acceptance includes mutation-spy tests over the entire protected region, healthy/corrupt/missing markers, valid/invalid/occupied backup, first/subsequent boot, errors/interruption and updater/wipe/installer paths; source selection must fail closed rather than silently repair trim. Review all writes, not only the startup guard. Hardware/native target/Mac qualification and actual install remain NOT_RUN.

These findings are included under the existing planning assignment. Same-reviewer affected-delta consumption precedes delivery; no new source owner or review budget is admitted.

## Immutable upstream references

- [Pinned source](https://github.com/jvitkauskas/m-bootloader/tree/64b74d009e6d21fe85ca60754b7c01d438214804)
- [Guard implementation](https://github.com/jvitkauskas/m-bootloader/blob/64b74d009e6d21fe85ca60754b7c01d438214804/src/core.c#L788-L825)
- [Startup caller](https://github.com/jvitkauskas/m-bootloader/blob/64b74d009e6d21fe85ca60754b7c01d438214804/src/boot.c)
