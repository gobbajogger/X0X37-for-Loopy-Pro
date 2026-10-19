# Proposed script checks for Build and Test Runner (from rules-v4-map.md, ENFORCEMENT column)

| # | Rule | Check | Hook point |
|---|---|---|---|
| 1 | core 6A, install 6 (full set) | QUEUE.md lists each queued change with its commit. Refuse if an open line is not an ancestor of the install commit | tools/require_gate_pass.sh (fm1_install.py --commit calls it) |
| 2 | 13 (owner messages) | --owner-msg mode: max 3 sentences, max 20 words each, no hex of 7+ chars, no path, no backticks, max one "?", flag "install" + "?" | ste_check.py. Orchestrator runs it before each relay |
| 3 | 7 (foreground install) | refuse to start if stdin is not a TTY and $STY and $TMUX are empty | tools/fm1_install.py start |
| 4 | 7 (bootloader) | refuse a package whose write range covers the bootloader address | tools/fm1_install.py, before the write |
| 5 | 2 (no pkill -f) | SIGTERM trap writes the sender PID and cwd to the KILLED log | x0x37_gate.sh. status.sh already lists KILLED runs |
| 6 | 1 (push only to box/Mac) | allowlist of remote URLs, block all others | client pre-push hook (box and Mac) |
| 7 | 1 (never package refs-oscuro) | refuse a tree with refs/jieli or refs/oscuro paths | /workspace/x0x37-release-packager script |
| 8 | 10 (lane only, MI moves integration) | A push to integration must have `git push -o by=merge-integrator`. This option is a guard against mistakes, not an access check. A lane/<agent>-* push changes only the files of that lane (see "Lane files" in AGENTS_AND_ROOMS.md). Status: not live. The integration half is on integration (fe95e74, from lane/btr-hook-names), and the installed box hook does not have it yet. The lane-file half goes live only together with script check 31 | server update hook |
| 9 | 10 (branch names) | reject a new branch that is not lane/(mb\|sb\|opt\|btr\|dk)-*, integration, archive/*, smk37 | server update hook |
| 10 | 11 (git push only, no update-ref) | flag an integration tip with no QUICK PASS line in the hook log | status.sh |
| 11 | 11 (one gate per batch) | flag 2+ gate rows for one batch without --force | status.sh |
| 12 | 3 (idle) | count lane/<agent>-* commits as agent output (false STALL at 02:03) | status.sh |
| 13 | 3 (shared script edit) | log when the script file mtime changed during the run | x0x37_gate.sh, status.sh |
| 14 | 3 (hardware one at a time) | Mac lock file, refuse a second install or device run | tools/fm1_install.py |
| 15 | 4 (Decision check stop words) | grep agent logs for "probably", "should work", "just this once" for the hourly audit | status.sh |
| 16 | 8 (record bug first) | a commit tagged "device bug" must add a tests/*inlog* replay test, unless the message names the owner skip (01:49) | quickcheck.sh |
| 17 | 8, builder-loop 1 (test fails on base) | run the new test on the base commit and require FAIL there | quickcheck.sh |
| 18 | 8 (2 device fails -> bisect) | flag 2 consecutive FAIL rows in BLE_ATTEMPTS.md | status.sh |
| 19 | 9 (one host test per change) | warn if a commit changes src/ and no tests/ file | quickcheck.sh |
| 20 | 9 (commit author) | refuse a commit whose author is not genkigenki <3080081+genkigenki@users.noreply.github.com> | quickcheck.sh / server update hook |
| 21 | 12 (do not invent data) | flag a time in a changed *.md that is later than the commit time | quickcheck.sh STE step |
| 22 | 12 (BEHAVIOR.md same task) | flag a commit that names a BEHAVIOR.md ID and does not change BEHAVIOR.md | status.sh or quickcheck.sh |
| 23 | 12 (checklist) | 3 header lines present, 10 tasks or fewer, each task line starts with "Goal:" | new checklist lint (Docs Keeper runs it) |
| 24 | 14 (ask before owner-visible change) | warn if a commit changes an owner-visible BEHAVIOR.md ID and the message has no owner decision time | quickcheck.sh |
| 25 | 4B (supersedes) | a USER_PREFERENCES entry that names an older time must carry "supersedes" | ste_check.py or Docs Keeper lint |
| 26 | 15 (terms) | flag "user", "parent agent", "bonding", "behaviour", "color" outside code | ste_check.py term list |
| 27 | 12 (owner rule pointers) | each "(rules/<file> rule N)" pointer in USER_PREFERENCES.md names a rule that exists in that file. Status: not live. Docs Keeper checks it by hand until Build and Test Runner delivers it | quickcheck.sh STE step |
| 28 | 12A (rule changes on a lane) | a push to smk37 that changes docs/rules/ or docs/agent-rules.md must be a fast-forward to a commit of a lane/dk-rules-* branch. Status: not live | server update hook |
| 29 | 12 (BEHAVIOR.md and ROADMAP.md in the same task) | status.sh flags ROADMAP INSTALLED MISMATCH when the installed hash in smk37:docs/ROADMAP.md is not the hash of the last row of INSTALLED.tsv. It flags BEHAVIOR ITEM MISSING when an integration commit after the last install names an ID that integration:BEHAVIOR.md does not have. Status: on lane/btr-status (bb13f39), not live. See the 02:25 row of rules-breaks.md | status.sh |
| 30 | 9 (test isolation) | status.sh writes only to its output folder (--out DIR or X0X_STATUS_OUT). If X0X_TEST_RUN is not empty, it refuses to write the live folder. Tests use a temporary folder. Status: on lane/btr-status (bb13f39), not live | status.sh, tests/run_tests.sh |
| 31 | 10 (lane files cover all files) | each tracked file on integration matches one or more lanes in "Lane files" in AGENTS_AND_ROOMS.md (an owned row, a shared row or a default). If a file has no lane, the lane-file half of check 8 refuses correct work. Owner: Build and Test Runner. Status: not live. Docs Keeper checked the coverage by hand at 03:29 (integration fe95e74, 439 of 439 files) | lint or host test, quickcheck.sh |
