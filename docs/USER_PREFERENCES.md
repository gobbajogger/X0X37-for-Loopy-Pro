# Current authority notice — 2026-10-19

The current owner request and repository [agent rules](agent-rules.md) govern this work. The text below the historical-record divider is the unchanged preference record captured on 2026-10-17. Its role IDs, machine paths, installed hashes, scope exceptions, and procedure summaries are historical. Product decisions still apply unless a later owner instruction supersedes them.

## Development default and explicit release builds

- 2026-10-21 Source: owner in the side conversation attached to local stage 4: "Edit rules to launch release build only when explicitly asked by user. Full gate is always skipped until release build actually has been requested. Always assume it's dev build!" The owner also clarified: "We are not doing a release install. Only a dev build and install". This supersedes treating generic build, install, validation, stage-4 or successor instructions as release-build permission, including earlier dev/release gate handoff wording and the historical 2026-10-17 01:11 release-install preference for current work. Default to development builds and installs with pad calibration enabled. Skip the full gate, including trial/force/owner-validation, until the user explicitly requests a release build for the current work. Then apply the normal scoped/full-gate routing and release safeguards. Active rules: integration.md rule 11; install.md rule 6; core.md rule 6A; commit.md rule 9; workflow.md rule 17. Publication still requires its own authorization.

## Build target scope

- 2026-10-20 Source: owner in this chat: "we dont need to build and test fm1 builds. they are only for reference to port". FM-1 is reference-only for porting. Build and test SMK-37 Elite and Pro; do not require FM-1 target builds or FM-1 target tests. This supersedes the gate’s FM-1 build, budget, and target-test requirements. Keep shared code and checks needed for the SMK-37 port, including tools with legacy `fm1` names. Active rules: integration.md rule 11; owner-messages.md rules 4B and 14; docs.md rule 12. The assigned integration chat owns the workflow change.

## SMC mapping correction

- 2026-10-21 Source: owner in audit chat `01a159f4-de3d-7d90-9124-b09940980f92`, relayed to repair chat `01a159fc-b487-7ed0-ae49-12e13cc06aef`: "Because thats intended. And there's 3 modes for faders and buttons to map like they do on keyboard faders and to then map to individual sounds of either 909 and 808". This supersedes audit/HANDOFF.txt finding 1, proposed stage-2 item 1, and the original stage-2 prompt's generic page-following repair direction. Preserve the intended three fader/button modes: keyboard-equivalent mapping, individual 909 sounds, and individual 808 sounds. The older screen-following answer does not authorize undoing this current choice. The owner then requested "Map knobs too" in the same audit chat. Include knobs in those three modes while preserving visible-field menu, sound-page and clip-editor access. This knob work does not establish a cause for the live input failure. The active bug is all SMC controls failing while light feedback works, with both ends in DAW mode. Active rules: owner-messages.md rules 4B, 13 and 14; docs.md rule 12; device-bug.md rule 8.

## Controller-input evidence

- 2026-10-20 (owner, recorded in `cloud-handoff/2026-10-20/smc_receive_repair_20261020/cloud-transition.txt`): "No I did not. And there's no need in that. We have previous captures that can be reused and we can stimulate sends without ever needing them"; then "Make it a rule too". This supersedes requiring another hands-on capture or control movement for controller-input diagnosis and regression, including that scope of the historical 2026-10-16 22:39 and 22:59 entries. The earlier zero-packet capture was idle and proves no receive failure. Reuse existing captures and simulated sends, with evidence labels and limits defined in device-bug.md rule 8. Other device-bug evidence requirements remain in force.

## On-demand rules

- 2026-10-20 Source: owner in this chat requested rules initialized only when needed, fewer redundant rules, and faster progress with fewer errors and wasted usage. This supersedes mandatory startup loading of both writing skills in the 2026-10-19 entries below. Plain, correct, concise writing still applies to all output. Read core once; load step rules and full skills only for the triggers in agent-rules.md and SKILLS.md. Active rules: core.md rule 6A; style.md rule 15; docs.md rule 12A.

