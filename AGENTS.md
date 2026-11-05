# x0x37 project instructions

Read the [current owner contract](docs/rules/current-owner-contract.md) first for
cloud execution, meaningful stage handoffs, Grok delivery and docs ownership.
It supersedes conflicting older process routing while preserving target safeguards.

Before you call any feature missing or start a feature, check [docs/FEATURE-REGISTRY.md](docs/FEATURE-REGISTRY.md) first.

Read [x0x37-task](skills/x0x37-task/SKILL.md) at task start and [x0x37-writing](.agents/skills/x0x37-writing/SKILL.md) for output. Load only the project skill relevant to the current step. [x0x37-workflow](skills/x0x37-workflow/SKILL.md) routes code and PR work through an explicitly selected tstack mode; installing a skill selects no mode. Current owner instructions take precedence over saved handoffs and older checkout text.

Keep firmware source integration with the assigned integrator under Grok. Resolve the current staging branch and exact candidate before integration; `wip/one-build`, development `nightly` and production `main` have different roles under the [current owner contract](docs/rules/current-owner-contract.md). Review exact source and artifact evidence and use [scoped checks](skills/x0x37-check/SKILL.md). A documentation-only workflow change does not start a firmware build or automatic successor loop.

Determine device permission for the exact model and action from the latest owner instruction. Do not infer a device write or recovery grant from a workflow mode, a connected device, or an old standing instruction. Do not force push, bypass a protected merge, or treat an incomplete check as PASS.

For firmware integration and packaging qualification, require the full host test suite plus the Elite and Pro builds with 0 fail / 0 skip. NOT_RUN does not meet this bar for required checks. Documentation-only changes require content, link and policy checks, not firmware builds. A check that runs and fails remains FAIL. Review comments are evidence to inspect, not substitutes for local proof. Follow [local proof](docs/rules/local-proof.md) where it applies.

## Where work runs

Use the [current owner contract](docs/rules/current-owner-contract.md) and
[task routing](skills/x0x37-task/references/routing.md). Work in the cloud through
Grok's assigned stages and qualified executors. Do not use the owner's Mac.
Use the GitHub plugin or API for source delivery; no network Git in chats.
`x0x37-capability-first` is a ChatGPT plugin, not a repository script.
Never change `.github/` or add/edit GitHub Actions. Keep SDK V1.2.1 and the full
host plus Elite/Pro zero-failure, zero-skip qualification bar above. Agents never
install or flash; preserve model, backup/readback/CRC and queued-feature safeguards.

## Public development context

Both `gobbajogger/X0X37-for-Loopy-Pro` and `gobbajogger/x0x37-for-Loopy-Pro_nightly` are public project repositories. **Closed DEV means the owner is the sole tester; it does not mean private source.** Project source, documentation, sanitized test/review evidence and nonsecret provenance may be delivered to their authorized public destinations. Internal creation or temporary private Library storage does not itself make those materials confidential or prohibit later authorized delivery. Credentials, unrelated private samples/backups, and proprietary or license-restricted third-party bytes remain excluded. Owner-designated personal samples selected as stock loops may be included in their authorized stock distribution after verifying the exact selection, provenance and intended destination; this does not authorize other personal files. Preserve copyright notices and applicable distribution obligations.

Never commit binaries: `.img`, `.component`, `.vst3` or `.fwsc` files. The approved exception is the Miami `.f32` golden files (the golden DSP renders used by the host tests): commit them with the tests that use them. Private headers (`*_private.h`, including the vendor and wobble headers, and the #684 synth headers such as `state_codec_private.h`, `fx_tables.h` and `miami_factory_typed.h`) stay in the repository. They are not a security problem; do not remove or flag them. This rule replaces the earlier rule against committing the #684 private synth headers.

Use the latest action-specific owner authorization. A completed stage's read-only, local-only, or no-publication limit is not a permanent project-wide ban; a later authorized stage may expand that scope. Do not infer a new grant merely from a stage change. Actual tool denials remain binding for the denied operation; do not retry or route around them without the required changed authorization or permitted resolution. Continue independent authorized work. Source delivery, review, build, merge, publication and owner-available results remain separate states.

For queued work and handoffs, apply the [continuation contract](skills/x0x37-workflow/references/handoff.md). Consume the existing request and checkpoint, then perform the next authorized action. A bounded unit limits ownership and concurrency; its report does not end an unfinished outcome. Preserve current delegated owners and continue independent ready work without waiting for owner acknowledgment. Use the maintained development policy at the verified `nightly` revision; older production-main instructions do not reinstate retired stage loops or authorize firmware promotion.

When the task uses the merge queue or shared automation, or updates request/acceptance records, resolve the live `coordination/merge-queue` ref and read the [shared automation contract](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/coordination/merge-queue/coordination/automation/runtime-contract.md), `coordination/automation/roles.json` and relevant request/delivery records at that same commit. New authorized work updates the existing request and acceptance data; keep each role bootstrap stable. Skip this for isolated docs-only or advisory-only work that does not touch those refs. Actual permissions, role ownership and action-specific denials still govern.

## Autonomous visual stages

Within the agreed task, presentation/UX changes to visual UI, glyphs, menus, the website and 3D work follow **mockup → internal independent review → implementation → actual validation**. Typo or factual status-text edits skip the mockup stage: implement, then run link/content checks. Reuse unchanged approved presentations. Label mockups as proposals; they are not implemented renders. Resolve blocking design findings before implementation, then validate the actual candidate-pinned render or interaction and report it before rollout. Report each meaningful completed stage with evidence and its next action, then continue the next authorized step without waiting for an owner reply. These stage reports and previews are informative, not another aesthetic approval gate.

This supersedes earlier owner-aesthetic-response waits within the agreed scope, not actual preview evidence or independent correctness checks. A required UI-completion-before-boot dependency means finish and validate the UI stage first, not wait for the owner to answer. Current action-specific authority, real tool denials, licensing, device validation, production and publication boundaries remain. A materially new scope or action still needs its applicable authority; continue independent authorized work meanwhile.
