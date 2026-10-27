# Continue authorized work and hand back results

Consume the existing LX request, current owner and checkpoint. Preserve its outcome,
accepted scope, exact source/artifact identities, valid proof and remaining actions.
Report meaningful results with evidence and the next action, then continue authorized
work. Reports, previews and internal stage completion do not require owner acknowledgment.
Current owner instructions and real action-specific gates still govern.

## Read, select and continue

Read the current queue and relevant run/receiver state through supported readers.
An unread, failed, stale or incomplete required observation is unknown; it is never
an empty queue, NO_CHANGE or completion. Check only inputs and gates relevant to the
selected action. A blocked item remains pending and does not block an independent
ready item. Do not retry a known denial on timer ticks or through another route.

Keep one sole ledger writer and the existing source, integration and publication
owners. The coordinator may serialize a batch of new claims within verified free
capacity after checking current ownership and disjoint scopes. A bounded unit controls
the claim and evidence; it does not mean stop after its report. Reviewers remain
independent. QA and builds can run on the same immutable candidate while disjoint
implementation proceeds. Do not serialize the whole queue behind one candidate.

Use the existing intake, priority and continuity helpers on the verified
`coordination/merge-queue` revision. They propose decisions; they do not authenticate
remote evidence, create workers, acquire locks or write the ledger. Preserve request
IDs and history. A scheduler's completed run or an intake ACCEPTED label is not a
completed user outcome. The retired `tools/stage_handoff.py` numbered-thread pipeline
is historical tooling, not the current continuation mechanism.

## Evidence and stopping conditions

Mark a request COMPLETED/DONE only when its requested completion criterion and
actual delivery are evidenced. Record its applicable evidence levels at scope
definition; do not select fewer levels to obtain a pass. Source review, host tests,
native build, merge, publication, owner availability and hardware proof remain separate.
A source-only request does not acquire hardware or publication prerequisites.
Authenticate the applicable proof and delivery readback before accepting the receipt.
The ledger checker rejects missing, negative, stale or contradictory completion
receipts; it checks consistency, not the truth of arbitrary supplied JSON.

Reuse unchanged accepted proof when its full input closure and stored bytes still
match. Refresh only changed observations and affected checks. Future artifacts,
optional timing/model-tier metadata and root's direct access to a child are not
admission gates when the required inputs and a supported result reader exist.

At a stage boundary, perform the next authorized action in the current capable
execution. If that bounded execution must end with work remaining, persist its
checkpoint and verify one supported successor or existing authorized relay before
yielding. Reconcile uncertain launches before retrying. If no route exists, record
the exact missing action/capability and responsible owner; keep other ready work moving.
Do not claim future monitoring without an active owner and supported reader.

End the requested workflow when acceptance is evidenced or the owner cancels it.
When only genuinely gated items remain, record pending dependencies and their actual
retry triggers. Do not create repeated unchanged audits, duplicate jobs or no-op commits.
The queue's next changed observation may reactivate its existing authorized owner.

## Transfer a scope only when needed

1. Save the bounded assignment: request/outcome, current authority, exact inputs,
   owned scope, completed/failed/unrun checks, acceptance condition, next action and
   supported progress/result reader. Reuse the originating project context.
2. Check for an existing receiver, including pending or uncertain creation. Use
   only the supported authorized creation route required by this assignment; no
   particular vendor or new thread is required for each logical stage. Before a
   receiver writes shared scope, checkpoint final bytes and stop/release the old writer.
3. Verify receiver consumption through its actual first bounded action against
   the pinned inputs. Record the real receiver/run identity, action and result
   reference. A title, queued prompt, active flag or acknowledgment alone is not
   consumption. Creation is not delivery. Keep one accountable owner/relay to read
   the actual result and verify acceptance; that role does not become a competing writer.
4. If the receiver lacks successor creation or its parent cannot read it directly,
   use an already qualified authorized relay. Do not duplicate a pending receiver,
   invent an ID, bypass a denial or change approval settings. Missing capability
   blocks only its dependent action; it does not reset accepted work.

Follow [chat routing](../../x0x37-task/references/routing.md) for a genuinely new
task or specifically requested destination. If classic project creation is required
and no supported authorized route exists, preserve the prompt and exact launch
dependency honestly; do not substitute a different destination without authority.
Do not move already-running authorized work merely to change placement.

Failed checks need repair and affected proof before their dependent action advances.
A missing device leaves installation pending while independently authorized source
work continues. Carry existing permission under
[rule 14](../../x0x37-owner-decisions/SKILL.md); use
[rule 2](../../x0x37-task/SKILL.md) for authorized context and completion notices.
