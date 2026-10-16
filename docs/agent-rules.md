# Agent Rules (version 2, 2026-10-16 22:25)

All agents must follow these 15 rules. This Mac root file is the main copy. x0x37/docs/agent-rules.md on smk37 is an exact copy.
"The owner" is the user. The "task agent" owns a task in the lanes table (AGENTS_AND_ROOMS.md).
Skills: asd-ste (/home/box/agent-data/workflows/asd-ste/SKILL.md), caveman (/home/box/agent-data/workflows/caveman/SKILL.md), x0x37-builder-loop (/home/box/agent-data/workflows/x0x37-builder-loop/SKILL.md). Owner decisions: USER_PREFERENCES.md. Version 1 (41 rules): agent-rules-v1-2026-10-16.md on the Mac, not in git.

1. Docs, logs, commits and checklists use the asd-ste skill. Agent-to-agent messages use the caveman skill. If they conflict, asd-ste wins.
2. Write to the owner in plain words about what the owner hears, feels and does. Send a question to the owner only through Orchestrator or the task agent. Tell the owner before a change that affects how the keyboard plays.
3. Ask the owner only about a change that the owner can see, hear or play. Decide all other changes and continue.
4. Put each planned change on the working branch integration. Make one commit and one host test for each change. Run only the affected tests, at time scale 20. Builders follow the x0x37-builder-loop skill.
5. Fetch and rebase before each commit. Change only the files of your lane. Give other work to its task agent with SendToAgent.
6. Merge Integrator owns the branch order and runs one full gate at the end of the batch. Split or bisect only if that gate fails.
7. Install only the batch hash. The gate must report PASS for its exact tree, with 0 fail and 0 skip. Do not use a list of known failures.
8. Only Firmware Main Builder installs. Before the install, back up the slots and send the owner notice with the exact hash. After it, read the image back, compare each per-slot CRC and report. Tell the owner about slot checks only when one does not match.
9. Never write the bootloader. Run each install in the foreground or in a named screen session that you watch. Never install in the background.
10. Run hardware work and edits to one shared file one at a time. While you wait for a result, do the next task that does not need it.
11. Log a device PASS only for the items that the owner names, with the build. Mark each other item NOT TESTED. Before a fix for a device bug, the owner shows the bug once with INLOG on, and the builder makes a host replay test from that log. Do not make a guess fix.
12. Never push outside the box and the machines of the owner. Never push, merge or package refs-oscuro.
13. Stop only the processes that you started, by their PIDs. Never use pkill -f or killall on a shared script.
14. Before an action that is hard to undo or shared, do the Decision check (x0x37-builder-loop steps 12 to 16). Send skill changes through Orchestrator.
15. Do not invent data. Docs and logs hold only development information. Update BEHAVIOR.md (reference copy on integration), ROADMAP.md, USER_PREFERENCES.md and the log in the same task.
