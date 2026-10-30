# X0X37 project: user preferences and decisions

Read this file before you work on the X0X37 project. It records how the user works and what the user decided. Also follow agent-rules.md, and use BEHAVIOR.md (in the x0x37 tree) as the reference for intended behaviour.

## How to work with the user

- Talk plainly and briefly. Lead with the result. Explain hardware and code findings in simple words.
- Do the work. Ask only when a wrong choice has real consequences or the user must decide.
- Changes outside clip mode need the user's explicit approval first.
- Do not push outside the box. The only allowed remote is box (/workspace/x0x37.git). Do not change release scripts unless the user asks.
- Never touch the bootloader. Never risk a brick. Run every install in the foreground, never in the background.
- Before every install, back up the imported sample slots and compare the slot CRCs after the install. The user has 5 imported loops (3 in slot 1, 2 in slot 2). Backups are in slot_backups/.
- Install approval: in the current BLE build-test phase, the user said "Install, no need to ask" for fixed test builds. For other installs, ask first.
- In build-test mode, do not install fallback or recovery builds (for example a Bluetooth-free build) only to make the keyboard playable. Go straight to the fixed build. The user is testing, not making music.
- Commits: local or to the box remote only, author genkigenki <3080081+genkigenki@users.noreply.github.com>, messages in Simplified Technical English. Log each change in x0x37-progress-log.md.

## Hardware and targets

- M-Vave SMK-37 (Pro and Elite) with a JieLi AC79 chip. The user's unit is an Elite.
- Only the SMK-37 build matters. The FM-1 build is not maintained. Its XIP overflow (about 650 B) is accepted (LIM-12).
- The user knows the SMK-37 port by genkigenki. This project has its own source.

## Audio and samples

- Use 44.1 kHz wherever possible, at all times. This includes factory breaks and imported samples.
- No quality cuts and no length cuts. No halved sample rates. Factory breaks stay 2 bars at 44.1 kHz.
- IMA ADPCM at 44.1 kHz is the preferred storage format for all samples. In a blind A/B test the user heard no difference for the 909 ride, crash and hi-hat.
- Keep the imported-sample memory. Space-saving work must not take from sample storage. Use freed flash for the imported-sample (break) slots. Whether to keep spare room for growth is not decided yet.
- Imported samples are for BREAK mode only.
- The installer must back up imported sample slots automatically before an install and restore them after (for example when the flash layout changes). This is implemented.

## Clip mode (APC40 mkII emulation for Loopy Pro)

- OCT- moves the clip view down. OCT+ moves the clip view up.
- Keep the existing pad-and-key blocking behaviour (a pad blocks the keybed white key) as it is.
- Pad brightness stays at the previous level. Do not add pad dimming or a pad brightness floor.
- LCD clip colours: keep the exact hue when a colour becomes brighter, with no hue drift (for example, blue must not go toward purple). The user likes the current red. The approved table is the OKLCH lift with the "strong" hue fix for 0x32 (0x325D) and 0x33 (0x3A17). The table is in apc40_lcd565.h and is made by tools/apc_pad_colors.py. It must go into the firmware after the BLE test.
- Loopy Pro start/stop can also start the X0X sequencer, but it must not conflict with external MIDI clock sync.
- The usb.c APC identity reply that is not gated is not a problem. Leave it.
- FREE PLAY: keep it and document it as it is.

## Bluetooth MIDI

- MIDI only. No Bluetooth audio.
- The firmware, including any Bluetooth stack, must be freely shareable under an open license.
- Before you use licensed third-party code, check if it can be rebuilt in a legally distinct way (for example from public specifications).
- JieLi's Bluetooth controller (btctrler.a) is closed. Do not bundle JieLi files. The user's computer downloads them from the official source and builds the controller image locally. The controller runs as a separate image (our MPL shim, HCI mailbox, core 1).
- Keep the Bluetooth image in one contiguous flash block.
- Bluetooth settings are in the GLOBAL menu (BLUETOOTH MIDI section). The BT buttons stay as they are, or work only in clip mode.
- The preferred delivery is an HTML web installer that gets the needed files automatically, with no toolchain for end users if possible.

## Where things are

- Source tree: x0x37/ (branch smk37). Test clone: flash_review/ (branch ble-stage3 for the BLE work).
- Rules: agent-rules.md. Intended behaviour: x0x37/BEHAVIOR.md. Change log: x0x37-progress-log.md.
- BREAK mode guide: BREAK_MODE_GUIDE.md and BREAK_MODE_GUIDE.pdf.
- Flash plan: ble_split/flash_plan.md.

Last update: 2026-10-15.
- 2026-10-16 00:08: Bluetooth: use JieLi's own BLE-only host (fetched from the official SDK and built locally,
  never bundled) with SMP bonding for auto-reconnect; strip classic BT and audio. Sample slot 3 may shrink to
  about 16 KiB (0.7 s) for the host (0xAB000-0xB6FFF). Bonding keys in an X0X area, never BTIF/USRTRIM/efuse/key_mac.

## Clip mode keybed, arp, chord and scale (decided 2026-10-16)

- 2026-10-16 00:45: The user approved this change outside clip mode: when the external MIDI clock stops (0.5 s with no tick), the internal tempo takes the last measured external tempo (whole BPM). Nothing must jump. This applies to the main sequencer too.
- In clip mode, the keybed sends MIDI notes to Loopy Pro only. It does not control the X0X UI. A held pad still blocks its white key.
- The note path in clip mode is: key, then SCALE, then CHORD, then ARP, then the CLIP KEYS channel on USB and Bluetooth MIDI.
- GLO > MIDI has CLIP KEYS CH: channel 2 to 15, default 2. Channels 1 and 16 stay free for the APC40 emulation.
- The new settings are new bytes at the end of the settings. Older projects load the defaults. Do not use rsv_bright.
- The arp follows the incoming MIDI clock when a tick came in the last 0.5 s. Else it uses the internal tempo. It runs when Loopy is stopped. MIDI Start resets it. The arp sends no clock.
- Send only the processed notes. The default scale snap is NEAR.
- Knob 1 is always MASTER volume. Knobs 2 to 5 set the mode that has the knobs.
- GLO opens on the LCD in clip mode. The clip grid comes back when the menu closes.
- In clip mode, the mode chip on the LCD reads "LP".
- Installs need no question (2026-10-16). The builder installs a verified build without a question. The builder backs up the slots first and compares the slot CRCs after the install. The install runs in the foreground only. Before each install starts, the user gets a notice. The builder waits for the go from the parent agent after that notice. After the install, the user gets the commit hash and the CRC result. The bootloader rule does not change. Never write the bootloader.
- Report UX changes at once (2026-10-16). As soon as the builder decides on a change that can affect the user experience (latency, packet or MTU size, memory margins, sample length, connection behaviour), the builder sends a short note to the parent agent before it continues. The parent agent tells the user.
- 2026-10-16 02:09: The user approved a shared bare repository on the box, /workspace/x0x37.git, remote name box. Each agent pushes only its own branch there and fetches from it. Do not push outside the box.
- 2026-10-16 02:11: The user gave the team more agency. Generic work is approved automatically: builds, tests, commits, pushes to box, doc updates, merges on the box. Before anything that can change how the device works or feels (installs, new behavior rules, changes outside clip mode, flash layout changes, removed features), notify the user first. Safety rules do not change: backups, CRC checks, never touch the bootloader, never push outside the box.
- 2026-10-16 02:14: If the Mac does not respond, retry for 5 minutes. Report the Mac as offline only after 5 minutes of no response. This replaces the one-minute rule.
