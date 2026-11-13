# DnBOX protocol identifiers (developer reference)

This table documents the identifiers in firmware source `94197fc839b00aff47b98ca02f3c6396e4a6eabf`. It is not a device identification or installation checklist. For installation, match the **physical model**, verify the selected firmware package in the installer, and back up samples. Public firmware packages currently target SMK-37 Elite and SMK-37 Pro only.

Player-facing names can say DnBOX. The identifiers below are what installers, M-UPGRADE, Loopy and the editor protocol use. They must stay the same. This book describes what the unit uses today. It does not propose a rename.

| Identifier | Elite | Pro | Must stay? |
|---|---|---|---|
| USB product (app) | `DnBOX <version> SMK-37 Elite` | `DnBOX <version> SMK-37 Pro` | Keep the `SMK-37` token. Do not use this string to choose a package. |
| USB manufacturer (app) | `X0X` | `X0X` | User-visible on some hosts. Changing it can change the port name. This book does not change it. |
| USB product (update loader) | `SMK-37 Elite Update` | `SMK-37 Pro Update` | Prefer to keep. The loader manufacturer string is `Hügelton Instruments`. |
| USB VID / PID (app) | `0x1209` / `0x0001` | Same | Yes. Never change. Shared by Elite and Pro. Not a model test. |
| USB VID / PID (loader) | `0x1209` / `0x0002` | Same | Yes. |
| Package / OTA identity | `SMK-37 Elite_900` (release adds a version code) | `SMK-37 Pro_900` | Yes. This admits a package. |
| Loader OTA identity | `ota-SMK-37 Elite_900` | `ota-SMK-37 Pro_900` | Yes. Hosts look for `ota-`. |
| Stock update-record parm id | `ota-SMK-37 Elite_016` | `ota-SMK-37 Pro_016` | Yes. So a factory package still applies. |
| Handshake SysEx | Query `F0 00 32 45 …`, command `0x11`. The identity reply is the package id. | Same mechanism | Yes. Port names must not gate discovery. |
| M-UPGRADE upgrade SysEx | `F0 22 24 35 7F F7` | Same | Yes. Soft key `F0 22 24 35 7D F7`. |
| BLE advertised / GATT name | `SMK-37 Elite-` plus four hex | `SMK-37 Pro-` plus four hex | The four-hex suffix and address derivation must stay. |
| BLE-MIDI service UUID | `03B80E5A-EDE8-4B33-A751-6CE34EC4C700` | Same | Yes. Standard BLE-MIDI. |
| BLE-MIDI characteristic UUID | `7772E5DB-3868-4112-A1A9-F2669D106BF3` | Same | Yes. Standard BLE-MIDI. |
| APC / Loopy identity | Manufacturer `0x47` (Akai), family `0x29` (APC40 mkII), firmware 1.2, device id `0x7F`. Introduction `F0 47 7F 29 61 …`. | Same | Yes. Loopy mappings depend on this, not on the USB product. |
| Editor INFO version string | `"X0X "` plus the version (24-char field) | Same | Treat as protocol until tools change with it. |
| Storage magic | `"X0X0"` and data family `"X0X"` | Same | Yes. |
| About, crash, boot | `ABOUT DnBOX`, `DnBOX CRASH`, `DnBOX SAFE MODE`, boot `DnBOX` and `909 808 CHICAGO MIAMI` | Same | Display. Already spelled DnBOX. The boot subtitle does not list BREAK, Juno or Drone. |

The onboard package handshake, not the USB display name, is the safe way to identify a unit for a write.
