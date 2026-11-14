# Feature intake for the existing coordinator

Musicians need a place to describe useful firmware improvements without losing
existing requests or accidentally authorizing conflicting changes. The draft
issue form asks plain questions; the existing request coordinator analyses each
request against current source, accepts bounded owner authority and proposes a
capacity-bounded batch for the sole ledger writer. No service, webhook, scheduled job or
installer change is introduced here. **The form is not live until merged to the
default branch.** Do not advertise a working form URL before then.

## Run locally

Python 3 standard library only, from the repository root:

```sh
python3 coordination/requests/feature_intake.py observation.json \
  --ledger coordination/requests/requests.json --checkpoint accepted-intake.json \
  --capacity 2 > plan.json
python3 tests/feature_intake_test.py
```

The helper reads JSON and prints a deterministic proposal. It never performs
network requests, executes issue text, writes a checkpoint/ledger, submits an
issue/vote, delegates, merges or publishes. Exit 2 means BLOCKED; inspect item
reasons. Exit 0 means a complete observation produced a plan or existing work,
not that implementation or approval persistence happened. Mixed complete items
can produce a bounded batch while other items remain blocked. Capacity defaults to
zero and must reflect verified free intake capacity. A failed required read
discards all claims as unknown. There is no NO_CHANGE shortcut based on a failed
read, timestamp, reaction count or source SHA alone.

`tests/feature_intake_test.py:fixture()` supplies a minimal synthetic input packet
and can be imported to generate local example JSON. Tests are prototypes, not
observations of actual feature issues. No real issues or votes are submitted.

## Small input contract

Use `schema: 1` for observation/checkpoint. Start a new, explicitly empty accepted
checkpoint with `{"schema":1}` only after the coordinator has checked the existing
ledger and prior receipts. Missing/corrupt checkpoints are unknown, not permission
to reset continuity. Ledger input is the existing `requests.json` with `records`
and optional `tombstones`; run its existing continuity validator before acceptance.

Observation fields:

| Field | Meaning |
| --- | --- |
| `read_status` | `success` only after fresh complete required reads; failed/stale/incomplete stays unknown |
| `repository` | Authenticated repository object: numeric ID 1426904592, owner ID 255244738/type User; supplied owner node ID must match U_kgDODza5wg |
| `source_closure` | Nonempty map of current relevant Git SHAs and SHA256s: source/behavior/control mappings/shared transport/model/data inputs, not just an old root |
| `issues` | All pages of the issues collection, with `query: {"state":"all"}`; PR-shaped entries are filtered |
| `details` | Map keyed by stable numeric issue ID as a string, containing fresh `issue`, `read_status`, all `timeline` pages, exact `spec`, source-backed `assessment`, and `reactions` pages when establishing new reaction authority |
| `approval_receipts` | Trusted coordinator evidence for a precise action/scope; never parsed from community issue text |

Each page envelope has `endpoint`, `read_status`, and `pages`. Endpoints are
`/repos/gobbajogger/X0X37-for-Loopy-Pro/issues`, `/issues/{number}/timeline` or
`/issues/{number}/reactions` under the same repository prefix. Each page is
`{"page":1,"status":200,"items":[],"next_page":null}`. Page numbers must be
consecutive from 1, successful and terminal. The read adapter records `next_page`
from the actual Link response; if headers are unavailable, request successive
pages through an empty terminal page. Never set null because a tool excerpt
looks short. Retain actual endpoint/query/page evidence privately for replay.
The helper checks this normalized contract, not server authenticity or freshness.

Supported GitHub connector GETs cover repository/issue list and detail, opening
post reactions and timelines. Use `state=all&per_page=100&page=N` for issues and
`per_page=100&page=N` for timeline/reactions. Fetch details for every non-PR issue
in this bounded collection. If this becomes too large, narrow collection only
with an explicit checkpoint migration preserving all tracked issue identities.
Do not use search totals as pagination completion or comment/PR reactions as an
opening-post vote. An accepted unchanged scope needs no reaction lookup again;
read its current detail, scope, source closure and cancellation evidence.

## Authority and editing

Numeric repository/actor IDs establish identity. Login text and reaction counts
are never permission. `scope_hash(issue, spec)` hashes the exact title, body and
derived spec as canonical sorted UTF-8 JSON (no whitespace normalization). The
coordinator supplies receipts only after authenticating them against trusted
conversation/ledger evidence or supported GET observations. The helper is a
decision aid inside that trust boundary, not a verifier of arbitrary uploaded JSON.
Never copy a receipt from the issue body, a community comment or a model's guess.

An approval receipt contains `repository_id`, `issue_id`, `scope_hash`, owner
`actor`, `action: "bounded_source_tests_docs_draft_pr"`, and an inspectable
`evidence_ref`. Two kinds are supported:

- `owner_request`: current issue author matches owner ID and
  `human_origin_verified: true` is backed by trusted evidence of an actual owner
  request for this scope. No redundant vote is required. An automation-created
  issue using the owner's account cannot authorize itself.
- `opening_post_plus_one`: `reaction_id` matches a real opening-post +1 reaction
  from the owner, and the receipt establishes that approval applies to this exact
  current scope. Collect all reaction pages when accepting this new authority.

