# Code workflow

### Rule 17: one fresh thread per stage

For an authorized code workflow, use this order. Each stage runs in a new
thread; subagents support a stage and do not replace its thread. A rules-only
request does not start a code audit or release cycle.

| Stage | Goal and starting state | Completion evidence |
| --- | --- | --- |
| 1. Audit | Inspect the identified code revision and current intended behavior. | Defects with evidence, affected scope, and proposed fixes; identify which fixes already have approval and which need a decision. |
| 2. Implement approved fixes | Use the audit and recorded approvals. | Checked lane commits for approved fixes and a record of deferred items. Request only missing decisions; preserve existing authorization. |
| 3. Test and investigate | Test the complete candidate from stage 2. Use subagents for tests and failure investigation, with separate scopes. | Relevant tests pass; failures have causes and dispositions. New code fixes return to a fresh stage 2 thread, then a fresh stage 3 thread for affected checks. |
| 4. Validate build and packages | Use the tested candidate, target configurations, and dependencies. | Exact candidate build/install-package identities, checks, and logs under [validation](integration.md) and [install/release](install.md) rules. |
| 5. Merge | Use validated commits after all contributing lanes have handed off. | The sole assigned integrator merges the complete set and records the resulting revision and its validation identity. |
| 6. Install | Use the merged revision, matching validated artifacts, and recorded install scope. | The designated installer records installation and data-preservation checks under [install rules](install.md). |
| 7. Push to Git | Use the completed merge and install evidence, approved repository/ref, and recorded push scope. | Verify the remote ref matches the intended commit. End this workflow; do not start another audit automatically. |

Use [scoped checks and reuse](commit.md). Stage 4 validates development builds
by default. Apply [rule 11](integration.md) before any release build or full gate;
stage progression does not supply release-build authorization.
Stage 4 validates candidate/install packages. Public-release packaging remains
conditional on the owner's device PASS for the same build after installation.
If requested, complete it in a fresh thread when that evidence exists; stage 7
Git push does not itself authorize public-release packaging or publication.
Before installation, compare the merged result with stage 4's checked inputs.
A changed commit ID alone need not invalidate matching evidence. A change to
checked source, configuration, tools, dependencies, or artifacts requires a
fresh thread for the earliest affected stage. Resume with a fresh install
thread only after required checks pass; do not repeat an already completed
merge when no further merge is needed.

The stage 5 thread receives integration ownership through the handoff; it must
not compete with an existing integrator. Recheck project activity and the
target branch before shared changes. Any additional merge returns through
the applicable validation and merge stages. Preserve the source and artifact
guards in [core](core.md); neither stage order nor thread creation expands
approval to an unspecified install, remote, publication, or new defect fix.
Carry forward valid approvals without asking again.

### Rule 18: create the successor before finishing

At completion, the current stage owner creates and starts exactly one new
thread for the next required stage, then reports that thread and ends its
own execution. This is authorized continuation of the owner's requested
workflow. Do not ask the owner to launch each stage or create all seven
threads in advance. The fresh thread already assigned to a stage performs
that stage without creating another copy of itself.

Before creation, check project activity and the handoff record for an
existing successor. Record the returned thread ID; if creation is uncertain,
inspect thread state before retrying. Only the stage owner creates its
successor. Give the new thread the workflow/stage, current rules entry,
repository/lane and exact revision, approved scope and missing decisions,
files/commits, validation inputs and evidence paths, artifact identities,
known failures, completion criteria, and the next stage. Record one receiving
owner and pass on this successor requirement.

A failed or blocked stage is not complete. Resolve authorized work in its
scope, or start the fresh repair-stage thread required above with a concrete
handoff; do not advance to merge, install, or push as if checks passed.
Missing approval pauses only dependent work. A missing device leaves install
pending and does not silently skip to push; a different order needs an owner
instruction. Reuse valid evidence and do not restart completed work.
Follow [chat ownership and completion notices](core.md) for context from
other threads; no timer automation is needed for these handoffs.
