# x0x37 Agents and Lanes

This file lets another model recreate the multi-agent setup for the x0x37 firmware project. Owner: Docs Keeper. "TO CONFIRM" marks a fact that no project file or agent profile shows. This root file is the main copy. x0x37/docs/AGENTS_AND_ROOMS.md is an exact copy, committed on smk37. Update both together.

Last updated: 2026-10-16 21:07 (Vladivostok, UTC+10).

## Agents

### Orchestrator
- ID: 6816f054-4f75-496c-b5a8-67dc1a4daf99
- Purpose: Primary bot. Routes tasks, enforces project rules and the style rules. Relays messages between the agents and the owner.
- Never: Installs firmware.

### Firmware Main Builder
- ID: a02ff4ed-d19b-42fc-8286-d5854da64a2a
- Purpose: Builds, debugs and installs custom firmware. Owns the Bluetooth/JieLi work on the live BLE lane ble-a73. Only agent allowed to install (with slot backups, a per-slot CRC compare and a notice before the install).
- Never: Writes the bootloader. Installs in the background. Installs without a slot backup and a per-slot CRC compare.
- Worktrees (Mac, flash_review repo): /private/tmp/a65wt (ble-a65), /private/tmp/a63wt (ble-a56), /private/tmp/refswt (refs-oscuro), flash_review (ble-a51), wt-a55 (ble-a55).
- Box copy: /workspace/x0x37-firmware-main-builder (a git repository). Commits there, then syncs the commits to the Mac repo before each install.

### Second Builder
- ID: d9628bfa-aaf6-4aa0-81b3-db56618755f2
- Purpose: Builds clip-mode keybed, scale/chord/arp, tempo carry-over on clip-keys. (TO CONFIRM: the agent profile has no description.)
- Never: Installs. Does not edit the flash_review tree (it belongs to Firmware Main Builder).
- Worktree: /Users/mac/Documents/x0x37/flash_review_clipkeys (branch clip-keys).
- Box copy: /workspace/x0x37-second-builder. Commits there, then syncs to the Mac branch clip-keys.

### Firmware Q&A
- ID: 421cc513-a96f-4a66-b299-af00df87fbf4
- Purpose: Watches branches. Answers questions about firmware and branches.
- Never: Installs or changes code.

### Merge Integrator
- ID: 6d5025ab-17f1-4d00-abb1-a1475550e792
- Purpose: Merges branches into the integration branch on commits. Runs the build and unit tests. Reports conflicts early to Orchestrator and the builders. Keeps caches and worktrees clean. Writes merge-monitor-log.md.
- Never: Installs firmware. Merges or pushes refs-oscuro. Pushes to GitHub or any outside remote.
- Worktree: /Users/mac/Documents/x0x37/flash_review_integration (branch integration).

### Build and Test Runner
- ID: a35e97c4-ec2a-4d3b-bea2-e32d8aedaa97
- Purpose: Headless builds, unit tests (smk_clip_keys_test etc.), size and flash-map checks. Keeps caches warm.
- Never: Installs firmware.

### Release Packager
- ID: 47f0d1b2-8691-46f3-9c06-c99fd805761c
- Purpose: Packages passing merged builds with CRCs, slot backup paths, and plain-English changelog.
- Never: Installs (TO CONFIRM: implied by the profile, not written). Packages anything from refs-oscuro. Pushes refs-oscuro. Pushes to GitHub or any outside remote.

### Docs Keeper
- ID: ffd7fca0-6687-4102-bc74-4dda79e8604a
- Purpose: Keeps BEHAVIOR.md, USER_PREFERENCES.md, ROADMAP.md, AGENTS_AND_ROOMS.md and the progress log in sync. Handles rules 23 to 25 and 28.
- Never: Installs or changes firmware code.
- Box folder: /workspace/x0x37-docs-keeper.

### Test Plan Writer
- ID: 64d26138-589a-454f-90c9-389404c9c990
- Purpose: Writes short hardware test checklists for changed items only. Sends them to Orchestrator.
- Never: Installs or changes code. (TO CONFIRM: implied, not written.)

### Optimizer
- ID: e9e30685-7138-4a21-8945-13dffc12afc9
- Purpose: Suggests optimizations that are safe for the user experience. Gives patches to the lane owners.
- Never: Installs firmware. Commits to the branch of another agent.

Note: the iChord agents belong to a different project. They are not part of this setup.

## Agent identities and lanes

Each task has one owner. Find the owner in this table. Heavy agents write code or run builds. Light agents write docs, do checks or give answers.

