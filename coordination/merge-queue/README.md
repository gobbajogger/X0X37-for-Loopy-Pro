# LoopyX0X37 merge queue

Shared queue on branch **`coordination/merge-queue`** in
`gobbajogger/X0X37-for-Loopy-Pro`. Read this branch explicitly; the queue does
not live on the protected source branches. This branch is based on main
`c9718ae833eb64a5191c350922593df8a36017c9` and changes only this directory.

**State is a snapshot, not approval.** Reviewed heads, scopes, dependencies,
evidence and next actions remain separate from live PR state and merge facts.
Metadata observed 2026-10-25T08:07:55Z. No code audit or test was repeated.

## Current handoff

- 138 unique repository/PR records: all 137 imported records retained, plus
  source PR161. All 137 imported heads still match their reviewed heads.
- Source PR160 is merged at `660a60aac2989146d16c28c4e86af33f6dc3d076`,
  tree `3cddc2572cc920272521b0fa76cba3be666adc54`. Source integration does
  not imply combined native build or hardware qualification.
- Source PR161 is open/draft at `516301da13f1c48f8ce6a17147be00b47937da7c`:
  reviewed fastmath repair with bounded passing host evidence; applicable
  combined-source checks, native Elite/Pro DEV plus matching-controller build,
  and hardware qualification remain pending. Integration task prefix
  `01a1720a` owns that handoff; public PR161 identifies the other active roles.
- Source PR28 remains open/draft; PR161 has not yet supplied accepted
  integration. Do not close it or merge historical popup ancestry.
- Source PR23/145 require an eligible non-author approval covering the latest
  push. Connected login `gobbajogger` is also their author; no self-approval or
  protection bypass is implied. Website public deployment separately awaits
  approval.

## Single-writer update protocol

1. The merge queue coordinator assigned by the parent integration coordinator
   is the **one queue writer**. Other agents send scoped evidence to that
   writer. Hand over ownership explicitly before another writer changes this
   branch; if ownership is unclear, prepare a scoped handoff instead.
2. Read the latest remote branch head and queue. Refresh the affected PR's
   exact head, state, draft flag, target branch head, dependencies and actual
   approval/protection gates before taking an action. Retain the reviewed
   head. A different head becomes `STALE_REVIEW_NEEDS_DELTA_REVIEW`; do not
   inherit readiness. Base/dependency changes require re-evaluating scope.
3. Update only for meaningful head, state, disposition, dependency, evidence
   or accountable-owner changes. Preserve `review_history`; record the prior
   state in `state_history` when changing a disposition or integration claim.
   Retain original reviewed bases; use `current_base` for live metadata.
4. Keep `merge-queue.json` authoritative and regenerate `merge-queue.csv`
   from its `entries`. Validate unique case-insensitive repository/PR keys,
   counts, exact heads, original history and full JSON/CSV parity. Store only
   project-relative paths and public GitHub evidence. Never add private paths,
   Library links/identifiers, credentials, raw captures or personal data.
5. Commit only changed queue files to this branch with the last observed head
   as parent. Push by fast-forward with an expected-head check, never force.
   If the head advanced, read it and reconcile with the writer first. Verify
   the remote commit, parent and queue blob hashes. No no-op commits, per-run
   reports or routine snapshot PRs.

This coordination role does not close PRs, delete branches, change schedulers,
run builds/hardware, publish websites, or merge protected source. Do not update
Cursor, main, nightly or gh-pages through this queue branch. Readiness remains
scoped; partial/equivalent integration is not a completed PR merge.

## Files and schema

- `merge-queue.json`: schema 1.2, counts, original review snapshot metadata
  and 138 entries. Each entry exposes reviewed/current heads, original
  scope and base, live target/state, dependencies, public evidence, next action,
  accountable role and active task when known. Unknown tasks are null.
- `merge-queue.csv`: same entries and columns, in the same order. Arrays,
  objects, booleans, numbers and null use JSON cell encoding; string fields
  remain text. `dependencies` retains the imported array-or-prose format.
- Original `review_history` is unchanged. `state_history` retains the imported
  PR28/160 states before this update. `imported_snapshot_metadata` retains
  original counts and audit scope. Input artifact identity and digests remain
  in private local provenance, outside this public directory.

`MERGED` requires live merge facts. `ALREADY_INTEGRATED` and `SUPERSEDED` are
scoped review dispositions, and do not assert the historical PR merged or
authorize its closure. `READY_FOR_INTEGRATION` still requires actual current
gates. Test evidence belongs to the recorded source and reviewer; this queue
writer executed no firmware tests and claims no hardware or release PASS.
