# Automatic task priority

The existing sole request coordinator uses `task_priority.py` after intake and
before choosing its next bounded source/test/docs stage. It reads the current
ledger and prints recommendations only. It creates no scheduler, task, claim,
owner assignment, checkpoint or external write. PR184's pinned intake trial
`efa57425600a23d88efde6152c977542883da4a7` keeps its authority/deduplication logic;
this helper orders existing requests after intake, without changing that trial.

```sh
python3 coordination/requests/task_priority.py --as-of 2026-10-25T12:00:00Z --read-status success
# Only after checking current execution, free slots and conflicting source areas:
python3 coordination/requests/task_priority.py --as-of 2026-10-25T12:00:00Z \
  --read-status success --capacity 2 --busy-area firmware-runtime
python3 tests/task_priority_test.py
python3 coordination/requests/validate_requests.py --base-ref <accepted-tracker-sha>
```

Set `--read-status success` only after fresh complete required queue reads; the
default `unknown` returns BLOCKED with exit 2 and no candidates, even for an empty
file-level queue. A local parse alone does not authenticate remote freshness.
The explicit observation time makes output reproducible. Capacity defaults to zero
new lanes. Capacity means verified free worker slots in addition to existing work;
`--busy-area` supplements retained reported-active areas with fresh runtime claims.
A successful plan does not establish that supplied observations are current.
Malformed inactive assessments exclude only that item with a concrete correction
action; missing assessments/readiness remain unknown. Duplicate request IDs remain
a global structural error (BLOCKED, exit 2), because records cannot be addressed
unambiguously. Unknown active-writer areas instead set `claim_admission.state` to
BLOCKED and list the affected IDs in `blocked_by`. They suppress every new claim
and leave both lanes empty, even when verified free slots exist; unknown overlap
cannot establish safe new ownership.
Without that uncertainty, admission is ELIGIBLE only for scoped recommendations
when free capacity exists, or NO_FREE_CAPACITY otherwise; neither state grants authority.

That admission hold does not suppress `continue_existing` or `recovery_actions`.
Known-scope accepted commitments retain their owner and next action. For an active
record whose occupied areas are unknown, preserve its original action in
`recorded_next_action` and first recommend read-only reconciliation of its current
owner checkpoint, liveness and occupied scope. `recovery_actions` explicitly labels
this READ_ONLY_OWNERSHIP_RECONCILIATION; it grants no write, reassignment or expansion
of scope. The existing owner then resumes only the verified authorized action.
These records remain actionable at capacity zero, since capacity counts only new
claims. A valid scoped decision exits 0 with WORK_REQUIRED, rather than hiding
unrelated accepted work behind a whole-queue BLOCKED result. Authentic completion,
owner deferral and action-specific denials retain their own meaning; a read-only
ownership recovery does not reopen a completed or deferred request.

Delivery categories, in order:

1. `safety_integrity`: safety, data loss, crash and release-integrity defects.
2. `ux_repair`: broken musical controls and recurring user-facing workflow friction.
3. `small_improvement`: useful bounded improvements, including cosmetic clarity.
4. `feature_research`: larger features, new behavior and exploratory work.
5. `unassessed`: needs evidence before automatic selection.

This is a transparent order, with no summed score. Within each category, severity
orders critical, major, moderate, minor, unknown; then explicit commitments,
oldest retained observation, confidence and stable LX ID break ties. Reach stays
a separate descriptive field: there are no invented user counts or numerical
reach weights. Evidence confidence is source_verified, reported or unknown.
`source_verified` requires direct inspected source evidence for the assessment;
reported QA/build claims are still reported. Classification is not fresh acceptance.

An optional `priority_assessment` object adds category, severity, reach, confidence,
readiness (ready/blocked/unknown), effort (small/complex/unknown), work_areas,
rationale and evidence_refs. Optional `committed_at` is a timezone-aware timestamp
of a real commitment with supporting evidence, never inferred from a role name.
Legacy `priority` labels remain compatible. The validator checks the new object
when present; old records remain valid. Current classifications use existing scope,
source summaries and acceptance criteria. Effort remains unknown except the clearly
cross-cutting multi-controller, engine and preset scopes marked complex; no duration
or remaining-work estimates are invented. Obtain a source-backed estimate for the
next bounded stage before treating anything as quick.

Only explicit `dependencies` block: a dependency must have accepted completion evidence, not merely
cancelled, superseded, source-ready or related. Keep readiness conservative and
fresh for the proposed stage. A denied write remains blocked until properly
resolved. Related requests do not become invented dependencies. A blocked long task
cannot hold unrelated ready work. Source/test readiness never authorizes later
merge, publication, device or security actions.

Keep reported ACTIVE/ACTIVE_STAGE_REPORTED commitments with their existing owner
and next action; verify actual liveness before continuing, never auto-reassign.
Recommendations carry `implementation_owner` separately from rationale and
`suggested_next_action`. Unassigned remains an explicit gap for the parent to resolve.
Area tags are conservative overlap hints, not proof of independence; coordinator
must check current source/write scope and pass additional busy areas before dispatch.

