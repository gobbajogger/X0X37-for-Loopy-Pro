---
name: loopyx0x37-classic-build
description: Run verified LoopyX0X37 / X0X37 source work, SMK-37 Elite and Pro development builds, independent QA, and NIGHTLY handoffs in Cursor. Use for firmware changes, native builds, installer or catalog work, publication gates, and cross-run Git evidence. The user's request and the checked-out repository rules override this procedure.
---

# LoopyX0X37 verified development in Cursor

This skill is the Cursor form of the LoopyX0X37 classic-build contract (plugin v0.1.47, exported 2026-10-24). It supplies procedure, not permission. The user's current request, `BUILDING.md`, `LICENSING.md`, `BEHAVIOR.md`, and any `docs/rules/` files in the checkout govern. A missing `docs/rules/` link is not permission to invent the missing rule.

Reading this skill does not install a toolchain, SDK, credential, scheduler, or publication right. Dated SHAs, ChatGPT file IDs, and `/mnt/data` paths in the historical export are provenance. See [provenance](references/provenance.md) before treating any of them as current.

## Five-step loop

1. **Start from the user's job.** Name the experience that should become clearer, the friction to remove, and the invariants. Search this repo for a proven method before designing a new one. Pin the repository, full commit, and root tree. Read this file once per task, then only the reference the next action needs.
2. **Plan the smallest useful step.** Write `owner | action | done evidence | stop condition`. Use a Cursor Task subagent for bounded investigation, implementation, build, or review. Verify what it returns. One worktree has one writer. Parallel build and QA use separate worktrees of the same commit.
3. **Measure the intended result.** For each important test, know the risk it can falsify, the stimulus, and the observation that proves the outcome. Reuse valid exact-input evidence and rerun coverage the change invalidates. A passing count is not quality. `BUILD_ONLY` and QA may run in parallel on the same immutable commit. Docs, skills, and browser-only work use the checks that apply to that diff and do not require a firmware rebuild.
4. **Integrate or publish only when that stage is the assigned job** and its evidence in the stage table exists. Open a pull request for source. Merging, Pages publication, a release gate, a device install, and production promotion are separate authorizations.
5. **Verify delivery, then continue or stop.** Read back the remote commit. When publication is claimed, fetch the served catalog and download bytes and compare lengths and hashes. Record the actual status and the next owner. On failure, repair the evidenced cause. Stop at completion, a real external dependency, or a checkpoint that names the resume evidence.

## Default end of a Cursor run

When this run produces a commit, push the branch and open a pull request whose body contains the evidence packet in [handoff](references/handoff.md). When there is no eligible change, record `NO_NEW_DELIVERABLE` and do not open an empty pull request. Leave these stages pending until the user explicitly assigns them in this run:

- the release gate (`tools/gate.sh`)
- merge into `nightly` or `main`
- catalog or GitHub Pages publication
- device install or flash
- production promotion

## Owner decision, 2026-10-25

Automatically validated firmware merges without another owner prompt.

Automatic validation means the applicable host checks exited 0 and `tools/dev_validate.py` recorded `DEV CHECKED` for the affected SMK-37 Elite and Pro development packages on that exact commit. A missing distribution-license receipt, corresponding-source receipt, or native-build receipt does not block this, because the owner said the project is MIT and those are not stops. A hash mismatch, a failed package check, or a compiler failure still blocks.

The merge is a normal fast-forward of firmware `nightly` in `gobbajogger/X0X37-for-Loopy-Pro`. If the remote refuses, record the refusal and do not force-push. Production `main`, production GitHub Pages, and a device flash are not this merge. Independent human QA stays `QA_PENDING` when the author cannot supply it. Same-account approval is not that review.

This dated decision is the assignment for that `nightly` fast-forward only. It does not assign a release gate, a merge to production `main`, production GitHub Pages, or a device flash.

A draft or failed producer check stays a draft pull request. A source-complete change with its producer checks recorded can be ready for review. Ready for review is not QA approval, merge eligibility, or installability.

## Stage evidence

Call a stage done only with the evidence in its row. A failed required gate stays incomplete. Continue independent work that does not depend on it.

| Stage | Done when |
| --- | --- |
| Source ready | Scoped non-secret diff, applicable producer checks, remote SHA read back, pull request open. WIP or failed producer checks stay draft. |
| `BUILD_ONLY` | Exact source and verified toolchain, SDK, and DSP inputs. Development validation covers the affected SMK-37 Elite and Pro models, or an honest `BUILD_NOT_RUN` / `BUILD_FAILED`. A release request also builds MKE when `tools/build_smk_release.sh` does. Record command exits, sizes, SHA-256, CRC, model identity, and code, RAM, pool, and flash budgets. QA, merge, hosting, and hardware do not block compilation. A scoped subtree build is not full-root QA. Docs, skills, and browser-only work have no firmware build requirement. |
| Independent QA | A reviewer who did not author the change ran the affected positive, negative, and regression checks on that exact candidate. The author of the change records `QA_PENDING`. Same-account approval does not satisfy a required human review. |
| Integration | The user assigned the merge. Fresh expected head and base, required reviews resolved, applicable combined-tree checks, resulting ref read back. An assigned firmware integration targets `nightly` after that ref is read. Release publication is a separate stage. |
| Qualified NIGHTLY | Independent checks for source, host, model, controller, package, installer, backup, readback, restore, OTA, and interruption; notices and corresponding source; the user assigned publication. This checkout's `tools/make_pages.py` accepts release identities only, so a dev-identity package stays `PACKAGE_NOT_PUBLISHED` until a tested publisher path accepts it. A package that path actually serves is labeled `DEV / HARDWARE TEST — DEVICE NOT TESTED`. |
| Served verification | After publication, fetched HTML, catalog, and model downloads match the qualified bytes. A Pages commit alone is not live delivery. |
| Production | Same-byte, affected-model, owner-operated physical proof, plus a separate promotion approval. Elite proof does not certify Pro. |

Report each gate with the glossary in [handoff](references/handoff.md). A compile result, a `DEV CHECKED` record, QA, a `gh-pages` update, a fetched download, and a device session use different names.

## Where to read next

- Cursor tools, subagents, Git, private inputs, and automations: [cursor-runtime.md](references/cursor-runtime.md)
- NIGHTLY versus production, ownership, and publication: [stages.md](references/stages.md)
- How to compile this tree: [builds.md](references/builds.md)
- The evidence packet and consumer acceptance: [handoff.md](references/handoff.md)
- What the 2026-10-24 export does and does not prove: [provenance.md](references/provenance.md)
