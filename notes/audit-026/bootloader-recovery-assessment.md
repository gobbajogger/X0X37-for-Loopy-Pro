# M-bootloader and update-recovery assessment

Date: 13 November 2026, America/Chicago.

Scope: research and planning only.
This work does no device write, no flash, no bootloader replacement, no build, and no host test.
Repo: `gobbajogger/X0X37-for-Loopy-Pro`.
Devices: M-Vave SMK-37 Elite and SMK-37 Pro.

Chip: JieLi AC79 (WL82).
SDK tag: `AC79NN_SDK_V1.2.14_2026-10-13`.
Source: `tools/build.py` at `94197fc839b00aff47b98ca02f3c6396e4a6eabf`.

Every claim has a label.
`[measured]` means this run measured it from Git objects or from a file on this computer.
`[sourced: path, commit, or URL]` means a source states it. This run did not measure it on a device.
`[hypothesis]` means a possible cause or effect. A test does not prove it yet.

Owner reports in this document use `[sourced: owner report relayed by Orchestrator]`.
This run did not measure those reports.

Hard rules for any later plan:

- Identify a device by hardware fields and backup fields. Never use a USB name.
- The FM-1 USB label does not prove a wrong model. The owner does not own an FM-1.
- Preserve BTIF, USRTRIM, efuse, `key_mac`, and all other protected areas.
- The bootloader is off limits, except through an approved M-bootloader install plan.
- The owner must see that plan before any flash. Section 4 is that plan.
- Section 4 is for owner review only. This document executes nothing.
- Do not use `tools/fm1_rescue.py`. Do not use `--force`.

## 0. Owner-relayed recovery history

These points are reports. This run did not measure them.

- 0.0.26 got stuck on restart with a black screen for 1 to 2 minutes.
  [sourced: owner report relayed by Orchestrator]
- 0.0.25.1 recovered the device.
  [sourced: owner report relayed by Orchestrator]
- The path 0.0.25 to 0.0.26 and the path 0.0.25.1 to 0.0.26 first behaved in different ways.
  Later 0.0.26 was still slow. No test proves an upgrade-path cause.
  [sourced: owner report relayed by Orchestrator]
- M-Flasher worked.
  [sourced: owner report relayed by Orchestrator]
- The latest owner report: install, auto-restart, and a normal boot succeeded.
  The report infers the model and the hash. It does not confirm them.
  [sourced: owner report relayed by Orchestrator]

## 1. What bootloader and update path exist now

### 1.1 Mask-ROM UBOOT

The AC79 chip has a mask-ROM USB download mode.
It shows up on USB as `WL80UBOOT1.00` with VID:PID `4C4A:8057`.
[sourced: `tools/fm1_rescue.py` and `firmware/hal/fm1_sys.h` at `94197fc8`]

This mode can start in these cases:

- The app calls `fm1_enter_uboot()`.
- The owner holds OCTAVE- and OCTAVE+ for more than 5 seconds while the app runs.
- Bootguard records four failed starts, in `fm1_cstart` before LCD init.
- A host sends a UBOOT USB request (`usb.uboot_req`).

[sourced: `firmware/src/app/main_fm1.c` at `94197fc8`]

The 5-second key hold draws the words `UPDATE MODE IN n`, then calls `enter_uboot("UBOOT")`.
That path enters mask-ROM UBOOT. It does not start the Felucca update loader.
[sourced: `firmware/src/app/main_fm1.c` near the 5-second hold]

Felucca update mode starts when a host sends the M-UPGRADE USB-MIDI request (`usb.ota_req`).
The loader then appears as a USB-MIDI device with an `ota-` identity.
An example is `ota-SMK-37 Pro_900`.
[sourced: `firmware/loader/loader.c` at `94197fc8`]

### 1.2 Flash head and SDK SPL

Flash range `[0x00000, 0x04000)` is the head.
It holds the flash header, JLFS entries, the SDK SPL `uboot.boot`, and `isd_config.ini`.
The flash map marks this range `NEVER`.
[sourced: `firmware/hal/fm1_flash_map.h` at `94197fc8`]

`tools/build.py` pins three SDK files for `AC79NN_SDK_V1.2.14_2026-10-13`.

- `uboot.boot` SHA-256 `64ccc63e4df3238046fadb200f1b134311d8fe6ec6e3ccb8c8b81379231b867e`
- `cfg_tool.bin` SHA-256 `276579954f076886a6a7694f65dc71c034a63a2c204b76749065c0ac7b010d1b`
- `cfg/eq_cfg_hw.bin` SHA-256 `179ac9f4501d2cf8c3bdbc06fcda1e46228b7ace7e339342c767bb6568de57d8`

[sourced: `tools/build.py` and `tests/sdk_pin_test.py` at `94197fc8`]

The copy of `uboot.boot` on this computer at `/workspace/sdk-v1.2.14/cpu/wl82/tools/uboot.boot` has that same SHA-256.
Its size is 14444 bytes. [measured]

`tools/fm1pkg_make.py` puts those SDK files into the package `flash.bin` head at build time.
The Felucca update loader does not write that head.
[sourced: `tools/fm1pkg_make.py` at `94197fc8`]

This packaged head is a stand-in for the package format.
It does not prove that a given unit already holds this exact head.
[sourced: M-Flasher 0.11.0 notes at `https://github.com/jvitkauskas/m-flasher/releases`]

### 1.3 SMK-37 flash map and protected ranges

Elite and Pro both use `BOARD_SMK37`. They share this 1 MiB map.
[sourced: `firmware/hal/fm1_flash_map.h` at `94197fc8`]

