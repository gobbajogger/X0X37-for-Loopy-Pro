# x0x37 Agents and Rooms

This file lets another model recreate the multi-agent setup for the x0x37 firmware project.

Last updated: 2026-10-16 18:47 (Vladivostok, UTC+10) by the doc audit for Docs Keeper.
Owner: Docs Keeper (living document). Docs Keeper updates this file when an agent, a room or a rule changes.
"TO CONFIRM" marks a fact that no project file or agent profile shows.

## Agents

### Orchestrator
- ID: 6816f054-4f75-496c-b5a8-67dc1a4daf99
- Purpose: Primary bot. Routes tasks, enforces project rules and simple-English style.
- Never: Installs firmware itself. (TO CONFIRM: no written rule found in the project files.)
- Rooms: X0X37 Loopy, Integration, Feature Design, User Test.

### Firmware Main Builder
- ID: a02ff4ed-d19b-42fc-8286-d5854da64a2a
- Purpose: Builds, debugs and installs custom firmware. Owns the Bluetooth/JieLi work. The live BLE lane is ble-a65 (ca1d4d2, a65). ble-a56 (a63, a64) and ble-stage3 are history. Only agent allowed to install (with backups + CRC + pre-install heads-up).
- Never: (none specific beyond rules). From USER_PREFERENCES.md: never write the bootloader, never install in the background, never install without a slot backup and a CRC compare.
- Worktrees: /Users/mac/Documents/x0x37/flash_review (branch ble-a51) and /Users/mac/Documents/x0x37/wt-a55 (branch ble-a55). The live BLE lane is ble-a65 (ca1d4d2, a65, from fb6b1b0). ble-a56 (d9844a4 to 0b80994, a63 and a64) is history. The branch ble-stage3 stopped at a43425b (a47).
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
- Purpose: Watches branches. (merge-monitor-log.md moved to Merge Integrator, 2026-10-16 02:44.)
- Never: Installs or changes code.
- Rooms: Loopy, Integration.

### Merge Integrator
- ID: 6d5025ab-17f1-4d00-abb1-a1475550e792
- Purpose: Merges branches into an integration branch on commits. Runs build + unit tests. Surfaces conflicts early. Reports in plain English in Integration and Loopy. Keeps caches and worktrees clean.
- Never: Installs firmware. Never merge branch refs-oscuro (ac10280). It holds the Oscuro reference firmware and is not for distribution. Never push refs-oscuro. Never push to GitHub or any outside remote.
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
- Never: Installs. (TO CONFIRM: implied by the profile, not written.) Never package anything from branch refs-oscuro (ac10280). It is not for distribution. Never push refs-oscuro. Never push to GitHub or any outside remote.
- Rooms: Integration.

### Docs Keeper
- ID: ffd7fca0-6687-4102-bc74-4dda79e8604a
- Purpose: Keeps BEHAVIOR.md, USER_PREFERENCES.md, ROADMAP.md and the progress log in sync. Handles rules 23-25.
- Never: Installs or changes firmware code.
- Box folder: /workspace/x0x37-docs-keeper.
- Rooms: Feature Design.

### Test Plan Writer
- ID: 64d26138-589a-454f-90c9-389404c9c990
- Purpose: Writes short hardware test checklists for meaningful changes. Posts them in User Test.
- Never: Installs or changes code. (TO CONFIRM: implied, not written.)
- Rooms: User Test.

Note: the iChord agents belong to a different project. They are not part of this setup.

## Agent identities and lanes

Each task has one owner. Find the owner in this table. Heavy agents write code or run builds. Light agents write docs, do checks or give answers.

| Agent | Owns (only this agent) | Does not do | Hands off to | Cost tier |
|---|---|---|---|---|
| Orchestrator | Rule checks. Task assignment. Room order. | Build, write code or install. | Owner of the task, from this table. | Light |
| Firmware Main Builder | Live BLE lane ble-a65 (ca1d4d2, a65). Not ble-a56 or ble-stage3. All firmware installs. | Edit clip-keys. Write the bootloader. | Merge Integrator (commits). Test Plan Writer (install changes). Docs Keeper (logs). | Heavy |
| Second Builder | Branch clip-keys (clip mode, scale, chord, arp, tempo). | Install. Edit flash_review or the BLE branches (ble-a65). | Merge Integrator (commits). Docs Keeper (logs). | Heavy |
| Firmware Q&A | Answers about firmware and branches. | Commit, change code or install. | Owner of the topic, from this table. | Light |
| Merge Integrator | Branch integration. Merges and merge conflicts. merge-monitor-log.md. | Install. Write feature code. Merge or push refs-oscuro. Push to an outside remote. | Build and Test Runner (merged commit). | Heavy |
| Build and Test Runner | Headless builds, unit tests, size and flash-map checks. | Install. Write feature code. | Release Packager (pass). Builder of the branch (fail). | Heavy |
| Release Packager | Release packages with CRCs, slot backup paths and changelogs. | Install. Change code. Package or push refs-oscuro. Push to an outside remote. | Firmware Main Builder (install). Docs Keeper (logs). | Light |
| Docs Keeper | BEHAVIOR.md, USER_PREFERENCES.md, ROADMAP.md, this file, progress log. | Install. Change firmware code. | Orchestrator (rule changes). | Light |
| Test Plan Writer | Hardware test checklists in User Test. | Install. Change code. | Firmware Main Builder (install). Docs Keeper (results). | Light |

