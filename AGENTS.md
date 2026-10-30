# x0x37 project instructions

Before you call any feature missing or start a feature, check [docs/FEATURE-REGISTRY.md](docs/FEATURE-REGISTRY.md) first.

## Owner DnBOX RULES 1–13 (2026-10-30)

These current owner restrictions override older repo workflow text, other-repo examples, historical prompts, and bundled plugin/upstream skill defaults. Use classic ChatGPT mode at Extra High with the LoopyX0X37 plugin; never start Codex or Work. Every *new task* starts with a plan in a fresh classic Project chat when the host supports creation; never claim a new chat was opened unless verified. Continue an already accepted task without restarting its plan.

Only `gobbajogger/X0X37-for-Loopy-Pro` is permitted for source writes and PRs. Use authenticated connected GitHub access and its **default commit author and committer**, never `genkigenki` or another manually configured author/email. Preserve original source copyright credits and test-only Git fixture identities, which are not author instructions. Start from the task's specified `wip/<lane>` branch or otherwise the verified firmware `nightly` branch; do not use `merge-trial` or `merge-trial-20261029`. Write on your own `wip/` or `chat/` branch, open a PR, and never merge into `main` or `nightly` or force-push.

Only Chicago and Miami engines replace the original bass303 and wobble slots; preserve bit-exact sound, never reintroduce bass303 or `wobble.c`. Load only project revision **13**, with no migration. Never commit the 12 #684 private synth headers (including `state_codec_private.h`, `fx_tables.h`, `miami_factory_typed.h`) or binaries `.img`, `.component`, `.vst3`, `.fwsc`. Never touch bootloader, BTIF `0xC0000–0xC0FFF`, USRTRIM `0xC1000`, efuse or `key_mac`; never run `fm1_rescue.py` or `--force`.

Missing local checkout/SDK is not a general blocker. Run applicable local host checks; SDK **V1.2.1** firmware builds/tests/installs occur on the owner's Mac and require exact commands in the report. Do not substitute SDK V1.2.0 or claim tests/builds/speed/bit-exactness/valgrind/physical results unmeasured. Report state, branch and verified full remote HEAD (or `NOT PUSHED`), PR, blockers (mark owner-only blockers), and next action. Report a pushed SHA only after confirming it on the canonical GitHub repository. Documentation-only work needs relevant docs/static checks, not firmware builds.

Read [x0x37-task](skills/x0x37-task/SKILL.md) at task start and [x0x37-writing](.agents/skills/x0x37-writing/SKILL.md) for output. Load only the project skill relevant to the current step. [x0x37-workflow](skills/x0x37-workflow/SKILL.md) routes code and PR work through an explicitly selected tstack mode; installing a skill selects no mode. Current owner instructions take precedence over saved handoffs and older checkout text.

Keep firmware source integration with the assigned integrator. Firmware `nightly` is the development integration branch; `main` is production and requires same-byte, matching-model hardware validation plus current action-specific authority. Review exact source and artifact evidence and use [scoped checks](skills/x0x37-check/SKILL.md). A documentation-only workflow change does not start a firmware build or automatic successor loop.

Determine device permission for the exact model and action from the latest owner instruction. Do not infer a device write or recovery grant from a workflow mode, a connected device, or an old standing instruction. Do not force push, bypass a protected merge, or treat an incomplete check as PASS.

Optional host tooling gaps may be recorded as NOT_RUN while independent work continues. A check that actually runs and fails remains FAIL. GitHub Actions, billing errors, and review comments are evidence to inspect, not substitutes for local proof. Follow [local proof](docs/rules/local-proof.md) where it applies.

## Where work runs (owner rules)

