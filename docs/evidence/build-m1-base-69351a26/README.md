# Native evidence build of M1 base 69351a26 (UNREVIEWED)

native evidence build of M1 base 69351a26 (UNREVIEWED), baseline for the M1 size delta, not release qualification.

This record is the unreviewed native size of base `69351a26a756bec914b81633636793f3df3ea827`. It is the baseline for the size of M1 `184d737d40749742980c07c30838f5b222f9551d` minus this base. It is an unreviewed evidence build. It is not release qualification and it is not a publish. The parent commit was made by a historical privaterelay address. This evidence commit does not rewrite that commit. No firmware, package, controller, toolchain, or SDK bytes are in this commit.

## Source

| Item | Value |
| --- | --- |
| Commit | `69351a26a756bec914b81633636793f3df3ea827` |
| Tree | `25f03ce94fa3997faa18ab47f6561b17703b6ad3` |
| Parent | `66073c68dd80d51a7053432c2c161a8b720a1341` |
| Working tree before the evidence commit | clean |
| M1 evidence used for the other side of the delta | `8af0d17b03d829041a8e34018c1ea522a4e1bd18` on `evidence/build-m1-184d737d-20261105` |

`tools/get_toolchain.sh`, `build.sh`, `BUILDING.md`, `tools/build.py`, `ble/ctl/build_ctl.sh`, and `tools/add_ctl.py` are the same blobs in this tree and in that M1 evidence tree. `git diff --stat` on those paths was empty.

## Inputs

Public downloads only. No authentication challenge. The toolchain response was HTTP 302, then HTTP 200. The SDK clone returned the expected commit with no 401 or 403.

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

The three SDK pins in `tools/build.py` are those hashes. Host packages installed before the firmware link: `libclang-rt-18-dev` 1:18.1.3-1ubuntu1 and `lua5.1` 5.1.5-9build2. Pillow 13.0.0 and Node 22.23.4 were already present.

Inodes on `/` before the downloads: 267989 used, 10545451 free. After the builds: 296397 used, 10517043 free.

## Environment

`JIELI_TOOLCHAIN=$HOME/.jieli/toolchain`, `AC79_SDK=$HOME/fw-AC79_AIoT_SDK`, `JIELI_DOCKER=0`, `X0X_DEV_PADCAL=0`, `CTL_NO_CACHE=1`. The controller build also set `SDK_LOCAL=$AC79_SDK` and `CTL_NATIVE=1`, and those two stayed set for `add_ctl.py`. `X0X_JOBS` was unset, so `tools/build.py` used its default of 4. `check_locked_settings.py` reported 20 settings ok (rc 0).

Command log with the environment captured at execution: `commands.txt`.

## Outputs

Sizes and sha256 of the unversioned `X0X_DEV_PADCAL=0` packages. These stayed on the build machine.

| File | Size | sha256 |
| --- | --- | --- |
| `build/x0x-smk37-elite.fwsc` | 651419 | `7032e5182713d4805fe4ee5ecf164cc27605ab6f68d6237b8379c683aa61f678` |
| `build/x0x-smk37.fwsc` | 651419 | `bd55903df769fa904abd22f1c287bfad3fa93ccc10dde517bd5c36c99d634bf6` |
| `build/x0x-smk37-elite-ble.fwsc` | 651419 | `ab52ef3bf39040a0833f89025c56713bb7830e081ed29c9b7edb877dfdfc1666` |
| `build/x0x-smk37-ble.fwsc` | 651419 | `66a2ad34d59e30b6ebf927857f5fe0da679925c5f6b881d00452bd5024c955af` |
| `build/smk37-elite/x0x.bin` | 335460 | `a45be5662b23eec879e986a89baa63dcea054dbbb195fc1c7b534f6ceca7c362` |
| `build/smk37/x0x.bin` | 335444 | `de7cc3a2e6f6681c6728540b73ad7b38c97c95a5c85fa15977985450e6fb22ff` |
| `ble/ctl/out/ctl.img` | 65492 | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

`add_ctl.py --check` on `ctl.img` returned rc 0 (`ble/ctl/out/ctl.img: ok`). Controller CRC32 is `77b7a496`.

`ctl.img` is byte-identical to the M1 image. Size 65492 and sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` match the M1 record, and the CRC32 `77b7a496` matches that record too.

## Headroom

Unversioned size build. App code area is 552672 B. RAM cap is 98304 B. Pool cap is 344064 B, and pool free must be at least 8192 B. BLE flash cap is 65536 B. BLE RAM cap is 26368 B. The controller image is one build shared by both models.

| Metric | Elite | Pro |
| --- | ---: | ---: |
| App code free | 217212 | 217228 |
| RAM used | 95560 | 95560 |
| RAM free | 2744 | 2744 |
| Pool used | 323000 | 323000 |
| Pool free | 21064 | 21064 |
| BLE flash used | 65492 | 65492 |
| BLE RAM used | 25824 | 25824 |

Pool free 21064 is above 8192. RAM used 95560 is within 98304 (2744 free). App code free is 217212 on Elite and 217228 on Pro. BLE flash is 65492 of 65536, which is 44 B free. BLE RAM is 25824 of 26368, which is 544 B free. The BLE flash figure is the same 44 B free reported for M1.

Elite link rc 0 in 5 s. Pro link rc 0 in 4 s. Controller link rc 0 in 1 s, 0 undefined symbols. Build lines are in `elite-build.log`, `pro-build.log`, and `ctl-build.log`.

## Delta against M1 184d737d

M1 numbers used here, Elite and Pro the same in that run: app free 216044, RAM 95576 used and 2728 free, pool 323000 used and 21064 free, BLE flash 65492, BLE RAM 25824, `x0x.bin` 336628, `ctl.img` 65492 B sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`, Elite fwsc sha256 `4fed26faa4b97461218715544b8f819d17cc57ed7da6d90c55b822283c6aaae4`, Pro fwsc sha256 `c0d286f95fa2083dfbacf3bb3d020431c8fdc959696873293da77515b8b20a1b`. Both M1 packages were 651419 B.

