# Bluetooth MIDI on the SMK-37

This folder builds the Bluetooth controller image (`ctl.img`) for X0X on the SMK-37.
New qualified NIGHTLY packages must include the matching image; current legacy packages
require it locally. Combined public distribution remains undecided. See [LICENSING.md](../LICENSING.md).

## How it fits together

The SMK-37's chip (JieLi AC79) has the Bluetooth radio on the chip. JieLi's controller software
for it (`btctrler.a` and five other archives) has SDK-level Apache-2.0 evidence, with
component-specific runtime/source/notice questions still open. X0X is GPL-3.0-only.
The controller runs as a separate program:

| Part | Where | Licence | Who builds it |
| --- | --- | --- | --- |
| The X0X app, with the BLE-MIDI host (`firmware/src/ble.c`, `firmware/src/ble/`) | app area, main RAM, BLERAM 0x01C7C400-0x01C7D3FF | GPL-3.0-only (host: MPL-2.0) | X0X |
| The interface (`firmware/hal/x0x_ble_abi.h`) | compiled into both | MPL-2.0 | both |
| The controller image: the shim (`ble/ctl/src/`) and JieLi's files | flash block 0x8B000-0x9AFFF (XIP 0x02087000), RAM 0x01C01800-0x01C07EFF | shim MPL-2.0, JieLi's files JieLi's terms | you (`build_ctl.sh`) |

The two programs share no code. The app calls only the image's two entry points:

- `init` runs once on core 1 at power-on (only when GLO > BLUETOOTH MIDI > BLUETOOTH is ON). Core 0 waits in RAM. The image copies its RAM code, does the RF trim, starts the controller, runs it until it settles and reports READY.
- `poll` runs on core 1 between audio jobs, with interrupts on. The controller's interrupts also run on core 1. X0X routes none of its own interrupts there.

Everything else goes through the mailbox (in BLERAM): two byte rings of HCI packets
(`[u16 length][u8 H4 type][bytes]`), one writer and one reader each. At boot the app selects
one of two BLE-MIDI roles. The default is the existing peripheral: it advertises, accepts one
connection, and serves BLE-MIDI without pairing. The SMC-Mixer setting selects a central:
it scans for an `SMC` advertising name, connects, discovers the BLE-MIDI GATT service,
subscribes to notifications, and sends LED MIDI. The controller image contains both roles,
but the app uses one role per boot. A change to the role setting takes effect after reboot.
If the mixer is absent, central mode keeps scanning; it does not fall back to peripheral mode.
The one-link controller cannot use both roles at once.

Before X0X turns the flash off (store writes, sample uploads, SPI reads), it parks core 1 in RAM
with its interrupts off (`fm1_cpu1_park`). The image runs from the flash, so it must not run then.

Without an image, the app starts as before (BEHAVIOR.md BLE-02).

## Files

| File | What |
| --- | --- |
| `build_ctl.sh` | The whole build on your computer: fetch, generate, compile and link (in Docker or natively on Linux x86-64), make and check `out/ctl.img`, copy it to `~/.config/x0x/ctl.img`. |
| `fetch_sdk.sh` | Fetches the JieLi files from gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK at commit `d179b44` into `lib/` and checks each SHA-256. |
| `gen_rf_tables.py` | Makes `gen/rf_tables.c` from the fetched SDK sources. |
| `build_in_container.sh` | The compile and link steps (`-Wall -Wextra -Werror` for the shim). |
| `ctl.ld` | The image layout: header, code and constants at 0x02087000 (64 KiB at most), RAM code, data, heap and stacks at 0x01C01800 (26,368 B at most). |
| `mkimage.py` | ELF to `ctl.img`: fills in the size and the CRC-32 and checks the header as the app does. |
| `src/` | The MPL-2.0 shim: the 11 port functions the controller library needs (context switch, interrupts, timers, the OS loop), the entry points and the mailbox side. |

`lib/`, `gen/` and `out/` hold fetched and built files. Git ignores them. Never commit them.

## Build and install

```
ble/ctl/build_ctl.sh                                   # needs tools/get_toolchain.sh and Docker
python3 tools/fm1_install.py build/x0x-smk37.fwsc --ctl ble/ctl/out/ctl.img
```

The command above is the legacy unbundled-package path. The installer never selects
`~/.config/x0x/ctl.img` implicitly. `tools/add_ctl.py PACKAGE ctl.img -o NEW.fwsc` and the legacy
web file field provide the same composition; a current package with an empty block is refused
unless its matching controller is supplied.

Qualified NIGHTLY packages include the matching controller and need no separate file selection.
Use [the checked NIGHTLY packaging command](../docs/bundled-nightly-packages.md) to compose or
inspect one complete Elite or Pro package. All byte, source, component-term, independent QA and
publication requirements still apply. This source support does not publish a controller.

## Numbers (2026-10-15 baseline)

- Image: 64,504 B of 65,536 (1,032 B free). RAM 0x01C01800-0x01C0767F (24,192 B of 26,368: RAM code 3,504 B, data 1,804 B, bss 6,592 B, heap 8 KiB, run stack 2 KiB, interrupt stack 2 KiB).
- App: BLERAM 3,460 B of 4,096. Main RAM .data+.bss 98,080 B of 98,304. Pool headroom 8,496 B. App code 544,184 B (8,488 B below the block).

The 2026-10-19 combined-role controller image builds at 65,492 B of 65,536 B (44 B free),
with 25,824 B of 26,368 B RAM used. These figures exclude the app and do not establish
hardware behavior. The size trim removes the old per-delay RF stack and caller diagnostics;
the delay loop retains its stopped-clock check.

## Not tested on hardware

See docs/plans/2026-10-15-ble-stage3-test-plan.md.
The SMC central role has host-side parser/state tests, but has not been connected to a mixer
or tested on the unit. The controller's pairing feature is disabled; neither role offers pairing.