| Agent | Owns (only this agent) | Does not do | Hands off to | Cost tier |
|---|---|---|---|---|
| Orchestrator | Rule checks. Task assignment. Messages to and from the owner. | Build, write code or install. | Owner of the task, from this table. | Light |
| Firmware Main Builder | Live BLE lane ble-a73. All firmware installs. | Edit clip-keys. Write the bootloader. | Merge Integrator (commits). Test Plan Writer (install changes). Docs Keeper (logs). | Heavy |
| Second Builder | Branch clip-keys (clip mode, scale, chord, arp, tempo). | Install. Edit flash_review or the BLE branches (ble-a65, ble-a73). | Merge Integrator (commits). Docs Keeper (logs). | Heavy |
| Firmware Q&A | Answers about firmware and branches. | Commit, change code or install. | Owner of the topic, from this table. | Light |
| Merge Integrator | Branch integration. Merges and merge conflicts. merge-monitor-log.md. | Install. Write feature code. Merge or push refs-oscuro. Push to an outside remote. | Build and Test Runner (merged commit). | Heavy |
| Build and Test Runner | Headless builds, unit tests, size and flash-map checks. | Install. Write feature code. | Release Packager (pass). Builder of the branch (fail). | Heavy |
| Release Packager | Release packages with CRCs, slot backup paths and changelogs. | Install. Change code. Package or push refs-oscuro. Push to an outside remote. | Firmware Main Builder (install). Docs Keeper (logs). | Light |
| Docs Keeper | BEHAVIOR.md, USER_PREFERENCES.md, ROADMAP.md, this file, progress log. | Install. Change firmware code. | Orchestrator (rule changes). | Light |
| Test Plan Writer | Hardware test checklists, sent to Orchestrator. | Install. Change code. | Firmware Main Builder (install). Docs Keeper (results). | Light |
| Optimizer | Patches for optimizations that are safe for the user experience. | Install. Commit to the branch of another agent. | Owner of the lane, from this table. | Light |

## Messages

There are no rooms. Agents send messages direct to each other with SendToAgent. Orchestrator relays messages to the owner. Test Plan Writer sends checklists to Orchestrator.

## Coordination rules

Agent rules: agent-rules.md (37 rules). The rules below are the team rules from agent-rules.md, USER_PREFERENCES.md and the owner.

Work and messages:
- Rule 1: commits, logs, docs and checklists follow the asd-ste skill (/home/box/agent-data/workflows/asd-ste/SKILL.md). Posts and agent messages follow the caveman skill (/home/box/agent-data/workflows/caveman/SKILL.md). If they conflict, asd-ste wins.
- Rule 23: if you change intended behavior, update BEHAVIOR.md in the same task.
- Rule 24: read USER_PREFERENCES.md before you start. Add each new lasting user decision to it.
- Rule 25: when a step finishes or a plan changes, update ROADMAP.md.
- Rule 26: no rooms. Send a delegation request direct to the owner of the task with SendToAgent: task, inputs, done criteria and deadline, in two sentences or fewer. The owner of the task replies with the result and the commit hash.
- Rule 27: each task has one owner (lanes table). Do not work outside your lane. Give each task to the lightest agent that can do it. If you have no result, blocker or decision, do not send a message.
- Rule 28: docs and logs hold only information that matters for development: behavior, decisions, rules, the attempt results that matter, open items, and how to build, install and test. Do not write chatter, repeated status, process narration or acknowledgements.
- Rule 33: a test checklist holds only the changed items. Do not retest proven items. The owner reply has three fields: Connected, Seconds held, Loopy notes.
- Rule 34: never log a hardware pass unless the owner names the build that was tested.
- Rule 35: when a build is ready to test, tell Orchestrator. Orchestrator tells the owner.
- Messages hold only results, blockers and decisions, in two sentences or fewer.
- Text for the owner (questions and explanations): use plain words about what the owner hears, feels and does on the keyboard. Explain each button and each term. Do not use code words, step numbers or millisecond terms (owner, 2026-10-16 20:12). Docs, logs and commits stay in asd-ste.
- Generic work is approved automatically (user, 2026-10-16 02:11). Only changes that the owner can see, hear or play need the approval of the owner (owner, 2026-10-16 19:25). Decide other changes and continue.
- If the Mac does not respond, retry for 5 minutes. Report it offline only after 5 minutes.

