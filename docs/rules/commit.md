# Tests, commits, and handoffs

Read before a build, test, commit, push, or handoff.

### Rule 9: supported environments and truthful checks

Build and test in a supported environment with the required tools. Native host tests can run on macOS or Linux when the test supports it. The JieLi target tools need Linux x86-64; a working local Linux container is valid. The box is one option, not a completion requirement. Read `BUILDING.md` and the script header. Configure paths for the environment in use. Never change `HOME` to pretend to be another machine.

Use `JIELI_TOOLCHAIN` or `X0X_TOOLCHAIN`, `AC79_SDK`, and the reference variables for actual mounted paths. Use a single compiler executable for `CC` and `CXX` (for example `CC=cc CXX=c++`). Do not put compiler flags in the executable name. Keep the gate reference pins exact. A missing optional reference blocks that reference check, not all independent host tests.

Run the affected tests and required checks. Add a regression test when it proves the reported fault or guards a meaningful behavior. Docs edits need docs checks, not a firmware build for every sentence. Use `X0X_TEST_TIME_SCALE` only for tests that read it; formal gates use their fixed real time scale. Tests use temporary folders and repositories. They never write live results, status files, queues, or shared repositories.

Use commit author `genkigenki <3080081+genkigenki@users.noreply.github.com>`. Keep each commit coherent. Several related repairs can share one commit and one relevant validation run. Report exact checks and limits. Keep required code, static RAM, and pool margins; do not reduce features or sample quality to make a check pass.

### Rule 10: isolated lanes

Use a short lived branch `lane/<agent>-<topic>` (`mb`, `sb`, `opt`, `btr`, or `dk`). Rule repairs use `lane/dk-rules-<topic>`. Check the base and existing edits before work. Fetch or rebase when the approved source is reachable and needed; do not require a network fetch for a clean local docs commit. Never rebase another agent's work.

The current integration task owns firmware integration and branch order. Follow the current task assignment, not the IDs in an old roster. Coordinate a file overlap before editing. Complete an authorized repair in your own scope; do not pass it round retired roles. A handoff gives the commit, files, checks, limitations, and next action. One receiving agent owns each remaining action.

Push only to approved local repositories when the task needs it. A local commit and a concrete handoff can complete a lane task. Do not push directly onto another agent's active integration branch. The owner can authorize rule and enforcement script repairs together.
