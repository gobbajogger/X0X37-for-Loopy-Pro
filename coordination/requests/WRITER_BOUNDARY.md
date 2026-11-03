# Proposed coordinator writer boundary

This is a local integration candidate for the existing sole admission/ledger
writer. It is not deployed enforcement. No production evidence authenticator,
durable store adapter or persistent coordinator executor is connected to
[`coordinator_writer_boundary.py`](coordinator_writer_boundary.py). Its
[`tests`](../../tests/coordinator_writer_boundary_test.py) use synthetic evidence
and a disposable SQLite database. They do not authenticate a real owner request
or change the canonical ledger.

The baseline is canonical coordination commit
`4a35a6cdaa0f112db6b3f577542138afd170c20d`. At that revision,
[`feature_intake.evaluate`](feature_intake.py) returns proposals and delegates
authentication and serialized writes to its caller.
[`task_priority.completion_errors`](task_priority.py) checks consistency. A forged
but internally consistent completion can therefore release a synthetic dependent
when a caller treats that check as authenticated acceptance. Existing helper
contracts describe this boundary correctly; another receipt field or digest alone
cannot close it.

The bounded source audit found the LX-REQ-019/027/020 maintenance scopes released
with `active_write_areas: []`, `execution_scope_released: true` and
`ownership_transfer: false`. Those releases do not transfer the standing writer
or integration role. Recorded active SMC and installer UI scopes are disjoint
from these new candidate files. Targeted public branch/PR reads found no
overlapping safeguard implementation. These reads do not establish liveness of
inaccessible Work threads. Reconcile any newly discovered overlapping writer
before implementing or adopting this candidate.

The adapter supports only an existing request's bounded `complete` or `claim`
transition. It does not append feature-intake requests, assign itself the sole
writer role, dispatch workers or replace the existing priority planner. It does
not modify automations, Actions, security settings, protections, Safari, hardware,
integration refs or publication. The separately reviewed R3 private checker is
outside this change; its implementation and existing budget remain untouched.

The actual writer supplies two trusted ports and its already assigned identity:

```python
boundary = WriterBoundary(store, verifier, writer_id=assigned_writer_id)
binding = boundary.preview(command, current_inputs)
# The independent evidence source authorizes this exact binding.
new_revision = boundary.apply(command, current_inputs)
```

`preview()` reads the trusted store and computes the proposed exact binding. It
does not authenticate evidence or write state. `apply()` reads again, checks the
command and state, obtains independent verification, then calls
`store.compare_and_swap(expected_revision, complete_after_state)`. The actual
store call must check the expected revision at the mutation boundary. A prior
read followed by an unconditional write does not satisfy this contract.

A schema-1 command has exactly these fields:

| Field | Meaning |
| --- | --- |
| `command_id` | Stable operation identity for replay prevention and uncertain-result recovery |
| `evidence_id` | Stable pointer into an independent trusted evidence source; not issue-supplied approval JSON |
| `kind` | `complete` or `claim` |
| `request_id` | Existing stable LX request identity |
| `expected_revision` | Exact nonrepeating revision returned by the trusted store |
| `inputs_hash` | `digest(current_inputs)`, including the writer's current source/input closure |
| `payload` | For completion: exactly `evidence` and `completion`; for claim: exactly `owner` |

`current_inputs` is supplied by the actual writer, independently of untrusted
command text. It includes the pinned source/input identities, a timezone-qualified
`as_of` observation and an integer `capacity`. In this candidate, `capacity` is the
total admitted-worker bound, compared with recorded occupied requests; it is not
an unchecked assertion of free slots. The writer and verifier must reconcile
actual worker liveness, accepted continuations and any external reservations.
The candidate does not discover those facts. Its binding covers the writer ID,
request ID and full previous-record hash, normalized scope hash, full input hash,
expected revision, entire command hash, and complete before/after bundle hashes.
Changing receipt bytes, scope, inputs, request, checkpoint or revision requires
verification of the resulting new binding.

`EvidenceVerifier.verify(evidence_id, binding)` must retrieve proof through a
supported trusted source outside the command and return `VerifiedEvidence` only
after authenticating the source, principal and exact action authority. It must
check required independent-review identity, current cancellation/revocation and
freshness, applicable completion evidence, capacity and scope. A claimed owner
login, `human_origin_verified: true`, reaction count, a model report or a locally
constructed `VerifiedEvidence` object is not an authentication mechanism.