Build and install:
- Build, test and analyze on the box copy. Use the Mac only for installs and device access.
- Box build paths: JieLi toolchain /home/box/.jieli/toolchain, AC79 SDK /home/box/fw-AC79_AIoT_SDK (agent rule 12).
- All agents share one box. Each agent uses its own box folder, /workspace/x0x37-<agent-name>. Never use the folder of another agent. Each builder works only in its own worktree.
- Rule 31: tests read the shared reference files in /workspace/x0x37-refs/schwung.
- Rule 32: run hardware tests (device, USB, BLE, installer) one at a time. Run them in parallel only when a plan shows that their results cannot interact.
- Rule 36: tests use X0X_TEST_JOBS=8. Scenario tests run one at a time. Do not run tests for a tree with only .md changes. Results go to /workspace/x0x37-results/<tree>.md. Keep LIVE_BRANCHES and BUDGET.md there current. Gate: /workspace/x0x37-refs/gate/x0x37_gate.sh. Its box-wide flock on X0X_GATE_LOCK (default /workspace/x0x37-refs/gate.lock) stops two gate test runs at the same time. Its lock X0X_GATE_WORK/.lock-<hash> blocks a second run of the same commit only. X0X_TEST_TIME_SCALE: 20 in development. The gate always uses 1 and writes it in the result file. Give each dead branch the name archive/<name>. Optimizer gives patches to the builders.
- Commit early so the Merge Integrator has a base. Build and Test Runner checks every commit. Merge Integrator merges as soon as both sides have a commit.
- Builders commit on the box, then sync the commits to the Mac repo with a git bundle before an install (same hashes, no patches).
- Only Firmware Main Builder installs. Before each install: back up the sample slots, notify the user with the exact commit hash, and wait for the go from the parent agent. After the install: compare the per-slot CRCs and send the commit hash and the CRC result. Never write the bootloader. A verified build installs with no question to the user.
- Rule 29: before an install notice, the gate must report PASS for that exact commit hash. All tests pass, with no list of known failures. The installer reads the image back from the device. Compare each per-slot CRC before and after. The CRC32 of a whole .x0xslots file is always 2144df1c (the CRC-32 residue) and proves nothing. Run the install in the foreground. A watched, named screen or tmux session is allowed. If the link drops, reattach and watch the install to the end and the read-back. Never install in the background.
- Rule 30: keep the tag last-good-hw on the newest build that passed a device test. After 2 failed device tests in a row, bisect from last-good-hw.
- Rule 37: Firmware Main Builder, Second Builder and Optimizer follow the skill /home/box/agent-data/workflows/x0x37-builder-loop/SKILL.md for each change.

Repositories:
- Shared bare repository on the box: /workspace/x0x37.git, remote name box. Each agent pushes only its own branch there and fetches from it (user, 2026-10-16 02:09).
- Code stays only on the box and on the machines of the owner. Never push to GitHub or any other outside remote (owner, 2026-10-16 18:09). The GitHub origin of the Mac x0x37 repo stays, and no one pushes to it.
- Keep the JieLi files local on the box and on the Mac. Do not commit them. Exception: branch refs-oscuro (ac10280, tip babd349) is a reference (user, 2026-10-16 17:44 and 17:45). It holds the Oscuro reference firmware (refs/oscuro/) and the JieLi libraries and AC79 SDK headers (refs/jieli/). Not for distribution. Never merge, package or push refs-oscuro (owner, 2026-10-16 18:09). The full SDK and the toolchain stay out of git.
- Pre-push blocks for refs-oscuro: box /workspace/x0x37.git and the builder box folders. Mac ~/Documents/x0x37/x0x37/.git/hooks/pre-push and ~/Documents/x0x37/flash_review/.git/hooks/pre-push (covers all flash_review worktrees). The hook blocks only a push that contains refs-oscuro or its commits.

## Current project context

- Mac: Mac-mini.local, /Users/mac/Documents/x0x37. Main source tree: x0x37 (branch smk37). The smk37 docs/ folder holds copies of agent-rules.md, USER_PREFERENCES.md, ROADMAP.md and this file. Box smk37 and Mac smk37 stay identical (git bundle sync).
- BLE: live lane ble-a73 (a73 firmware 99faa0a, installed, partial owner test, branch tip c3ee3ef). Tag last-good-hw = 94285b5 (a67, iPhone PASS 19:18). ble-a74 14fb352 (Firmware Main Builder, ledger tip 462ba7c): clock source setting INTERNAL / EXTERNAL / AUTO and the PLAY note. Installed 21:03, owner test pending. Side branches, built, not installed: ble-a68-opt a1d8ea1, ble-a69-readback 75c42e1, ble-a70 77fa067, ble-a71 9ef7bc2. Archived: archive/ble-a65 4d1608a, archive/ble-a72 fb5d720. History: ble-a54 to ble-a56 (a54 to a64, the CTL_HOST image host), ble-stage3 (to a47), ble-a48 to ble-a51. Live list: /workspace/x0x37-results/LIVE_BRANCHES.
- Clip keys and integration: clip-keys-a65 at 81f6418. integration at 7e44e94. Box archive/clip-keys keeps ce5236e. The Mac clip-keys (c2f7906) is old.
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
