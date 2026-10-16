# Agent Rules

All agents must follow these rules.

1. Use Simplified Technical English (ASD-STE100). Commits, logs, docs and checklists follow the asd-ste skill (/home/box/agent-data/workflows/asd-ste/SKILL.md). Posts and agent messages follow the caveman skill (/home/box/agent-data/workflows/caveman/SKILL.md). If they conflict, asd-ste wins.
2. Write short sentences. One idea per sentence.
3. Use the active voice.
4. Do not invent data. Check the source first.
5. Change only the code that is required for the task.
6. Record each change in a log file.
7. Do not stop if a test fails. Fix the cause and retest.
8. Report only the result of the assigned task.
9. Do not change files that are not named in the task.
10. Keep the original sequencer path unchanged when clip mode is off.
11. When an agent needs a file, the sender must attach it to the SendToAgent message or give a path that the agent can read. Do not assume the agent can see /workspace/artifacts or /tmp.
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
22. Before you report a defect, compare the behaviour with BEHAVIOR.md in the source tree. Report a difference as a defect only if it contradicts BEHAVIOR.md. Report behaviour that BEHAVIOR.md does not cover as unspecified.
23. If you change intended behaviour, update BEHAVIOR.md in the same task.
24. Before you start, read USER_PREFERENCES.md in this folder. Follow the preferences and decisions in it. If the user gives a new lasting preference or decision, add it to USER_PREFERENCES.md.
25. Keep ROADMAP.md in this folder current: when a step finishes or a plan changes, update it.
26. There are no rooms. Agents send messages direct to each other with SendToAgent. Orchestrator relays messages to the owner. If you are blocked or a task can run in parallel, send a delegation request to the owner of the task (see AGENTS_AND_ROOMS.md). Give the task, the inputs (commit, branch or file), the done criteria and the deadline, in two sentences or fewer. The owner of the task works in its own box folder /workspace/x0x37-<agent> and replies with the result and the commit hash. Do not do the same work as the owner of the task at the same time. Only Firmware Main Builder installs firmware. Generic requests are approved automatically by the agency rule. If a task can change the user experience, notify the user first.
27. Each task has one owner, as the lanes table in AGENTS_AND_ROOMS.md shows. Do not do work outside your lane. Delegate it by rule 26. Give each task to the lightest agent that can do it. If you have no result, blocker or decision, do not send a message.
28. Docs and logs hold only information that matters for development: behavior, decisions, rules, the attempt results that matter, open items, and how to build, install and test. Do not write chatter, repeated status, process narration or acknowledgements.
29. Before an install notice, the gate must report PASS for that exact commit hash. All tests must pass. Do not use a list of known failures. The installer must read the image back from the device and compare the CRC. Run the install in the foreground. A named screen or tmux session that an agent watches is allowed. If the link drops, reattach and watch the install to the end and the read-back. Never install in the background.
30. Keep a tag last-good-hw on the newest build that passed a device test. After 2 failed device tests in a row, bisect from last-good-hw.
31. Tests read the shared reference files in /workspace/x0x37-refs/schwung.
32. Run hardware tests (device, USB, BLE, installer) one at a time. Run them in parallel only when a plan shows that their results cannot interact.
33. A test checklist holds only the changed items. Do not retest items that a device test already proved. The owner reply has three fields: Connected, Seconds held, Loopy notes. Test Plan Writer sends checklists to Orchestrator.
34. Never log a hardware pass unless the owner names the build that was tested.
35. When a build is ready to test, tell Orchestrator. Orchestrator tells the owner.
36. Test workflow:
- Run tests with X0X_TEST_JOBS=8 (the default). Run scenario tests one at a time.
- Do not run tests for a tree that has only .md changes.
- Write results to /workspace/x0x37-results/<tree>.md. Keep /workspace/x0x37-results/LIVE_BRANCHES and /workspace/x0x37-results/BUDGET.md current.
- The gate script is /workspace/x0x37-refs/gate/x0x37_gate.sh. It has two locks. The box-wide lock is a flock on X0X_GATE_LOCK (default /workspace/x0x37-refs/gate.lock). It stops the tests of two gate runs from any agent at the same time. The second lock, X0X_GATE_WORK/.lock-<hash>, blocks only a second gate run of the same commit.
- Set the time scale with X0X_TEST_TIME_SCALE. Development runs use 20. The gate always uses 1 and writes the time scale in the result file.
- Give each dead branch the name archive/<name> (see LIVE_BRANCHES).
- Optimizer gives patches to the builders. Optimizer does not commit to their branches.
