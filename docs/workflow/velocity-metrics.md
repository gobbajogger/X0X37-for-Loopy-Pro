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