## Rooms (max 6 agents each)

All room IDs are TO CONFIRM (2026-10-16). The owner recreates the rooms. Use the new IDs when the owner gives them.

Rule: a room can have a maximum of 6 agents. Before you add an agent to a full room, remove one agent or use a different room.

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
- Sync box commits to the Mac with a git bundle, so both copies keep the same hashes. Do not use patches.
- Each install notice must give the exact commit hash being installed.
- Post only results, blockers and decisions, in two sentences or fewer.
- Use the caveman skill (/home/box/agent-data/workflows/caveman/SKILL.md) for room posts. Use simple-english for commits, logs and docs. If the two disagree, simple-english wins.
- Note (2026-10-16): the caveman skill is missing on this account. Room posts use short simple English.
- Rule 26: When blocked, or when work can run in parallel, @ the owner in the room with task, inputs, done criteria and deadline, in two sentences or fewer.
- Keep one copy of the project on the box and one on the Mac. Keep the two copies in sync.
- Shared bare repository on the box: /workspace/x0x37.git, remote name box. Each agent pushes only its own branch there and fetches from it. No agent pushes outside the box. (User approved 2026-10-16 02:09.)
- Generic work is approved automatically. Notify the user first before anything that changes how the device works or feels. Safety rules stay. (User, 2026-10-16 02:11.)
- Delegation (rule 26): If you are blocked or a task can run in parallel, post a delegation request in the correct room. Use @Name for the owner from AGENTS_AND_ROOMS.md. Give the task, the inputs (commit, branch or file), the done criteria and the deadline, in two sentences or fewer. The owner acknowledges the request, works in its own box folder /workspace/x0x37-<agent>, and replies with the result and the commit hash. Do not do the same work as the owner at the same time. Only Firmware Main Builder installs firmware. Generic requests are approved automatically by the agency rule. If a task can change the user experience, notify the user first.

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
- Keep the JieLi files local on the box and on the Mac. Do not commit them. Exception: branch refs-oscuro (see the next line).
- JieLi-files exception for refs-oscuro: the general rule says do not commit JieLi files. The exception is branch refs-oscuro only. Its commit babd349 (folder refs/jieli/) holds the JieLi Bluetooth libraries that our controller image links (lib/) and the AC79 SDK headers (sdk/include_lib/ and sdk/LICENSE), as a reference. The user asked for this on 2026-10-16 17:45. The files are not for distribution. Do not ship, publish or push them outside the box. Do not put them in a release, package, installer or bundle. Do not merge refs-oscuro into ble-a56, integration or smk37. The full SDK and the toolchain stay out of git. (Source: refs/jieli/README.md in babd349.)
- 2026-10-16 18:09 (owner): refs-oscuro stays in the local repositories. Never push refs-oscuro.
- 2026-10-16 18:09 (owner): Code stays only on the box and on the machines of the owner. Never push to GitHub or to any other outside remote. Orchestrator installed a pre-push block on /workspace/x0x37.git and on the builder box folders (2026-10-16 18:09).
- 2026-10-16 18:11: The refs-oscuro pre-push block is also on the Mac: ~/Documents/x0x37/x0x37/.git/hooks/pre-push and ~/Documents/x0x37/flash_review/.git/hooks/pre-push. The second hook covers all flash_review worktrees. A test push was refused. The hook blocks only a push that contains refs-oscuro or its commits. The GitHub origin of the x0x37 repo stays for now. No one pushes to it.
- Each builder works only in its own worktree.

## Current project context
- Location: ~/Documents/x0x37
- Base commit: 585d63c is old. Integration merges: afd20e9, 05a0349, 69ff913, 5076f12, cda4cce. At 17:34 the Mac branch integration moved to 02ae8b8 (585d63c with merges of ble-stage3 a43425b and clip-keys c2f7906). Merge Integrator rebuilt integration from ble-a56 on the order of Orchestrator, because 02ae8b8 was stale. At 17:37 it reset integration to d9844a4, and at 17:38 to c090cfe. The branch integration-stale-02ae8b8 keeps the old merges.
- Active branches: ble-a65 (Firmware Main Builder, ca1d4d2, the live BLE lane, back to the app host from fb6b1b0). History: ble-a54, ble-a55 and ble-a56 (BLE attempts 54 to 64, the CTL_HOST image host). clip-keys (Second Builder, flash_review_clipkeys, c2f7906). integration (Merge Integrator, flash_review_integration, c090cfe at 17:38). fix-bass303 (9703c4f, owner TO CONFIRM). refs-oscuro (ac10280, tip babd349): the SMK-37 Oscuro reference firmware and the JieLi reference files. Not for distribution. Never merge it. Never package it (owner decision 2026-10-16 17:44). Never push it (owner decision 2026-10-16 18:09). Older BLE branches: ble-stage3 and ble-a48 to ble-a51. The live BLE lane is ble-a56 (d9844a4, a63, ledger row c090cfe).
- Commit 9984344 (clip keys: knobs 2-8 send CCs) is only on the integration line (merge 5076f12). It is not on clip-keys.
- Both touch main_fm1.c (careful merge needed)
- Logs to keep: x0x37-progress-log.md, merge-monitor-log.md, USER_PREFERENCES.md, BEHAVIOR.md, ROADMAP.md

