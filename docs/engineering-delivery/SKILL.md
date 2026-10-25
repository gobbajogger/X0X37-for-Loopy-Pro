---
name: coordinate-engineering-delivery
description: Coordinate bounded LoopyX0X37 engineering stages across authorized agents and environments with exact evidence, explicit ownership and separate delivery gates.
---

# Engineering delivery

This is the canonical project workflow for every provider and environment. Root
`AGENTS.md` requires it; adapters reference it. Pin repository revision and file
hash in [handoffs](handoff.md) when available. A verified local checkout or supplied
pinned copy suffices offline; a redundant network fetch is not a gate.

Start with the musician's job: explain the problem, reason for the change, chosen
design and observable benefit. Reuse maintained implementations and verified prior
work before inventing a solution. Expected improvements are not measured results.
This contract aims to reduce avoidable stalls; it cannot guarantee continuity.

Current user instructions, actual permissions and denials govern. Read applicable
`BUILDING.md`, `BEHAVIOR.md`, `LICENSING.md` and other subject-specific rules.
This text grants no tools, credentials, contact rights or publication permission.
Missing referenced rules do not authorize inventing them. See
[adoption and validation](migration-and-validation.md) for scope and provenance.

## Own the next bounded stage

The root coordinates integration/progress; specialists implement and a different
reviewer supplies required independent QA. Respect assigned integrator/publisher
ownership until an authorized migration. Each stage has one owner, exact inputs,
allowed action, acceptance evidence,
next owner and stop. Pull existing progress; resolve known overlaps and verify
handoff before reassignment. Silence is not proof. Isolate mutable outputs or
serialize conflicts; run independent ready work in parallel.

Scheduler task IDs, role splits and frequencies may be optimized, consolidated,
rescheduled, paused or retired when authorized. Preserve essential capabilities,
independent QA, history and single-writer safety through an explicit ownership and
checkpoint migration; a new identity does not silently inherit old acceptance.

Start each new task/stage in a fresh execution thread; steering an active stage
is allowed. Completed threads supply evidence, not execution destinations. Carry
accessible artifacts/hashes/receipts and a supported progress reader; do not depend
on READY callbacks or rediscover unchanged inputs. Verify actual supported tools
and input access. Request STANDARD/MEDIUM where exposed; omit unsupported fields,
report requested versus observed settings, and do not retune active work.
No GUI computer use without explicit user request; authorized native/API and
headless tests are distinct from operating the user's GUI.

## Incremental execution

Use the existing ledger if available; otherwise keep a small private JSON
checkpoint. [Checkpoint procedure](checkpoint.md) defines exact identity,
per-output reuse, cheap observations and run receipts. Observe current relevant
heads, pending review/artifact/deploy/hardware work, retry conditions and policy
changes before expensive discovery. Failed or missing reads are never no-change.
Same SHA alone is insufficient. Exit unchanged accepted work without another
source commit, PR comment, full history scan or retest. Preserve immutable proof;
recheck only changed inputs and affected dependents, plus current action gates.

Each lower-effort run gets one unit: role, exact inputs, allowed action, done
criterion and next owner. Coordinator selects work; implementer produces it;
independent validator checks it; integrator applies protected source gates;
publisher handles only authorized hosting. Resolve known overlaps with one owner;
use an action lease only where the actual service supports one. No mandatory
account-wide writer inventory. A genuinely
complex unresolved problem may receive one bounded handoff to a more capable
available agent with evidence, question, budget/stop and return criterion.

Use PLANNED → ACTIVE → DELIVERED → ACCEPTED (or BLOCKED/CANCELLED). Admission and
input access establish ACTIVE; durable output establishes DELIVERED; applicable
acceptance, independent where required, establishes ACCEPTED. A changed scope
creates a new evidence revision. Optional runtime/history metadata is not a gate.
The coordinator uses a supported task reader or shared log; a pending launch is
not accepted work. Record commands, exits, source/tree, environment and exact
output hashes with PASS/FAIL/NOT_RUN/BLOCKED. Behavioral and hardware claims need
observed results; counts, mocks, compilation and badges do not substitute.

Agents run tests/builds in authorized environments. GitHub holds source, review
and nonsecret evidence. Never dispatch, retry, delegate or wait for test/build
execution through Actions, including self-hosted Actions. Preserve actual
repository protections; never forge statuses or call missing checks green.

## Reuse and qualification

Reuse outputs only with unchanged complete [input closures](checkpoint.md),
authentic stored bytes/hash and applicable accepted evidence. Record old-to-current
mapping and affected current-head checks. A root SHA alone proves neither reuse
nor invalidation. Keep app/controller closures separate: build affected apps,
reuse an unchanged verified controller, then recompose/check exact model packages
and pairing. Follow build adapters for CRC, budgets and isolation. BUILD_ONLY is
not installable qualification.

Before qualification, integration, hardware or publication, read the binding
[safety gates](safety-gates.md). Preserve exact-candidate independent QA, protected
normal merges (`nightly` after applicable automated validation; `main` also needs
all applicable successful hardware validation), data protections and licensing.
Publication, website integration and flashing retain their separate authority.

## Recovery and finish

Missing capability blocks only its dependents. Verify supported relevant routes,
then hand a bounded packet to one authorized capable fresh-stage owner. Real
denials are binding; do not reroute to evade them. Check uncertain writes/creation
before retrying. Continue independent work and repair demonstrated instruction
defects narrowly rather than restarting broad audits or same-SHA review loops.

At usage exhaustion preserve a nonsecret packet. With the user's conditional
fallback authorization, request Cursor via a new legitimate PR and verify URL/head;
no dummy diff, early contact or claim of acceptance without evidence. If unavailable,
report HANDOFF NOT OPENED with the exact dependency. Report useful result, evidence,
limits and next owner/action; finish at the requested stop condition or a truthful
blocked handoff. Do not claim monitoring continues without an active owner/reader.