| Range | Role | Write policy |
| --- | --- | --- |
| `0x00000-0x03FFF` | Head: SPL, `isd_config` | NEVER |
| `0x04000-0x9CFFF` | App area, SFC-encrypted | Felucca loader only (`FL_APP`) |
| `0x8B000-0x9AFFF` | Bluetooth controller, 64 KiB | Inside the app area |
| `0x9D000-0xAFFFF` | VM: X0X store | App store (`FL_DATA`) |
| `0xB0000-0xBAFFF` | Sample shadows and slot 3 | App store (`FL_SMPV`) |
| `0xBB000-0xBFFFF` | M-UPGRADE staging and record | OTA only (`FL_OTA`) |
| `0xC0000-0xC0FFF` | BTIF (pairing) | NEVER |
| `0xC1000-0xC1FFF` | USRTRIM (key, pad, and wheel calibration) | NEVER |
| `0xC2000-0xF3FFF` | User sample slots 1 and 2 | App store (`FL_SMP`) |
| `0xF4000-0xFDFFF` | USR stock resources | NEVER |
| `0xFF000-0xFFFFF` | `key_mac` | NEVER |

[sourced: `firmware/hal/fm1_flash_map.h` at `94197fc8`]

Compile-time asserts and the RAM flash driver (`FL_GUARD`, `FL_PROTECTED`) refuse any window that hits a protected range.
[sourced: same header, and `firmware/loader/ldr_core.c`]

A Pro unit read over mask-ROM UBOOT on 9 October 2026 reported JEDEC id `0xEB6014` (1 MiB).
The driver also accepts `0x856014`.
[sourced: comment in `firmware/hal/fm1_flash_map.h`]

These files have the same Git blob at 0.0.25.1, published 0.0.26, and combined 0.0.26:

- `firmware/hal/fm1_flash_map.h`
- `tools/fm1_install.py`
- `tools/package_nightly.py`
- `tools/fm1_rescue.py`
- `firmware/loader/ldr_core.c`

The three source commits are `79280e763341d03ba43266d3e25dad66f12de860`, `600b3b45f8d59c3432d958238b05753eb6f2fcb5`, and `94197fc839b00aff47b98ca02f3c6396e4a6eabf`.
[measured: identical blob SHAs from the GitHub trees and from local Git objects]

So the update path and the flash layout did not change between those three sources.

### 1.4 Package format (UFW in a `.fwsc` file)

The build makes a JieLi UFW package with the `.fwsc` suffix.
The package holds `flash.bin` and `ota.bin`.
[sourced: `tools/fm1pkg_make.py` and `tools/build.py` at `94197fc8`]

The packager encrypts the app area with a fixed default chip-key constant in `fm1pkg_make.py`.
The loader on the device reads the chip key from that unit's own `isd_config.ini` in the flash head.
If the package does not decrypt with that key, the loader refuses the package before any erase.
[sourced: `firmware/loader/ldr_core.c` functions `ldr_chip_key` and `ldr_session`]

This default key in source is a packaging value.
It is not a measurement of the owner's efuse or of any one unit.
[sourced: `tools/mboot/prepare.py` and `tools/fm1pkg_make.py`]

### 1.5 How an ordinary update writes the device

The host path is the same for `tools/fm1_install.py` and the web installer (`web/fm1ota.js`).

1. The running app reads part of the package and stages the Felucca loader in `FL_OTA` (`0xBB000-0xBFFFF`).
2. The device restarts into the Felucca loader.
3. The loader writes only the SMK app area `[0x4000, 0x9D000)`, sector by sector, and checks each sector.
4. The loader then asks the host for success, clears the update record in `[0x9D000, 0xC0000)`, and resets.
5. The existing SPL starts the new app.

[sourced: `tools/fm1_install.py` header, `firmware/src/ota.c`, and `firmware/loader/ldr_core.c` at `94197fc8`]

The SMK loader window stays above the head and below BTIF and USRTRIM.
[sourced: `_Static_assert` in `ldr_core.c`]

If the staged loader is the official M-Vave loader, `ota_stage()` first compares all 16 KiB of the package head with the head on the device.
A mismatch refuses the update.
The official loader can write the whole `flash.bin`, including the head.
The Felucca loader does not.
[sourced: `firmware/src/ota.c` official branch]

A stalled host in `ota_session()` gives up after 120 seconds.
[sourced: `firmware/src/ota.c`]

### 1.6 Nightly packaging and the Bluetooth controller

`tools/package_nightly.py` composes or inspects a complete Elite or Pro package.
It requires all of the items below.

- Model `elite` or `pro`, and a matching product identity.
- A Felucca loader mark in `ota.bin`.
- `isd_config.ini` in the flash head, with the chip-key range inside `[0xA0, 0x4000)`.
- The current SMK sample slot map.
- A bundled Bluetooth controller that matches an exact SHA-256 and size pin.
- `--check` to inspect only, with no package write.

[sourced: `tools/package_nightly.py` at `94197fc8`]

