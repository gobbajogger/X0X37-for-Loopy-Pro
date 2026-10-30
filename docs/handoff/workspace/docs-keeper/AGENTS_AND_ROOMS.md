Rule 1 (owner, explicit, 2026-10-31): Don't do stupid work. Only do work that moves the build forward.

Rule 2 (owner, explicit, 2026-10-31): Always start a new Codex/ChatGPT thread for each job. Never reuse old threads.

Rule 3 (owner, explicit, 2026-10-31): Speed first. Fix blockers (like forbidden files) first, run tests and builds in parallel, and cut anything the build does not need.

# x0x37 Agents and Rooms

This file lets another model recreate the multi-agent setup for the x0x37 firmware project.

Last updated: 2026-10-16 02:15 (Vladivostok, UTC+10) by Docs Keeper.
Owner: Docs Keeper (living document). Docs Keeper updates this file when an agent, a room or a rule changes.
"TO CONFIRM" marks a fact that no project file or agent profile shows.

## Agents

### Orchestrator
- ID: 2ffad88e-a21e-45d2-a50d-545c22bcdd01
- Purpose: Primary bot. Routes tasks, enforces project rules and simple-English style.
- Never: Installs firmware itself. (TO CONFIRM: no written rule found in the project files.)
- Rooms: X0X37 Loopy, Integration, Feature Design, User Test.

### Firmware Main Builder
- ID: cd078dcf-119e-4eb7-8a01-bc7cd78bfd48
- Purpose: Builds, debugs and installs custom firmware. Owns Bluetooth/JieLi work on ble-stage3. Only agent allowed to install (with backups + CRC + pre-install heads-up).
- Never: (none specific beyond rules). From USER_PREFERENCES.md: never write the bootloader, never install in the background, never install without a slot backup and a CRC compare.
- Worktree: /Users/mac/Documents/x0x37/flash_review (branch ble-stage3).
- Box copy: /workspace/x0x37-firmware-main-builder. Commits there, then syncs the commits to the Mac repo before each install.
- Rooms: Loopy, Integration, Feature Design, User Test.

### Second Builder
- ID: c05ee676-c48e-4c91-82a7-f72d9e2999ae
- Purpose: Builds clip-mode keybed, scale/chord/arp, tempo carry-over on clip-keys. (TO CONFIRM: the agent profile has no description.)
- Never: Installs. Does not edit the flash_review tree (it belongs to Firmware Main Builder).
- Worktree: /Users/mac/Documents/x0x37/flash_review_clipkeys (branch clip-keys).
- Box copy: /workspace/x0x37-second-builder. Commits there, then syncs to the Mac branch clip-keys.
- Rooms: Loopy, Feature Design, User Test.

### Firmware Q&A
- ID: 0f40e61f-32f1-41b8-8bdf-3f1248fea9ff
- Purpose: Watches branches, maintains merge-monitor-log.md.
- Never: Installs or changes code.
- Rooms: Loopy, Integration.

### Merge Integrator
- ID: 74f84c15-8626-459c-9991-6ae8d1fb3eae
- Purpose: Merges branches into an integration branch on commits. Runs build + unit tests. Surfaces conflicts early. Reports in plain English in Integration and Loopy. Keeps caches and worktrees clean.
- Never: Installs firmware.
- Worktree: /Users/mac/Documents/x0x37/flash_review_integration (branch integration).
- Rooms: Loopy, Integration.

### Build and Test Runner
- ID: 5e1ab02f-7ef4-4d1c-a1c6-e9723c4297f1
- Purpose: Headless builds, unit tests (smk_clip_keys_test etc.), size and flash-map checks. Keeps caches warm.
- Never: Installs firmware.
- Rooms: Loopy, Integration.

### Release Packager
- ID: 80ee3bbf-c4eb-4766-a2f2-2e8630622548
- Purpose: Packages passing merged builds with CRCs, slot backup paths, and plain-English changelog.
- Never: Installs. (TO CONFIRM: implied by the profile, not written.)
- Rooms: Integration.

### Docs Keeper
- ID: a1b0f316-d57b-4658-bab6-cfa9b19e17bb
- Purpose: Keeps BEHAVIOR.md, USER_PREFERENCES.md, ROADMAP.md, progress and merge logs in sync. Handles rules 23-25.
- Never: Installs or changes firmware code.
- Box folder: /workspace/x0x37-docs-keeper.
- Rooms: Feature Design.

### Test Plan Writer
- ID: faf66b58-8e0c-4222-ae06-ac2ca1eb6fa9
- Purpose: Writes short hardware test checklists for meaningful changes. Posts them in User Test.
- Never: Installs or changes code. (TO CONFIRM: implied, not written.)
- Rooms: User Test.

Note: the iChord agents belong to a different project. They are not part of this setup.

## Rooms (max 6 agents each)

Rule: a room can have a maximum of 6 agents. Before you add an agent to a full room, remove one agent or use a different room.

| Room | Room ID | Agents now | Full? |
|---|---|---|---|
| X0X37 Loopy | 360e3fb0-f335-40f0-8824-89d953a17837 | 6 | Yes |
| X0X37 Integration | 3fb43bfe-fd4c-4130-8cbe-55bb8461ce44 | 6 | Yes |
| X0X37 Feature Design | 0f264707-d2fd-4cf4-90ce-5a4916ef9d06 | 4 | No (2 free) |
| X0X37 User Test | 4bfa62f7-a67e-4703-84dc-b8a97230ac7e | 4 | No (2 free) |

### X0X37 Loopy
- Purpose: Main coordination room for the firmware + Loopy work.
- Members: Orchestrator, Firmware Main Builder, Second Builder, Firmware Q&A, Merge Integrator, Build and Test Runner.

