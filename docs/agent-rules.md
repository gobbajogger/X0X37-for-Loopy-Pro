# Agent Rules

All agents must follow these rules. This Mac root file is the main copy. x0x37/docs/agent-rules.md on smk37 is an exact copy.
In these rules, "the owner" is the user. The "task agent" is the agent that owns a task in the lanes table (AGENTS_AND_ROOMS.md).

1. Commits, logs, docs and checklists follow the asd-ste skill (/home/box/agent-data/workflows/asd-ste/SKILL.md, ASD-STE100). Rules 2 and 13 to 21 give its main points. Agent-to-agent messages follow the caveman skill (/home/box/agent-data/workflows/caveman/SKILL.md). Messages to the owner use plain words (USER_PREFERENCES.md, 2026-10-16 20:12 owner entry). If asd-ste and caveman conflict, asd-ste wins.
2. Write one idea per sentence. Rules 14 and 15 give the sentence length limits.
3. Use the active voice.
4. Do not invent data. Check the source first.
5. Change only the code that the task needs.
6. Record each change in a log file.
7. Do not stop if a test fails. Find the cause. If the cause is in your lane, fix it and test again. Else send it to the task agent of that lane (rule 27).
8. Report only the result of the assigned task.
9. Do not change files that the task does not name. Exception: the updates that rules 6 and 23 to 25 require (log, BEHAVIOR.md, USER_PREFERENCES.md, ROADMAP.md).
10. Keep the original sequencer path unchanged when clip mode is off.
11. If an agent needs a file, attach the file to the SendToAgent message. Or give a path that the agent can read. Do not assume that the agent can see your /tmp.
12. The JieLi toolchain is at /home/box/.jieli/toolchain. The AC79 SDK is at /home/box/fw-AC79_AIoT_SDK. Set JIELI_TOOLCHAIN and AC79_SDK when you build.
13. Use only approved words from ASD-STE100. Use technical names as given.
14. Procedural sentences have a maximum of 20 words.
15. Descriptive sentences have a maximum of 25 words.
16. Do not use the -ing form of a verb except approved words.
17. Do not use helping verbs with past participles.
18. Use only can, must, and will as helping verbs.
19. Do not use semicolons or contractions.
20. Keep one topic per paragraph. Maximum 6 sentences.
21. Use the imperative for procedures.
22. Before you report a defect, compare the behavior with BEHAVIOR.md on the branch integration. That file is the one reference copy. The smk37 copy (x0x37/BEHAVIOR.md) is synced from it. Report a difference as a defect only if it contradicts BEHAVIOR.md. Report behavior that BEHAVIOR.md does not cover as unspecified.
23. If you change intended behavior, update BEHAVIOR.md in the same task.
24. Before you start, read USER_PREFERENCES.md in this folder. Follow the preferences and decisions in it. If the user gives a new permanent preference or decision, add it to USER_PREFERENCES.md.
25. Keep ROADMAP.md in this folder current: when a step finishes or a plan changes, update it.
26. There are no rooms. Agents send messages direct to each other with SendToAgent. Orchestrator relays messages to the owner. If you cannot continue, or a task can run in parallel, send a delegation request to the task agent (see AGENTS_AND_ROOMS.md). Give the task, the inputs (commit, branch or file), the done criteria and the deadline, in two sentences or fewer.
    The task agent works in its own folder (AGENTS_AND_ROOMS.md lists each folder). It replies with the result and the commit hash. Do not do the same work as the task agent at the same time. Only Firmware Main Builder installs firmware. Generic work needs no approval (USER_PREFERENCES.md). A change that the owner can see, hear or play needs the approval of the owner.
27. Each task has one task agent, as the lanes table in AGENTS_AND_ROOMS.md shows. Do not do work outside your lane. Delegate it by rule 26. Give each task to the lightest agent that can do it. If you have no result, blocker or decision, do not send a message. Optimizer gives patches to the builders. Optimizer does not commit to their branches.
28. Docs and logs hold only information that matters for development. This is behavior, decisions, rules, the important attempt results, open items, and the build, install and test steps. Do not write chatter, repeated status, process narration or acknowledgements.
29. Install only the batch hash (rule 39). Before an install notice, the gate must report PASS for the exact tree of that hash. A gate result for the same tree (without *.md files) is valid. All tests must pass. Do not use a list of known failures. The installer must read the image back from the device and compare the CRC.
    Back up the slots before the install. After the install, compare each per-slot CRC with the backup. The CRC32 of a whole .x0xslots file is always 2144df1c (the CRC-32 residue), so it proves nothing.
    Run the install in the foreground, or in a named screen or tmux session that an agent watches. If the link drops, reattach and watch the install to the end and the read-back. Never install in the background.
30. Keep a tag last-good-hw on the newest build that passed a device test. After 2 failed device tests in a row, bisect from last-good-hw. Give each dead branch the name archive/<name> (see LIVE_BRANCHES).
31. Tests read the shared reference files in /workspace/x0x37-refs/schwung.
32. Run hardware tests (device, USB, BLE, installer) one at a time. Run them in parallel only when a plan shows that their results cannot interact.
33. A test checklist holds only the changed items. Do not retest items that a device test already proved. The owner reply has three fields: Connected, Seconds held, Loopy notes.
34. Never log a hardware pass unless the owner names the build that the owner tested.
35. When a build is ready to test, tell Orchestrator. Test Plan Writer sends the checklist to Orchestrator. Orchestrator tells the owner.
36. Test workflow:
- Run tests with X0X_TEST_JOBS=8 (the default). Run scenario tests one at a time.
- Do not run tests for a tree that has only .md changes.
- Write results to /workspace/x0x37-results/<tree>.md. Keep /workspace/x0x37-results/LIVE_BRANCHES and /workspace/x0x37-results/BUDGET.md current.
- The gate script is /workspace/x0x37-refs/gate/x0x37_gate.sh. It has two locks. The box-wide lock is a flock on X0X_GATE_LOCK (default /workspace/x0x37-refs/gate.lock). It stops the tests of two gate runs from any agent at the same time. The second lock, X0X_GATE_WORK/.lock-<hash>, blocks only a second gate run of the same commit.
- Set the time scale with X0X_TEST_TIME_SCALE. Development runs use 20. The gate always uses 1 and writes the time scale in the result file.
37. Firmware Main Builder, Second Builder and Optimizer follow the skill /home/box/agent-data/workflows/x0x37-builder-loop/SKILL.md for each change.
38. All agents share one box. Stop only the processes that you started, by their PIDs. Never use pkill -f or killall on shared scripts (the gate, the tests, the installer).
39. Batch flow (owner, 2026-10-16 22:00). All planned changes go on one working branch, integration. Make one commit for each change, with a host test for that change. Run only the affected tests, with time scale 20. Before each commit, fetch and rebase. Change only the files of your lane.
    Merge Integrator owns the branch order and the batch gate. At the end of the batch, Merge Integrator runs one full gate. Split or bisect the batch only if that gate fails. Do not run a gate for each lane before a handoff. The install is the batch hash with gate PASS (rule 29).
