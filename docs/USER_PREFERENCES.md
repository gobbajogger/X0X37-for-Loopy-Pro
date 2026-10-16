# X0X37 project: owner preferences and decisions

Read this file before you work on the X0X37 project. Also obey the agent rules (rules/core.md). This Mac root file is the main copy, and x0x37/docs/USER_PREFERENCES.md on smk37 is an exact copy. The reference for the intended behavior is BEHAVIOR.md on the branch integration. The smk37 copy x0x37/BEHAVIOR.md is a synced copy of it.

## How to work with the owner

- Talk plainly and briefly. Give the result first. Explain hardware and code findings in simple words.
- Do the work. Ask only when a wrong choice has real results or when the owner must decide.
- Only a change that the owner can see, hear or play needs the approval of the owner first (see the 19:25 entry). Decide the other changes and continue.
- Do not change release scripts unless the owner asks.
- In build-test mode, do not install fallback or recovery builds only to make the keyboard playable. An example is a build with no Bluetooth. Install the fixed build directly. The owner tests the keyboard and does not make music with it.
- Commits: author genkigenki <3080081+genkigenki@users.noreply.github.com>, messages in ASD-STE100 (asd-ste skill). Log each change in x0x37-progress-log.md.
- 2026-10-16 18:41 (owner): Optimize without cutting features. Font size is not a feature.
- 2026-10-16 20:12 (owner):
  - Questions and explanations to the owner use plain words about what the owner hears, feels and does on the keyboard. Explain each button and each term. Do not use code words, step numbers or millisecond terms.
  - Example: "When you record live and hit a pad a little before the beat, the keyboard puts the note on the step before the one you meant, so your recording sounds a bit early. I can make it snap to the closest step instead."
  - This rule applies only to text for the owner. Docs, logs and commits stay in asd-ste. (Extended 2026-10-16 23:11: these messages are also in ASD-STE100. See the 23:11 entry.)
