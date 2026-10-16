# x0x37 Agents and Lanes

This file lets another model make the multi-agent setup for the x0x37 firmware project again. Docs Keeper maintains this file. "TO CONFIRM" marks a fact that no project file or agent profile shows. This root file is the main copy, and x0x37/docs/AGENTS_AND_ROOMS.md on smk37 is an exact copy. Update both together.

Last updated: 2026-10-16 23:27 (Vladivostok, UTC+10).

## Agents

### Orchestrator
- ID: 6816f054-4f75-496c-b5a8-67dc1a4daf99
- Purpose: Orchestrator is the primary bot. It routes the tasks and makes sure that the agents obey the project rules and the style rules. It relays the messages between the agents and the owner.
- Never: Installs firmware.
- Box folder: /workspace/x0x37-orchestrator.

### Firmware Main Builder
- ID: a02ff4ed-d19b-42fc-8286-d5854da64a2a
- Purpose: Builds, debugs and installs custom firmware. Owns the Bluetooth/JieLi lane files. Commits on a lane/mb-<topic> branch, and Merge Integrator moves each commit onto integration (rules 4 to 6). Installs the batch hash with a gate PASS. It is the only agent that can install (with loop slot backups, a per-slot CRC compare and a notice before the install).
- Never: Writes the bootloader. Installs in the background. Installs without a loop slot backup and a per-slot CRC compare.
- Worktrees (Mac, flash_review repo): /private/tmp/a74wt (ble-a74, 14fb352, the build to install again if batch 1 fails), /private/tmp/refswt (refs-oscuro, read only).
- Box copy: /workspace/x0x37-firmware-main-builder (a git repository). Commits there, then syncs the commits to the Mac repo before each install.

### Second Builder
- ID: d9628bfa-aaf6-4aa0-81b3-db56618755f2
- Purpose: Builds the clip-mode keybed, scale, chord, arp and tempo carry-over features. Owns the clip pad colours and the < and > buttons in all modes (Orchestrator 22:21). Commits on a lane/sb-<topic> branch, and Merge Integrator moves each commit onto integration (rules 4 to 6). Runs an Elite build before each handoff.
- Never: Installs. Edits the Bluetooth/JieLi lane files (Firmware Main Builder owns them).
- Box copy: /workspace/x0x37-second-builder. Commits there on its lane/sb-<topic> branches.

### Firmware Q&A
- ID: 421cc513-a96f-4a66-b299-af00df87fbf4
- Purpose: Watches the branches. Answers questions about the firmware and the branches.
- Never: Installs or changes code.

### Merge Integrator
- ID: 6d5025ab-17f1-4d00-abb1-a1475550e792
- Purpose: Owns the working branch integration: the branch order, the merges and the gate of the batch (rule 6). Requests one gate at the end of each batch, and Build and Test Runner runs it. Splits or bisects the batch only if that gate fails. Reports conflicts early to Orchestrator and the builders. Keeps the caches and worktrees clean. Writes merge-monitor-log.md.
- Never: Installs firmware. Merges or pushes refs-oscuro. Pushes to GitHub or to an outside remote.
- Worktree: /Users/mac/Documents/x0x37/flash_review_integration (branch integration).

### Build and Test Runner
- ID: a35e97c4-ec2a-4d3b-bea2-e32d8aedaa97
- Purpose: Runs headless builds, unit tests (for example smk_clip_keys_test), size checks and flash-map checks. Runs the gate script when Merge Integrator requests it (rule 6). Keeps the build caches ready.
- Never: Installs firmware.
- Box folder: /workspace/x0x37-build-and-test-runner.

### Release Packager
- ID: 47f0d1b2-8691-46f3-9c06-c99fd805761c
- Purpose: Packages the merged builds that pass, with CRCs, loop slot backup paths and a changelog in plain words.
- Never: Installs. Packages a part of refs-oscuro. Pushes refs-oscuro. Pushes to GitHub or to an outside remote.
- Box folder: /workspace/x0x37-release-packager.

