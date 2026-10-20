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

### Rule 18: verify the successor before finishing

At completion, the current stage owner creates and starts exactly one new
thread for the next required stage. This is authorized continuation of the
owner's requested workflow; carry its source into the prompt. Do not ask the
owner to launch each stage or create all seven threads in advance. The fresh
thread assigned to a stage performs it without creating another copy of itself.
Only the recorded stage owner launches its successor.

1. Save the complete launch prompt and a successor record in the lane's evidence
   folder before creation. Give the record a unique workflow/stage marker and
   the source thread ID. Include the current rules entry, repository/lane and
   exact revision, approved scope and missing decisions, files/commits,
   validation inputs/logs, artifact identities, known failures, completion
   criteria, next stage, and one receiving owner. Preserve attempts and errors.
2. Check project activity and that record for an existing successor, including
   idle or setting-up threads. Resolve the project with `list_projects`; use
   `create_thread` with that project and local execution unless the owner chose
   another environment. The prompt must say to begin the assigned work now,
   read the saved handoff, and carry rule 18 forward. Preserve model defaults.
   Record the returned host/thread IDs immediately. A pending client ID is not
   a thread ID; resolve setup through supported status tools before using it.
3. Verify the returned thread with `read_thread` or `wait_threads`. Creation,
   a title, queued input, or an active flag alone does not prove work started.
   Require a new assistant response or tool activity for this handoff that
   acknowledges the assigned scope or reads its inputs. A fast completed turn
   can also qualify; a failed/interrupted turn or a request to launch itself
   cannot. Save the observed turn ID and evidence, then mark `started`, report
   the receiving thread, and end this stage's execution. Do not await completion
   of the successor stage.
4. If creation or startup fails, keep the handoff `pending` or `blocked`, never
   `started`. Inspect the saved IDs, project activity, and matching thread
   contents before any retry; use bounded status waits, not a timer automation.
   For a transient service/setup error, allow one retry after confirming no
   successor exists and refreshing the relevant state. After a timeout or lost
   response, do not create again while the outcome is uncertain. If a successor
   exists but its launch failed before work began, recover that same thread
   with the saved prompt only after verifying no turn/input is active or queued.
   This request authorizes that handoff message; do not resend to a running or
   completed stage. Preserve the original error and recovery result. Approval
   denials, policy refusals, missing access, or unresolved setup need their real
   dependency resolved; do not bypass them or change permission settings. When
   recovery cannot proceed, report the exact blocker, saved prompt/record, any
   successor ID, and next action. Never claim successful handoff from an attempt.

A failed or blocked stage is not complete. Resolve authorized work in its
scope, or start the fresh repair-stage thread required above with a concrete
handoff; do not advance to merge, install, or push as if checks passed.
Missing approval pauses only dependent work. A missing device leaves install
pending and does not silently skip to push; a different order needs an owner
instruction. Reuse valid evidence and do not restart completed work.
Follow [chat ownership and completion notices](core.md) for other-thread context.
These checks prevent silent success claims; service availability is not a
100% guarantee, and unresolved outcomes must stay visible.
