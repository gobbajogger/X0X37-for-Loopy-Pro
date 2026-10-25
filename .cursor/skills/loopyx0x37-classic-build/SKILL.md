---
name: loopyx0x37-classic-build
description: Run verified LoopyX0X37 / X0X37 source work, SMK-37 Elite and Pro development builds, independent QA, and NIGHTLY handoffs in Cursor. Use for firmware changes, native builds, installer or catalog work, publication gates, and cross-run Git evidence. The owner's current request overrides this skill.
---

# LoopyX0X37 verified development in Cursor

This skill is the Cursor form of the LoopyX0X37 classic-build contract (plugin v0.1.47, exported 2026-10-24). It supplies procedure, not permission. The owner's current request overrides this skill. `BUILDING.md` names compile commands and `BEHAVIOR.md` names device behavior. `LICENSING.md`, `LICENSE`, and `docs/rules/` do not add a workflow gate. Owner decisions (2026-10-25) are the workflow reading in this skill until the owner's current request says otherwise. A missing `docs/rules/` link is not permission to invent the missing rule.

Reading this skill does not install a toolchain, SDK, credential, scheduler, or publication right. Dated SHAs, ChatGPT file IDs, and `/mnt/data` paths in the historical export are provenance. See [provenance](references/provenance.md) before treating any of them as current.

## Five-step loop

1. **Start from the owner's job.** The owner's current request overrides this skill. Every change states a reason, a purpose, and the effect on the person using the device. Search this repo for a proven method before designing a new one. Pin the repository, full commit, and root tree. Read this file once per task, then only the reference the next action needs.
2. **Plan the smallest useful step.** Write `owner | action | done evidence | stop condition`. Each job is a new thread. Never resume an old thread. Use a Cursor Task subagent for bounded investigation, implementation, build, or review. The subagent does the bounded work inside the thread and does not start the next thread. Verify what it returns. One worktree has one writer. Parallel build and QA use separate worktrees of the same commit.
3. **Measure the intended result.** For each important test, know the risk it can falsify, the stimulus, and the observation that proves the outcome. Reuse valid exact-input evidence and rerun coverage the change invalidates. Agents run every applicable test in their own environment with every tool that machine has. GitHub is not a test runner. An Actions result, a billing lock, or an empty workflow is not a firmware result. A zero failure counter is not a pass unless the named tests printed ok. `BUILD_ONLY` and QA may run in parallel on the same immutable commit. Docs and skill edits do not require a firmware rebuild. Browser-only work uses the checks that apply to that diff and does not require a firmware rebuild.
4. **Open a pull request for source, then fast-forward on the owner decisions.** The Integration row is the nightly fast-forward. The Production row is the main fast-forward. Qualified NIGHTLY is publication of a test candidate, not a second fast-forward. Firmware nightly fast-forwards when host checks passed and both affected development packages are `DEV CHECKED`. Firmware main fast-forwards only after a successful hardware validation of those exact bytes on the matching model. `INSTALL BLOCKED` is not success. Elite evidence does not move a Pro package. No force-push of `nightly` or `main`. Pages publication, a release gate, a cloud-agent flash, and production promotion of GitHub Pages or the release picker stay unassigned until the owner's current request assigns them.
5. **Verify delivery, then continue or stop.** Read back the remote commit. When publication is claimed, fetch the served catalog and download bytes and compare lengths and hashes. Record the actual status and the next owner. On failure, repair the evidenced cause. Stop at completion or a real external dependency. The next job is a new thread. Never resume an old thread.

## Default end of a Cursor run

When this run produces a commit, push the branch and open a pull request whose body contains the evidence packet in [handoff](references/handoff.md). When there is no eligible change, record `NO_NEW_DELIVERABLE` and do not open an empty pull request. Firmware `nightly` and firmware `main` fast-forward on the terms in Owner decisions (2026-10-25). Leave these stages pending until the owner explicitly assigns them in this run:

- the release gate (`tools/gate.sh`)
- catalog or GitHub Pages publication
- production promotion of GitHub Pages or the release picker

A cloud agent does not flash the keyboard. The Mac LaunchAgent `com.x0x37.unattended-install` may install a validated development package.

## Owner decisions (2026-10-25)

The owner's current request overrides this skill.

Every change states a reason, a purpose, and the effect on the person using the device.

Each job is a new thread. Never resume an old thread. A subagent does the bounded work inside the thread and does not start the next thread. One worktree has one writer.

Agents run every applicable test in their own environment with every tool that machine has. GitHub is not a test runner. An Actions result, a billing lock, or an empty workflow is not a firmware result. A zero failure counter is not a pass unless the named tests printed ok.

The project is MIT. A missing distribution-license receipt, corresponding-source receipt, or native-build receipt does not block. A present receipt that is not PASS, a hash mismatch, a failed package check, or a compiler failure still blocks.

