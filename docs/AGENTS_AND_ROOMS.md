# x0x37 Agents and Rooms

This file lets another model recreate the multi-agent setup for the x0x37 firmware project. Owner: Docs Keeper. "TO CONFIRM" marks a fact that no project file or agent profile shows. This root file is the main copy. x0x37/docs/AGENTS_AND_ROOMS.md is an exact copy, committed on smk37. Update both together.

Last updated: 2026-10-16 19:40 (Vladivostok, UTC+10).

## Agents

### Orchestrator
- ID: 6816f054-4f75-496c-b5a8-67dc1a4daf99
- Purpose: Primary bot. Routes tasks, enforces project rules and simple-English style.
- Never: Installs firmware itself. (TO CONFIRM: no written rule found in the project files.)
- Rooms: X0X37 Loopy, Integration, Feature Design, User Test.

### Firmware Main Builder
- ID: a02ff4ed-d19b-42fc-8286-d5854da64a2a
- Purpose: Builds, debugs and installs custom firmware. Owns the Bluetooth/JieLi work on the live BLE lane ble-a65. Only agent allowed to install (with slot backups, CRC compare and a notice before the install).
- Never: Writes the bootloader. Installs in the background. Installs without a slot backup and a CRC compare.
- Worktrees (Mac, flash_review repo): /private/tmp/a65wt (ble-a65, the live lane), /private/tmp/a63wt (ble-a56), /private/tmp/refswt (refs-oscuro), flash_review (ble-a51), wt-a55 (ble-a55).
- Box copy: /workspace/x0x37-firmware-main-builder (a git repository). Commits there, then syncs the commits to the Mac repo before each install.
- Rooms: Loopy, Integration, Feature Design, User Test.

### Second Builder
- ID: d9628bfa-aaf6-4aa0-81b3-db56618755f2
- Purpose: Builds clip-mode keybed, scale/chord/arp, tempo carry-over on clip-keys. (TO CONFIRM: the agent profile has no description.)
- Never: Installs. Does not edit the flash_review tree (it belongs to Firmware Main Builder).
- Worktree: /Users/mac/Documents/x0x37/flash_review_clipkeys (branch clip-keys).
- Box copy: /workspace/x0x37-second-builder. Commits there, then syncs to the Mac branch clip-keys.
- Rooms: Loopy, Feature Design, User Test.

### Firmware Q&A
- ID: 421cc513-a96f-4a66-b299-af00df87fbf4
- Purpose: Watches branches. Answers questions about firmware and branches.
- Never: Installs or changes code.
- Rooms: Loopy, Integration.

### Merge Integrator
- ID: 6d5025ab-17f1-4d00-abb1-a1475550e792
- Purpose: Merges branches into the integration branch on commits. Runs the build and unit tests. Reports conflicts early in Integration and Loopy. Keeps caches and worktrees clean. Writes merge-monitor-log.md.
- Never: Installs firmware. Merges or pushes refs-oscuro. Pushes to GitHub or any outside remote.
- Worktree: /Users/mac/Documents/x0x37/flash_review_integration (branch integration).
- Rooms: Loopy, Integration.

### Build and Test Runner
- ID: a35e97c4-ec2a-4d3b-bea2-e32d8aedaa97
- Purpose: Headless builds, unit tests (smk_clip_keys_test etc.), size and flash-map checks. Keeps caches warm.
- Never: Installs firmware.
- Rooms: Loopy, Integration.

### Release Packager
- ID: 47f0d1b2-8691-46f3-9c06-c99fd805761c
- Purpose: Packages passing merged builds with CRCs, slot backup paths, and plain-English changelog.
- Never: Installs (TO CONFIRM: implied by the profile, not written). Packages anything from refs-oscuro. Pushes refs-oscuro. Pushes to GitHub or any outside remote.
- Rooms: Integration.

### Docs Keeper
- ID: ffd7fca0-6687-4102-bc74-4dda79e8604a
- Purpose: Keeps BEHAVIOR.md, USER_PREFERENCES.md, ROADMAP.md, AGENTS_AND_ROOMS.md and the progress log in sync. Handles rules 23 to 25 and 28.
- Never: Installs or changes firmware code.
- Box folder: /workspace/x0x37-docs-keeper.
- Rooms: Feature Design.

### Test Plan Writer
- ID: 64d26138-589a-454f-90c9-389404c9c990
- Purpose: Writes short hardware test checklists for meaningful changes. Posts them in User Test.
- Never: Installs or changes code. (TO CONFIRM: implied, not written.)
- Rooms: User Test.

### Optimizer
- ID: e9e30685-7138-4a21-8945-13dffc12afc9
- Purpose: Suggests optimizations that are safe for the user experience to the lane owners.
- Never: Installs firmware. Commits to the branch of another agent.
- Rooms: TO CONFIRM.

Note: the iChord agents belong to a different project. They are not part of this setup.

## Agent identities and lanes

Each task has one owner. Find the owner in this table. Heavy agents write code or run builds. Light agents write docs, do checks or give answers.