### Docs Keeper
- ID: ffd7fca0-6687-4102-bc74-4dda79e8604a
- Purpose: Keeps BEHAVIOR.md, USER_PREFERENCES.md, ROADMAP.md, AGENTS_AND_ROOMS.md and the progress log in sync. Applies rule 15.
- Never: Installs or changes firmware code.
- Box folder: /workspace/x0x37-docs-keeper.

### Test Plan Writer
- ID: 64d26138-589a-454f-90c9-389404c9c990
- Purpose: Writes short hardware test checklists, only for the changed items. Sends them to Orchestrator.
- Never: Installs or changes code.
- Box folder: /workspace/x0x37-test-plan-writer.

### Optimizer
- ID: e9e30685-7138-4a21-8945-13dffc12afc9
- Purpose: Suggests optimizations that are safe for the experience of the owner. Gives patches to the lane builders.
- Never: Installs firmware. Commits to the files of a builder lane. Runs a gate or a trial merge (Orchestrator audit 22:21). Pushes outside the box and the Mac.
- Box folder: /workspace/x0x37-optimizer.

Note: the iChord agents are part of a different project. They are not part of this setup.

## Agent identities and lanes

Each task has one task agent, and this table shows it. Integration is the one working branch, and Merge Integrator moves each commit onto it. Lanes are short-lived branches named lane/<agent>-<topic> (mb, sb, opt), for example lane/mb-clock. Each lane pushes only its own lane branch (rules 4 to 6). A lane is a set of files. Heavy agents write code or run builds, and light agents write docs, do checks or give answers.

| Agent | Owns (only this agent) | Does not do | Hands off to | Cost tier |
|---|---|---|---|---|
| Orchestrator | Rule checks. Task assignment. Messages to and from the owner. | Build, write code or install. | Task agent, from this table. | Light |
| Firmware Main Builder | Bluetooth/JieLi lane files on integration. All firmware installs (the batch hash with a gate PASS). | Edit the clip mode lane files. Write the bootloader. | Merge Integrator (commits). Test Plan Writer (install changes). Docs Keeper (logs). | Heavy |
| Second Builder | Clip mode lane files on integration (clip mode, scale, chord, arp, tempo). The clip pad colours. The < and > buttons in all modes. An Elite build before each handoff. | Install. Edit the Bluetooth/JieLi lane files. | Merge Integrator (commits). Docs Keeper (logs). | Heavy |
| Firmware Q&A | Answers about the firmware and the branches. | Commit, change code or install. | Task agent of the topic, from this table. | Light |
| Merge Integrator | Branch integration: branch order, merges, merge conflicts, the request for the gate of the batch. merge-monitor-log.md. | Install. Write feature code. Merge or push refs-oscuro. Push to an outside remote. | Firmware Main Builder (batch hash with a gate PASS). | Heavy |
| Build and Test Runner | Headless builds, unit tests, size checks and flash-map checks. The gates (when Merge Integrator requests them). | Install. Write feature code. | Release Packager (pass). Task agent (fail). | Heavy |
| Release Packager | Release packages with CRCs, loop slot backup paths and changelogs. | Install. Change code. Package or push refs-oscuro. Push to an outside remote. | Firmware Main Builder (install). Docs Keeper (logs). | Light |
| Docs Keeper | BEHAVIOR.md, USER_PREFERENCES.md, ROADMAP.md, this file, progress log. | Install. Change firmware code. | Orchestrator (rule changes). | Light |
| Test Plan Writer | Hardware test checklists, sent to Orchestrator. | Install. Change code. | Orchestrator (checklists). Docs Keeper (results). | Light |
| Optimizer | Patches for optimizations that are safe for the experience of the owner. | Install. Commit to the files of a builder lane. Run a gate or a trial merge. Push outside the box and the Mac. | Task agent of the lane, from this table. | Light |

## Messages