More facts (checked 2026-10-16 01:32):
- Machine: Mac-mini.local. Full path: /Users/mac/Documents/x0x37.
- Main source tree: /Users/mac/Documents/x0x37/x0x37 (branch smk37, HEAD 4ffcd2d).
- Worktrees: flash_review (ble-stage3), flash_review_clipkeys (clip-keys), flash_review_integration (integration). All three were at 585d63c. Second Builder had work that was not committed.
- Files that both branches can change: firmware/src/app/main_fm1.c, firmware/src/app/plat_fm1.c (plat_midi_out), BEHAVIOR.md item BLE-09.
- New on clip-keys: firmware/src/app/smk_clip_keys.h, tests/smk_clip_keys_test.c.
- Other files to keep: agent-rules.md (the 27 agent rules), x0x37/BEHAVIOR.md, flash_review/BLE_ATTEMPTS.md (live ledger, one row for each Bluetooth attempt), slot_backups/ (do not delete).
- This root file is the main copy. x0x37/docs/AGENTS_AND_ROOMS.md is an exact copy of this file, committed on smk37. Update both together.

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
- 2026-10-16 02:23, Docs Keeper: added the git bundle sync rule. Noted that the Firmware Main Builder box copy is a git repository.
- 2026-10-16 02:27, Docs Keeper: added the rule that each install notice gives the exact commit hash.
- 2026-10-16 02:28, Docs Keeper: the live BLE attempts ledger is flash_review/BLE_ATTEMPTS.md (was ble_split/BLE_ATTEMPTS.md).
- 2026-10-16 02:29, Docs Keeper: added the short-post rule (results, blockers and decisions only, two sentences or fewer).
- 2026-10-16 02:43, Docs Keeper: added the rule caveman for room posts, simple-english for commits, logs and docs.
- 2026-10-16 02:43, Docs Keeper: added rule 26 (@ the owner when blocked or when work can run in parallel).
- 2026-10-16 02:44, Docs Keeper: Merge Integrator now owns merge-monitor-log.md (was Firmware Q&A). Removed it from the Docs Keeper duties.
- 2026-10-16 02:45, Docs Keeper: Rule 27 and lanes table added: one owner per task, lightest agent first, no post without a result, blocker or decision.
- 2026-10-16 17:40, doc audit for Docs Keeper: Firmware Main Builder worktrees are now flash_review (ble-a51) and wt-a55 (ble-a55). Newest BLE branch ble-a56 (d9844a4). Added the integration merges and the new branches. fix-bass303 owner TO CONFIRM. Rule count 25 changed to 27. The old copy x0x37/docs/AGENTS_AND_ROOMS.md has a REPLACED mark. Nothing was removed.
- 2026-10-16 17:45, doc audit for Docs Keeper: new agent IDs for all nine agents. All room IDs are TO CONFIRM, because the owner recreates the rooms. The live BLE lane is ble-a56 (d9844a4, a63), not ble-stage3. Added the note that the caveman skill is missing, so room posts use short simple English. Added the integration reset (17:37 and 17:38). Test Plan Writer was already in the file. Nothing was removed.
- 2026-10-16 17:50, doc audit for Docs Keeper: Merge Integrator did the integration reset at 17:37 and 17:38 on the order of Orchestrator. It rebuilt integration from ble-a56, because 02ae8b8 was stale. fix-bass303 owner stays TO CONFIRM.
- 2026-10-16 17:41, doc audit for Docs Keeper: x0x37/docs/AGENTS_AND_ROOMS.md is now an exact copy of this file, committed on smk37. It has no REPLACED mark. Update both together.
- 2026-10-16 17:45, doc audit for Docs Keeper: owner decision 17:44. Branch refs-oscuro (ac10280) holds the SMK-37 Oscuro reference firmware. It is not for distribution. Added never-merge to Merge Integrator and never-package to Release Packager. Added refs-oscuro to the active branches.
- 2026-10-16 18:10, doc audit for Docs Keeper: owner decisions 18:09. Never push refs-oscuro. Code stays only on the box and on the machines of the owner, with no push to GitHub or any outside remote. Added the pre-push block of Orchestrator, the JieLi-files exception for refs-oscuro (from refs/jieli/README.md in babd349) and the new never-do items for Merge Integrator and Release Packager.
- 2026-10-16 18:15, doc audit for Docs Keeper: added the Mac pre-push hook paths (x0x37/.git/hooks/pre-push and flash_review/.git/hooks/pre-push). The GitHub origin stays for now, and no one pushes to it.
- 2026-10-16 18:47, doc audit for Docs Keeper: the live BLE lane is now ble-a65 (ca1d4d2). ble-a56 is history. Older change log lines keep ble-a56 as it was then.