### X0X37 Integration
- Purpose: Merges, parallel builds, tests, and packaging only.
- Members: Orchestrator, Merge Integrator, Build and Test Runner, Firmware Main Builder, Firmware Q&A, Release Packager.

### X0X37 Feature Design
- Purpose: Agree behavior rules (BLE-09, MID-07 etc.) before code is written.
- Members: Orchestrator, Firmware Main Builder, Second Builder, Docs Keeper.

### X0X37 User Test
- Purpose: Install notices, test checklists, and user results.
- Members: Orchestrator, Test Plan Writer, Firmware Main Builder, Second Builder.
- Note: User posts results here from the sidebar.

## Coordination rules
- Commit early so the Integrator has a base.
- Build and Test Runner does checks on every commit (parallel, cache-warm).
- Merge Integrator merges as soon as both sides have a commit.
- Only Firmware Main Builder installs. Give a heads-up just before each install so the device can be prepared.
- Use simple English. Keep logs (x0x37-progress-log.md, merge-monitor-log.md) updated.
- Docs Keeper handles paperwork.
- Build, test and analyze on a copy of the project on the box. Use the Mac only for installs and device access.
- If the Mac does not respond, retry for 5 minutes. Report it offline only after 5 minutes.
- All agents share one box. Each agent uses its own box folder, /workspace/x0x37-<agent-name>. Never use another agent's folder.
- Builders commit on the box, then sync the commits to the Mac repo before an install.
- Keep one copy of the project on the box and one on the Mac. Keep the two copies in sync.
- Shared bare repository on the box: /workspace/x0x37.git, remote name box. Each agent pushes only its own branch there and fetches from it. No agent pushes outside the box. (User approved 2026-10-16 02:09.)
- Generic work is approved automatically. Notify the user first before anything that changes how the device works or feels. Safety rules stay. (User, 2026-10-16 02:11.)

More rules from agent-rules.md, USER_PREFERENCES.md and merge-monitor-log.md:
- Rule 23: if you change intended behavior, update BEHAVIOR.md in the same task.
- Rule 24: read USER_PREFERENCES.md before you start. Add each new lasting user decision to it.
- Rule 25: when a step finishes or a plan changes, update ROADMAP.md.
- Before each install, back up the sample slots. After the install, compare the slot CRCs. Backups are in slot_backups/.
- Run each install in the foreground. Never write the bootloader.
- Before each install, notify the user and wait for the go from the parent agent. After the install, send the commit hash and the CRC result.
- A verified build installs with no question to the user (decision of 2026-10-16). The backup, CRC and notice steps stay.
- Do not push outside the box. The only allowed remote is box (/workspace/x0x37.git). Write commit messages in Simplified Technical English.
- Changes outside clip mode need the explicit approval of the user first.
- Keep the JieLi files local on the box and on the Mac. Do not commit them.
- Each builder works only in its own worktree.

## Current project context
- Location: ~/Documents/x0x37
- Base commit: 585d63c
- Active branches: ble-stage3 (Main Builder, flash_review), clip-keys (Second Builder, flash_review_clipkeys)
- Both touch main_fm1.c (careful merge needed)
- Logs to keep: x0x37-progress-log.md, merge-monitor-log.md, USER_PREFERENCES.md, BEHAVIOR.md, ROADMAP.md

More facts (checked 2026-10-16 01:32):
- Machine: Mac-mini.local. Full path: /Users/mac/Documents/x0x37.
- Main source tree: /Users/mac/Documents/x0x37/x0x37 (branch smk37, HEAD 4ffcd2d).
- Worktrees: flash_review (ble-stage3), flash_review_clipkeys (clip-keys), flash_review_integration (integration). All three were at 585d63c. Second Builder had work that was not committed.
- Files that both branches can change: firmware/src/app/main_fm1.c, firmware/src/app/plat_fm1.c (plat_midi_out), BEHAVIOR.md item BLE-09.
- New on clip-keys: firmware/src/app/smk_clip_keys.h, tests/smk_clip_keys_test.c.
- Other files to keep: agent-rules.md (the 25 agent rules), x0x37/BEHAVIOR.md, ble_split/BLE_ATTEMPTS.md (one row for each Bluetooth attempt), slot_backups/ (do not delete).
- A second copy of this setup is at x0x37/docs/AGENTS_AND_ROOMS.md. This root file is the main copy.

## How to recreate
1. Create the nine agents above with the exact names and the purposes listed.
2. Create the four rooms and add the members listed (respect the 6-agent limit).
3. Give each agent its standing duty via a prompt.
4. Post the coordination rules in the rooms.
5. Point everyone at this file and the project folder.

## Change log
- 2026-10-16 01:35, Docs Keeper: added the update line and the owner note. Added agent IDs, worktree paths and more never-do items. Added the 6-agent room rule with counts. Added rules 23 to 25 and the install rules. Added verified project facts. Added TO CONFIRM marks. Nothing was removed.
- 2026-10-16 02:08, Docs Keeper: added box copy paths for the two builders. Added four rules: box-first work, Mac offline retry, one box folder for each agent, commit on the box then sync before an install.
- 2026-10-16 02:09, Docs Keeper: added the two-copy sync rule and the shared box repository rule (/workspace/x0x37.git, remote box). Changed the push rule and the JieLi rule.
- 2026-10-16 02:15, Docs Keeper: added the two-copy sync rule, the box repository rule (user approved 02:09) and the auto-approval rule (02:11). Push rule allows only the box remote. JieLi rule changed. Offline rule is now 5 minutes. Added the Docs Keeper box folder.