| Metric | Elite base − M1 | Elite M1 − base | Pro base − M1 | Pro M1 − base |
| --- | ---: | ---: | ---: | ---: |
| App code free | +1168 | −1168 | +1184 | −1184 |
| RAM used | −16 | +16 | −16 | +16 |
| RAM free | +16 | −16 | +16 | −16 |
| Pool used | 0 | 0 | 0 | 0 |
| Pool free | 0 | 0 | 0 | 0 |
| BLE flash used | 0 | 0 | 0 | 0 |
| BLE RAM used | 0 | 0 | 0 | 0 |
| `x0x.bin` bytes | −1168 | +1168 | −1184 | +1184 |
| fwsc bytes | 0 | 0 | 0 | 0 |
| `ctl.img` bytes | 0 | 0 | 0 | 0 |

The fwsc sha256 values differ. The sizes do not. `ctl.img` sha256 matches, so the controller bytes match.

## Symbol diff

`symbols-elite.txt` and `symbols-pro.txt` list the top 25 pool symbols and the top 25 RAM symbols, then the pool and RAM symbols from `ui.c`, `smc_mixer.c`, `engine.c`, `drum909.c`, `drum808.c`, and `sequencer.c`. Sizes come from the linked ELF (`objdump -t`). The object name is the `.o` whose symbol table defines that name. Pro matches Elite for every `.pool`, `.bss`, and `.data` symbol in name, section, size, address, and object. That comparison is the note at the top of `symbols-full-elite.txt`.

Compared with the names in the M1 `symbols-elite.txt`:

The top 25 pool names and the top 25 RAM names are the same names in the same order. All 25 compared `.pool` addresses match. Both compared `.data` addresses match. Of the compared `.bss` symbols, 11 addresses match and 48 differ. Where a `.bss` address differs, the M1 address minus this base address is 4, 8, or 16.

173 names appear in both symbol files. None of those names differ in size. Where both files give a section and an object, those match too.

Names present in the M1 list and absent from this base ELF:

| Name | M1 size | M1 section | M1 object |
| --- | ---: | --- | --- |
| `kbd_panic` | 2 | `.bss` | `x0x.o` |
| `smc_panic` | 2 | `.bss` | `x0x.o` |
| `smc_panic_held` | 4 | `.bss` | `x0x.o` |
| `smc_panic_mode_seen` | 4 | `.data` | `x0x.o` |
| `notes_off_req` | 1 | `.bss` | `app_engine.o` |

`engine.c` on this base has 104 pool/RAM symbols and 232308 B. The M1 list has 105 symbols and 232309 B. The extra M1 symbol is `notes_off_req`, 1 B. The omitted engine line is 65 symbols and 345 B on both sides.

`smc_mixer.c` on this base has 21 symbols and 882 B. The M1 list has 24 symbols and 892 B. The 10 B gap is `smc_panic` 2 + `smc_panic_held` 4 + `smc_panic_mode_seen` 4.

`ui.c` on this base has 25 symbols and 1832 B. The M1 list has 22 symbols and 1142 B. The M1 ui section includes `kbd_panic` (2 B) and does not name `main_rowh` (552 B), `main_colh` (120 B), `blit_hash` (16 B), or `undo_lanes` (4 B). `main_rowh` is in the M1 top 25 at 552 B, the same size as here. `blit_hash`, `main_colh`, and `undo_lanes` are file-scope variables in `firmware/src/app/ui.c` on this tree. The M1 symbol file does not name them, so this record has no M1 size for those three. 1142 + 552 + 120 + 16 + 4 − 2 = 1832.

`drum909.c` is 3 symbols and 92 B on both sides. `drum808.c` is 2 symbols and 8 B on both sides. `sequencer.c` has no pool, `.data`, or `.bss` symbol on either side.

`symbols-full-elite.txt` is every `.pool`, `.bss`, and `.data` symbol from the Elite ELF, 455 lines, sorted by name, with size, section, address, and object. Nineteen `.L_MergedGlobals*` lines are compiler containers. Their members are separate lines at overlapping addresses. Ten size-0 symbols are included: six linker labels (`_bss_start`, `_bss_end`, `_data_start`, `_data_end`, `_pool_start`, `_pool_end`) and four cpu1 stack labels.

## Not run

| Item | Reason |
| --- | --- |
| Host tests (`sh tests/run_tests.sh`) | NOT_RUN = not requested by the authorization |
| Packaging (`package_nightly.py`, versioned `--release` rebuild) | NOT_RUN = not requested |
| `tools/gate.sh` | release-only, not part of this authorization |
| `tools/dev_validate.py` | NOT_RUN = not requested by the authorization |
| `publish_nightly.py` | Not run. No `--push` and no `--remote` |
| `tools/release_alloc.py` and ledger edits | Not run |
| NIGHTLY catalog, gh-pages, `artifacts/nightly-dev` | Not touched |
| Device, publish, merge, force-push, `main` | Not done |

## What this commit contains

Documentation only: this file, `commands.txt`, the symbol lists, and the trimmed logs. `docs/handoff/build/manifest-m1-base-69351a26.json` is the same facts in JSON. The repo pattern `build/` ignores that path because of the directory name, so the JSON file was added with `git add -f`. It is text. No binaries are committed.