The controller pin in current 0.0.25.1 and 0.0.26 hardware-test packages is SHA-256 `a334208b51f62bc7231ebb76a6de0dfa260033d7f7316e5b92053c540bc1d984`.
The size is 61552 bytes.
[sourced: PR #880 `versions.json`, and `/workspace/kept-026-94197fc8/PROVENANCE.txt`]

`tools/add_ctl.py` refuses an empty controller block on a current SMK package.
An install then does not erase a live controller.
[sourced: `tools/fm1_install.py` and `tools/add_ctl.py`]

### 1.7 `fm1_install.py` gates, and why this plan forbids `--force`

For a real device, the order is: gate guard, foreground guard, hardware lock, bootloader (head) guard, then the device.
[sourced: `tools/fm1_install.py` at `94197fc8`]

Model rule: `same_model()` requires an exact match between the package product and the model the keyboard reports in the handshake.
A family match is not enough. An FM-1 package never goes to an SMK device.
Port names do not gate discovery.
[sourced: `same_model()`, `same_keyboard()`, and the port-name comment in `tools/fm1_install.py`]

Head guard: bytes `[0, 0x4000)` of package `flash.bin` must match a reference head.
The reference comes from `--head-ref`, `X0X_HEAD_REF`, or `~/.config/x0x/flash-head.bin`.
A mismatch exits 11 and writes nothing.
If no reference exists, the tool prints a warning and continues.
[sourced: `head_guard()` in `tools/fm1_install.py`]

`--force` only overrides the check for the `FELUCCA-LOADER-1` mark.
A package without the Felucca loader can then pass `load_package()`.
That can admit an official-style loader, which can write the head.
[sourced: `load_package()` in `tools/fm1_install.py`, and `ota_stage()` in `firmware/src/ota.c`]

This plan never uses `--force`.

The tool can also back up sample slots and write them back after the install.
`--no-slots` skips that check.
[sourced: `tools/fm1_install.py` header]

`SMK37.md` says the installer accepts any SMK-family package.
The code in `same_model()` is stricter. This assessment follows the code.
[sourced: `SMK37.md` compared with `same_model()` at `94197fc8`]

### 1.8 MAIN installer draft (PR #880)

Draft PR #880 pins MAIN to 0.0.25.1 source `79280e763341d03ba43266d3e25dad66f12de860`.
That commit is tag `v0.0.25.1` and branch `release/0.0.25.1`.
[sourced: `https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/880`]

Catalog hashes in that draft:

- Elite package SHA-256 `5e24e0d4afccc30906b334fbe73bcad3621df8df3c325790b7af2f94accffb1a`, 651721 bytes, product `SMK-37 Elite_900`.
- Pro package SHA-256 `6f135a3d03bb1a2a4d09d63040c719313612b85590a6af69b293a928e22285bf`, 651721 bytes, product `SMK-37 Pro_900`.

The firmware string inside the image is `0.0.25` with `GIT 79280e76`.
The catalog label is 0.0.25.1.
[sourced: PR #880 body and `versions.json` on `fix/main-installer-0251-20261113`]

The same catalog withdraws the 600b 0.0.26 build.
It treats 0.0.26 at `94197fc8` as a hardware-test build that this installer does not offer.
The draft does not claim that MAIN already deploys this catalog.
[sourced: PR #880 body and `versions.json`]

The `versions.json` field `hardware_tested` is `false`, and the label is `DEVICE NOT TESTED`.
The description records owner use and recovery to 0.0.25.1.
It also says that file has no independent exact-package hardware validation for both models.
[sourced: PR #880 `versions.json`]

### 1.9 M-bootloader: offline only, HOLD

M-bootloader is the MIT project `jvitkauskas/m-bootloader` at pin `64b74d009e6d21fe85ca60754b7c01d438214804`.
[sourced: `tools/mboot/README.md` and PR #640]

PR #640 merged an offline in-memory adoption tool.
The tool does not add M-bootloader to a package.
It does not change the default updater.
It does not authorize a device write.
[sourced: PR #640 and `tools/mboot/README.md`]

On success, only boot-directory bytes `0x20..0x3f` and boot-slot `0xa0..0x38cf` change in the memory candidate.
Configuration, application, projects, samples, calibration, and key/MAC stay byte-identical in that transform.
[sourced: `tools/mboot/README.md`]

The pinned upstream bootloader, on first boot, reserves `0xff000..0xff23b`.
It can restore calibration at `0xc1000` (USRTRIM).
The offline stage refuses an occupied reservation.
It does not remove that upstream write path.
[sourced: `tools/mboot/README.md` and `docs/mbootloader-usrtrim-install-plan.md`]

The USRTRIM investigation found no qualifying SMK-37 variant that leaves USRTRIM untouched.
The status stays `PLAN_ONLY HOLD`.
The contract check is `FAIL_FOR_PINNED_SMK_VARIANT`.
[sourced: `docs/mbootloader-usrtrim-install-plan.md` at `94197fc8`, same blob as 0.0.25.1]

A real pinned SMK M-bootloader container from that work was 14000 bytes, with entry `0x01C02000`.
That size differs from the SDK SPL of 14444 bytes.
[sourced: PR #640 body]
[measured: SDK `uboot.boot` size on this computer]

The repository closed PR #761 and did not merge it.
That PR added a disabled MBOOT read-only session wire.
Production reports zero session capabilities and refuses OPEN and READ.
Nothing in that work writes the bootloader.
[sourced: PR #761]

### 1.10 Official M-UPGRADE and third-party M-Flasher

M-UPGRADE is M-Vave's official updater.
The project docs tell the owner to keep the official package for the exact model ready.
They also tell the owner to keep M-UPGRADE ready, before a custom install.
[sourced: `SMK37.md` and `https://www.m-vave.com/download`]

M-Flasher (`jvitkauskas/m-flasher`, public release 0.11.0) is a third-party tool.
It talks to mask-ROM download mode.
It can back up the whole 1 MiB flash, and it can restore a backup.
It can install a `.fwsc` package.
It can also install M-bootloader, or put a stock bootloader back.
[sourced: `https://github.com/jvitkauskas/m-flasher` and its `docs/usage.md`]

M-Flasher 0.11.0 no longer writes a custom (Felucca-family) package head.
A Felucca package head is only a stand-in.
A write of that stand-in can leave the unit dark and reachable only in download mode.
The tool now writes a package boot area only when the image is a real bootloader that can start the unit.
The chip key must stay intact.
Otherwise the tool refuses the head write and can still install the firmware alone.
[sourced: M-Flasher 0.11.0 release notes]

The M-Flasher window can start with "Also erase settings and Bluetooth pairings" ticked.
That path can clear settings and BTIF pairings at addresses in the package.
It is not a USRTRIM-safe default.
[sourced: `https://github.com/jvitkauskas/m-flasher/blob/main/docs/usage.md`]

Published 0.0.26 source `600b3b45` used USB names `DnBOX 0.0.26 37PRO/37ELITE`.
Those names lacked the `SMK-37` token that M-Flasher v0.11 uses for discovery.
This is not evidence for the slow-boot cause.
PR #873 changes those names. That change is in `94197fc8`.
[sourced: `docs/FEATURE-REGISTRY.md` at `94197fc8`, and PR #873]

### 1.11 `fm1_rescue.py` is not an SMK path

`tools/fm1_rescue.py` is an FM-1-only rescue.
It accepts only one exact FM-1 V15 package, chip key `980F`, flash `856014`, and write window `[0x4000, 0x93000)`.
That window is the FM-1 map, not the SMK map.
[sourced: `tools/fm1_rescue.py` at `94197fc8`]

The owner does not own an FM-1.
This assessment does not use `fm1_rescue.py` or `fm1_rescue.sh`.
It does not recommend those tools.
PR #874 also removes V15 rescue advice from the public site.
That advice can send an SMK owner down the wrong path.
[sourced: PR #874]

## 2. Compatibility risks

### 2.1 Bootguard LTO defect in published 0.0.26

Published 0.0.26 source `600b3b45f8d59c3432d958238b05753eb6f2fcb5` declares `bootguard` without `volatile`.
PR #877 shows that this source, with LTO, compiles the 32-bit magic `0x42475244` into a byte boolean.
A nonzero stale RAM byte then passes the magic check.
The PR gives this example: retained magic `0x01000001`, `failed=4`, `pending=0`.
That state wrongly enters UBOOT instead of a fresh boot record.
[sourced: PR #877]

`fm1_cstart` calls `fm1_enter_uboot()` when `failed >= 4`, before LCD init.
The screen stays black.
[sourced: `firmware/src/app/main_fm1.c` at `94197fc8`]

PR #877 calls this a confirmed compiled defect.
It also says no physical test proves causality for every reported long or black boot.
[sourced: PR #877]

The 0.0.25.1 source at `79280e76` uses the same non-`volatile` declaration.
[measured: declaration text at `79280e76`]
The section "Open question (b) result" records the ELF checks for 0.0.25.1 and for `94197fc8`.

PR #877 makes the whole retained structure `volatile`.
The merge commit `d4088e60652adcd4b49d360c6fff7fd27f312494` is an ancestor of combined head `94197fc8`.
[measured: `git merge-base --is-ancestor`]

### 2.2 Flash map and reserved areas

The flash map is the same at 0.0.25.1, published 0.0.26, and combined `94197fc8`.
[measured]

An ordinary Felucca update does not move BTIF, USRTRIM, USR, `key_mac`, or the head.
[sourced: `ldr_core.c` and `fm1_flash_map.h`]

Risks that still remain:

- An official loader plus a package whose head differs from the unit can refuse the update.
  On a different tool, that pair can write the head.
  [sourced: `ota.c` official branch, and M-Flasher `--with-bootloader`]
- `--force` can admit a package without the Felucca loader mark. This plan forbids `--force`.
- M-bootloader upstream can write USRTRIM at `0xc1000` on first boot.
  That breaks the no-USRTRIM rule. See section 4.
  [sourced: `docs/mbootloader-usrtrim-install-plan.md`]
- A full-flash tool that restores another unit's image can overwrite factory data if the tool does not preserve it.
  M-Flasher documents that a restore of another unit's backup keeps this unit's factory calibration and Bluetooth address.
  That is a vendor claim. This run did not measure it.
  [sourced: M-Flasher `docs/usage.md`]

### 2.3 Elite versus Pro

The two models share the SMK flash map and the same loader windows.
[sourced: `BOARD_SMK37` in `fm1_flash_map.h` and `ldr_core.c`]

They need different package identities: `SMK-37 Elite_900` and `SMK-37 Pro_900`.
`same_model()` requires an exact match.
[sourced: `tools/fm1_install.py` and `tools/package_nightly.py`]

A USB name is not identity.
A name such as `DnBOX 0.0.26 37PRO` does not prove the hardware model.
It also does not prove an FM-1.
[sourced: `tools/fm1_install.py` port-name comment, PR #873, and the owner rule]

### 2.4 Black-screen and slow-boot candidates

These are candidates, not a proved cause.
No test proves an upgrade-path cause.
[sourced: owner report relayed by Orchestrator]

1. Bootguard LTO on published 0.0.26 enters UBOOT in `fm1_cstart` with no LCD.
   [hypothesis, mechanism sourced from PR #877 and `main_fm1.c`]
2. Mask-ROM UBOOT has no app screen.
   A host that treats the unit as a disk can disturb it and cause a restart loop.
   The FM-1 rescue comments describe that host behaviour for FM-1.
   This run did not measure it on the owner's SMK units.
   [hypothesis, mechanism sourced from comments in `tools/fm1_rescue.py`]
3. Safe mode after two failed starts shows the text `DnBOX SAFE MODE`.
   A long black screen does not match that screen.
   [hypothesis, mechanism sourced from `safe_main()` in `main_fm1.c`]
4. App start can take time for Bluetooth, project load, and LCD work, even when bootguard is correct.
   [hypothesis]
5. Combined 0.0.26 packages at `94197fc8` carry a hardware-test label.
   The provenance file does not verify boot and restart.
   [sourced: `/workspace/kept-026-94197fc8/PROVENANCE.txt`]

Emu gate numbers for `94197fc8` exist (Elite worst 11561 us, Pro worst 11525 us, 0 late).
Those numbers are audio-block times. They are not device boot times.
[sourced: `/workspace/kept-026-94197fc8/GATE-RESULT.txt`]

### 2.5 SDK V1.2.14 versus the app

The current app builds pin SDK V1.2.14.
They embed the pinned `uboot.boot` only inside the package head stand-in.
The Felucca path does not program that stand-in onto the unit.
[sourced: `tools/build.py`, `tools/fm1pkg_make.py`, `ldr_core.c`]

Compatibility risk with SDK V1.2.14 is therefore in the app and the loader, not in a required bootloader swap.
[hypothesis]
A later M-bootloader flash is a separate act, and it stays off limits unless the owner approves section 4.
[sourced for the write windows]

## 3. Recovery the owner can do

Agents do not install or flash. The owner does this work.
[sourced: project rule R7, and `dnbox-work-gate`]

Do not use `fm1_rescue.py`. Do not use `--force`.
Do not install M-bootloader.
Do not use `--with-bootloader`.
The only exception is the plan in section 4, after the owner approves it, and only if every gate passes, including "never write USRTRIM".

### 3.1 Preconditions and backup

Complete these steps before any write.

1. Read the model from the firmware handshake or from a backup field (`SMK-37 Elite` or `SMK-37 Pro`).
   Do not use the USB port name.
2. Record the running firmware version and, if present, the Git short hash from the About or MIDI identity.
   The latest owner report leaves model and hash inferred. Confirm them before a new write.
3. Export projects through the project export path.
4. Back up sample slots to an `.x0xslots` file (`tools/x0x_slots.py` or the installer's slot backup).
5. If the owner uses M-Flasher, start with a full 1 MiB flash backup.
   A backup writes nothing. Keep that backup private. Do not post it.
6. Keep the matching 0.0.25.1 package, with the SHA-256 values in section 1.8.
   Also keep the official M-Vave package for that exact model, and keep M-UPGRADE.

[sourced: `tools/fm1_install.py`, `SMK37.md`, M-Flasher `docs/usage.md`, PR #880]

### 3.2 0.0.25.1 rollback (app only)

This path does not write the bootloader when it uses the Felucca or web installer without `--force`.

1. Confirm the handshake model.
2. Select the 0.0.25.1 package whose SHA-256 matches that model (section 1.8).
3. Install with the web installer or with `fm1_install.py` without `--force`.
   Give a slot backup.
   Give the bundled controller already in the package, or an explicit `--ctl` that matches pin `a334208b…`.
4. After restart, confirm the reported version (`0.0.25`, `GIT 79280e76`) and a normal boot time.

The owner already reports that 0.0.25.1 recovered a unit.
The PR #880 catalog still lacks independent exact-package hardware validation for both models.
[sourced: owner report relayed by Orchestrator, and PR #880 `versions.json`]

### 3.3 Felucca update mode, safe mode, and UBOOT

- If an install stops halfway, the device can already be in the Felucca loader (`ota-` identity).
  Replug and install the same matching package again, after the owner confirms a slot backup exists.
  [sourced: `SMK37.md` and `fm1_install.py`]
- After two failed starts, safe mode shows `DnBOX SAFE MODE`. USB stays on.
  The web installer can reach the unit. PLAY tries a normal start again.
  [sourced: `safe_main()` in `main_fm1.c`]
- After four failed starts, or after a 5-second OCTAVE- and OCTAVE+ hold, the unit enters mask-ROM UBOOT.
  The screen can be black.
  [sourced: `fm1_cstart` and the 5-second hold in `main_fm1.c`]
- OCTAVE- and OCTAVE+ held at power-on start hardware calibration of buttons and knobs.
  That is a different act. Do not use it to enter update mode.
  [sourced: `SMK37.md` and `fm1_main()`]

### 3.4 M-Flasher, firmware only

The owner reports that M-Flasher worked.
[sourced: owner report relayed by Orchestrator]

If the owner uses M-Flasher again:

1. Identify the unit from hardware and backup fields, not from the USB name.
2. Take a full flash backup first.
3. Install firmware only.
   Do not select "Install m-bootloader".
   Do not select "put back the package's stock bootloader" on a Felucca package (`--with-bootloader`).
4. Do not tick "Also erase settings and Bluetooth pairings" unless the owner intends to clear pairings.
5. Use the package that matches the reported model, with a known SHA-256.

[sourced: M-Flasher `docs/usage.md` and 0.11.0 release notes]

M-UPGRADE with the official package for that exact model is the documented path back to stock firmware.
[sourced: `SMK37.md` and `https://www.m-vave.com/download`]

### 3.5 What this recovery set does not do

- It does not use `fm1_rescue.py`.
- It does not use `--force`.
- It does not replace the bootloader.
- It does not write BTIF, USRTRIM, efuse, or `key_mac`.
- It does not treat a successful inferred install as a confirmed model or hash.

## 4. M-bootloader install plan (for owner review only)

This section is the M-bootloader install plan required by the owner rule of 30 October 2026.
Show this plan to the owner before any flash of the bootloader.
This document executes nothing.
This run does not flash.
It does not recommend a flash of the current pinned M-bootloader.

The bootloader stays off limits until the owner approves a plan that meets all five gates below, including "never write USRTRIM".

### 4.1 Why the current pinned image is not installable

The pinned upstream M-bootloader at `64b74d009e6d21fe85ca60754b7c01d438214804` can restore calibration at USRTRIM `0xc1000` on first boot.
The project contract check is `FAIL_FOR_PINNED_SMK_VARIANT`.
Status: `PLAN_ONLY HOLD`.
[sourced: `docs/mbootloader-usrtrim-install-plan.md` and `tools/mboot/README.md` at `94197fc8`]

A plan that flashes that image fails gate 5.
This plan therefore refuses that image.
The next source work must be a bounded, separately reviewed variant.
That variant must remove every USRTRIM program and erase path.
It must leave SMK identity, BTIF, efuse, `key_mac`, and wipe protections unchanged.
Nobody completed that work.
[sourced: same install-plan document]

### 4.2 Gates (all must pass before any flash)

The owner rule requires five items.
This plan uses them as hard gates.
If any gate fails, stop. Do not write the device.

| # | Requirement | How this plan meets it | Status now |
| --- | --- | --- | --- |
| 1 | Device presence and identity check by hardware and backup fields | Before any write, read the live handshake model, flash JEDEC id, and backup identity fields. The fields include model, head SHA-256, chip-key prefix CRC, and `key_mac` presence. Refuse a USB-name-only identity. Refuse an FM-1 package. Refuse a mismatch between live fields and the backup. | Specified in 4.3. This assessment did not run it. |
| 2 | Full backup before any write | Take a complete 1 MiB raw flash snapshot. Compute SHA-256 and a per-sector CRC list. Confirm size 1048576 bytes and full range coverage. Keep the file private. Do not write until this backup is complete and checked. | Specified in 4.3. Not run. |
| 3 | CRC-verified image | Admit only an image with exact source pin, SHA-256, CRC, and model. The mutation record must list every byte the writer will change. Recompute CRC and SHA-256 on the file that the writer will use, immediately before the write. | Specified, but no no-USRTRIM image exists. This plan refuses the current pin. |
| 4 | Recovery path | Restore the full backup from gate 2. Or reinstall 0.0.25.1 with the Felucca app-only path (section 3.2). Or use M-UPGRADE with the official package for the same model. Do not use `fm1_rescue.py`. Do not use `--force`. | Specified in 4.4. |
| 5 | Never write USRTRIM | The writer allow-list is only boot record `[0x20, 0x40)` and boot slot `[0xa0, 0x38d0)`. The plan refuses any image whose source, first-boot path, or writer map programs or erases `0xC1000-0xC1FFF`. Mutation-spy tests must cover the whole protected region and must report zero USRTRIM writes. The current pinned upstream fails this gate. | **Not met** for pin `64b74d00`. This plan fails closed. |

Also apply the same refuse-and-stop rule to these ranges:

- The rest of the 16 KiB head, outside the allow-list.
- BTIF `0xC0000-0xC0FFF`.
- USR `0xF4000-0xFDFFF`.
- Efuse, and `key_mac` `0xFF000-0xFFFFF`.

[sourced: `fm1_flash_map.h`, `tools/mboot/README.md`, and `docs/mboot/nightly-adoption-contract.md`]

### 4.3 Step list (not executed)

Do these steps only after the owner approves this plan, and only with an image that already passed gate 5.

1. Stop if no no-USRTRIM image has passed independent review. The current pin has not.
2. Connect one unit only. Take the hardware-lock so no other install runs at the same time.
3. Presence and identity (gate 1):
   - Read the handshake product (`SMK-37 Elite` or `SMK-37 Pro`). Ignore the USB port name.
   - Read the flash JEDEC id. Accept only `0xEB6014` or `0x856014` for SMK.
     [sourced: `FL_JEDEC_OK` in `fm1_flash_map.h`]
   - Compare these fields with the same fields in an earlier backup, if one exists.
   - If any field is missing or does not match, stop.
4. Full backup (gate 2):
   - Read the whole 1 MiB flash. Write nothing.
   - Check length 1048576, SHA-256, and per-sector CRCs.
   - Confirm the backup contains BTIF, USRTRIM, the current head, and `key_mac`.
     The backup receipt must record those ranges.
   - If the backup fails, stop.
5. Image admission (gates 3 and 5):
   - Require a documented source pin that has no USRTRIM program or erase path.
   - Require a matching Elite or Pro model.
     Require a matching chip-key prefix CRC (this is not a live efuse measurement).
     If an app package is also in scope, require a complete bundled controller.
   - Recompute SHA-256 and CRC on the exact file.
   - Require a mutation map that changes only `[0x20, 0x40)` and `[0xa0, 0x38d0)`.
   - Run mutation-spy tests over the protected region.
     Include healthy, corrupt, and missing markers.
     Include first-boot and later-boot paths.
     [sourced: acceptance list in `docs/mbootloader-usrtrim-install-plan.md`]
   - If USRTRIM appears in any write path, stop.
6. Show the identity receipt, the backup hashes, the image hashes, the mutation map, and the recovery steps to the owner.
   Do not write until the owner approves this exact set.
7. Writer step, only after that approval:
   - Write only the allow-listed boot bytes.
   - Do not use `fm1_rescue.py`. Do not use `--force`.
   - Do not use M-Flasher "Install m-bootloader" on the current pin, because that pin fails gate 5.
   - Read back the written ranges. Compare CRC and SHA-256 with the admitted map.
   - Read back USRTRIM, BTIF, USR, and `key_mac`. They must match the gate-2 backup.
   - If read-back fails, go to section 4.4 at once.
8. Restart the unit. Confirm the handshake model, a normal boot, and unchanged calibration.

### 4.4 Rollback and recovery for this plan

If a write starts and then fails, or if the unit does not start:

1. Enter mask-ROM download mode if the app does not run (see section 3.3).
2. Restore the gate-2 full backup, with a tool that keeps this unit's factory BTIF, USRTRIM, and `key_mac`.
3. If a backup restore is not possible, install 0.0.25.1 with the Felucca app-only path in section 3.2.
   That path does not write the head.
4. If the write damages the head, use a stock bootloader from the official M-Vave package for the same model.
   Check its CRC.
   That image must still leave USRTRIM unchanged.
   Show that exact stock image to the owner before that write.
5. Do not use `fm1_rescue.py`. Do not use `--force`.
   Do not flash the current M-bootloader pin as a repair.

### 4.5 Risks

- The current pinned M-bootloader fails the no-USRTRIM gate.
  A flash of that image can change calibration. This plan refuses it.
  [sourced: `docs/mbootloader-usrtrim-install-plan.md`]
- A Felucca package head is a stand-in. A write of that head can leave the unit dark.
  [sourced: M-Flasher 0.11.0 release notes]
- Ordinary Felucca updates after a future approved adoption must still preserve the whole 16 KiB head.
  They must keep the app-only loader.
  [sourced: `docs/mboot/nightly-adoption-contract.md`]
- No owner-reviewed complete unit snapshot is on file for this stage.
  Synthetic fixtures are not a per-unit candidate.
  [sourced: `tools/mboot/README.md` remaining gate]
- This plan, until gate 5 passes on a real image, is not an approval to flash.

### 4.6 Checklist against the owner rule

| Owner rule (30 October 2026) | Met in this written plan? | Evidence |
| --- | --- | --- |
| Show the plan to the owner before any flash | Yes. Section 4 is that plan. Step 6 requires a second look at the exact hashes before a write. | This document. This run executed nothing. |
| (1) Presence and identity by hardware and backup fields | Yes, as a required gate. The plan does not use USB names. The plan does not use FM-1 labels as model proof. | Section 4.3 step 3. |
| (2) Full backup before any write | Yes, as a required gate. 1 MiB, SHA-256, per-sector CRC. No write if this fails. | Section 4.3 step 4. |
| (3) CRC-verified image | Yes, as a required gate. Recompute CRC and SHA-256 on the file that the writer will use. | Section 4.3 step 5. No qualifying image exists yet. |
| (4) Recovery path | Yes. Backup restore, 0.0.25.1 app-only install, or official stock head for the same model. | Section 4.4. |
| (5) Never write USRTRIM | Yes as a rule of the plan: the plan refuses any image that can write `0xC1000-0xC1FFF`. The current pin does not pass, so the plan fails closed and does not flash. | Section 4.1 and 4.2 gate 5. Source: the USRTRIM HOLD document. |
| Do not use `fm1_rescue.py` | Yes. The plan forbids it. | Sections 3, 4.3, and 4.4. |
| Do not use `--force` | Yes. The plan forbids it. | Sections 1.7, 3, and 4.3. |

Result for owner review: do not flash M-bootloader on this pin. Keep the factory head.
For firmware recovery, use the app-only path in section 3, after a backup.
For a later 0.0.26 trial, use the same path.
Use only a build with a measured boot and restart on the real Elite and Pro units.

## 5. Open questions and measurements that are still needed

Run these later in classic ChatGPT threads, or ask the owner to run them.
Do not run them on this computer as device writes or as box host tests for this task.

1. Did the published 0.0.26 black screen and slow boot come from the LTO bootguard defect?
   Need a timed boot log, and a note of whether `WL80UBOOT1.00` appeared.
   If a safe debug path exists, also record the retained bootguard words.
   PR #877 already says no physical test proves causality.
   [sourced: PR #877]
2. Does the shipped 0.0.25.1 Elite and Pro ELF have the same LTO scalarization of `0x42475244`?
   This follow-up answers that question in the section "Open question (b) result".
   The official store check does not pass.
   The images do not show the PR #877 byte-boolean defect.
   The formal verdict is inconclusive.
3. Does combined `94197fc8` (with PR #877) boot and restart in normal time on real Elite and Pro hardware?
   The package label does not verify boot and restart.
   Also confirm the model and package hash of the owner's latest successful install.
   Those values remain inferred.
4. What did the owner's successful M-Flasher run write: firmware only, settings wipe, or a bootloader change?
   A backup receipt or a tool log can close this. Do not post raw backups.
5. Do the factory heads on the owner's Elite and Pro units match the SDK stand-in head, or do they differ?
   A read-only head SHA-256 from a backup answers this.
   This check must come before any future head write.
6. After a later test qualifies a bootguard-fixed 0.0.26, compare two boots on the same build.
   First install 0.0.25.1 then 0.0.26. Then do a power cycle only.
   This closes the "upgrade path not proved" point.

### Measurements that close the top questions

- Time to first UI frame, and time to stable audio, on both models.
  Measure 0.0.25.1, published 0.0.26 if it is still on a unit, and `94197fc8`.
  Repeat at least three times. Record USB devices that appear during the wait.
- PR #877 ELF check on archived 0.0.25.1 and combined `94197fc8` images: see "Open question (b) result".
  The published 0.0.26 (`600b3b45`) ELF check remains the PR #877 report.
- Handshake product string, About version, Git hash, and package SHA-256 at the time of the latest successful boot.
- A private 1 MiB backup inventory (hashes only in public notes) that lists head, BTIF, USRTRIM, and `key_mac` CRCs.

## 6. Sources used

GitHub, read only: repo `gobbajogger/X0X37-for-Loopy-Pro`.
Refs: `79280e76`, `600b3b45`, `94197fc8`, `01c2673a`, `d4088e60`.
PRs: #640, #761, #795, #873, #874, #877, #880, #869.

Local files, read only: `/workspace/kept-026-94197fc8/PROVENANCE.txt`, `GATE-RESULT.txt`, `/workspace/pr880-installer/versions.json`.
This run hashed `/workspace/sdk-v1.2.14/cpu/wl82/tools/uboot.boot` (hash and size only).
It also read Git objects for the paths cited above.
The follow-up for open question (b) hashed the four ELFs and the two 0.0.25.1 packages in section "Open question (b) result" (hash and size only).
It used `/home/box/.jieli/jieli-linux-toolchains-20260730.1/common/bin/objdump` and `tests/host/test_bootguard_retention.py` at `94197fc8`.
It also read `/workspace/kept-025-20261112/79280e763/SHA256SUMS` and `COMMIT`.

Public web, read only: M-Vave download page, M-Flasher README, usage, and 0.11.0 notes, and the `jvitkauskas/m-bootloader` pin cited in repo docs.

This document copies no firmware binary, no raw capture, no token, and no private unit data.

## Open question (b) result

This section records a read-only static check of the PR #877 LTO bootguard tests.
This follow-up did not build firmware.
This follow-up did not write to a device.
This follow-up did not write to GitHub.

### Method

The method is the `native()` function in `tests/host/test_bootguard_retention.py`.
[sourced: that file at `01c2673a` and at `94197fc8`]

The function runs `objdump -t` and `objdump -d` on one ELF.
It then applies five checks:

1. The `bootguard` symbol stays in `.noinit` with size `0000000c` (12 bytes).
2. The `fm1_cstart` disassembly contains the full constant `42475244`.
3. A word load of the form `rN = [rM+0]` occurs before that constant.
4. A compare of the form `if (rX != rMagic)` occurs after that constant.
5. A full-word store of the form `[rM+0] = rMagic` occurs after that constant.

This follow-up did not run the `seeded()` function in that file.
That function compiles a host program, and this task forbids builds.

The objdump program is
`/home/box/.jieli/jieli-linux-toolchains-20260730.1/common/bin/objdump`.
That tool reports LLVM 4.0.1 with registered target `pi32v2`.
PR #877 names `$JIELI_TOOLCHAIN/common/bin/objdump`.
[measured: `objdump --version`]
[sourced: PR #877]

This follow-up called only `native(elf, objdump)` from that test file.
It did not run the file as a main program.
A main-program run also starts `seeded()`.

A `PASS` from `native()` is the verdict **fixed**.
A fail that matches the PR #877 byte-boolean defect is the verdict **scalarized**.
Any other fail, or a tool error, is the verdict **inconclusive**.

Commands (read only). Replace `ELF` with one path from the tables below:

```
OBJDUMP=/home/box/.jieli/jieli-linux-toolchains-20260730.1/common/bin/objdump
sha256sum ELF
$OBJDUMP -t ELF
$OBJDUMP -d ELF
```

Then, in Python, load `tests/host/test_bootguard_retention.py` and call only `native(ELF, OBJDUMP)`.

### Combined 0.0.26 `94197fc8` ELFs

| File | SHA-256 | Size | Official `native()` | Verdict |
| --- | --- | --- | --- | --- |
| `/workspace/kept-026-94197fc8/elite/x0x.elf` | `720f851c83a62fbef71fe52ad84e669409a69fde1d1fb313427d7942ed3ca1ea` | 628872 | PASS | **fixed** |
| `/workspace/kept-026-94197fc8/pro/x0x.elf` | `7e67c3b33fe2e690e5f913b5d048727a715c70daca06ddd326a10e37eacb9535` | 628864 | PASS | **fixed** |

[measured: `sha256sum` and official `native()`]

Named copies under `/workspace/kept-026-94197fc8/named-elf/` have the same SHA-256 values.
This follow-up did not treat them as a second pair of images.

The symbol line on both files is:
`01c7c10c l       .noinit          0000000c bootguard`
[measured: `objdump -t`]

The `fm1_cstart` sequence is the same on Elite and on Pro.
The addresses match.
Excerpt from the Elite file, around `0x2041d22`:

```
r2 = bootguard
r1 = [r2+0]
r0 = 42475244
if (r1 != r0) {
    [r2+0] = r0
    [r2+4] = r6
    [r2+8] = r6
}
```

[measured: `objdump -d`]

All five official checks pass on both files.
This follow-up records the verdict **fixed** for both files.
[measured]

### Shipped 0.0.25.1 ELFs: local copies are checkable

This follow-up searched `/workspace` package trees and `SHA256SUMS` files for 0.0.25.1 images.
It ran `native()` on a local ELF only when a derived package SHA-256 matched section 1.8.
This follow-up did not rebuild a package.
This follow-up did not download a package and then assume a match.

Local packages at `/workspace/kept-025-20261112/79280e763/` match the published hashes.

| Package | SHA-256 | Size | Matches section 1.8 |
| --- | --- | --- | --- |
| `pkg-elite.fwsc` | `5e24e0d4afccc30906b334fbe73bcad3621df8df3c325790b7af2f94accffb1a` | 651721 | yes |
| `pkg-pro.fwsc` | `6f135a3d03bb1a2a4d09d63040c719313612b85590a6af69b293a928e22285bf` | 651721 | yes |

[measured: `sha256sum`]
[sourced: section 1.8 published hashes]

The `COMMIT` file in that tree is `79280e763341d03ba43266d3e25dad66f12de860`.
That commit is the 0.0.25.1 pin in section 1.8.
[measured: `COMMIT` file]
[sourced: section 1.8]

This follow-up compared each flash `PT_LOAD` segment of the adjacent ELF with the decrypted app area of that package.
All three flash `PT_LOAD` segments match, for Elite and for Pro.
The adjacent ELFs are therefore images of the published 0.0.25.1 packages.
[measured]

| ELF | SHA-256 | Size |
| --- | --- | --- |
| `/workspace/kept-025-20261112/79280e763/elite/x0x.elf` | `e176ba4c55d3f7e36ee137c944b00c45f03856eba63208ea35b1dca3330d4fef` | 644408 |
| `/workspace/kept-025-20261112/79280e763/pro/x0x.elf` | `fd46d7c68ed010462baf8e19ecac682fa6ef1e9b1f6fd000a69a0d9555f17973` | 644404 |

[measured: `sha256sum`]

Other local 0.0.25 trees (`cd3128e45`, `1ec5d683c`) have package hashes that do not match section 1.8.
This follow-up did not run `native()` on those ELFs.

The raw model packages `elite/x0x-smk37-elite.fwsc` and `pro/x0x-smk37.fwsc` also do not match the published hashes.
Their decrypted app areas still match the same ELF flash `PT_LOAD` segments.
The published hashes are the composed `pkg-*.fwsc` files.
[measured]

### Shipped 0.0.25.1: official result

| ELF | Official `native()` | Assertion | Verdict |
| --- | --- | --- | --- |
| 0.0.25.1 Elite | FAIL | `native startup must store the full magic word` | **inconclusive** |
| 0.0.25.1 Pro | FAIL | `native startup must store the full magic word` | **inconclusive** |

[measured: official `native()`]

The first four official checks pass on both files.

- `bootguard` is in `.noinit` with size `0000000c`.
  The symbol line is `01c7c10c g       .noinit          0000000c bootguard`.
- `fm1_cstart` contains the full constant `42475244`.
- A word load `r0 = [r2+0]` occurs before that constant.
- A compare `if (r0 != r1) goto ...` occurs after that constant.

[measured: `objdump -t` and `objdump -d`]

The fifth check fails because the store is not `[rM+0] = r1`.
The magic register is `r1`.
The initializer at `0x202d658` is the same on Elite and on Pro:

```
r0 = r1
r1 = 0
d[r2+0] = r1_r0
[r2+8] = 0x1
```

[measured: `objdump -d`]

This form is not the PR #877 byte-boolean defect.
The image keeps a 12-byte retained object.
The image keeps the full 32-bit magic constant.
The image compares that full word.
[measured]

The official regex does not accept the store form `d[r2+0] = r1_r0`.
[measured]

[hypothesis: that instruction writes the full magic word from `r0`.]
[hypothesis: the same instruction writes zero to `failed`.]

[hypothesis: 0.0.25.1 LTO did not narrow the magic to a byte boolean here.]

The 0.0.25.1 source still lacks `volatile`.
A later LTO build of the same source can still differ.
[sourced: declaration at `79280e76`]

### Open questions that stay open

Open question (a) stays open.
That question is the causality of the published 0.0.26 black screen (section 5, item 1).

Open question (c) stays open.
That question is real-hardware boot and restart time for `94197fc8` (section 5, item 3).
