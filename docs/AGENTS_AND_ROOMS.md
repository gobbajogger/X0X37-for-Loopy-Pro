# x0x37 Agents and Lanes

This file lets another model recreate the multi-agent setup for the x0x37 firmware project. Owner: Docs Keeper. "TO CONFIRM" marks a fact that no project file or agent profile shows. This root file is the main copy. x0x37/docs/AGENTS_AND_ROOMS.md is an exact copy, committed on smk37. Update both together.

Last updated: 2026-10-16 22:25 (Vladivostok, UTC+10).

## Agents

### Orchestrator
- ID: 6816f054-4f75-496c-b5a8-67dc1a4daf99
- Purpose: Primary bot. Routes tasks, enforces project rules and the style rules. Relays messages between the agents and the owner.
- Never: Installs firmware.
- Box folder: /workspace/x0x37-orchestrator.

### Firmware Main Builder
- ID: a02ff4ed-d19b-42fc-8286-d5854da64a2a
- Purpose: Builds, debugs and installs custom firmware. Owns the Bluetooth/JieLi lane files. Commits on integration (rules 4 to 6). Installs the batch hash with gate PASS. Only agent allowed to install (with slot backups, a per-slot CRC compare and a notice before the install).
- Never: Writes the bootloader. Installs in the background. Installs without a slot backup and a per-slot CRC compare.
- Worktrees (Mac, flash_review repo): /private/tmp/a65wt (ble-a65), /private/tmp/a63wt (ble-a56), /private/tmp/refswt (refs-oscuro), flash_review (ble-a51), wt-a55 (ble-a55).
- Box copy: /workspace/x0x37-firmware-main-builder (a git repository). Commits there, then syncs the commits to the Mac repo before each install.

### Second Builder
- ID: d9628bfa-aaf6-4aa0-81b3-db56618755f2
- Purpose: Builds clip-mode keybed, scale/chord/arp, tempo carry-over. Commits on integration (rules 4 to 6). (TO CONFIRM: the agent profile has no description.)
- Never: Installs. Does not edit the flash_review tree (it belongs to Firmware Main Builder).
- Worktree: /Users/mac/Documents/x0x37/flash_review_clipkeys (branch clip-keys).
- Box copy: /workspace/x0x37-second-builder. Commits there, then syncs to the Mac branch clip-keys.

### Firmware Q&A
- ID: 421cc513-a96f-4a66-b299-af00df87fbf4
- Purpose: Watches branches. Answers questions about firmware and branches.
- Never: Installs or changes code.

### Merge Integrator
- ID: 6d5025ab-17f1-4d00-abb1-a1475550e792
- Purpose: Owns the working branch integration: the branch order, the merges and the batch gate (rule 6). Runs one full gate at the end of each batch. Splits or bisects the batch only if that gate fails. Reports conflicts early to Orchestrator and the builders. Keeps caches and worktrees clean. Writes merge-monitor-log.md.
- Never: Installs firmware. Merges or pushes refs-oscuro. Pushes to GitHub or any outside remote.
- Worktree: /Users/mac/Documents/x0x37/flash_review_integration (branch integration).

### Build and Test Runner
- ID: a35e97c4-ec2a-4d3b-bea2-e32d8aedaa97
- Purpose: Headless builds, unit tests (smk_clip_keys_test etc.), size and flash-map checks. Keeps caches warm.
- Never: Installs firmware.
- Box folder: /workspace/x0x37-build-and-test-runner.

### Release Packager
- ID: 47f0d1b2-8691-46f3-9c06-c99fd805761c
- Purpose: Packages passing merged builds with CRCs, slot backup paths, and plain-English changelog.
- Never: Installs (TO CONFIRM: implied by the profile, not written). Packages anything from refs-oscuro. Pushes refs-oscuro. Pushes to GitHub or any outside remote.
- Box folder: /workspace/x0x37-release-packager.

### Docs Keeper
- ID: ffd7fca0-6687-4102-bc74-4dda79e8604a
- Purpose: Keeps BEHAVIOR.md, USER_PREFERENCES.md, ROADMAP.md, AGENTS_AND_ROOMS.md and the progress log in sync. Handles rule 15.
- Never: Installs or changes firmware code.
- Box folder: /workspace/x0x37-docs-keeper.

