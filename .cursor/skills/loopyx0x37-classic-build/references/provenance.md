# Provenance

This skill adapts the single-file export of plugin `loopyx0x37-classic-build-handoff`, verified version **0.1.47**, release `pluginrel_6adce6992d408191b7df8bf46ec27616`, retrieved 2026-10-24 UTC. The export contained 52 UTF-8 files. Their original SHA-256 values attest to that package text. They do not attest to compiled artifacts or to the current Git head.

The owner's current request overrides this skill. Docs and skill edits do not require a firmware rebuild. The export's canonical contract is preserved here as procedure: one pinned candidate, separate stage evidence, build-before-publication for firmware, independent QA as its own stage, nightly test candidates distinct from production, and no hosted GitHub runner for firmware builds. Independent QA stays `QA_PENDING` when the author cannot supply it and does not block the firmware nightly fast-forward. Where that export disagrees with this checkout's tools, the checkout's tools win: the commands in `BUILDING.md`, `tools/make_pages.py`, `tools/publish_pages.sh`, `tools/dev_validate.py`, and `tools/build.py`. Those tools do not replace Owner decisions (2026-10-25). Cursor execution replaces ChatGPT Library, Codex cloud tasks, Work agents, Dots schedules, and plugin-creator APIs. Those routes are not installed by this skill.

## Do not replay the 2026-10-23 patches

The export embeds `native-replay-20261023`, including `reproduce_20261023.sh` and four source-projection scripts. They reconstruct one historical firmware subtree (`19840d87388fb1f5734d9765bf8a2c0b7d3ebaaf`) from a synthetic snapshot. They are not in this skill. Do not apply them to the current tree. A current build uses the checked-out commit and [builds.md](builds.md).

Historical identities, for recognition only:

| Item | Identity recorded 2026-10-23 |
| --- | --- |
| Toolchain zip `jieli-linux-toolchains-20260730.1.zip` | SHA-256 `ed379c483468c44f10cf3be7db32356b9623e16320a021cf2a4e06ec96ab5f19`, Clang 4.0.1 |
| Private SDK kit `x0x37-private-build-inputs-20261021.zip` | SHA-256 `2570c8f9472968afb16ea94372a05edda2e1f9c72d22b2431688c87d906c4805`, 11 pinned V1.2.1 files |
| Bootstrap source zip | Base `7537a46`. Not current `main` or a current pull request. |
| Inspected `main` tip in the 2026-10-23 note. Nightly was created from this commit. It is not a current `nightly` tip. | `c9718ae833eb64a5191c350922593df8a36017c9` |

Refresh every SHA against live Git before acting. If a file is absent, record the absence. Do not invent a Library file ID or a `/mnt/data` path.

## Authority that does not carry over

The export describes standing nightly publication permission and hourly specialist roles (Safe Merge, Hourly Build, Catalog Sync, Pages publisher, and others). Those roles are a scope map from that scheduler. They are not assignments in a Cursor run. Discover live branch protection and pull-request state. The owner's current request overrides this skill. Owner decisions (2026-10-25) assign the firmware nightly fast-forward and the firmware main fast-forward. Firmware nightly fast-forwards when host checks passed and both affected development packages are `DEV CHECKED`. Firmware main fast-forwards only after a successful hardware validation of those exact bytes on the matching model. `INSTALL BLOCKED` is not success. Elite evidence does not move a Pro package. No force-push of `nightly` or `main`.

Do not restore the removed skill pack. Project skills that once lived at `skills/x0x37-*/SKILL.md` were removed from `main` by `5b38047` ("Remove project rules and skills"). This directory is the current Cursor skill.
