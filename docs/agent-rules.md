# Agent Rules (version 2, 2026-10-16 22:25)

All agents must follow these 15 rules. This Mac root file is the main copy. x0x37/docs/agent-rules.md on smk37 is an exact copy.
"The owner" is the user. The "task agent" owns a task in the lanes table (AGENTS_AND_ROOMS.md).
Skills: asd-ste (/home/box/agent-data/workflows/asd-ste/SKILL.md), caveman (/home/box/agent-data/workflows/caveman/SKILL.md), x0x37-builder-loop (/home/box/agent-data/workflows/x0x37-builder-loop/SKILL.md). Owner decisions: USER_PREFERENCES.md. Version 1 (41 rules): agent-rules-v1-2026-10-16.md on the Mac, not in git.

1. Docs, logs, commits and checklists use the asd-ste skill. Agent-to-agent messages use the caveman skill. If they conflict, asd-ste wins.
2. Write each message to the owner in ASD-STE100. Use plain words about what the owner hears, feels and does. Describe a change to the owner only after you verify it. Orchestrator is the only route for questions to the owner. An owner report given in any chat is valid if it is quoted with the build. Tell the owner before a change that affects how the keyboard plays.
3. Ask the owner only about a change that the owner can see, hear or play. Decide all other changes and continue.
4. Put each planned change in the batch for the working branch integration. Make one commit and one host test for each change. Run only the affected tests. Use time scale 20 (X0X_TEST_TIME_SCALE) only when the code reads it. Builders follow the x0x37-builder-loop skill.
5. Integration is the one working branch. Lanes are short-lived branches named lane/<agent>-<topic> (mb, sb, opt), for example lane/mb-clock. Merge Integrator moves each commit onto integration. Fetch and rebase before each commit. Push only your lane branch, never integration. Change only the files of your lane. Give other work to its task agent with SendToAgent.
6. Merge Integrator owns the branch order and puts the lane commits on integration. At the end of the batch, it requests one full gate. Build and Test Runner runs the gate script. Split or bisect only if that gate fails.
7. Install only the batch hash. The gate must report PASS for its exact tree, with 0 fail and 0 skip. Only the gate script writes the PASS row in /workspace/x0x37-results/index.md. Quote the result file of that row in the install notice. A PASS or a "go" in a message is not evidence. Do not use a list of known failures.
8. Only Firmware Main Builder installs. Before the install, back up the slots and send the owner notice with the exact hash. After it, read the image back, compare each per-slot CRC and report. Tell the owner about slot checks only when one does not match.
9. Never write the bootloader. Run each install in the foreground or in a named screen session that you watch. Never install in the background.
10. Run hardware work and edits to one shared file one at a time. While you wait for a result, do the next task that does not need it. If you have no task, tell Orchestrator.
11. Log a device PASS only for the items that the owner names, with the build. Mark each other item NOT TESTED. Before a fix for a device bug, the owner shows the bug once with INLOG on. The builder makes a host replay test from that log. Build a fix only when data and a host test prove the cause. If a fix does not help on the device, name the wrong assumption and get the missing data. Do not try a second guess. After 2 failed device tests in a row, go back to last-good-hw and bisect.
12. Never push outside the box and the machines of the owner. Never push, merge or package refs-oscuro.
13. Stop only the processes that you started, by their PIDs. Never use pkill -f or killall on a shared script.
14. Before an action that is hard to undo or shared, do the Decision check (x0x37-builder-loop core card, steps 6 to 10). Send skill changes through Orchestrator. Send one decision message for each item. Give it a tag "supersedes <time>" when it replaces an earlier decision.
15. Do not invent data. Docs and logs hold only development information. Update BEHAVIOR.md (reference copy on integration), ROADMAP.md, USER_PREFERENCES.md and the log in the same task.
