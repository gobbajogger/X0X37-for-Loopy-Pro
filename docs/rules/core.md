# Core rules

### Rule 1: protect data and sources

Keep code on owner machines and approved local repositories. No external push
without an explicit owner request. Never push, merge, or package `refs-oscuro`.
Keep proprietary JieLi SDK/toolchain files out of commits and bundles.
Preserve licensing, target model guards, flash/RAM limits, features, and sample quality.

### Rule 2: process ownership

Stop only your own processes, by PID. No `pkill -f` or `killall` on shared scripts.

### Rule 4: shared actions

Use isolated lanes. Preserve other agents' edits. Before shared or hard to undo
work, apply [the Decision check](shared-work.md).

### Rule 6A: finish authorized work

Complete authorized changes, relevant checks, review, commit, and requested
handoff in a supported environment. Missing hardware, box access, or retired
roles do not block independent local completion. Name each real dependency and
next action; do not poll or request the same approval endlessly.

Report evidence separately: source review, host tests, target build, formal
gate, install, device test. Name the commit/tree, configuration, checks, and
unrun checks. A focused PASS is not a gate or device PASS.

Install only within authorized scope, through the designated installer, with
the whole queued set ready and [install guards](install.md) passed. Never install
a fallback just to make the keyboard playable. Local completion grants no
install or release permission.

### Rule 12A: rule changes

Treat rules as code; follow [docs](docs.md).

### Rule 16: bounded delegation

Use helpers when requested or when required for independent review. Give bounded
scope and done criteria; reuse them for follow-up. The task agent owns completion.