## Team rules

- 2026-10-20 (owner, direct request in `01a1592b-f948-7f30-9fad-0d77131922eb`): "Every step should be in a new thread and when thread finish it should start a new thread to continue work". The specified order is "1 Audit existing code and identify defects", "2 Implement approved fixes", "3 Run tests and investigate failures with subagents", "4 Validate final build and packages", "5 merge", "6 install", "7 push to git". Each stage owns creation of its next fresh thread; the push stage ends the chain. This supersedes treating an entire multi-stage code workflow as one task in one chat. It retains scoped validation, existing approvals, integration ownership, and exact merged-artifact checks. Follow-up: "Update rules only"; this request records the workflow and does not launch stage 1. Active rules: workflow.md rules 17 and 18; core.md rule 2.

- 2026-10-20 (owner, originating chat `01a15922-cd41-72f3-8774-692f97f8c359`, relayed into this fresh rule-recording chat): "need to start new thread for each task and never reuse old threads to continue work. Old threads can be searched for info but nothing else". Start every task in a fresh chat, including continuations. Finish a task already assigned to its fresh chat there without recursive chat creation. This supersedes earlier permission to reuse old chats or helpers for task execution. The later contact authorization below supersedes the absolute read-only restriction for context and handoff messages. Active rule: core.md rule 2.

- 2026-10-20 (owner, direct follow-up in `01a15923-4d24-73a0-a57d-dfd02e5928da`): "Always look if other threads are currently working on the project". Always check other project chats for current activity at task start, and inspect relevant scope before edits or shared actions. Recheck when the snapshot may be stale. Inspection does not assign execution work to old chats. This adds a required activity check to isolated lane ownership. Active rule: core.md rule 2.

- 2026-10-20 (owner, direct follow-up in `01a15923-4d24-73a0-a57d-dfd02e5928da`): "New agents may contact other threads to continue work of old threads". This supersedes the earlier absolute ban on contacting old chats: new agents may request context and handoff details needed for continuation. Continued task execution stays in the fresh chat. This is standing authorization for those messages, not unrelated messaging. Active rule: core.md rule 2; owner-messages.md rule 13.

- 2026-10-20 (owner, clarification reply in `01a15923-4d24-73a0-a57d-dfd02e5928da`): "Yes continue in new chat. Contact old thread to tell them to notify when they are done working so that work can be continued without wait or an automation timer". New agents may ask chats still working for a completion notice to the fresh chat; the responding chat has standing authorization to send that notice. Continue promptly in the fresh chat after completion, without polling or a timer automation. This clarifies the contact exception and supersedes treating the earlier read-only restriction as a ban on completion replies. It does not authorize new execution work in old chats. Active rule: core.md rule 2; owner-messages.md rule 13.

- 2026-10-20 (owner, gate audit chat): "Need to audit when full gate run is performed and make sure that it won't run without a concrete reason for it and will only run when it's actually makes sense". Require an explicit current release purpose or owner validation request and a concrete reason before invoking the full gate; development uses scoped checks. This strengthens gate admission and does not authorize a full gate for this audit. Active rule: integration.md rule 11.

- 2026-10-20 (owner, current chat): Asked whether gating is always necessary, then requested "fix?". Routine edits, commits, merges, handoffs, and completed development batches use scoped checks. Supersedes the automatic full gate for every finished batch. Active rule: integration.md rule 11. Owner then selected "Yes—use focused checks for development installs; full gate for releases" and asked that checks depend on specific changes. This also supersedes a full gate before every development install. Scoped development records must retain artifact, model, budget, queue, and data checks. Active rules: integration.md rule 11 and install.md rule 6.

- 2026-10-20 (owner, current chat): "checks must be scoped to what we edited". Select checks by changed behavior and dependencies; expand only for a concrete risk or applicable gate. Supersedes interpreting required checks as every available check. Active rule: commit.md rule 9; check selection: rules/SCRIPT-CHECKS.md.