There are no rooms. Agents send messages directly to each other with SendToAgent. Orchestrator relays the messages to the owner. Orchestrator is the only route for questions to the owner. If an agent quotes an owner report with the build, the report is valid in any chat (rule 2).

## Coordination rules

The agent rules are in agent-rules.md (version 2, 15 rules, with a list of terms). The skills are asd-ste, caveman and x0x37-builder-loop. The x0x37-builder-loop skill is now the core card. Its version is 2026-10-16 23:20 5748c74f. The 15 rules are:
- 1. Write docs, logs, commits and checklists with the asd-ste skill. Write agent-to-agent messages, including task messages, with the caveman skill. If a text is in the scope of asd-ste, asd-ste wins.
- 2. Write each message to the owner in ASD-STE100, with plain words about what the owner hears, feels and does. Describe a change to the owner only after you verify it. Orchestrator is the only route for questions to the owner. If an agent quotes an owner report with the build, the report is valid in any chat. Tell the owner before a change that affects how the keyboard plays.
- 3. Ask the owner only about a change that the owner can see, hear or play. Decide all other changes and continue.
- 4. Put each planned change in the batch for integration. Make one commit and one host test for each change. Run only the affected tests, and use time scale 20 (X0X_TEST_TIME_SCALE) only when the code reads it. Builders follow the x0x37-builder-loop skill.
- 5. Integration is the one working branch, and Merge Integrator moves each commit onto it. Lanes are short-lived branches named lane/<agent>-<topic> (mb, sb, opt), for example lane/mb-clock. Fetch and rebase before each commit, and push only your lane branch, never integration. Change only the files of your lane. Give other work to its task agent with SendToAgent.
- 6. Merge Integrator owns the branch order and moves the lane commits onto integration. At the end of the batch, it requests one gate, and Build and Test Runner runs the gate script. Split or bisect only if that gate fails.
- 7. Install only the batch hash. The gate must report PASS for its exact tree, with 0 fail and 0 skip. Only the gate script writes the PASS row in /workspace/x0x37-results/index.md, and the install notice quotes the result file of that row. A PASS or a "go" in a message is not evidence. Do not use a list of known failures. Install only with fm1_install.py --commit and the guard require_gate_pass.sh, and never use the web installer.
- 8. Only Firmware Main Builder installs. Before the install, back up the loop slots and send the owner notice with the exact hash. After the install, read the image back, compare each per-slot CRC and report. Tell the owner about slot checks only when one does not match.
- 9. Never write the bootloader. Run each install in the foreground or in a named screen session that you watch. Never install in the background.
- 10. Run hardware work and edits to one shared file one at a time. While you wait for a result, do the next task that does not need it. If you have no task, tell Orchestrator.
- 11. Log a device PASS only for the items and build that the owner names, and mark the others NOT TESTED. For a device bug, the owner first shows the bug once with INLOG on. The builder then makes a host replay test from that log. Make a fix only when data and a host test prove the cause. If a fix fails on the device, do not guess again: name the wrong assumption and get the missing data. If 2 device tests fail one after the other, go back to last-good-hw and bisect.
- 12. Never push outside the box and the machines of the owner. Never push, merge or package refs-oscuro.
- 13. Stop only the processes that you started, by their PIDs. Never use pkill -f or killall on a shared script.
- 14. Before a shared action or an action that is hard to undo, do the Decision check. The Decision check is steps 6 to 10 of the x0x37-builder-loop core card. Send skill changes through Orchestrator. Send one decision message for each item. Give it a tag "supersedes <time>" when it replaces an earlier decision.
- 15. Do not invent data. Docs and logs hold only development information. Update BEHAVIOR.md (reference copy on integration), ROADMAP.md, USER_PREFERENCES.md and the log in the same task.

