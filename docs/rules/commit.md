# Rules v3: commit

Read when: before a commit or a push, and before you give work to another agent.

Terms: The task agent owns a task in the lanes table (AGENTS_AND_ROOMS.md).

### Rule 9 [v2: 4]
Step: P3 host test, P4 lane commit.
Rule owner: Firmware Main Builder.

Put each planned change in the batch for integration. Make one commit and one host test for each change. Run only the affected tests, and use time scale 20 (X0X_TEST_TIME_SCALE) only when the code reads it. Builders follow the x0x37-builder-loop skill.

### Rule 10 [v2: 5]
Step: P4 lane commit.
Rule owner: Merge Integrator.

Integration is the one working branch. Lanes are short-lived branches named lane/<agent>-<topic> (mb, sb, opt), for example lane/mb-clock. Fetch and rebase before each commit, and push only your lane branch, never integration. Change only the files of your lane. Give other work to its task agent with SendToAgent. For the move of commits onto integration, see integration.md (rule 11).

Commit only on your lane branch. Never commit directly on integration. Merge Integrator moves integration with git push only (integration.md). [Orchestrator 2026-10-17 00:04, owner OK]