| Agent | Owns (only this agent) | Does not do | Hands off to | Cost tier |
|---|---|---|---|---|
| Orchestrator | Rule checks. Task assignment. Room order. | Build, write code or install. | Owner of the task, from this table. | Light |
| Firmware Main Builder | Live BLE lane ble-a65. All firmware installs. | Edit clip-keys. Write the bootloader. | Merge Integrator (commits). Test Plan Writer (install changes). Docs Keeper (logs). | Heavy |
| Second Builder | Branch clip-keys (clip mode, scale, chord, arp, tempo). | Install. Edit flash_review or the BLE branches (ble-a65). | Merge Integrator (commits). Docs Keeper (logs). | Heavy |
| Firmware Q&A | Answers about firmware and branches. | Commit, change code or install. | Owner of the topic, from this table. | Light |
| Merge Integrator | Branch integration. Merges and merge conflicts. merge-monitor-log.md. | Install. Write feature code. Merge or push refs-oscuro. Push to an outside remote. | Build and Test Runner (merged commit). | Heavy |
| Build and Test Runner | Headless builds, unit tests, size and flash-map checks. | Install. Write feature code. | Release Packager (pass). Builder of the branch (fail). | Heavy |
| Release Packager | Release packages with CRCs, slot backup paths and changelogs. | Install. Change code. Package or push refs-oscuro. Push to an outside remote. | Firmware Main Builder (install). Docs Keeper (logs). | Light |
| Docs Keeper | BEHAVIOR.md, USER_PREFERENCES.md, ROADMAP.md, this file, progress log. | Install. Change firmware code. | Orchestrator (rule changes). | Light |
| Test Plan Writer | Hardware test checklists in User Test. | Install. Change code. | Firmware Main Builder (install). Docs Keeper (results). | Light |
| Optimizer | Suggestions for optimizations that are safe for the user experience. | Install. Commit to the branch of another agent. | Owner of the lane, from this table. | Light |

## Rooms (max 6 agents each)

All room IDs are TO CONFIRM: the owner recreates the rooms. A room can have a maximum of 6 agents.

| Room | Room ID | Agents now | Full? |
|---|---|---|---|
| X0X37 Loopy | TO CONFIRM (old ID 360e3fb0-f335-40f0-8824-89d953a17837) | 6 | Yes |
| X0X37 Integration | TO CONFIRM (old ID 3fb43bfe-fd4c-4130-8cbe-55bb8461ce44) | 6 | Yes |
| X0X37 Feature Design | TO CONFIRM (old ID 0f264707-d2fd-4cf4-90ce-5a4916ef9d06) | 4 | No (2 free) |
| X0X37 User Test | TO CONFIRM (old ID 4bfa62f7-a67e-4703-84dc-b8a97230ac7e) | 4 | No (2 free) |

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

Agent rules: agent-rules.md (31 rules). The rules below are the team rules from agent-rules.md, USER_PREFERENCES.md and the owner.

Work and posts:
- Rule 23: if you change intended behavior, update BEHAVIOR.md in the same task.
- Rule 24: read USER_PREFERENCES.md before you start. Add each new lasting user decision to it.
- Rule 25: when a step finishes or a plan changes, update ROADMAP.md.
- Rule 26: if you are blocked or a task can run in parallel, post a delegation request in the correct room. Use @Name for the owner. Give the task, the inputs (commit, branch or file), the done criteria and the deadline, in two sentences or fewer. The owner acknowledges, works in its own box folder /workspace/x0x37-<agent>, and replies with the result and the commit hash. Do not do the same work as the owner at the same time.
- Rule 27: each task has one owner (lanes table). Do not work outside your lane. Give each task to the lightest agent that can do it. If you have no result, blocker or decision, do not post.
- Rule 28: docs and logs hold only information that matters for development: behavior, decisions, rules, the attempt results that matter, open items, and how to build, install and test. Do not write chatter, repeated status, process narration or acknowledgements.
- Rule 29: before an install notice, the gate must report PASS for that exact commit hash.
- Rule 30: keep a tag last-good-hw on the newest build that passed a device test. After 2 failed device tests in a row, bisect from last-good-hw.
- Rule 31: tests read the shared reference files in /workspace/x0x37-refs/schwung.
- Post only results, blockers and decisions, in two sentences or fewer.
- Docs, logs and commits follow the asd-ste skill (/home/box/agent-data/workflows/asd-ste/SKILL.md). Messages to agents follow the caveman skill (/home/box/agent-data/workflows/caveman/SKILL.md). The caveman skill applies to every chat post and room post. If they conflict, asd-ste wins.
- Generic work is approved automatically (user, 2026-10-16 02:11). Notify the user first before anything that changes how the device works or feels. Changes outside clip mode need the explicit approval of the user.
- If the Mac does not respond, retry for 5 minutes. Report it offline only after 5 minutes.