### Test Plan Writer
- ID: 64d26138-589a-454f-90c9-389404c9c990
- Purpose: Writes short hardware test checklists for changed items only. Sends them to Orchestrator.
- Never: Installs or changes code. (TO CONFIRM: implied, not written.)
- Box folder: /workspace/x0x37-test-plan-writer.

### Optimizer
- ID: e9e30685-7138-4a21-8945-13dffc12afc9
- Purpose: Suggests optimizations that are safe for the user experience. Gives patches to the lane owners.
- Never: Installs firmware. Commits to the branch of another agent.
- Box folder: /workspace/x0x37-optimizer.

Note: the iChord agents belong to a different project. They are not part of this setup.

## Agent identities and lanes

Each task has one task agent. Find it in this table. All lanes commit on one working branch, integration (rules 4 to 6). A lane is a set of files. Heavy agents write code or run builds. Light agents write docs, do checks or give answers.

| Agent | Owns (only this agent) | Does not do | Hands off to | Cost tier |
|---|---|---|---|---|
| Orchestrator | Rule checks. Task assignment. Messages to and from the owner. | Build, write code or install. | Task agent, from this table. | Light |
| Firmware Main Builder | Bluetooth/JieLi lane files on integration. All firmware installs (the batch hash with gate PASS). | Edit clip-keys. Write the bootloader. | Merge Integrator (commits). Test Plan Writer (install changes). Docs Keeper (logs). | Heavy |
| Second Builder | Clip mode lane files on integration (clip mode, scale, chord, arp, tempo). | Install. Edit the Bluetooth/JieLi lane files. | Merge Integrator (commits). Docs Keeper (logs). | Heavy |
| Firmware Q&A | Answers about firmware and branches. | Commit, change code or install. | Task agent of the topic, from this table. | Light |
| Merge Integrator | Branch integration: branch order, merges, merge conflicts, the batch gate. merge-monitor-log.md. | Install. Write feature code. Merge or push refs-oscuro. Push to an outside remote. | Firmware Main Builder (batch hash with gate PASS). | Heavy |
| Build and Test Runner | Headless builds, unit tests, size and flash-map checks. | Install. Write feature code. | Release Packager (pass). Builder of the branch (fail). | Heavy |
| Release Packager | Release packages with CRCs, slot backup paths and changelogs. | Install. Change code. Package or push refs-oscuro. Push to an outside remote. | Firmware Main Builder (install). Docs Keeper (logs). | Light |
| Docs Keeper | BEHAVIOR.md, USER_PREFERENCES.md, ROADMAP.md, this file, progress log. | Install. Change firmware code. | Orchestrator (rule changes). | Light |
| Test Plan Writer | Hardware test checklists, sent to Orchestrator. | Install. Change code. | Firmware Main Builder (install). Docs Keeper (results). | Light |
| Optimizer | Patches for optimizations that are safe for the user experience. | Install. Commit to the branch of another agent. | Task agent of the lane, from this table. | Light |

## Messages

There are no rooms. Agents send messages direct to each other with SendToAgent. Orchestrator relays messages to the owner (the user).

## Coordination rules