Source: the owner's current rules audit request, relayed into this task. "Need to audit all project rules and docs to prevent it from happening. And make sure that all agents use simply English skill and caveman at all times. start a new thread with subagents to finish this job".

- 2026-10-19 (owner): Supersedes earlier box-only work rules and role-only handoffs for authorized work. Supported local environments can complete builds and tests. Keep truthful validation, data protection, licensing, target model checks, and memory budgets (rules/commit.md rule 9; rules/core.md rule 6A).
- 2026-10-19 (owner): Supersedes earlier writing scope exceptions. Plain correct English and concise caveman style apply to every project agent, subagent, output, doc, handoff, and task prompt. Read exact skills and report a missing source; do not claim a substitute is the original (rules/style.md rule 15).
- 2026-10-19 (owner): The owner authorized this audit, its rule and enforcement repairs, isolated commits, checks, independent review, and project rollout. Keep firmware integration with its current task. No repeated approval for necessary reversible work (rules/docs.md rule 12A; rules/owner-messages.md rule 14).
- 2026-10-19 (owner): Local completion, formal gate, install, device test, and release are separate evidence levels. Finish independent authorized work and name real pending dependencies (rules/core.md rule 6A; rules/integration.md rule 11; rules/install.md rule 6).

- 2026-10-19 (owner, "Optimize project rules" chat): "check what other thread that works on rules had came up with and make sure that all project rules are optimized for minimal token usage while meaningfully improve speed of the development and reliability of the builds". Keep one home per requirement, read step rules on demand, reuse only matching validation evidence, and retain required checks and guards. Supersedes repeated full-rule/history reading (rules/docs.md rule 12A; rules/commit.md rule 9).

- 2026-10-19 (owner, "Install caveman and English skills" chat): "install skill caveman and simply english into project andmake sure every agent thatt works on project complies with their rules". The owner selected "Install public skills (recommended)" after the missing originals and public substitutes were explained. Use the pinned public Caveman and ASD-STE100 copies for every project agent and helper. This replaces the prior original-only recovery requirement (rules/style.md rule 15; SKILLS.md).

---

## Historical preference record — preserved text

# X0X37 project: owner preferences and decisions

Read this file before you work on the X0X37 project. Also obey the agent rules (rules/core.md). This Mac root file is the main copy, and x0x37/docs/USER_PREFERENCES.md on smk37 is an exact copy. The reference for the intended behavior is BEHAVIOR.md on the branch integration. The smk37 copy x0x37/BEHAVIOR.md is a synced copy of it.

## How to work with the owner

- Talk plainly and briefly. Give the result first. Explain hardware and code findings in simple words.
- Do the work. Ask only when a wrong choice has real results or when the owner must decide.
- Only a change that the owner can see, hear or play needs the approval of the owner first (see the 19:25 entry). Decide the other changes and continue.
- Do not change release scripts unless the owner asks.
- In build-test mode, do not install fallback or recovery builds only to make the keyboard playable (rules/core.md rule 6A). An example is a build with no Bluetooth. Put the fix into the current set of changes, and install it with that set (owner rule 01:57). The owner tests the keyboard and does not make music with it.
- Commits: author genkigenki <3080081+genkigenki@users.noreply.github.com>, messages in ASD-STE100 (asd-ste skill). Log each change in x0x37-progress-log.md.
- 2026-10-16 18:41 (owner): Optimize without cutting features. Font size is not a feature (rules/device-bug.md rule 8).
- 2026-10-16 20:12 (owner):
  - Questions and explanations to the owner use plain words about what the owner hears, feels and does on the keyboard. Explain each button and each term. Do not use code words, step numbers or millisecond terms (rules/owner-messages.md rule 13).
  - Example: "When you record live and hit a pad a little before the beat, the keyboard puts the note on the step before the one you meant, so your recording sounds a bit early. I can make it snap to the closest step instead."
  - This rule applies only to text for the owner. Docs, logs and commits stay in asd-ste. (Extended 2026-10-16 23:11: these messages are also in ASD-STE100. See the 23:11 entry.)