The verifier must retain immutable, independently resolvable provenance for the
accepted `evidence_id`, including principal and source references. The candidate
persists that ID and the exact command hash; its returned principal/source fields
are not themselves a durable authentication service. No supported source with
this complete verification behavior was found in the inspected coordinator
implementation. Supplying a permissive callback would remove the intended gate.
Python type checks are integration checks, not isolation from arbitrary code
running with the writer's privileges.

The store returns a complete schema-1 bundle containing `ledger`, `checkpoint`,
`claims`, `accepted_completions`, `used_evidence` and `used_commands`. Reads must
authenticate the canonical source and be strongly consistent. Revisions must
never repeat. The expected-revision comparison and persistence of every bundle
component must be one atomic operation. A backend must reject a stale revision
without partial writes and must preserve unrelated records and their history.

There is no implicit bootstrap from a missing or corrupt checkpoint. The actual
writer must migrate from a pinned, authenticated complete ledger/checkpoint,
reconcile existing ownership and populate any trusted acceptance/replay state
from independently established receipts. An empty acceptance map does not certify
old completions. A dependency counts as completed only when its current terminal
record passes the existing consistency check and its entire current record hash
matches `accepted_completions`. Legacy completions need independent acceptance;
copying their own supplied hashes into that map would recreate the defect.
Acceptance evidence tied to a changed record must be reconciled, not silently
carried forward. This conservative migration must preserve prior valid work.

For new claims, the candidate requires ready source-backed assessment, allowed
owner state, authenticated prerequisite completion and available admission
capacity. It retains active/prepared reservations from the existing helper and
checks the durable claim map against ledger ownership. Scope strings must be
canonical relative area names; exact matches and slash-prefix containment overlap.
Unknown active scopes block new claims until reconciled. A conflict on one scope
does not reject a separately submitted disjoint ready command. The writer still
selects work through the existing priority contract and supplies current evidence;
this adapter is not a second prioritizer. Existing accepted continuations remain
with their current owner and are not new claims through this interface.

Completion records retain history, add the authenticated acceptance hash, release
that request's durable claim and add checkpoint/replay markers in the same
proposed bundle. Command IDs and evidence IDs cannot be reused after a committed
transition. Stale revision, replay, invalid proof or overlap rejects without a
mutation. The adapter performs no automatic retry.

After a timeout or lost response, use `lookup(original_command)` through a fresh
trusted read and reconcile the actual store/ref outcome. A matching command hash
shows the recorded operation was applied. A different hash under that ID is a
collision and must stop. No returned marker is not permission to resubmit while
the prior operation may still be pending; resolve the supported operation's
terminal outcome first. A successful commit followed by a lost response must not
consume evidence twice or create another claim.

One possible Git integration keeps all bundle components in the tree of one
authoritative coordination ref. The actual writer reads its exact commit/tree,
constructs the reviewed next tree and child commit, then requests a nonforced ref
update against that exact expected head. Immutable object creation alone does
not accept state. Only the successful authoritative ref transition exposes the
combined new ledger/checkpoint/claim/acceptance/replay state to readers.

The exposed connected Git `update_ref(expected_sha, force=false)` contract is a
candidate primitive, not evidence that this integration exists. Before adoption,
verify the actual server-side expected-head comparison and its race behavior,
the adapter's complete tree construction, the assigned writer identity and the
serialization of every write route. An application-side read/check followed by
an unconditional ref update is insufficient. Separate sequential updates to a
ledger file, checkpoint ref and receipt store are not one atomic acceptance.
Do not weaken repository protections, move canonical refs or change writers to
simulate a transaction.

Run the local contract tests from the repository root:

```sh
python3 tests/coordinator_writer_boundary_test.py
python3 tests/task_priority_test.py
python3 tests/feature_intake_test.py
```

The boundary suite covers a self-consistent forged receipt, authenticated
completion followed by a dependent claim, absent and preexisting forged
dependency acceptance, altered receipt/request/scope/inputs, replay, a stale
revision at actual fixture persistence, overlapping claims, unknown active scope,
disjoint progress, interrupted persistence and a committed operation with a lost
response. SQLite proves the disposable fixture transaction's behavior only. It
does not prove Git atomicity, production authentication, actual worker capacity
or live coordinator enforcement.

The next action belongs to the existing sole writer: identify its concrete
persistent invocation path and independently trusted evidence reader; implement
and review the verifier and complete-bundle store adapter; migrate the current
accepted state without transferring roles; run the same adversarial cases against
that actual mutation path; then make every relevant writer route invoke the
boundary before acceptance or dependency release. Feature-intake append and
checkpoint mapping need their own reviewed transition support. Until those steps
are verified, direct existing helper callers retain their present behavior and
must not be described as protected by this local candidate.
