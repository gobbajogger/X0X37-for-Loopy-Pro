# Verified source delivery through connected Git

Use this procedure for an already-authorized source or sanitized evidence change when the current executor cannot use native Git transport. It does not grant permission to bypass a denial, replace the current source owner, move protected integration refs, publish firmware or operate a device.

Read the current [workflow](rules/workflow.md), exact candidate rules and action-specific authority first. Use a disjoint source branch and retain the existing sole integration owner. A source push is not a merge, build, published package or delivered device function.

## Classify the actual failure

Record the attempted action, repository, ref, exact input SHA, executor, time, exit/status and returned error. Keep the raw receipt; do not reduce all failures to “cannot push.”

| Observed failure | Meaning and permitted next action |
| --- | --- |
| Native Git DNS/connection failure, before a server response | A transport failure in that executor. If independently authorized, use the exposed connected Git API and perform its exact readback. A working connector read does not by itself prove connector writes. |
| Authentication failure, HTTP 401/403, token/app access problem | Verify the exact connection and required permission. Stop the affected write until corrected; do not use another identity to evade the denial. |
| Repository protection or rejected non-fast-forward | Read the current ref, relevant rules and ownership. Reconcile normally or hand off to the sole authorized integrator. Never force or weaken protection. |
| Local pre-push policy/hook rejection | A policy decision, not DNS. Inspect the installed hook and its applicable ownership. Do not disable it, change the remote or use an API to route around the rejected action. |
| Tool/automatic approval review rejection | Preserve the exact action and reason. Make any scope or evidence correction explicit and use only a permitted approval/review path. Do not encode, split, rename or reroute the same denied operation to escape review. |
| Expected-head mismatch, conflict or existing branch | Read the remote outcome before retrying. Preserve the other writer's changes; a stale base requires reconciliation. |
| Input/schema error | Inspect the current supported tool signature and correct the named field. Preserve exact source/tree/parent distinctions. |
| Timeout or uncertain outcome | Read the remote object/ref first. A lost response is not proof the mutation failed. Do not create duplicate commits, branches or PRs blindly. |

The repository's `tools/pre-push.hook` is a **local box-transfer hook**. Its network-URL rejection and restricted-history checks must not be treated as proof that GitHub is unavailable, nor silently removed for public delivery. Establish whether this hook is actually installed in the failing executor. A source-file inspection alone does not establish the cause of a past Dot failure.

## Freeze a concrete authorized payload

Before writing:

1. Read the current repository metadata and visibility, destination branch, parent commit and its root tree through the actual connection. A default-branch read is not a substitute for the intended development ref.
2. List the exact changed paths and complete replacement bytes. Inspect the change for its agreed source/evidence scope. Exclude credentials, private backups/captures, restricted SDK/toolchain/DSP inputs and firmware/controller binaries from a source-only delivery. Preserve executable modes and any unchanged file identities.
3. Record the expected parent and tree, local file SHA-256/Git blob identities, applicable test/review receipts, owning writer and intended receiver. Do not make a dummy write merely to test capability; deliver the reviewed useful change.
4. Keep review/build evidence pinned to the actual tested input closure. A remote commit with changed metadata must not be described as the original locally tested commit. If the full tree and ordered parents match, record that equivalence explicitly and retain both identities.

Do not change another active worker's branch. A new isolated branch does not confer integration ownership.

## Supported Git-data delivery

Inspect the currently exposed schemas; tool names and available fields can differ between executors. With the connected Git tools used by this workflow, the operations are:

1. `create_blob`: provide the complete exact text and `encoding: "utf-8"`. Compare every returned blob SHA with the locally calculated Git blob identity.
2. `create_tree`: set `base_tree_sha` to the observed **root tree SHA**, not the parent commit. Include only reviewed delta entries with exact `path`, `mode`, `type: "blob"` and returned `sha`. Do not create a root from only changed files.
3. `create_commit`: set `parent_sha` to the observed **commit SHA**, and `tree_sha` to the returned tree. Use additional ordered parents only when an explicitly authorized normal merge is the actual task. Source delivery alone normally has one parent.
4. For a new isolated branch, use `create_branch` with exactly one of `sha` or `base_ref`; choose the returned commit SHA. For an existing authorized branch, use `update_ref` with the last observed `expected_sha` and `force: false`.
5. On a stale expected head or uncertain result, read the live ref and reconcile before another mutation. Never substitute a forced update.

Blob/tree/commit creation without the ref pointing to the commit is **not completed source delivery**. Reaching an isolated branch is not integration into `nightly` or `main`.

## Verify and hand off

Read back the destination ref, commit, ordered parent list, root tree and changed blobs through the connection. Require:

- The destination ref equals the returned commit.
- Its parent and complete resulting tree match the prepared change.
- Every changed path has the intended mode and blob; unchanged paths are inherited from the verified base.
- Exact file contents match the supplied bytes; no restricted or unrelated paths entered the payload.

Record the remote commit URL, full SHA, tree, verified paths, evidence scope and next accountable consumer in the existing coordination/request flow. The receiving owner's first real scoped action can confirm acceptance/consumption; no ceremonial ACK or user reply is needed before authorized work.

A proposed patch, created object, successful push, PR opening, review, merge, native build, catalogue publication, owner availability and hardware acceptance are different states. Keep unfinished work visible. A proved narrow blocker may end a bounded run, but it does not close the stage or request and does not stop independent ready work.

## Read-only source recovery is separate

The maintained `nightly` document `docs/github-api-nightly-checkout.md`, read at a freshly resolved full commit SHA, describes how to reconstruct source for native execution and performs no push. A successful API source read or checkout proves neither a firmware build nor write authorization. Verify the required source closure before tests; do not repeat compiler/SDK downloads for a documentation-only delivery.