- 2026-10-17 01:45 (owner, through Orchestrator): Trust the reports and the setup of the owner. Search our code for the cause first. Never tell the owner that the owner made a setup mistake (agent rules 8 and 13).
- 2026-10-17 00:59 (owner, supersedes 00:05): Each message to the owner, from Orchestrator and from all agents, has 1 to 3 short sentences (caveman length). Use simple ASD-STE100 words, with no shorthand, no IDs and no code references. Give only a result, a blocker or one question (agent rule 13).
- 2026-10-17 00:05 (owner): Make each message to the owner as short as possible, and keep every fact (caveman length). Use full, simple ASD-STE100 words: no shorthand, no IDs and no hashes, unless the owner needs them (agent rule 13).
- 2026-10-16 19:25 (owner): Do not ask the owner to approve a change that has no visible, audible or playable effect. Decide and continue. Ask only when the change affects the experience of the owner (rules/owner-messages.md rule 14).
- 2026-10-16 19:02 (owner): Docs and logs hold only development information (agent rule 12).

## Installs and safety

- Never touch the bootloader. Never risk a brick. Run each install in the foreground, never in the background.
- Before each install, back up the loop slots. After the install, compare the slot CRCs. The owner has 5 imported loops (3 in slot 1, 2 in slot 2). The backups are in slot_backups/.
- 2026-10-16: A verified build installs with no question to the owner, but only when all queued changes are complete. Never ask the owner whether to install a partial build (owner rule 01:57). Before each install, send the owner a notice. The 00:59 entry wins for this notice and for the report after the install: no hash, no CRC and no IDs. Write the commit hash and the result file of the gate PASS row (/workspace/x0x37-results/index.md) only in the ledger (BLE_ATTEMPTS.md). A "go" in a message does not replace that row (agent rule 6).
- 2026-10-17 01:57 (owner, through Orchestrator): "install only when editing is done and all queued features are ready to test together; never ask the owner whether to install a partial build." (agent rules 6 and 14)
- 2026-10-17 01:56 (owner, through Orchestrator): Do not install b8255f2 alone. The next install is batch 9 together with the pad clear fix of Firmware Main Builder. That fix covers the end of the USB link, Loopy quit on the Mac and the iPhone pads that light again (rules/core.md rule 6A).
- 2026-10-16: Report UX changes immediately. Some changes can affect the experience of the owner: latency, packet or MTU size, memory margins, sample length and connection behavior. If the builder decides on such a change, it sends a short note to Orchestrator before it continues. Orchestrator tells the owner (rules/owner-messages.md rule 14).
- 2026-10-16 02:11:
  - Generic work needs no approval. Generic work is builds, tests, commits, pushes to the box, doc updates and merges on the box. Tell the owner first before a change that can change how the device works or feels.
  - Examples: new behavior rules, changes that the owner can see, hear or play, flash layout changes, removed features. The safety rules do not change.
  - The 19:25 entry replaces these examples. Only a change that the owner can see, hear or play needs approval. The install notice stays (agent rule 5).
- 2026-10-16 17:3x (the owner gave this decision to Firmware Main Builder):
  - The pool limit is 4096 B, from the build.py rule. heap2 (3584 B) is already inside .pool. A release build must turn off the test-26 RAM canary (it writes 0xFF at 0x01C72000-0x01C74000), or keep at least 8192 B of free pool.
  - Enforced: build.sh and the gate require 8192 B of free pool (2026-10-16 21:53). The 8192 B limit replaces the 4096 B limit (rules/integration.md rule 11).

## Repositories and remotes