Agent rules: agent-rules.md (version 2, 15 rules). Skills: asd-ste, caveman, x0x37-builder-loop. The 15 rules:
- 1. Docs, logs, commits and checklists use the asd-ste skill. Agent-to-agent messages use the caveman skill. If they conflict, asd-ste wins.
- 2. Write to the owner in plain words about what the owner hears, feels and does. Send a question to the owner only through Orchestrator or the task agent. Tell the owner before a change that affects how the keyboard plays.
- 3. Ask the owner only about a change that the owner can see, hear or play. Decide all other changes and continue.
- 4. Put each planned change on the working branch integration. Make one commit and one host test for each change. Run only the affected tests, at time scale 20. Builders follow the x0x37-builder-loop skill.
- 5. Fetch and rebase before each commit. Change only the files of your lane. Give other work to its task agent with SendToAgent.
- 6. Merge Integrator owns the branch order and runs one full gate at the end of the batch. Split or bisect only if that gate fails.
- 7. Install only the batch hash. The gate must report PASS for its exact tree, with 0 fail and 0 skip. Do not use a list of known failures.
- 8. Only Firmware Main Builder installs. Before the install, back up the slots and send the owner notice with the exact hash. After it, read the image back, compare each per-slot CRC and report. Tell the owner about slot checks only when one does not match.
- 9. Never write the bootloader. Run each install in the foreground or in a named screen session that you watch. Never install in the background.
- 10. Run hardware work and edits to one shared file one at a time. While you wait for a result, do the next task that does not need it.
- 11. Log a device PASS only for the items that the owner names, with the build. Mark each other item NOT TESTED.
- 12. Never push outside the box and the machines of the owner. Never push, merge or package refs-oscuro.
- 13. Stop only the processes that you started, by their PIDs. Never use pkill -f or killall on a shared script.
- 14. Before an action that is hard to undo or shared, do the Decision check (x0x37-builder-loop steps 12 to 16). Send skill changes through Orchestrator.
- 15. Do not invent data. Docs and logs hold only development information. Update BEHAVIOR.md (reference copy on integration), ROADMAP.md, USER_PREFERENCES.md and the log in the same task.

Other team notes:
- Text for the owner (questions and explanations): use plain words about what the owner hears, feels and does on the keyboard. Explain each button and each term. Do not use code words, step numbers or millisecond terms (owner, 2026-10-16 20:12). Docs, logs and commits stay in asd-ste.
- Generic work needs no approval (user, 2026-10-16 02:11). Only changes that the owner can see, hear or play need the approval of the owner (owner, 2026-10-16 19:25). Decide other changes and continue.
- If the Mac does not respond, retry for 5 minutes. Report it offline only after 5 minutes.
- Build, test and analyze on the box copy. Use the Mac only for installs and device access.
- Box build paths: JieLi toolchain /home/box/.jieli/toolchain, AC79 SDK /home/box/fw-AC79_AIoT_SDK (build.sh defaults).
- All agents share one box. Each agent uses its own box folder (see the agent list). Merge Integrator uses its Mac worktree. Firmware Q&A only reads and has no folder. Never use the folder of another agent. Each builder works only in its own worktree.
- Builders commit on the box, then sync the commits to the Mac repo with a git bundle before an install (same hashes, no patches).

Repositories:
- Shared bare repository on the box: /workspace/x0x37.git, remote name box. Each agent pushes only its own branch there and fetches from it (user, 2026-10-16 02:09).
- Code stays only on the box and on the machines of the owner. Never push to GitHub or any other outside remote (owner, 2026-10-16 18:09). The GitHub origin of the Mac x0x37 repo stays, and no one pushes to it.
- Keep the JieLi files local on the box and on the Mac. Do not commit them. Exception: branch refs-oscuro (ac10280, tip babd349) is a reference (user, 2026-10-16 17:44 and 17:45). It holds the Oscuro reference firmware (refs/oscuro/) and the JieLi libraries and AC79 SDK headers (refs/jieli/). Not for distribution. Never merge, package or push refs-oscuro (owner, 2026-10-16 18:09). The full SDK and the toolchain stay out of git.
- Pre-push blocks for refs-oscuro: box /workspace/x0x37.git and the builder box folders. Mac ~/Documents/x0x37/x0x37/.git/hooks/pre-push and ~/Documents/x0x37/flash_review/.git/hooks/pre-push (covers all flash_review worktrees). The hook blocks only a push that contains refs-oscuro or its commits.

## Current project context