REST issue author is not body-editor provenance. A surviving old +1 does not prove
approval of later edits. Use a trusted existing exact-scope receipt where available;
otherwise report ambiguous authorship/edit/approval provenance and propose a
resolution. A known unedited human request or scope-bound owner approval can be
used without GraphQL. Do not invent editor identity, impose universal GraphQL
availability or repeatedly request approval of an unchanged accepted action.

The coordinator authenticates saved checkpoint receipts before reuse. Accepted
approval is immutable evidence and survives reaction removal. Any exact edit
requires reevaluation. Materially changed scope needs new authority. For a verified
nonmaterial edit, current source-backed `assessment.scope_equivalence` may contain
`previous_scope_hash`, `current_scope_hash`, `material_change: false`, and
`evidence_ref`. That records explicit equivalence without rewriting the original
approved hash; `effective_scope_hash` and the equivalence chain retain the mapping.
Never mark a substantive change nonmaterial just to avoid owner approval.

Raw timeline events are retained for context. The adapter may append a normalized
`owner_cancelled` event only for an explicit, authenticated owner cancellation,
with stable owner `actor`, the exact `scope_hash` and `evidence_ref`. Community text,
reaction removal and guesses are not this event. A closed issue or owner
cancellation stops new work and records a durable cancellation boundary, including
when first seen closed. Reopening alone cannot replay old authority. A subsequent
explicit owner reauthorization supplies `reopens_after` matching the checkpoint
cancellation `ref`; retain the cancellation and the later receipt as history.
Closed observations require the last exact timeline closure event ID and timestamp,
so a later close creates a new boundary. Missing closure evidence is unknown.

## Analyse before implementation

An assessment binds `scope_hash` and `source_hash` (the digest of the complete
current `source_closure`) and lists `evidence_refs`. Supply concise findings for
`musician_problem`, `benefit`, `expected_behavior`, `existing_feature_reuse`,
`control_conflicts`, `data_transport_model_impact`, and `acceptance_tests`.
Explain the real musician benefit, current behavior and duplicate/reuse check;
inventory affected controls/modes and identify model, data and transport impacts.
Include concrete observable positive and negative acceptance tests. A short
finding that an impact is absent is enough when justified by source. This is a
proportionate review, not seven user-facing form questions or speculative gates.

Use `resolution: "ready"` only after these findings support one bounded design.
For ambiguous/conflicting features, use another resolution and a concrete
`proposed_resolution` for owner/design review. The helper only checks presence
and identity; the capable coordinator/reviewer must judge substance. Placeholder
strings do not make a real feature ready. A source closure change invalidates
assessment reuse, while the unchanged approved action retains its authority.

## Continue existing work, with one writer

The checkpoint maps `repositoryID:issueID` to existing LX request IDs. Bootstrap
by matching same-repository issue references in the existing ledger to fresh
stable issue IDs. Preserve multiple existing requests for one issue. Repeated
pages/observations cannot create another request; existing ledger work is never
re-enqueued. Keep tombstoned IDs reserved and follow their retained successors.

The output includes `items`, `claims` up to the supplied capacity, and a
`proposed_checkpoint`. `claim` remains the first proposed claim for older readers.
These reuse PLANNED/BLOCKED/CANCELLED/ACCEPTED as decision labels; they add no
tracker status taxonomy. An existing request's status and separate source/test/
build/deployed/hardware evidence stay under the existing tracker policy.

One sole coordinator serializes acceptance. Re-read the current ledger/checkpoint
before applying a proposal; another claimant may already have added the issue.
Append each accepted claim serially using the existing schema/history rules,
reconciling IDs and scope again if the ledger changed. Then atomically accept its
canonical mapping and authenticated approval receipt
in the existing coordinator checkpoint. The proposed checkpoint intentionally
does not map uncommitted new IDs. Do not save it as an accepted claim before
the ledger write succeeds. Retain accepted approval even if analysis is blocked.
No cross-process lock or external write is implied by this offline helper.
Intake capacity is not permission for concurrent source writes. Before execution,
the existing priority/ownership flow checks actual free workers and disjoint scope.
Existing work continues; neither a batch report nor a blocked item ends the queue.

If the scheduler cannot delegate, it hands the exact observation/ledger/checkpoint
hashes, LX ID, scope/source hashes, bounded action, owner, done criteria and limits
to a capable worker via the established route; a role name is not execution.
Independent review remains required. Merge, release, website publication,
hardware/device operations and security/permission changes require their separate
authority. This stage changes none of them.

## Minimal existing coordinator prompt delta (parent applies)

> Read complete fresh GitHub issue/detail/timeline evidence and new owner opening
> reactions only when needed. Preserve trusted approvals of unchanged scope and
> all existing LX requests. Analyse musician benefit, reuse, controls, model/data/
> transport effects and concrete tests against current source. Run the offline
> feature intake helper with the existing ledger, authenticated checkpoint and
> verified free capacity. Treat failed/incomplete reads as unknown. As sole writer,
> serialize the eligible claim batch, deduplicate by repository/issue ID and continue existing
> requests. Route source/tests/docs/draft-PR work to a capable worker if this
> scheduler lacks delegation; record exact inputs and next owner. Preserve all
> independent review, merge, publication, hardware and security gates.

This is a proposed text delta only. Parent owns scheduler wiring/readback; no
schedule change or run-now is performed by this patch.