- 2026-10-16 02:09: The shared bare repository on the box is /workspace/x0x37.git, remote name box. Each agent pushes only its own branch there and fetches from it. Lanes push only their lane/<agent>-<topic> branch, never integration. Merge Integrator moves each commit onto integration (agent rules 10 and 11, 23:15 entry).
- 2026-10-16 18:09: Code stays only on the box and on the machines of the owner. Never push to GitHub or to any other outside remote. The GitHub origin of the Mac x0x37 repo stays at this time. Nobody pushes to it (rules/core.md rule 1).
- Box first: build, test and analyze on the box copy. Use the Mac only for installs and device access. Copy the built files to the Mac just before an install. Sync box commits to the Mac with a git bundle (same hashes, no patches). Keep the box and Mac copies in sync (git commits and docs).
- Keep the JieLi files on the box and on the Mac. Do not commit them. Exception: branch refs-oscuro (see below).
- 2026-10-16 17:44 and 18:09:
  - Branch refs-oscuro (ac10280, tip babd349) is a reference. It holds the SMK-37 Oscuro reference firmware (refs/oscuro/). It also holds the JieLi Bluetooth libraries and AC79 SDK headers (refs/jieli/). It is not for distribution.
  - Never merge it into a branch. Never package a part of it. Never push it. It stays in the local repositories (rules/core.md rule 1).
  - The full SDK and the toolchain stay out of git. (Source: refs/jieli/README.md and refs/oscuro/README.md.)
- Pre-push blocks for refs-oscuro: on the box /workspace/x0x37.git and the builder box folders, on the Mac x0x37/.git/hooks/pre-push and flash_review/.git/hooks/pre-push (for all flash_review worktrees). The hook blocks only a push that contains refs-oscuro or its commits.

## Team rules

- 2026-10-16 02:14: If the Mac does not answer, try again for 5 minutes. Report the Mac offline only after 5 minutes with no answer (rules/shared-work.md rule 3).
- 2026-10-16 02:28: Posts and agent messages hold only results, blockers and decisions, in two sentences or fewer. "Idle, no task" counts as a blocker: tell Orchestrator (agent rule 3).
- 2026-10-16 02:43:
  - Docs, logs and commits obey the asd-ste skill (/home/box/agent-data/workflows/asd-ste/SKILL.md). Messages to agents obey the caveman skill (/home/box/agent-data/workflows/caveman/SKILL.md).
  - The caveman skill applies only to agent-to-agent messages, including task messages. If a text is in the scope of asd-ste, asd-ste wins (rules/style.md rule 15).
  - (2026-10-16 21:57, Orchestrator confirmed: messages to the owner obey the 20:12 owner entry in "How to work with the owner". From 23:11, they are also in ASD-STE100.)
- 2026-10-16 13:03: Do not ask the owner for approval to start a helper subagent. Start it (rules/owner-messages.md rule 14).
- 2026-10-16 22:00 (owner):
  - All planned changes go on one working branch, integration. Make one commit and one host test for each change. Run only the affected tests. Use time scale 20 only when the code reads X0X_TEST_TIME_SCALE (agent rule 9).
  - Run one gate at the end of the batch. Split or bisect only if that gate fails. Merge Integrator owns the branch order and the gate of the batch.
  - Builders fetch and rebase before each commit and change only the files of their lane. The install is the gated hash of the last batch in a finished set (owner rule 01:57). This replaces the gate for each lane (agent rules 6 and 9 to 11).