Other team notes:
- Text for the owner (questions and explanations): use plain words about what the owner hears, feels and does on the keyboard. Explain each button and each term. Do not use code words, step numbers or millisecond terms (owner, 2026-10-16 20:12). Write it in ASD-STE100, and describe a change only after you verify it (owner, 23:11, rule 2). Docs, logs and commits stay in asd-ste.
- Generic work needs no approval (owner, 2026-10-16 02:11). Only a change that the owner can see, hear or play needs the approval of the owner (owner, 2026-10-16 19:25). Decide the other changes and continue.
- If the Mac does not answer, try again for 5 minutes. Report the Mac offline only after 5 minutes.
- Build, test and analyze on the box copy. Use the Mac only for installs and device access.
- Box build paths: JieLi toolchain /home/box/.jieli/toolchain, AC79 SDK /home/box/fw-AC79_AIoT_SDK (build.sh defaults).
- All agents share one box. Each agent uses its own box folder (see the agent list). Merge Integrator uses its Mac worktree. Firmware Q&A only reads and has no folder. Never use the folder of another agent. Each builder works only in its own worktree.
- Builders commit on the box. Before an install, they sync the commits to the Mac repo with a git bundle (same hashes, no patches).

Repositories:
- The shared bare repository on the box is /workspace/x0x37.git, remote name box. Each agent pushes only its own branch there and fetches from it (owner, 2026-10-16 02:09). Lanes push only their lane/<agent>-<topic> branch, never integration (rule 5).
- Code stays only on the box and on the machines of the owner. Never push to GitHub or to any other outside remote (owner, 2026-10-16 18:09). The GitHub origin of the Mac x0x37 repo stays, and nobody pushes to it.
- Keep the JieLi files on the box and on the Mac. Do not commit them. Exception: branch refs-oscuro (ac10280, tip babd349) is a reference (owner, 2026-10-16 17:44 and 17:45).
- refs-oscuro holds the Oscuro reference firmware (refs/oscuro/) and the JieLi libraries and AC79 SDK headers (refs/jieli/). It is not for distribution. Never merge, package or push refs-oscuro (owner, 2026-10-16 18:09). The full SDK and the toolchain stay out of git.
- Pre-push blocks for refs-oscuro: box /workspace/x0x37.git and the builder box folders. Mac ~/Documents/x0x37/x0x37/.git/hooks/pre-push and ~/Documents/x0x37/flash_review/.git/hooks/pre-push (for all flash_review worktrees). The hook blocks only a push that contains refs-oscuro or its commits.

## Current project context

- Mac: Mac-mini.local, /Users/mac/Documents/x0x37. The main source tree is x0x37 (branch smk37). The smk37 docs/ folder holds copies of agent-rules.md, USER_PREFERENCES.md, ROADMAP.md and this file. Box smk37 and Mac smk37 stay identical (git bundle sync).
- Keyboard:
  - The keyboard runs integration 9a733fe (batch 1). Firmware Main Builder installed it at 22:25 (ledger b1, b913735).
  - The fwsc CRC32 is 3a738997. The ctl.img read-back is eb742dd2. Slots 1 and 2 did not change. DIAG shows READY.
  - The gate gave PASS at 22:29 (tree 1d3c048d, 93 ok, 0 FAIL, 0 skip). The gate Elite fwsc CRC32 3a738997 is the same as the installed file.
  - The install at 22:25 came before the gate PASS. This is a rule 7 break. Cause: Merge Integrator called a manual test run a gate PASS, and Firmware Main Builder accepted it without a check of index.md. We made no new rule.
  - The owner tests batch 1 only after the gate PASS (this is now true). If the owner test fails, Firmware Main Builder installs 14fb352 (a74) again. Device test: PENDING (checklist-batch1).
- Bluetooth branches:
  - Tag last-good-hw = 94285b5 (a67, iPhone PASS 19:18). It stays at 94285b5.
  - Before batch 1, the keyboard ran a74 (14fb352, installed 21:03, branch ble-a74, ledger tip 11d81a7). Owner test 1 (21:18): violet FAIL, pad clear FAIL.
  - ble-a75 38af0a0, ble-pad-a76 1c45b4b and ble-arrows-a77 59ddadd are in integration (installed in 9a733fe).
  - Side branches with builds that nobody installed: ble-a68-opt a1d8ea1, ble-a69-readback 75c42e1, ble-a70 77fa067, ble-a71 9ef7bc2. Live list: /workspace/x0x37-results/LIVE_BRANCHES.
