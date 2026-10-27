# Provenance

This skill adapts the single-file export of plugin `loopyx0x37-classic-build-handoff`, verified version **0.1.47**, release `pluginrel_6adce6992d408191b7df8bf46ec27616`, retrieved 2026-10-24 UTC. The export contained 52 UTF-8 files. Their original SHA-256 values attest to that package text. They do not attest to compiled artifacts or to the current Git head.

The export is historical provenance. The current root `AGENTS.md` and [project workflow](../../../../docs/rules/workflow.md) govern this adapter. Preserve pinned candidate evidence, build-before-publication, independent QA and the NIGHTLY/production distinction. Read actual build and publisher implementations before using their commands; a script cannot grant authority or override a current denial. Discover tools in the actual executor. This adapter neither installs other providers' routes nor proves that a currently exposed route is unavailable.

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

## Current authority and historical routing

The export's dated ownership, provider routing and force-publisher examples are
historical. Apply the current [project workflow](../../../../docs/rules/workflow.md),
live target rules and actual tool capabilities. Preserve existing delegated owners
and applicable standing permission; no repetition in a new run is required.
An old role name alone does not assign that role to this worker.

The removal of older project skills from main at `5b38047` was historical repository
state, not a permanent prohibition on maintained rule repairs. This adapter is
subordinate to root AGENTS and the maintained workflow. It does not create schedules,
grant protected writes or activate a new thread for each stage.
