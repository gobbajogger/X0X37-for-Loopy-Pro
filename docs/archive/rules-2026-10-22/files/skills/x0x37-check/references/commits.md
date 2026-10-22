# Commits and lane handoffs

Use commit author `genkigenki <3080081+genkigenki@users.noreply.github.com>`.
Group related repairs into coherent commits with relevant validation.

Check the base and existing edits. Use `lane/<agent>-<topic>` with `mb`, `sb`, `opt`, `btr`, or `dk`.
Use `lane/dk-rules-<topic>` for rules.

Fetch or rebase only when needed and when the approved source is reachable. Never rebase another agent's work.

The assigned integrator owns firmware branch order. Do not push to its active branch.
Push to approved local repositories only when needed. A checked local commit and requested handoff can complete a lane.

Include the commit, files, checks, limits, next action, and one receiving owner in each handoff.