Build and install:
- Build, test and analyze on the box copy. Use the Mac only for installs and device access.
- Box build paths: JieLi toolchain /home/box/.jieli/toolchain, AC79 SDK /home/box/fw-AC79_AIoT_SDK (agent rule 12).
- All agents share one box. Each agent uses its own box folder, /workspace/x0x37-<agent-name>. Never use the folder of another agent. Each builder works only in its own worktree.
- Commit early so the Merge Integrator has a base. Build and Test Runner checks every commit. Merge Integrator merges as soon as both sides have a commit.
- Builders commit on the box, then sync the commits to the Mac repo with a git bundle before an install (same hashes, no patches).
- Only Firmware Main Builder installs. Before each install: back up the sample slots, notify the user with the exact commit hash, and wait for the go from the parent agent. Run the install in the foreground. After the install: compare the slot CRCs and send the commit hash and the CRC result. Never write the bootloader. A verified build installs with no question to the user. Slot backups are in slot_backups/.

Repositories:
- Shared bare repository on the box: /workspace/x0x37.git, remote name box. Each agent pushes only its own branch there and fetches from it (user, 2026-10-16 02:09).
- Code stays only on the box and on the machines of the owner. Never push to GitHub or any other outside remote (owner, 2026-10-16 18:09). The GitHub origin of the Mac x0x37 repo stays, and no one pushes to it.
- Write commit messages in Simplified Technical English.
- Keep the JieLi files local on the box and on the Mac. Do not commit them. Exception: branch refs-oscuro (ac10280, tip babd349) is a reference (user, 2026-10-16 17:44 and 17:45). It holds the Oscuro reference firmware (refs/oscuro/) and the JieLi libraries and AC79 SDK headers (refs/jieli/). Not for distribution. Never merge, package or push refs-oscuro (owner, 2026-10-16 18:09). The full SDK and the toolchain stay out of git.
- Pre-push blocks for refs-oscuro: box /workspace/x0x37.git and the builder box folders. Mac ~/Documents/x0x37/x0x37/.git/hooks/pre-push and ~/Documents/x0x37/flash_review/.git/hooks/pre-push (covers all flash_review worktrees). The hook blocks only a push that contains refs-oscuro or its commits.

## Current project context

- Mac: Mac-mini.local, /Users/mac/Documents/x0x37. Main source tree: x0x37 (branch smk37, 4ffcd2d, docs commits on top).
- BLE: live lane ble-a65 (tip 4d1608a). Tag last-good-hw = 94285b5 (a67, iPhone PASS 19:18). Side branches, built, not installed: ble-a68-opt a1d8ea1, ble-a69-readback 75c42e1, ble-a70 77fa067, ble-a71 9ef7bc2. a65 (ca1d4d2) goes back to the app host from fb6b1b0. The keyboard runs a67 (94285b5, installed 19:00). History: ble-a54 to ble-a56 (a54 to a64, the CTL_HOST image host), ble-stage3 (to a47), ble-a48 to ble-a51.
- Clip keys and integration: clip-keys-a65 at 81f6418. integration at ceaac0a (merges of clip-keys-a65 81f6418 and ble-a65 4d1608a). Box archive/clip-keys keeps ce5236e. The Mac clip-keys (c2f7906) is old. integration-stale-02ae8b8 keeps the stale 17:34 merges.
- fix-bass303 (9703c4f): owner TO CONFIRM.
- refs-oscuro (ac10280, tip babd349): reference only, see the repository rules.
- main_fm1.c, plat_fm1.c (plat_midi_out) and BEHAVIOR.md BLE-09 can change on both BLE and clip-keys branches. Merge with care.
- Files to keep: agent-rules.md, USER_PREFERENCES.md, ROADMAP.md, x0x37-progress-log.md, merge-monitor-log.md, x0x37/BEHAVIOR.md and BLE_ATTEMPTS.md (the BLE ledger in the BLE branch). Do not delete slot_backups/.

## How to recreate
1. Create the ten agents above with the exact names and the purposes listed.
2. Create the four rooms and add the members listed (respect the 6-agent limit).
3. Give each agent its standing duty via a prompt.
4. Post the coordination rules in the rooms.
5. Point everyone at this file and the project folder.

## Change log
- 2026-10-16 01:35: file made: agent IDs, worktrees, never-do items, 6-agent room limit, rules 23 to 25, install rules.
- 2026-10-16 02:08 to 02:29: box-first work, own box folder, git bundle sync, shared box repository, auto-approval. Mac offline retry 5 min, commit hash in each install notice, short posts.
- 2026-10-16 02:43 to 02:45: rule 26 (delegation), rule 27 and the lanes table. Merge Integrator owns merge-monitor-log.md.
- 2026-10-16 17:45: new agent IDs. All room IDs TO CONFIRM.
- 2026-10-16 17:44 to 18:15: refs-oscuro rules (never merge, package or push), JieLi-files exception, no outside remotes, pre-push hooks on the box and the Mac.
- 2026-10-16 18:47: live BLE lane is ble-a65.
- 2026-10-16 18:52: new agent Optimizer.
- 2026-10-16 19:02: rule 28 (docs hold only development information). File trimmed to rule 28.
- 2026-10-16 19:13: rules 29 to 31 (gate PASS before an install notice, tag last-good-hw, shared schwung references). Box toolchain and SDK paths.
- 2026-10-16 19:39: branch tips (ble-a65 4d1608a, clip-keys-a65 81f6418, integration ceaac0a, side branches a68 to a71).