- 2026-10-16 22:02 (owner): Each agent, not only the builders, does the Decision check before an action that is hard to undo or shared. The check has five steps (evidence, damage, undo, cause, size) and stop words (shared-work.md rule 4). Changes to the skill go through Orchestrator.
- 2026-10-16 22:03 (owner): Do not wait idle. While you wait for a gate, a review, a merge or an owner reply, do the next task that does not need that result. Only hardware work and edits to the same shared file stay one at a time (agent rule 3).
- 2026-10-16 22:39 (owner): Record the bug first. Before a fix for a device bug, the owner shows the bug once with INLOG (the inbound MIDI recorder) on. The builder makes a host replay test from that log, then fixes the bug. Do not make guess fixes (agent rule 8).
- 2026-10-16 22:59 (owner): Never build a fix before data and a host test prove that it fixes the cause. If a fix does not help on the device, go back to the start. Name the wrong assumption and get the missing data. Do not try a second guess (agent rule 8).
- 2026-10-17 01:49 (owner, through Orchestrator): The owner can skip the INLOG step of the 22:39 entry for one bug. An example is the pad clear fix (agent rule 8).
- 2026-10-16 23:11 (owner): Write each message to the owner in ASD-STE100, also with the plain words of the 20:12 entry. Do not describe a change to the owner before you verify it (agent rule 13, asd-ste skill).
- 2026-10-16 23:15 (Orchestrator decision): Integration is the one working branch. Lanes are short-lived branches named lane/<agent>-<topic> (mb, sb, opt), for example lane/mb-clock. Merge Integrator moves each commit onto integration (agent rules 10 and 11).
- 2026-10-16 23:15 (Orchestrator decision): Orchestrator is the only route for questions to the owner. If an agent quotes an owner report with the build, the report is valid in any chat (agent rule 13).
- 2026-10-16 23:15 (Orchestrator decision): Send one decision message for each item. Give it a tag "supersedes <time>" when it replaces an earlier decision (agent rule 4B).
- 2026-10-16 23:19 (Orchestrator decision): Install only with fm1_install.py --commit and the guard require_gate_pass.sh. Never use the web installer for a team install (agent rule 6).
- 2026-10-16 23:29 (Orchestrator decision): The install guard of rule 6 applies from batch 3 (the batch with the install guard). Until then, Firmware Main Builder reads the index row and the result file before the install. Owner quotes stay word for word inside quotation marks, and STE does not apply to them (rules/install.md rule 6, rules/style.md rule 15).
- 2026-10-17 00:01 (Orchestrator decision, owner approved 00:00): Agent rules version 3. rules/core.md has rules 1, 2 and 4 and an index of the rule files. Read the core at the start of each task, and open each other rule file only at its step. Each rule number in this file is a version 4 number. Version 4 keeps the version 3 numbers and adds rule 6A (rules/docs.md rule 12).
  - At 2026-10-17 02:32, the owner applied version 4: "Agent rules version 4. Apply it now."
- 2026-10-17 02:36 (Orchestrator decision, from an owner question): The owner asked why the agents never read the rules. In the same commit, write each new owner rule also in the rule file of its step (rules/docs.md rule 12).
- 2026-10-17 02:37 (owner): "Treat writing rules the same as writing code." Commit each rule change on a lane branch. Merge it only after the review of Orchestrator (rules/docs.md rule 12A).
- 2026-10-17 03:21 (owner): "start new subagents for each new change. Cause rereading context is really aggressive on the limits" Start a new helper subagent for each new change. Resume a helper only for a fix to its own change. The reason is that a helper that reads an old context again uses much of the usage limit (rules/shared-work.md rule 16).
- 2026-10-17 03:24 (owner, to Orchestrator): "you should follow this rule too" Orchestrator also follows the 03:21 rule. Orchestrator starts a new helper subagent for each new change (rules/shared-work.md rule 16).
- 2026-10-16 23:36 (Orchestrator decision): From integration d6ae80d, builders edit the STE text of BEHAVIOR.md, and quickcheck checks it (ste_strict, rules/commit.md rule 9).
- 2026-10-16 23:36 (Orchestrator decision): Name a batch only by its number and its gated hash, for example batch 1 (9a733fe). Do not name a batch by a branch name (rules/style.md rule 15).
- 2026-10-16 23:32 (owner): Merge the small roles.
  - Merge Integrator does the release packaging. It packages only a hash with a gate PASS and a device PASS, with the same checks as before (CRCs, loop slot backup paths).
  - Docs Keeper writes the owner test checklists and sends them to Orchestrator. A checklist has only the changed items, in plain words and ASD-STE100. The owner copy has no agent notes (rules/docs.md rule 12).
  - Release Packager and Test Plan Writer retire after Docs Keeper logs the batch-1 checklist answers.
