---
name: classic-thread-build-to-git
description: Use in classic ChatGPT threads for file-based software or firmware work that should end with verified source changes, appropriate tests, real build artifacts, installer or deployment checks, and an approved Git push. Discover available tools before declaring a task blocked; coordinate staged work without treating a draft or CI startup failure as a successful build.
---

# Classic ChatGPT: from files to a verified Git delivery

## Activation and authority

Use this skill when a classic ChatGPT thread is asked to inspect or change repository files and deliver working software, firmware, a packaged installer, a site, or a Git commit/push. Apply it to the **current task**, not to every discussion of code. It is a workflow guide, not permission to access new resources, install firmware, expose private code, force-push, or publish a release.

**Finish the authorized outcome rather than stopping at a plan, diff, or untested archive.** A finished result has evidence for every applicable requested stage. If one stage is genuinely unavailable, finish all independent stages, identify the exact missing capability or approval, and hand off the smallest remaining operation. Never say that a blocked stage passed.

Current user instructions and maintained repository rules override this skill. At startup, inspect the checked-out revision's `AGENTS.md`, project rules, build instructions, licensing, and active ownership. If those files are absent, discover the actual repository layout rather than treating examples here as authoritative. Do not turn a documentation-only task into a firmware release.

## 1. Discover the environment before saying a tool is unavailable

Build a short **capability ledger** from evidence, not from memory. Mark each capability `available`, `needs connection`, `needs setup`, or `unavailable`:

| Capability | First place to look | What it can prove |
| --- | --- | --- |
| Existing conversation/Project/Library files | File tools, attachments, mounted files | Exact text or file bytes; not an assumed repository checkout |
| GitHub source and write access | Connected GitHub tools discoverable through `functions.exec` / `ALL_TOOLS` | Remote paths, refs, commits, PRs, text edits, pushes, workflow results |
| Native execution and file creation | `container.exec`, appropriate Python tools, or an authorized cloud workstation | OS, working directory, dependencies, test exits, binary build output |
| Agent skills and instructions | Installed skill catalog, repository `AGENTS.md`, `skills/` | Required conventions, commands, and safety gates |
| CI and artifact evidence | Connected GitHub checks, workflow jobs/logs, uploaded artifacts | Whether a run actually executed and produced verifiable outputs |
| Public reference information | Web lookup for official documentation | External specifications; never a substitute for private source or live test logs |
| Specialized execution | Available coding tools or Codex, **only if** an exact authorized operation cannot be done here | Bounded source inspection, test/build, packaging, or verification with returned evidence |

Examples in this ChatGPT environment:

- Enumerate relevant connected tools with `functions.exec`, filtering `ALL_TOOLS` by `mcp__GitHub__`, `files__`, or `skills__`. Read their current signatures before calling. The presence of a GitHub connector does **not** imply a shell with the repository mounted.
- Verify the actual execution machine with `pwd`, `uname -sm`, `git status`, and executable/dependency checks **in the target environment**. The chat's sandbox and a user's Mac or hardware are different machines until proven otherwise.
- If an uploaded file or older thread gives a path, verify it exists in this session. Do not announce build or local-device access based on a handoff note.
- A GitHub API can edit text and refs but cannot by itself compile firmware. Use a real runner with required SDK, or report the missing runner specifically.
- Before saying “cannot build/push,” inspect available connected GitHub actions, local/container tools, documented project CI, and any already-authorized execution environment. Do not substitute a simulated or speculative build.

When tools for an exact authorized step are missing, create **one compact Codex/execution handoff**, if such an execution channel is available and authorized: goal; repository and edit lane; exact starting commit and input files; relevant skill and rule paths; permitted operations; command/check requirements; destination permissions; expected artifact identities; completion evidence. Do not start duplicate agents, take firmware integration ownership, poll idle work, or silently transfer installation authority. If no execution channel exists, give an explicit blocker and the exact command or missing dependency.

## 2. Establish scope, source identity, and ownership

1. Identify the actual repository **full name, visibility, remote URL, base branch, commit SHA, target model(s), environment, and requested destination**. Read source from the approved repository, not a similarly named upstream fork.
2. Inspect relevant branches, diffs, worktrees, other active threads/agents, and handoffs before changing shared files. Use one isolated lane. Only the assigned integrator merges firmware lanes.
3. Classify the task: docs-only; host/web code; firmware development; device install; authorized release/publication. Development is the default. Do not infer release authorization from words such as “finish,” “build,” “test,” or “push.”
4. Record the approved push remote and ref separately from the installer-page destination. A repository may be **public even if older handoffs call it private**; check live visibility before transmitting sources or binaries.
5. Preserve sample/loop/user data, licensing, provenance, model identity, flash limits, memory budgets, and guarded install paths. Never commit proprietary SDK/toolchain files, private credentials, or forbidden source trees.

## 3. Change the files and test what could break