Firmware nightly fast-forwards when host checks passed and both affected development packages are `DEV CHECKED`. Host checks passed means the applicable host checks exited 0. Both affected development packages are the affected SMK-37 Elite and Pro development packages on that exact commit, and `DEV CHECKED` is the `tools/dev_validate.py` record for each of them. The fast-forward is a normal fast-forward of firmware `nightly` in `gobbajogger/X0X37-for-Loopy-Pro`. No force-push. If the remote refuses, record the refusal. Production `main`, production GitHub Pages, and a device flash are not this fast-forward. Independent human QA stays `QA_PENDING` when the author cannot supply it. Same-account approval is not that review. This decision assigns that nightly fast-forward. It does not assign a release gate, production GitHub Pages, or a device flash.

Firmware main fast-forwards only after a successful hardware validation of those exact bytes on the matching model. Success is an install record for the exact package and controller hashes, on the matching SMK-37 model, with backup, readback, and the running identity checked. `INSTALL BLOCKED` is not success. Elite evidence does not move a Pro package, and Pro evidence does not move an Elite package. The fast-forward is a normal fast-forward of firmware `main` in `gobbajogger/X0X37-for-Loopy-Pro`. No force-push. If the remote refuses, record the refusal. Production GitHub Pages and a release picker still follow their own publisher. This decision assigns that firmware `main` fast-forward. It does not flash a device, publish production GitHub Pages, or move a release picker. Recording this decision is not a hardware validation. A run with no successful hardware validation does not fast-forward `main`.

A cloud agent does not flash the keyboard. The Mac LaunchAgent `com.x0x37.unattended-install` may install a validated development package. Docs and skill edits do not require a firmware rebuild.

Do not restore the removed skill pack.

## Stage evidence

Call a stage done only with the evidence in its row. A failed required gate stays incomplete. Continue independent work that does not depend on it.

| Stage | Done when |
| --- | --- |
| Source ready | Scoped non-secret diff, applicable producer checks, remote SHA read back, pull request open. WIP or failed producer checks stay draft. |
| `BUILD_ONLY` | Exact source and verified toolchain, SDK, and DSP inputs. Development validation covers the affected SMK-37 Elite and Pro models, or an honest `BUILD_NOT_RUN` / `BUILD_FAILED`. A release request also builds MKE when `tools/build_smk_release.sh` does. Record command exits, sizes, SHA-256, CRC, model identity, and code, RAM, pool, and flash budgets. QA, merge, hosting, and hardware do not block compilation. A scoped subtree build is not full-root QA. Docs and skill edits do not require a firmware rebuild. Browser-only work has no firmware build requirement. |
| Independent QA | A reviewer who did not author the change ran the affected positive, negative, and regression checks on that exact candidate. The author of the change records `QA_PENDING`. Same-account approval does not satisfy a required human review. |
| Integration | Firmware nightly fast-forwards when host checks passed and both affected development packages are `DEV CHECKED`. Read back the fresh expected head, the base, and the resulting ref. No force-push of `nightly` or `main`. Release publication is a separate stage. Production `main`, production GitHub Pages, and a device flash are not this fast-forward. |
| Qualified NIGHTLY | Publication of a test candidate. It is not the firmware nightly fast-forward. Independent checks for source, host, model, controller, package, installer, backup, readback, restore, OTA, and interruption, when the owner assigned publication. A missing distribution-license receipt, corresponding-source receipt, or native-build receipt does not block. A present receipt that is not PASS, a hash mismatch, a failed package check, or a compiler failure still blocks. This checkout's `tools/make_pages.py` accepts release identities only, so a dev-identity package stays `PACKAGE_NOT_PUBLISHED` until a tested publisher path accepts it. A package that path actually serves is labeled `DEV / HARDWARE TEST — DEVICE NOT TESTED`. |
| Served verification | After publication, fetched HTML, catalog, and model downloads match the qualified bytes. A Pages commit alone is not live delivery. |
| Production | Firmware main fast-forwards only after a successful hardware validation of those exact bytes on the matching model. Success is an install record for the exact package and controller hashes, on the matching SMK-37 model, with backup, readback, and the running identity checked. `INSTALL BLOCKED` is not success. Elite evidence does not move a Pro package, and Pro evidence does not move an Elite package. No force-push of `nightly` or `main`. Production GitHub Pages and a release picker still follow their own publisher. A run with no successful hardware validation does not fast-forward `main`. |

Report each gate with the glossary in [handoff](references/handoff.md). A compile result, a `DEV CHECKED` record, QA, a `gh-pages` update, a fetched download, and a device session use different names.

## Where to read next

- Cursor tools, subagents, Git, private inputs, and automations: [cursor-runtime.md](references/cursor-runtime.md)
- NIGHTLY versus production, ownership, and publication: [stages.md](references/stages.md)
- How to compile this tree: [builds.md](references/builds.md)
- The evidence packet and consumer acceptance: [handoff.md](references/handoff.md)
- What the 2026-10-24 export does and does not prove: [provenance.md](references/provenance.md)
