# Commit

Read when: before a commit or a push, and before you give work to another agent.

### Rule 9
Step: P3 host test, P4 lane commit. Rule owner: Firmware Main Builder.

Put each planned change in the batch for integration. Make one commit and one host test for each change. Run only the affected tests. Use time scale 20 (X0X_TEST_TIME_SCALE) only when the code reads it.

Build, test and analyze on the box copy. Use the Mac only for installs and device access. Use the commit author genkigenki <3080081+genkigenki@users.noreply.github.com>. Do not commit the JieLi files. Only the branch refs-oscuro has them. Builders follow the x0x37-builder-loop skill.

Builders edit the STE text of BEHAVIOR.md, and quickcheck checks it (ste_strict).

### Rule 10
Step: P4 lane commit. Rule owner: Merge Integrator.

Integration is the one working branch. Lanes are short-lived branches named lane/<agent>-<topic> (mb, sb, opt).

Fetch and rebase before each commit. Commit and push only on your lane branch, never on integration. Change only the files of your lane. Do not change the release scripts unless the owner asks. Give other work to its task agent with SendToAgent.
