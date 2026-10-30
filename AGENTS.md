# x0x37 project instructions

The only authoritative rules file is [docs/rules/workflow.md](docs/rules/workflow.md).
Read that file before work and follow it. `docs/agent-rules.md` and
`docs/rules/core.md` were removed. They are not rules sources.

The feature registry in pull request
[#723](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/723)
is the only record of who owns each feature. That pull request added
`docs/FEATURE-REGISTRY.md` on `wip/one-build`.

Each builder keeps at most one open pull request at a time.

Feature merges stay stopped until CI is green. Progress is tracked in
`docs/workflow/velocity-metrics.md` when that file is in the checkout.
The current baseline is
[velocity-metrics.md on pull request #727](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/docs/bottleneck-audit-20261030/docs/workflow/velocity-metrics.md).