- 2026-10-17 00:15 and 00:17 (owner): Start each task of an owner test checklist with its goal, in the words of the owner. Each task starts from any screen (agent rule 12).

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

## Display

- 2026-10-16 19:10 (owner, from Optimizer items C3 and B3):
  - In the XS, S and B faces, only these lower-case letters keep their own glyphs: `a b d k m p r s t x z`. Each other lower-case letter shows as its capital letter, also in sample names.
  - The characters ` { | } ~ show as "?". Symbols outside the font show as ?. The owner approved this on 2026-10-16 at 19:25. See BEHAVIOR.md GEN-55.

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
- 2026-10-17 01:11 (owner, through Orchestrator): Install the release build. The pad colour page (PAD COLOURS) is not necessary now.
- 2026-10-17 01:15 (owner, through Orchestrator): The screen shows each idle clip square at half of the full shade of its colour group.
- 2026-10-17 01:17 and 01:19 (owner, through Orchestrator): Make the idle clip pads slightly brighter. The owner selected the level at 01:19 (2 x the old level).
- 2026-10-17 01:32 (owner, through Orchestrator): Empty slots become brighter too, because Loopy sends the same shade for them (LIM-03).
- 2026-10-17 01:42 and 01:43 (owner, through Orchestrator): This is for the PAD COLOURS page (CLP-79), which is only in dev builds. The LCD square of the selected entry shows brighter, so that the hue of a dark entry is easier to see. The LCD shows the gain below the entry number (for example "BOOST X8"). SEQ REC shows the real level ("REAL LEVEL"), and SEQ REC again shows the gain. The LCD also shows a colour name for the selected entry, for example "#47 BLUE DARK". The pads, the stored values and the export do not change.
- 2026-10-17 01:46 (owner, through Orchestrator): When the Mac sleeps or the USB cable is out, the clip pads go dark. Loopy on the Mac (USB) and Loopy on the iPhone (Bluetooth) run at the same time. Thus the end of the USB link clears only the pads that USB set.
- 2026-10-17 02:49 (owner, through Orchestrator): Regression on c6bf44a (batch 10): in clip mode, the knobs do nothing. Firmware Main Builder records a knob turn on the device, then finds the cause (rules/device-bug.md rule 8). The fix goes in batch 11 (ROADMAP.md). At 03:20, the owner said "Just fix them" (through Orchestrator). After the fix, knobs 2 to 8 edit ARP, CHORD and SCALE again.
- 2026-10-17 03:26 (owner, through Orchestrator): "Arp chord scales are used to play on keybed so loopy can record those". In clip mode, knobs 2 to 8 edit the keybed ARP, CHORD and SCALE, so that Loopy Pro records what the keybed plays.
- 2026-10-17 02:49 (owner, through Orchestrator): In clip mode, a tap on MODE latches the MODE UI open, and a second tap closes it. A hold works as before. Second Builder makes it for batch 11 (ROADMAP.md).
- 2026-10-17 02:50 (owner, through Firmware Main Builder): "And need to show waiting for loopy pro to connect instead of 8x5 grid". Show a "waiting for Loopy Pro to connect" screen instead of the 8x5 grid. Second Builder makes it for batch 11 (ROADMAP.md).
- 2026-10-17 02:52 (owner picks, through Second Builder): A tap on the stage latches it open, and a tap closes it. FADER BANK also closes it. The screen has no top bar, and it shows a help line at the bottom. The stage is the MODE UI of the 02:49 entry. The owner said at 02:51: "The SCALE, CHORD or ARP button" (Second Builder confirmed). Second Builder makes it for batch 11 (ROADMAP.md).

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

Last update: 2026-10-17 03:31.