Ready safety/integrity work gets first use of capacity and overlapping source areas.
When remaining capacity and independently verified scope permit, reserve at least
one bounded complex lane before filling an ordinary quick lane. A real commitment, then age of at least
14 days, protects complex work from an endless stream of small requests. Safety
complex work keeps first lane preference. Difficulty does not reduce importance;
this lane can progress alongside earlier-delivery UX repairs. One available slot
can therefore continue aged complex work while ordinary quick work waits for capacity.
Unknown effort never silently becomes small. The delivery list still preserves
safety/UX order; lane selection is parallel stage progress, not release order.
All quick and complex stages retain the same applicable QA and safety gates.

`claim_candidates` fills the remaining verified free slots with disjoint ready
work after the safety/complex reservation above. The `lanes.quick` and
`lanes.complex` fields retain the first recommendation of each kind for existing
readers; they are not a two-worker ceiling. The sole ledger writer serializes actual
claims after fresh ownership checks. The helper neither dispatches nor writes.

## Completion evidence and pending work

COMPLETED or DONE requires a `completion` receipt for the exact current
`completion_criterion`. Define `required_evidence` as the applicable subset of
source/test/build/deployed/hardware when scoping the outcome, not to hide a failed
gate afterward. This is a completion check, not an admission requirement for an
unfinished request. Existing nonterminal records remain valid.

The receipt has `state: ACCEPTED`, the exact `criterion`, `evidence_hashes` for
exactly those required levels, and `delivery: {state: VERIFIED, evidence_refs: [...]}`
for actual delivery of the requested result. Hash each complete evidence-level
object with `task_priority.evidence_hash`; refresh acceptance if that object changes.
Each required level needs an accepted result state: PASS, VERIFIED, ACCEPTED,
DELIVERED, BUILD_ONLY_PASS, DEV_CHECKED, DEVICE_VERIFIED, PUBLISHED_VERIFIED,
SOURCE_VERIFIED or SOURCE_DELIVERED. No unrequested hardware or publication gate
is added to a source-only outcome. An assigned reviewer authenticates the exact
criterion, proof and destination; the helper checks consistency, not arbitrary
JSON's truth or permission to merge/publish.

Invalid terminal evidence leaves that item and its dependents pending. It does not
exclude unrelated ready work. The continuity validator reports the mismatch; resolve
structural ID/ownership corruption before claims, but do not turn a selected item's
completion failure into a blanket action gate. An explicit BLOCKED status cannot be
selected from a stale ready assessment. The planner reports WORK_REQUIRED while ready,
existing active work or a read-only ownership recovery remains, PENDING when only gated/deferred work remains, and
QUEUE_COMPLETE only after a fresh complete read leaves no unfinished request.
An exit code 0 reports a valid decision, not delivered functionality.

Continue the next authorized action after each meaningful report. A bounded unit
controls ownership and evidence, not a stop-after-report policy. Preserve real
denials and retry triggers; do not create infinite unchanged audits or use scheduler
run completion as request completion.

New owner feedback revises the assessment with a full prior snapshot in history;
never delete earlier requests, reset their age or replace running claims. Age uses
the earliest retained created/updated/history observation. Explicit deferral or
false intake/implementation authorization excludes a record before any ordering.
LX-REQ-038 stays deferred until explicit owner reopening and scoped authority;
remembering it, votes, age, effort or a high category cannot authorize it.

## Current meaningful ordering and limits

Retained evidence places sample/project safety (001–004, 008, 011), release integrity
(013–014, 016–018, 024) ahead of cosmetics. Blocked dependencies/denials keep 004,
011, 013 and 024 out of ready selection. Broken musical controls and recurring UX
(005–006, 009, 012, 035) precede new engine/preset/duration/bank features (007,
030, 032–034). Installer Projects access (037) reuses existing functionality; its
transfer safety acceptance still applies. These are scoped handoff classifications,
not refreshed source, execution, build, publication or hardware results.

The current ledger has no justified small effort estimate, so it produces no quick
lane recommendation. This avoids presenting an invented effort as known. Complex
scope is recorded for 007, 032 and 033; actual source conflicts/free slots must be
checked before reserving a lane. Other unassessed records retain all prior work and
need a concise evidence-backed assessment when they next become relevant.

## Compact parent follow-on

> After pinned feature intake, run the pinned task_priority helper against the
> fresh validated ledger with explicit observation time and verified free capacity/
> busy source areas. Use safety/integrity, UX repair, small improvement, feature/
> research order; retain separate severity/reach/confidence/readiness/effort and
> rationale/next action/implementation owner. Preserve claims and all history;
> continue committed work, and reserve a bounded independent complex lane when
> capacity permits, using commitment/age against starvation. Blocked long work must
> not hold unrelated ready repairs. Owner-deferred LX-REQ-038 stays excluded until
> explicitly reopened. No quick-lane QA/safety waiver or inferred merge/publication/
> hardware authority. Existing assigned owners continue their accepted scope;
> the sole tracker writer serializes verified claims without a routine parent-response gate.

This is proposed scheduler/plugin wording only. Parent owns the single scheduler
update and its readback; this patch performs neither scheduler nor plugin mutation.
