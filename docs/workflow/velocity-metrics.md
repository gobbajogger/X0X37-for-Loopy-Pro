# Velocity metrics baseline (2026-10-30 20:30 VLAT)

| Metric | Value |
|---|---|
| PRs merged, last 7 days | 200 (145 on 26 Oct; 21, 28, 5, 1 on 27–30 Oct) |
| PRs merged/day, last 3 days | 11.3 (falling: 28 → 5 → 1) |
| Median open-to-merge (7 d) | < 1 min (auto-merged lane PRs); one-build PRs: none merged since #702 |
| CI failure rate (last 60 runs) | 97% (58/60) |
| Open PRs | 37 (most draft) |
| Branches | 1021 |

Queries: `gh pr list --state merged --search "merged:>=DATE"`, `gh run list -L 60 --json conclusion`.

## Follow-up 2026-10-30 21:00 VLAT (UTC+10)

| Metric | Value | Change vs 20:30 baseline |
|---|---|---|
| PRs merged since baseline | 9 (#699, #704, #706, #718, #720, #722, #723, #728, #729), all docs/tests-docs | +9 |
| PRs merged, 30 Oct (UTC day) | 10 | was 1 |
| Firmware PRs merged since #702 | 0 | no change |
| Median open-to-merge (9 merges above) | 140 min (range 2 min to 17 h) | first value |
| CI failure rate (last 60 runs) | 97% (58/60) | no change |
| CI runs since baseline | 0 (last run 09:55 UTC) | CI is not running on new pushes |
| Open PRs | 31 (most draft) | was 37 |

Stalls:
- Open: red CI / no CI on one-build PRs (#724 had 9 pushes after its last run, 09:41 UTC, failed; `wip/one-build` has only bugfinder-smoke, pages and smc-host-regressions workflows). Owner: Build and Test Runner. Nudged 21:00.
- Open: no firmware merge into one-build (#721, #724 still draft). Owner: Merge Integrator after green. Nudged 21:00.
- Open: docs PR #726 mergeable; #727 superseded by merged #729. Owner: Merge Integrator. Nudged 21:00.
- Resolved: docs PR backlog (#720, #722, #723 merged); rules alignment #720 merged.
- Waiting on Roman: branch bulk delete, #687 superseded decision.