- Read source and test contracts first. Find a root cause, implement the smallest complete repair, and add an effective regression for meaningful bugs. Do not call a code edit a fix until checks support it.
- Choose checks from the diff: syntax/static analysis or lint; unit/host regressions; integration/mock-hardware checks; build-system checks; installer/site tests. Inspect scripts and flags before running them. Record command, input revision, exit code, relevant logs, and coverage.
- Use a native build for code changes where a real target artifact is requested. Static checks alone do not make a `.fwsc` (or any other binary) installable. Mocked MIDI/device tests are not hardware tests.
- For supported target hardware, verify model identity, configuration, package structure, memory/flash limits, reproducible source/tool inputs, and required controller image. A successful shell exit without inspectable artifact bytes is insufficient.
- When checks fail, diagnose and repair within authorization, then rerun invalidated checks. Do not bypass tests, alter acceptance thresholds, relabel a partial pass, or push a known bad candidate as finished.
- Documentation-only edits need relevant link/content/rule checks, not fake firmware builds.

## 4. Build and attest the actual deliverables

For each applicable output, produce an **artifact manifest** that records:

- Repository/ref and immutable source commit/tree used by the build.
- Target model, build configuration, SDK/toolchain/reference versions or validation input identity.
- Exact artifact filename, byte length, SHA-256, and format-specific CRC/identity information when defined by the project.
- Check names, exit status, log locations, and applicable build/flash/RAM/pool limits.
- Whether the result is a static/host PASS, development build, formal release-gate PASS, installation PASS, or physical-device PASS. These states are **not interchangeable**.

Do not attach an old binary to a new commit. Validate that the generated installer contains the **same checked bytes** as the approved package and the correct model guard. If artifact files cannot be mounted into the publication environment, explicitly leave publication blocked; do not invent a successful upload.

## 5. Installer or GitHub Pages delivery is its own checked operation

1. Identify the real installer generator, its publication branch/workflow, version catalog, model picker, and source of firmware downloads. Read deployment commands before using them; some scripts rewrite or force-update `gh-pages`.
2. Test the generated site and mocked upgrade/downgrade/sample-preservation flows. Inspect links, model identity, checksums, assets, and whether any older published versions would be removed. A generated site folder is **not** a deployed site.
3. Keep an internal/local **development installer bundle** separate from a public release. Public firmware publication requires its own authorization and all project-specific release/device gates. A source-only installer shell may be publishable separately if explicitly authorized and verified.
4. On an approved publication, publish the exact validated artifact and page **without overwriting unrelated versions or losing existing pages**. Verify the remote deployment commit and, where accessible, the resulting URL/catalog/download checksum. A queued Action or `startup_failure` is not a deploy PASS.
5. If only source Git push is approved, push source to that destination and report the installer page as **not published**. Do not interpret approval for a repository push as approval for a separate public web/firmware destination.

## 6. Commit, push, and verify remote identity

- Check source/license safety, clean expected diff, generated-file boundaries, author policy, and selected tests before commit. Use a coherent commit with the actual validation inputs.
- Follow repository lane, integrator, install, and stage-order rules. A GitHub connector create/update action may create a real remote commit without a local clone, but it does not execute host tests. Never claim local tests based solely on its success.
- Push **only** to an approved remote/ref. Prefer fast-forward or reviewed PR flow; do not force-push shared firmware/installer branches without specific authorization and a verified protection/rollback plan.
- Read the resulting remote branch/ref SHA and compare it with the intended commit. Record both, plus any PR and CI result. “Committed” is not “pushed”; “pushed” is not “merged”; “merged” is not “published.”
- If the active project's staged workflow requires installation before final `main` push, preserve that order. A missing device leaves that step pending; it does not turn a build into a device-tested release.

## 7. End-of-task contract: evidence, not promises

Report the smallest accurate result for each requested component, with an exact link/path where verified:

| Item | Required status/evidence |
| --- | --- |
| Source | Exact repo, lane, base, resulting commit, changed files |
| Checks | Named tests/static checks, pass/fail/blocked, recorded logs and reason for coverage |
| Firmware/build | Exact model/config, artifact file, size, SHA-256, format/CRC, budget status |
| Installer package | Matching package and controller, local bundle/manifest, validation status |
| Installer page | Local-only vs published, page ref/deploy result/URL, download identity |
| Git | Commit SHA, pushed remote/ref SHA, PR/merge status if relevant |
| Device | Not tested vs installed/tested; backup and readback evidence if applicable |
| Blockers | Only unresolved prerequisites; one narrow follow-on operation and owner |

Do not present a plan as completed work. Do not say “fixed,” “built,” “installed,” “deployed,” or “pushed” without matching evidence. A helpful partial result is preferable to a fictitious complete delivery.

## Project adapter

For **X0X37-for-Loopy-Pro**, load [references/x0x37.md](references/x0x37.md) before planning tests, target builds, packaging, GitHub Pages updates, or an execution handoff. Its commands are **examples verified against repository files at the time of authoring**, not permission to override the current checked-out rules.