- Mac: Mac-mini.local, /Users/mac/Documents/x0x37. Main source tree: x0x37 (branch smk37). The smk37 docs/ folder holds copies of agent-rules.md, USER_PREFERENCES.md, ROADMAP.md and this file. Box smk37 and Mac smk37 stay identical (git bundle sync).
- BLE: the keyboard runs a74 (firmware 14fb352, installed 21:03, branch ble-a74, ledger tip 11d81a7). Owner test 1 (21:18): violet FAIL, pad clear FAIL. Tag last-good-hw = 94285b5 (a67, iPhone PASS 19:18). ble-a75 38af0a0, ble-pad-a76 1c45b4b and ble-arrows-a77 59ddadd are merged into integration. Not installed. Side branches, built, not installed: ble-a68-opt a1d8ea1, ble-a69-readback 75c42e1, ble-a70 77fa067, ble-a71 9ef7bc2. Live list: /workspace/x0x37-results/LIVE_BRANCHES.
- Working branch: integration 6ee9c69 (body 9a733fe, the same tree without *.md). Gate on 9a733fe: started, no result at 22:25. clip-keys-a65 5a89b76: its changes are in integration (cherry-picked). Next batch: apc-colors-int f175c30 (LCD clip colours, CLP-05).
- fix-bass303 (9703c4f): owner TO CONFIRM.
- refs-oscuro (ac10280, tip babd349): reference only, see the repository rules.
- main_fm1.c, plat_fm1.c (plat_midi_out) and BEHAVIOR.md BLE-09 can change on both BLE and clip-keys branches. Merge with care.
- Files to keep: agent-rules.md, USER_PREFERENCES.md, ROADMAP.md, x0x37-progress-log.md, merge-monitor-log.md, x0x37/BEHAVIOR.md and BLE_ATTEMPTS.md (the BLE ledger in the BLE branch). Do not delete slot_backups/.

## How to recreate
1. Create the ten agents above with the exact names and the purposes listed.
2. Give each agent its standing duty via a prompt. There are no rooms: agents use SendToAgent.
3. Send the coordination rules to each agent.
4. Point everyone at this file and the project folder.

## Change log
Rule numbers in entries before 2026-10-16 22:25 are version 1 numbers (agent-rules-v1-2026-10-16.md on the Mac).
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
- 2026-10-16 20:12: plain-language rule for text to the owner.
- 2026-10-16 20:28: live BLE lane is ble-a73 (a73 installed, full owner test pending).
- 2026-10-16 20:32: install rule (foreground, watched tmux or screen session). Now in rule 29.
- 2026-10-16 20:46: no rooms (SendToAgent, Orchestrator relays to the owner). Rules 1, 11, 26, 27 and 29 changed. Rules 32 to 36 added. Approval only for changes that the owner can see, hear or play. Orchestrator never installs (confirmed).
- 2026-10-16 20:47: rule 36 gate locks and X0X_TEST_TIME_SCALE. ble-a74 status.
- 2026-10-16 21:07: rule 29 per-slot CRC note (the whole-file CRC proves nothing). Rule 37 (builder loop skill). ble-a74 installed 21:03.
- 2026-10-16 21:42: live BLE lane is ble-a74 (owner test 1: 2 FAIL). ble-a75, ble-pad-a76, integration 110d4c2.
- 2026-10-16 21:48: rule 38 (stop only your own PIDs). Cause: a Firmware Main Builder worker ran pkill -f x0x37_gate.sh at 21:41 and 21:43.
- 2026-10-16 21:55: BEHAVIOR.md reference is integration (rule 22). Rule 29: gate PASS for the exact tree. Rules 5, 11, 26, 28, 29 and 34 in asd-ste. Branch tips.
- 2026-10-16 21:57: rule contradictions fixed (rules 1, 2, 7, 9, 11, 26, 27, 30, 33, 35, 36). "Task agent" replaces "owner of the task". Box folders for each agent.
- 2026-10-16 21:57: rule 1: caveman only for agent-to-agent messages. Messages to the owner use plain words (Orchestrator confirmed).
- 2026-10-16 22:00: rule 39 (owner 22:00): one working branch, integration, and one batch gate. Rule 29: install the batch hash. Lanes are file sets.
- 2026-10-16 22:03: rule 40 (Decision check for each agent, owner 22:02). Rule 37: skill changes go through Orchestrator.
- 2026-10-16 22:04: rule 41 (no idle waiting, owner 22:03).
- 2026-10-16 22:25: agent-rules.md version 2 (15 rules). Coordination rules list the 15 rules. Integration 6ee9c69, a76 and a77 merged.
