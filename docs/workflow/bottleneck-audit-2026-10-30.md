# Bottleneck audit — 2026-10-30 (Asia/Vladivostok, UTC+10)

Data taken 2026-10-30 20:30 VLAT from GitHub API.

## Ranked bottlenecks
1. **CI is red almost everywhere.** 58 of the last 60 workflow runs failed (2 passed: Pages deploy). Top failing workflows (7 days): SMC Linux host regressions 26, SMC mixer integration validation 9, GitHub Pages 7, web editor host checks 4. Failing step: "Focused SMC host tests". Root cause: host tests broke after firmware API changes (fix in draft #724, plus #708, #710, #721 private headers). Fix: land #721 + #724 first, into one green base; no other merge until CI is green. Owner: Build and Test Runner, then Merge Integrator.
2. **Merge flow stopped.** Merges per day: 26 Oct 145, 27 Oct 21, 28 Oct 28, 29 Oct 5, 30 Oct 1 (#702). 37 PRs are open; ~32 are drafts. Root cause: no green base (item 1) and work goes to many parallel PRs, not into `wip/one-build` (#698). Fix: Merge Integrator owns one queue order; each builder keeps max 1 open PR. Owner: Orchestrator.
3. **PR sprawl / duplicate work.** Overlapping PRs for the same feature: mixer (#714 closed, #717, #725, #688 closed, #686 closed), engines UI (#701 closed, #712), SMC (#687, #700, #709, #679 closed), author policy (#697 closed, #713), Miami opt (#691, #707 closed). Fix: feature registry (#723) as single source of who owns what; Orchestrator closes duplicates after owner of each lane agrees. Owner: Orchestrator.
4. **Docs PRs not merged.** #720 (rules align), #723 (registry), #722, #726 open, not draft for #720/#723. Docs PRs do not need firmware CI. Fix: Merge Integrator merges docs-only PRs at once when no conflict. Owner: Merge Integrator.
5. **Branch sprawl.** 1021 branches. Fix: list merged/closed-PR branches for Roman to approve bulk delete (not done here). Owner: Roman decision.
6. **Rules file location unclear.** `docs/agent-rules.md` was removed (see docs/rules/workflow-history.md); `AGENTS_AND_ROOMS.md` not on main. Bots still cite agent-rules.md. Fix: merge #720 and point AGENTS.md at one rules file. Owner: Orchestrator.
7. **Orchestrator transcript shows no live loop.** Last readable turns date from 25 Oct (GD-001 pilot); also a repeated "reply not sent via SendToUser" re-prompt. Fix: hourly follow-up routine (see below). Owner: Orchestrator.

Not fully checked: transcripts of other bots (only the Orchestrator was read).

## Needs Roman's decision
- Bulk-delete stale branches (item 5).
- Freeze: no feature merges until CI is green.
- Approve closing duplicate PRs (item 3).

## Follow-up routine (proposal)
Every 2 h: Orchestrator runs `docs/workflow/velocity-metrics.md` queries, compares to baseline, and pings the bot that owns the worst metric. Target: CI pass rate > 80%, merges >= 5/day on one-build, open PRs <= 10.
