# Agent Rules (version 2, 2026-10-16 22:25, STE edit 2026-10-16 23:21, terms 23:30)

All agents must follow these 15 rules. This Mac root file is the main copy, and x0x37/docs/agent-rules.md on smk37 is an exact copy. The "task agent" owns a task in the lanes table (AGENTS_AND_ROOMS.md).

The project skills are asd-ste (/home/box/agent-data/workflows/asd-ste/SKILL.md), caveman (/home/box/agent-data/workflows/caveman/SKILL.md) and x0x37-builder-loop (/home/box/agent-data/workflows/x0x37-builder-loop/SKILL.md). USER_PREFERENCES.md holds the owner decisions. Version 1 (41 rules) is agent-rules-v1-2026-10-16.md on the Mac. It is not in git.

Style scope: caveman is for agent-to-agent messages, including task messages. asd-ste is for files, commits, checklists and owner messages.

## Terms (these are not rules)

- owner: the person who owns the project. Do not write "user" for this person.
- Orchestrator: the agent that routes the tasks and relays the messages to the owner. Do not write "parent agent".
- loop slots: the flash slots that hold the imported loops (slots 1 to 3).
- gate: one run of the gate script. Each gate writes one row in /workspace/x0x37-results/index.md.
- Bluetooth (BLE): Bluetooth Low Energy MIDI. In text, write "Bluetooth". Use "BLE" only in names, for example ble-a74, BLE-16 and BLE_ATTEMPTS.md.
- behavior: use this US spelling in all files.
- pairing: the Bluetooth security step between two devices. It includes the bonds. Do not write "bonding".
- colour: British spelling. Use "color" only in code names. Keep "colour" in text.
- owner quotes: Owner quotes stay word for word inside quotation marks. Quotes are exempt from STE.

## Rules

1. Write docs, logs, commits and checklists with the asd-ste skill. Write agent-to-agent messages, including task messages, with the caveman skill. If a text is in the scope of asd-ste, asd-ste wins.
2. Write each message to the owner in ASD-STE100, with plain words about what the owner hears, feels and does. Describe a change to the owner only after you verify it. Orchestrator is the only route for questions to the owner. If an agent quotes an owner report with the build, the report is valid in any chat. Tell the owner before a change that affects how the keyboard plays.
3. Ask the owner only about a change that the owner can see, hear or play. Decide all other changes and continue.
4. Put each planned change in the batch for integration. Make one commit and one host test for each change. Run only the affected tests, and use time scale 20 (X0X_TEST_TIME_SCALE) only when the code reads it. Builders follow the x0x37-builder-loop skill.
5. Integration is the one working branch, and Merge Integrator moves each commit onto it. Lanes are short-lived branches named lane/<agent>-<topic> (mb, sb, opt), for example lane/mb-clock. Fetch and rebase before each commit, and push only your lane branch, never integration. Change only the files of your lane. Give other work to its task agent with SendToAgent.
6. Merge Integrator owns the branch order and moves the lane commits onto integration. At the end of the batch, it requests one gate, and Build and Test Runner runs the gate script. Split or bisect only if that gate fails.
7. Install only the batch hash, after a gate PASS for its exact tree with 0 fail and 0 skip. Only the gate script writes the PASS row in /workspace/x0x37-results/index.md, and the install notice quotes the result file of that row. Do not use a PASS or a "go" in a message as evidence, or a list of known failures. Install only with fm1_install.py --commit and the guard require_gate_pass.sh, and never use the web installer for a team install. This applies from batch 3 (install-gate-guard merged). Until then, Firmware Main Builder reads the index row and the result file himself before the install.
8. Only Firmware Main Builder installs. Before the install, back up the loop slots and send the owner notice with the exact hash. After the install, read the image back, compare each per-slot CRC and report. Tell the owner about slot checks only when one does not match.
9. Never write the bootloader. Run each install in the foreground or in a named screen session that you watch. Never install in the background.
10. Run hardware work and edits to one shared file one at a time. While you wait for a result, do the next task that does not need it. If you have no task, tell Orchestrator.
11. Log a device PASS only for the items and build that the owner names, and mark the others NOT TESTED. For a device bug, the owner first shows the bug once with INLOG on. The builder then makes a host replay test from that log. Make a fix only when data and a host test prove the cause. If a fix fails on the device, do not guess again: name the wrong assumption and get the missing data. If 2 device tests fail one after the other, go back to last-good-hw and bisect.
12. Never push outside the box and the machines of the owner. Never push, merge or package refs-oscuro.
13. Stop only the processes that you started, by their PIDs. Never use pkill -f or killall on a shared script.
14. Before a shared action or an action that is hard to undo, do the Decision check. The Decision check is steps 6 to 10 of the x0x37-builder-loop core card. Send skill changes through Orchestrator. Send one decision message for each item. Give it a tag "supersedes <time>" when it replaces an earlier decision.
15. Do not invent data. Docs and logs hold only development information. Update BEHAVIOR.md (reference copy on integration), ROADMAP.md, USER_PREFERENCES.md and the log in the same task.