- 2026-10-17 00:05 (owner): Make each message to the owner as short as possible, and keep every fact (caveman length). Use full, simple ASD-STE100 words: no shorthand, no IDs and no hashes, unless the owner needs them (agent rule 13).
- 2026-10-16 19:25 (owner): Do not ask the owner to approve a change that has no visible, audible or playable effect. Decide and continue. Ask only when the change affects the experience of the owner.
- 2026-10-16 19:10 (owner, from Optimizer items C3 and B3):
  - In the XS, S and B faces, only these lower-case letters keep their own glyphs: `a b d k m p r s t x z`. Each other lower-case letter shows as its capital letter, also in sample names.
  - The characters ` { | } ~ show as "?". Symbols outside the font show as ?. The owner approved this on 2026-10-16 at 19:25. See BEHAVIOR.md GEN-55.
- 2026-10-16 19:02 (owner): Docs and logs hold only development information (agent rule 12).

## Installs and safety

- Never touch the bootloader. Never risk a brick. Run each install in the foreground, never in the background.
- Before each install, back up the loop slots. After the install, compare the slot CRCs. The owner has 5 imported loops (3 in slot 1, 2 in slot 2). The backups are in slot_backups/.
- 2026-10-16: A verified build installs with no question to the owner. Before each install, send the owner a notice. Give the commit hash and the result file of the gate PASS row for the exact tree (/workspace/x0x37-results/index.md). A "go" in a message does not replace that row (agent rule 6). After the install, give the owner the commit hash and the CRC result.
- 2026-10-16: Report UX changes immediately. Some changes can affect the experience of the owner: latency, packet or MTU size, memory margins, sample length and connection behavior. If the builder decides on such a change, it sends a short note to Orchestrator before it continues. Orchestrator tells the owner.
- 2026-10-16 02:11:
  - Generic work needs no approval. Generic work is builds, tests, commits, pushes to the box, doc updates and merges on the box. Tell the owner first before a change that can change how the device works or feels.
  - Examples: installs, new behavior rules, changes that the owner can see, hear or play, flash layout changes, removed features. The safety rules do not change.
  - The 19:25 entry replaces these examples. Only a change that the owner can see, hear or play needs approval. The install notice stays (agent rule 5).
- 2026-10-16 17:3x (the owner gave this decision to Firmware Main Builder):
  - The pool limit is 4096 B, from the build.py rule. heap2 (3584 B) is already inside .pool. A release build must turn off the test-26 RAM canary (it writes 0xFF at 0x01C72000-0x01C74000), or keep at least 8192 B of free pool.
  - Enforced: build.sh and the gate require 8192 B of free pool (2026-10-16 21:53). The 8192 B limit replaces the 4096 B limit.

## Repositories and remotes

- 2026-10-16 02:09: The shared bare repository on the box is /workspace/x0x37.git, remote name box. Each agent pushes only its own branch there and fetches from it. Lanes push only their lane/<agent>-<topic> branch, never integration. Merge Integrator moves each commit onto integration (agent rules 10 and 11, 23:15 entry).
- 2026-10-16 18:09: Code stays only on the box and on the machines of the owner. Never push to GitHub or to any other outside remote. The GitHub origin of the Mac x0x37 repo stays at this time. Nobody pushes to it.
- Box first: build, test and analyze on the box copy. Use the Mac only for installs and device access. Copy the built files to the Mac just before an install. Sync box commits to the Mac with a git bundle (same hashes, no patches). Keep the box and Mac copies in sync (git commits and docs).
- Keep the JieLi files on the box and on the Mac. Do not commit them. Exception: branch refs-oscuro (see below).
- 2026-10-16 17:44 and 18:09:
  - Branch refs-oscuro (ac10280, tip babd349) is a reference. It holds the SMK-37 Oscuro reference firmware (refs/oscuro/). It also holds the JieLi Bluetooth libraries and AC79 SDK headers (refs/jieli/). It is not for distribution.
  - Never merge it into a branch. Never package a part of it. Never push it. It stays in the local repositories.
  - The full SDK and the toolchain stay out of git. (Source: refs/jieli/README.md and refs/oscuro/README.md.)
- Pre-push blocks for refs-oscuro: on the box /workspace/x0x37.git and the builder box folders, on the Mac x0x37/.git/hooks/pre-push and flash_review/.git/hooks/pre-push (for all flash_review worktrees). The hook blocks only a push that contains refs-oscuro or its commits.

## Team rules

- 2026-10-16 02:14: If the Mac does not answer, try again for 5 minutes. Report the Mac offline only after 5 minutes with no answer.
- 2026-10-16 02:28: Posts and agent messages hold only results, blockers and decisions, in two sentences or fewer. "Idle, no task" counts as a blocker: tell Orchestrator (agent rule 3).
- 2026-10-16 02:43:
  - Docs, logs and commits obey the asd-ste skill (/home/box/agent-data/workflows/asd-ste/SKILL.md). Messages to agents obey the caveman skill (/home/box/agent-data/workflows/caveman/SKILL.md).
  - The caveman skill applies only to agent-to-agent messages, including task messages. If a text is in the scope of asd-ste, asd-ste wins.
  - (2026-10-16 21:57, Orchestrator confirmed: messages to the owner obey the 20:12 owner entry in "How to work with the owner". From 23:11, they are also in ASD-STE100.)
- 2026-10-16 13:03: Do not ask the owner for approval to start a helper subagent. Start it.
- 2026-10-16 22:00 (owner):
  - All planned changes go on one working branch, integration. Make one commit and one host test for each change. Run only the affected tests. Use time scale 20 only when the code reads X0X_TEST_TIME_SCALE (agent rule 9).
  - Run one gate at the end of the batch. Split or bisect only if that gate fails. Merge Integrator owns the branch order and the gate of the batch.
  - Builders fetch and rebase before each commit and change only the files of their lane. The install is the batch hash with a gate PASS. This replaces the gate for each lane (agent rules 6 and 9 to 11).
- 2026-10-16 22:02 (owner): Each agent, not only the builders, does the Decision check before an action that is hard to undo or shared. The check is steps 12 to 16 of the x0x37-builder-loop skill (evidence, damage, undo, cause, size), with its stop words (agent rule 4). Changes to the skill go through Orchestrator. (2026-10-16 23:15: in the core card, the Decision check is steps 6 to 10.)
- 2026-10-16 22:03 (owner): Do not wait idle. While you wait for a gate, a review, a merge or an owner reply, do the next task that does not need that result. Only hardware work and edits to the same shared file stay one at a time (agent rule 3).
- 2026-10-16 22:39 (owner): Record the bug first. Before a fix for a device bug, the owner shows the bug once with INLOG (the inbound MIDI recorder) on. The builder makes a host replay test from that log, then fixes the bug. Do not make guess fixes (agent rule 8).
- 2026-10-16 22:59 (owner): Never build a fix before data and a host test prove that it fixes the cause. If a fix does not help on the device, go back to the start. Name the wrong assumption and get the missing data. Do not try a second guess (agent rule 8).
- 2026-10-16 23:11 (owner): Write each message to the owner in ASD-STE100, also with the plain words of the 20:12 entry. Do not describe a change to the owner before you verify it (agent rule 13, asd-ste skill).
- 2026-10-16 23:15 (Orchestrator decision): Integration is the one working branch. Lanes are short-lived branches named lane/<agent>-<topic> (mb, sb, opt), for example lane/mb-clock. Merge Integrator moves each commit onto integration (agent rule 5).
- 2026-10-16 23:15 (Orchestrator decision): Orchestrator is the only route for questions to the owner. If an agent quotes an owner report with the build, the report is valid in any chat (agent rule 13).
- 2026-10-16 23:15 (Orchestrator decision): Send one decision message for each item. Give it a tag "supersedes <time>" when it replaces an earlier decision (agent rule 4B).
- 2026-10-16 23:19 (Orchestrator decision): Install only with fm1_install.py --commit and the guard require_gate_pass.sh. Never use the web installer for a team install (agent rule 6).
- 2026-10-16 23:29 (Orchestrator decision): The install guard of rule 6 applies from batch 3 (the batch with the install guard). Until then, Firmware Main Builder reads the index row and the result file before the install. Owner quotes stay word for word inside quotation marks, and STE does not apply to them.
- 2026-10-17 00:01 (Orchestrator decision, owner approved 00:00): Agent rules version 3. rules/core.md has rules 1, 2 and 4 and an index of the rule files. Read the core at the start of each task, and open each other rule file only at its step. Each rule number in this file is a version 3 number.
- 2026-10-16 23:36 (Orchestrator decision): From integration d6ae80d, builders edit the STE text of BEHAVIOR.md, and quickcheck checks it (ste_strict).
- 2026-10-16 23:36 (Orchestrator decision): Name a batch only by its number and its gated hash, for example batch 1 (9a733fe). Do not name a batch by a branch name.
- 2026-10-16 23:32 (owner): Merge the small roles.
  - Merge Integrator does the release packaging. It packages only a hash with a gate PASS and a device PASS, with the same checks as before (CRCs, loop slot backup paths).
  - Docs Keeper writes the owner test checklists and sends them to Orchestrator. A checklist has only the changed items, in plain words and ASD-STE100. The owner copy has no agent notes.
  - Release Packager and Test Plan Writer retire after Docs Keeper logs the batch-1 checklist answers.

## Hardware and targets

- The target is the M-Vave SMK-37 (Pro and Elite) with a JieLi AC79 chip. The keyboard of the owner is an Elite.
- Only the SMK-37 build is important. We do not maintain the FM-1 build. The owner accepts its XIP overflow (about 650 B) (LIM-12).
- The owner knows the SMK-37 port by genkigenki. This project has its own source.

## Audio and samples

- Use 44.1 kHz wherever possible, at all times. This includes the factory breaks and the imported loops.
- Do not cut quality or length. Do not halve sample rates. The factory breaks stay 2 bars at 44.1 kHz.
- IMA ADPCM at 44.1 kHz is the preferred storage format for all samples. In a blind A/B test, the owner heard no difference for the 909 ride, crash and hi-hat.
- Keep the memory for the imported loops. Work that saves space must not take from the loop storage. Use free flash for the loop slots. We did not decide yet if we keep spare space for growth.
- The imported loops are for BREAK mode only.
- The installer backs up the loop slots automatically before an install and restores them after it (for example when the flash layout changes).

## Clip mode (APC40 mkII emulation for Loopy Pro)

- OCT- moves the clip view down. OCT+ moves the clip view up.
- Keep the current pad-and-key blocking behavior (a pad blocks the white key of the keybed).
- The pad brightness stays at the previous level. Do not add pad dimming or a pad brightness floor.
- LCD clip colours:
  - Keep the exact hue when a colour becomes brighter. Do not let the hue drift (for example, blue must not go toward purple). The owner likes the current red.
  - The approved table is the OKLCH lift with the "strong" hue fix for 0x32 (0x325D) and 0x33 (0x3A17). tools/apc_pad_colors.py makes the table in apc40_lcd565.h. It must go into the firmware after the Bluetooth test.
  - Updated 2026-10-16 23:34: the LCD colours are now the plain Akai colours as RGB565, from the Akai CSV (tools/apc40mk2_akai_protocol_v1.2.csv, CLP-05). They have no lift and no hue fix, so X0X does not use the OKLCH table. The plan puts pad colour calibration (CLP-79) in batch 3.
- Loopy Pro start/stop can also start the X0X sequencer, but it must not conflict with the external MIDI clock sync.
- The usb.c APC identity reply has no gate check, and this is not a problem. Leave it.
- FREE PLAY: keep it, and document it as it is.
- 2026-10-17 00:01 (owner): Pad calibration is only for development. The release build has X0X_DEV_PADCAL=0 (no PAD COLOURS row), with the pad table that the owner calibrated.

## Clip mode keybed, arp, chord and scale (decided 2026-10-16)

- 2026-10-16 00:45: The owner approved this change outside clip mode. When the external MIDI clock stops (0.5 s with no tick), the internal tempo takes the last measured external tempo (whole BPM). Nothing must jump. This applies to the main sequencer too.
- In clip mode, the keybed sends MIDI notes only to Loopy Pro. It does not control the X0X UI. A held pad still blocks its white key.
- The note path in clip mode is: key, then SCALE, then CHORD, then ARP, then the CLIP KEYS channel on USB and Bluetooth MIDI.
- GLO > MIDI has CLIP KEYS CH: channel 2 to 15, default 2. Channels 1 and 16 stay free for the APC40 emulation.
- The new settings are new bytes at the end of the settings. Older projects load the defaults. Do not use rsv_bright.
- The arp follows the incoming MIDI clock when a tick came in the last 0.5 s. If no tick came, it uses the internal tempo. It also runs when Loopy does not play. MIDI Start resets it. The arp sends no clock.
- Send only the processed notes. The default scale snap is NEAR.
- Knob 1 is always the MASTER volume. (First decision: knobs 2 to 5 set the mode that has the knobs. Branch clip-keys ce5236e now uses knobs 2-8 for CC 48-54 and the SCALE, CHORD and ARP pages for the settings: BEHAVIOR.md CLP-63, CLP-73, CLP-75.)
- GLO opens on the LCD in clip mode. The clip grid comes back when the menu closes.
- In clip mode, the mode chip on the LCD shows "LP".

## Bluetooth MIDI

- MIDI only. No Bluetooth audio.
- The firmware, with its Bluetooth stack, must be freely shareable under an open license.
- Before you use licensed third-party code, check if we can make it again in a legally distinct way. An example is code from public specifications.
- JieLi does not publish the source of its Bluetooth controller (btctrler.a). Do not bundle JieLi files. The computer of the end user downloads them from the official source and builds the controller image locally. The controller runs as a separate image (our MPL shim, HCI mailbox, core 1).
- Keep the Bluetooth image in one contiguous flash block.
- The Bluetooth settings are in the GLOBAL menu (BLUETOOTH MIDI section). The BT buttons stay as they are, or work only in clip mode.
- The preferred delivery is an HTML web installer that gets the necessary files automatically. If possible, end users need no toolchain.
- 2026-10-16 00:08:
  - Bluetooth: use the Bluetooth Low Energy host of JieLi with SMP pairing for auto-reconnect. Get it from the official SDK, build it locally and never bundle it. Remove classic Bluetooth and audio.
  - Loop slot 3 can shrink to about 16 KiB (0.7 s) for the host (0xAB000-0xB6FFF). Keep the pairing keys in an X0X area, never in BTIF, USRTRIM, efuse or key_mac.
  - (a38 to a64 used this host. a65 goes back to our own app host with no pairing: BEHAVIOR.md BLE-07.)
- 2026-10-16 18:18: The keyboard sends no connection parameter request. a65 sends none (the owner approved this at 18:18). On Oct 15, the request made the iPhone link drop (test 31) and gave a large MIDI delay (test 34). This replaces the 17:41 decision.

## Where things are

- Source tree: x0x37/ (branch smk37). Firmware work: one working branch, integration (batch flow, agent rules 6 and 9 to 11), box repo /workspace/x0x37.git and Mac worktree flash_review_integration. The keyboard runs integration 9a733fe (batch 1, installed 22:25).
- Rules: rules/core.md (agent-rules.md points to it). Agents and lanes: AGENTS_AND_ROOMS.md. Intended behavior: BEHAVIOR.md on integration (x0x37/BEHAVIOR.md on smk37 is a synced copy). Change log: x0x37-progress-log.md. Plan: ROADMAP.md. Bluetooth attempt ledger: BLE_ATTEMPTS.md in the Bluetooth branch.
- BREAK mode guide: BREAK_MODE_GUIDE.md and BREAK_MODE_GUIDE.pdf.
- Flash plan: ~/Documents/x0x37/ble_split/flash_plan.md (Mac).

Last update: 2026-10-17 00:06.