- Working branch: integration f357a2e (code tree the same as 9a733fe). The gate on 9a733fe gave PASS at 22:29 (tree 1d3c048d). The gate at 22:24 was VOID (setup error). clip-keys-a65 5a89b76: its changes are in integration (cherry-picked). Next batch: apc-colors-int f175c30 (LCD clip colours, CLP-05).
- fix-bass303 (9703c4f): owner TO CONFIRM.
- refs-oscuro (ac10280, tip babd349): reference only. See the repository rules.
- main_fm1.c, plat_fm1.c (plat_midi_out) and BEHAVIOR.md BLE-09 can change in the Bluetooth/JieLi lane and in the clip mode lane. Merge them carefully.
- Files to keep: agent-rules.md, USER_PREFERENCES.md, ROADMAP.md, x0x37-progress-log.md, merge-monitor-log.md, x0x37/BEHAVIOR.md and BLE_ATTEMPTS.md (the Bluetooth ledger in the Bluetooth branch). Do not delete slot_backups/.

## How to make the setup again
1. Make the ten agents above with the exact names and the purposes in the list.
2. Give each agent its permanent task in its prompt. There are no rooms: agents use SendToAgent.
3. Send the coordination rules to each agent.
4. Tell all agents to read this file and the project folder.

## Change log
Rule numbers in entries before 2026-10-16 22:25 are version 1 numbers (agent-rules-v1-2026-10-16.md on the Mac).
- 2026-10-16 01:35: We made this file. It had the agent IDs, the worktrees and the never-do items. It also had the room limit (6 agents), rules 23 to 25 and the install rules.
- 2026-10-16 02:08 to 02:29: We added box-first work, a box folder for each agent, the git bundle sync, the shared box repository and auto-approval. We also added the Mac offline retry (5 min), the commit hash in each install notice and short posts.
- 2026-10-16 02:43 to 02:45: We added rule 26 (delegation), rule 27 and the lanes table. Merge Integrator owns merge-monitor-log.md.
- 2026-10-16 17:45: We added new agent IDs. All room IDs were TO CONFIRM.
- 2026-10-16 17:44 to 18:15: We added the refs-oscuro rules (never merge, package or push) and the JieLi-files exception. We also added no outside remotes and the pre-push hooks.
- 2026-10-16 18:47: The live Bluetooth lane became ble-a65.
- 2026-10-16 18:52: We added the new agent Optimizer.
- 2026-10-16 19:02: We added rule 28 (docs hold only development information). We made the file shorter to obey rule 28.
- 2026-10-16 19:13: We added rules 29 to 31 (gate PASS before an install notice, tag last-good-hw, shared schwung references). We also added the box toolchain and SDK paths.
- 2026-10-16 19:39: We updated the branch tips (ble-a65 4d1608a, clip-keys-a65 81f6418, integration ceaac0a, side branches a68 to a71).
- 2026-10-16 20:12: We added the plain-language rule for text to the owner.
- 2026-10-16 20:28: The live Bluetooth lane became ble-a73 (a73 installed, full owner test pending).
- 2026-10-16 20:32: We added the install rule (foreground, watched tmux or screen session). It is now in rule 29.
- 2026-10-16 20:46: We removed the rooms (SendToAgent, Orchestrator relays to the owner). We changed rules 1, 11, 26, 27 and 29 and added rules 32 to 36. Only changes that the owner can see, hear or play need approval. Orchestrator never installs (confirmed).
- 2026-10-16 20:47: We added rule 36 (gate locks and X0X_TEST_TIME_SCALE) and the ble-a74 status.
- 2026-10-16 21:07: We added the per-slot CRC note to rule 29 (the CRC of the full file proves nothing) and rule 37 (builder loop skill). Firmware Main Builder installed ble-a74 at 21:03.
- 2026-10-16 21:42: The live Bluetooth lane became ble-a74 (owner test 1: 2 FAIL). We added ble-a75, ble-pad-a76 and integration 110d4c2.
- 2026-10-16 21:48: We added rule 38 (stop only your own PIDs). Cause: a Firmware Main Builder worker ran pkill -f x0x37_gate.sh at 21:41 and 21:43.
- 2026-10-16 21:55: The BEHAVIOR.md reference became integration (rule 22). Rule 29 now requires a gate PASS for the exact tree. We put rules 5, 11, 26, 28, 29 and 34 in asd-ste and updated the branch tips.
- 2026-10-16 21:57: We fixed the rule contradictions (rules 1, 2, 7, 9, 11, 26, 27, 30, 33, 35, 36). "Task agent" replaces "owner of the task". We added the box folders for each agent.
- 2026-10-16 21:57: Rule 1: caveman is only for agent-to-agent messages. Messages to the owner use plain words (Orchestrator confirmed).
- 2026-10-16 22:00: We added rule 39 (owner 22:00): one working branch, integration, and one gate for each batch. Rule 29 now says to install the batch hash. Lanes are file sets.
- 2026-10-16 22:03: We added rule 40 (Decision check for each agent, owner 22:02). Rule 37 now says that skill changes go through Orchestrator.
- 2026-10-16 22:04: We added rule 41 (no idle waiting, owner 22:03).
- 2026-10-16 22:25: We made agent-rules.md version 2 (15 rules). The coordination rules list the 15 rules. Integration became 6ee9c69, with a76 and a77 merged.
- 2026-10-16 22:27: We removed the old worktrees from the agent sections (live: a74wt, refswt, flash_review_integration). The Second Builder purpose comes from its profile. Optimizer runs no gate and no trial merge and pushes only to the box and the Mac. The lanes table has no branch names for the lanes, and Test Plan Writer sends checklists to Orchestrator. The profiles of Release Packager and Test Plan Writer confirm "Never installs".
- 2026-10-16 22:28: Second Builder owns the clip pad colours and the < and > buttons in all modes (Orchestrator 22:21). It runs an Elite build before each handoff.
- 2026-10-16 22:30: Integration became f357a2e. The keyboard runs integration 9a733fe (installed 22:25, ledger b1). The device test is pending.
- 2026-10-16 22:39: The gate on batch 1 (9a733fe) gave PASS at 22:29, after the install at 22:25 (rule 7 break). We added the lane file split as an open item.
- 2026-10-16 22:40: Rule 11 now says: record the bug first (owner 22:39). Make an INLOG replay test before a fix for a device bug.
- 2026-10-16 22:59: Rule 11 now says: make a fix only when data and a host test prove the cause. Do not try a second guess (owner 22:59).
- 2026-10-16 23:13: Skill-use audit C1-C14: we changed rules 2, 4, 5, 6, 7, 10 and 11. Lanes push only their own branch. Build and Test Runner runs the gate, and Merge Integrator requests it. The no-rebase OK of 22:03 (Orchestrator) is void. Rule 2 now has the owner rule of 23:11.
- 2026-10-16 23:16: Orchestrator decisions 23:15: lane branches lane/<agent>-<topic> (rule 5). Orchestrator is the only route for owner questions (rule 2). Send one decision message for each item, with "supersedes <time>" (rule 14). The x0x37-builder-loop skill became the core card (23:15, 0f296f5c), and the Decision check is steps 6 to 10.
- 2026-10-16 23:27: We rewrote the docs in ASD-STE100 (Orchestrator 23:19). agent-rules.md has a list of terms and the caveman scope. Rule 7 names the install tool fm1_install.py --commit and the guard require_gate_pass.sh. The x0x37-builder-loop core card is now 2026-10-16 23:20 5748c74f.