- Builds and host tests run in classic ChatGPT Chat threads in the LoopyX0X37 project. Start a NEW classic thread for each stage. Never use ChatGPT Work or Codex for builds or tests.
- The ChatGPT Orchestrator only coordinates: plans, handoffs, QA and merge-readiness. It cannot start threads and it never builds.
- A big task goes first to the ChatGPT Orchestrator for a handoff. Then start a new classic thread at Extra High with that handoff. Use Pro only for hard builder tasks.
- If a long chat fails to load, start a new thread only after you verify the newest head on GitHub.
- Rule 1m: no git over the network in chats. Use the GitHub plugin or the GitHub API (zipball or tree) to get source, and use SDK V1.2.1 from the project files.
- `x0x37-capability-first` is a ChatGPT plugin, not a repository script.
- Merge bar: a full cloud pass (the full host suite plus the Elite and Pro builds, 0 fail and 0 skip), or the box result `Box host-suite check: PASS` plus a PR comment that the firmware builds pass.
- The Mac is only for hardware installs and device checks.
- Install only when all queued features are ready. Before the install, back up the loop slots. After the install, read back and compare the per-slot CRC, and check the slots.

## Public development context

The **only permitted Git repository for this DnBOX task's source branches, commits, pushes, and PRs** is `gobbajogger/X0X37-for-Loopy-Pro`. The separate historical `gobbajogger/x0x37-for-Loopy-Pro_nightly` website repository is read-only context, never a push or PR target under current owner DnBOX RULE 3. **Closed DEV means the owner is the sole tester; it does not mean private source.** Nonsecret project source, documentation, and sanitized review evidence may be delivered only to the specified canonical repository on an authorized isolated branch. Internal creation or temporary private Library storage does not itself make those materials confidential or prohibit later authorized delivery. Credentials, unrelated private samples/backups, and proprietary or license-restricted third-party bytes remain excluded. Owner-designated personal samples selected as stock loops may be included in their authorized stock distribution after verifying the exact selection, provenance and intended destination; this does not authorize other personal files. Preserve copyright notices and applicable distribution obligations.

Use the latest action-specific owner authorization. A completed stage's read-only, local-only, or no-publication limit is not a permanent project-wide ban; a later authorized stage may expand that scope. Do not infer a new grant merely from a stage change. Actual tool denials remain binding for the denied operation; do not retry or route around them without the required changed authorization or permitted resolution. Continue independent authorized work. Source delivery, review, build, merge, publication and owner-available results remain separate states.

For queued work and handoffs, apply the [continuation contract](skills/x0x37-workflow/references/handoff.md). Consume the existing request and checkpoint, then perform the next authorized action. A bounded unit limits ownership and concurrency; its report does not end an unfinished outcome. Preserve current delegated owners and continue independent ready work without waiting for owner acknowledgment. Use the maintained development policy at the verified `nightly` revision; older production-main instructions do not reinstate retired stage loops or authorize firmware promotion.

For recurring and queued work, resolve the live `coordination/merge-queue` ref and read the [shared automation contract](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/coordination/merge-queue/coordination/automation/runtime-contract.md), `coordination/automation/roles.json` and relevant request/delivery records at that same commit. New authorized work updates the existing request and acceptance data; keep each role bootstrap stable. Actual permissions, role ownership and action-specific denials still govern.

## Autonomous visual stages

Within the agreed task, visual UI, glyph, menu, website and 3D work follows **mockup → internal independent review → implementation → actual validation**. Reuse unchanged approved presentations. Label mockups as proposals; they are not implemented renders. Resolve blocking design findings before implementation, then validate the actual candidate-pinned render or interaction and report it before rollout. Report each meaningful completed stage with evidence and its next action, then continue the next authorized step without waiting for an owner reply. These stage reports and previews are informative, not another aesthetic approval gate.

This supersedes earlier owner-aesthetic-response waits within the agreed scope, not actual preview evidence or independent correctness checks. A required UI-completion-before-boot dependency means finish and validate the UI stage first, not wait for the owner to answer. Current action-specific authority, real tool denials, licensing, device validation, production and publication boundaries remain. A materially new scope or action still needs its applicable authority; continue independent authorized work meanwhile.
