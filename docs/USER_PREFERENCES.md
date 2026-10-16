# X0X37 project: user preferences and decisions

Read this file before you work on the X0X37 project. Also follow agent-rules.md. This Mac root file is the main copy. x0x37/docs/USER_PREFERENCES.md on smk37 is an exact copy. The reference for intended behavior is BEHAVIOR.md on the branch integration. The smk37 copy x0x37/BEHAVIOR.md is synced from it.

## How to work with the user

- Talk plainly and briefly. Lead with the result. Explain hardware and code findings in simple words.
- Do the work. Ask only when a wrong choice has real consequences or the user must decide.
- Only changes that the owner can see, hear or play need the approval of the owner first (see the 19:25 entry). Decide other changes and continue.
- Do not change release scripts unless the user asks.
- In build-test mode, do not install fallback or recovery builds (for example a Bluetooth-free build) only to make the keyboard playable. Go straight to the fixed build. The user is testing, not making music.
- Commits: author genkigenki <3080081+genkigenki@users.noreply.github.com>, messages in Simplified Technical English. Log each change in x0x37-progress-log.md.
- 2026-10-16 18:41 (owner): Optimize without cutting features. Font size is not a feature.
- 2026-10-16 20:12 (owner): Questions and explanations to the owner use plain words about what the owner hears, feels and does on the keyboard. Explain each button and each term. Do not use code words, step numbers or millisecond terms. Example: "When you record live and hit a pad a little before the beat, the keyboard puts the note on the step before the one you meant, so your recording sounds a bit early. I can make it snap to the closest step instead." This rule applies only to text for the owner. Docs, logs and commits stay in asd-ste.
- 2026-10-16 19:25 (owner): Do not ask the owner to approve a change that has no visible, audible or playable effect. Decide and continue. Ask only when the change affects the user experience.
- 2026-10-16 19:10 (owner, from Optimizer items C3 and B3): In the XS, S and B faces, only the lower-case letters a b d k m p r s t x z keep their own glyphs. Each other lower-case letter shows as its capital letter, also in sample names. The characters ` { | } ~ show as "?". Symbols outside the font show as ?. Owner approved 2026-10-16 19:25. See BEHAVIOR.md GEN-55.
- 2026-10-16 19:02 (owner): docs and logs hold only development information (agent rule 28).

## Installs and safety

- Never touch the bootloader. Never risk a brick. Run every install in the foreground, never in the background.
- Before every install, back up the imported sample slots. After the install, compare the slot CRCs. The user has 5 imported loops (3 in slot 1, 2 in slot 2). Backups are in slot_backups/.
- 2026-10-16: A verified build installs with no question to the user. Before each install, the user gets a notice with the commit hash, and the builder waits for the go from the parent agent. After the install, the user gets the commit hash and the CRC result.
- 2026-10-16: Report UX changes at once. Some changes can affect the user experience: latency, packet or MTU size, memory margins, sample length, connection behaviour. When the builder decides on such a change, it sends a short note to the parent agent before it continues. The parent agent tells the user.
- 2026-10-16 02:11: Generic work needs no approval. This is builds, tests, commits, pushes to box, doc updates and merges on the box. Notify the user first before anything that can change how the device works or feels. Examples: installs, new behavior rules, changes that the owner can see, hear or play, flash layout changes, removed features. Safety rules do not change.
- 2026-10-16 17:3x (owner delegated this decision to Firmware Main Builder): The pool limit is 4096 B, per the build.py rule. heap2 (3584 B) is already inside .pool. A release build must turn off the test-26 RAM canary (it writes 0xFF at 0x01C72000-0x01C74000), or keep at least 8192 B of pool free. Enforced: build.sh and the gate require 8192 B pool free (2026-10-16 21:53).

## Repositories and remotes

- 2026-10-16 02:09: Shared bare repository on the box: /workspace/x0x37.git, remote name box. Each agent pushes only its own branch there and fetches from it.
- 2026-10-16 18:09: Code stays only on the box and on the machines of the owner. Never push to GitHub or to any other outside remote. The GitHub origin of the Mac x0x37 repo stays for now. No one pushes to it.
- Box first: build, test and analyze on the box copy. Use the Mac only for installs and device access. Copy the built files to the Mac just before an install. Sync box commits to the Mac with a git bundle (same hashes, no patches). Keep the box and Mac copies in sync (git commits and docs).
- JieLi files stay local on the box and on the Mac and are never committed. Exception: branch refs-oscuro (see below).
- 2026-10-16 17:44 and 18:09: Branch refs-oscuro (ac10280, tip babd349) is a reference. It holds the SMK-37 Oscuro reference firmware (refs/oscuro/). It also holds the JieLi Bluetooth libraries and AC79 SDK headers (refs/jieli/). Not for distribution. Never merge it into any branch. Never package anything from it. Never push it. It stays in the local repositories. The full SDK and the toolchain stay out of git. (Source: refs/jieli/README.md and refs/oscuro/README.md.)
- Pre-push blocks for refs-oscuro: on the box /workspace/x0x37.git and the builder box folders, on the Mac x0x37/.git/hooks/pre-push and flash_review/.git/hooks/pre-push (covers all flash_review worktrees). The hook blocks only a push that contains refs-oscuro or its commits.

## Team rules

- 2026-10-16 02:14: If the Mac does not respond, retry for 5 minutes. Report the Mac as offline only after 5 minutes of no response.
- 2026-10-16 02:28: Posts and agent messages hold only results, blockers and decisions, in two sentences or fewer.
- 2026-10-16 02:43: Docs, logs and commits follow the asd-ste skill (/home/box/agent-data/workflows/asd-ste/SKILL.md). Messages to agents follow the caveman skill (/home/box/agent-data/workflows/caveman/SKILL.md). The caveman skill applies to agent-to-agent messages only. If they conflict, asd-ste wins. (2026-10-16 21:57, Orchestrator confirmed: messages to the owner follow the 20:12 owner entry in "How to work with the user".)
- 2026-10-16 13:03: Do not ask the user for approval to start a helper subagent. Start it.
- 2026-10-16 22:00 (owner): All planned changes go on one working branch, integration. One commit for each change, with a host test (affected tests only, time scale 20). One full gate at the end of the batch. Split or bisect only if that gate fails. Merge Integrator owns the branch order and the batch gate. Builders fetch and rebase before each commit and change only the files of their lane. The install is the batch hash with gate PASS. This replaces the gate for each lane (agent rule 39).
- 2026-10-16 22:02 (owner): Each agent, not only the builders, does the Decision check before an action that is hard to undo or shared. The check is steps 12 to 16 of the x0x37-builder-loop skill (evidence, damage, undo, cause, size), with its red-flag words (agent rule 40). Changes to the skill go through Orchestrator.
- 2026-10-16 22:03 (owner): No idle waiting. While you wait for a gate, a review, a merge or an owner reply, do the next task that does not need that result. Only hardware work and edits to the same shared file stay one at a time (agent rule 41).

## Hardware and targets

- M-Vave SMK-37 (Pro and Elite) with a JieLi AC79 chip. The unit of the user is an Elite.
- Only the SMK-37 build matters. The FM-1 build is not maintained. The user accepts its XIP overflow (about 650 B) (LIM-12).
- The user knows the SMK-37 port by genkigenki. This project has its own source.

## Audio and samples

- Use 44.1 kHz wherever possible, at all times. This includes factory breaks and imported samples.
- No quality cuts and no length cuts. No halved sample rates. Factory breaks stay 2 bars at 44.1 kHz.
- IMA ADPCM at 44.1 kHz is the preferred storage format for all samples. In a blind A/B test the user heard no difference for the 909 ride, crash and hi-hat.
- Keep the imported-sample memory. Space-saving work must not take from sample storage. Use freed flash for the imported-sample (break) slots. Whether to keep spare room for growth is not decided yet.
- Imported samples are for BREAK mode only.
- The installer backs up imported sample slots automatically before an install and restores them after (for example when the flash layout changes).

## Clip mode (APC40 mkII emulation for Loopy Pro)

- OCT- moves the clip view down. OCT+ moves the clip view up.
- Keep the existing pad-and-key blocking behaviour (a pad blocks the keybed white key) as it is.
- Pad brightness stays at the previous level. Do not add pad dimming or a pad brightness floor.
- LCD clip colours: keep the exact hue when a colour becomes brighter, with no hue drift (for example, blue must not go toward purple). The user likes the current red. The approved table is the OKLCH lift with the "strong" hue fix for 0x32 (0x325D) and 0x33 (0x3A17). tools/apc_pad_colors.py makes the table in apc40_lcd565.h. It must go into the firmware after the BLE test.
- Loopy Pro start/stop can also start the X0X sequencer, but it must not conflict with external MIDI clock sync.
- The usb.c APC identity reply that is not gated is not a problem. Leave it.
- FREE PLAY: keep it and document it as it is.

## Clip mode keybed, arp, chord and scale (decided 2026-10-16)

- 2026-10-16 00:45: The user approved this change outside clip mode. When the external MIDI clock stops (0.5 s with no tick), the internal tempo takes the last measured external tempo (whole BPM). Nothing must jump. This applies to the main sequencer too.
- In clip mode, the keybed sends MIDI notes to Loopy Pro only. It does not control the X0X UI. A held pad still blocks its white key.
- The note path in clip mode is: key, then SCALE, then CHORD, then ARP, then the CLIP KEYS channel on USB and Bluetooth MIDI.
- GLO > MIDI has CLIP KEYS CH: channel 2 to 15, default 2. Channels 1 and 16 stay free for the APC40 emulation.
- The new settings are new bytes at the end of the settings. Older projects load the defaults. Do not use rsv_bright.
- The arp follows the incoming MIDI clock when a tick came in the last 0.5 s. Else it uses the internal tempo. It also runs when Loopy does not play. MIDI Start resets it. The arp sends no clock.
- Send only the processed notes. The default scale snap is NEAR.
- Knob 1 is always MASTER volume. (First decision: knobs 2 to 5 set the mode that has the knobs. Branch clip-keys ce5236e now uses knobs 2-8 for CC 48-54 and the SCALE, CHORD and ARP pages for the settings: BEHAVIOR.md CLP-63, CLP-73, CLP-75.)
- GLO opens on the LCD in clip mode. The clip grid comes back when the menu closes.
- In clip mode, the mode chip on the LCD reads "LP".

## Bluetooth MIDI

- MIDI only. No Bluetooth audio.
- The firmware, including any Bluetooth stack, must be freely shareable under an open license.
- Before you use licensed third-party code, check if it can be rebuilt in a legally distinct way (for example from public specifications).
- JieLi's Bluetooth controller (btctrler.a) is closed. Do not bundle JieLi files. The computer of the user downloads them from the official source and builds the controller image locally. The controller runs as a separate image (our MPL shim, HCI mailbox, core 1).
- Keep the Bluetooth image in one contiguous flash block.
- Bluetooth settings are in the GLOBAL menu (BLUETOOTH MIDI section). The BT buttons stay as they are, or work only in clip mode.
- The preferred delivery is an HTML web installer that gets the needed files automatically, with no toolchain for end users if possible.
- 2026-10-16 00:08: Bluetooth: use the BLE-only host of JieLi (fetched from the official SDK and built locally, never bundled) with SMP bonding for auto-reconnect. Strip classic BT and audio. Sample slot 3 can shrink to about 16 KiB (0.7 s) for the host (0xAB000-0xB6FFF). Bonding keys in an X0X area, never BTIF, USRTRIM, efuse or key_mac. (a38 to a64 used this host. a65 goes back to our own app host with no bonding: BEHAVIOR.md BLE-07.)
- 2026-10-16 18:18: The keyboard sends no connection parameter request. a65 sends none (user approved 18:18). On Oct 15 the request made the iPhone link drop (test 31) and gave a large MIDI delay (test 34). This replaces the 17:41 decision.

## Where things are

- Source tree: x0x37/ (branch smk37). BLE work: flash_review repo, live branch ble-a74. Clip keys: branch clip-keys (box repo).
- Rules: agent-rules.md. Agents and lanes: AGENTS_AND_ROOMS.md. Intended behavior: BEHAVIOR.md on integration (x0x37/BEHAVIOR.md on smk37 is a synced copy). Change log: x0x37-progress-log.md. Plan: ROADMAP.md. BLE attempt ledger: BLE_ATTEMPTS.md in the BLE branch.
- BREAK mode guide: BREAK_MODE_GUIDE.md and BREAK_MODE_GUIDE.pdf.
- Flash plan: ~/Documents/x0x37/ble_split/flash_plan.md (Mac).

Last update: 2026-10-16.
