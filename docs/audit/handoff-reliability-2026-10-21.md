# Thread handoff reliability repair — 2026-10-21

Trigger: The owner requested reliable handoffs and startup of successor threads
in task `01a15a0d-6c6a-7ae3-a094-94bb0cbfe40d`.

Cause: Rule 18 required creation/start but did not define observable startup
proof, a saved launch intent before creation, or bounded recovery. The observed
SMC stage-2 creation failed with `MCP tool call requires approval, but approval
policy is never`. The stage owner saved a blocker. After the owner explicitly
requested recheck, the same owner refreshed activity/project state and created
one successor successfully. The original app-policy cause is not established;
this repository cannot repair that application's approval implementation.

Observed runtime evidence: source thread `01a159fc-b487-7ed0-ae49-12e13cc06aef`,
receiving thread `01a15a0c-471c-7bd3-b989-4e306ca6a082`, and
`smc_regression_repair_20261021/evidence/smc-stage2/successor.json` relative to
the workspace. A read-only thread inspection found the receiving assistant's
response and actual rules/handoff reads in turn
`01a15a0c-4833-7913-8cd2-48b952dd3a5b`. These prove that particular handoff
started. They do not measure or establish a 100% success rate.

Effect: Rule 18 now requires saved launch intent and successor identity,
project/activity reconciliation, real assistant/tool startup evidence tied to
the handoff, and recorded recovery results before reporting success. Unknown
creation outcomes prohibit blind recreation. Transient failures permit one
bounded retry only after duplicate checks; failed launches can recover the
same fresh receiving thread before execution begins. Approval/policy refusals
need their actual dependency resolved. No permission settings are changed.
Unresolved launches remain pending/blocked with a concrete continuation record.
Existing stage order, approvals, integration ownership, development defaults,
install guards, and stage-7 termination remain in force.

Implementation: isolated `lane/dk-rules-handoff-reliability`, base `c33463c`.
Only rule 18, its current owner decision and this audit/evidence are committed.
The base already has the reviewed development-default rule. Its controller-input
decision is preserved in this lane; rollout adds only the new handoff decision
to maintained preferences and does not copy the lane's entire file over them.
The Create Rule skill's generic `.cursor/rules` layout yields to the project's
explicit maintained `docs/rules` authority; no competing rule store is added.

Checks: [evidence](handoff-reliability-2026-10-21/checks.txt) contains the focused
preference, link, history, whitespace and scratch-rollout results.
[Independent review](handoff-reliability-2026-10-21/independent-review.txt) covers
successful/quick startup, delayed setup, uncertain creation, existing successors,
transient failures, approval refusal, same-thread recovery, install/push scope
and end-of-chain behavior. No material findings. No enforcement implementation
changed, so these are content/scenario checks, not fault injection of the app.
No firmware build, formal gate, installation, external push, app configuration
change, or duplicate production successor was performed here.

Reading size: workflow grows from 769 to 1097 words (+328). Router and startup
core are unchanged; the added procedure loads only during stage handoffs.
The full counts are in [reading-size.json](handoff-reliability-2026-10-21/reading-size.json).

Reference: [official OpenAI workflow guidance](https://developers.openai.com/cookbook/examples/codex/iterating-development-workflows-with-codex)
supports phase context and evidence-backed handoffs; the local tools' supplied
schemas define the actual creation, status and message interfaces used here.
That guidance supplies no availability guarantee.

Rollout completed after committed-candidate `aa5d616` passed the docs-only
quickcheck. Its 41 grammar warnings are advisory. Preference pointers passed
19 entries in the lane and 18 in maintained sources; the lane base contains
one additional existing controller-input decision. Only this task's two-file
delta was applied to maintained sources, preserving all unrelated bytes.
The exact patch, before/after hashes and backups are recorded in
[maintained-rollout.json](handoff-reliability-2026-10-21/maintained-rollout.json).
Discovery rollout updated 22 mirror/managed-entry files with backups in
`repo-bak/rules-rollout-20261020T160714802502Z`. Twenty pointer files already
needed current-authority rollout before this repair, including the new lane.
[Final discovery check](handoff-reliability-2026-10-21/discovery-check.json)
reports zero drift. Workspace workflow and preference mirrors match maintained
sources byte for byte; startup core and router remain unchanged.

Receiving owner: the next authorized workflow stage owner reads current
`rules/workflow.md` rule 18. Firmware integration remains with its assigned
thread. This rules-only task has no next code stage to launch.
No execution work or notices were sent to other production chats. The existing
SMC workflow remains with its receiving owner.


## Automatic authorization follow-up — 2026-10-21

Trigger: The owner clarified, "Make sure that they automatically allow to do
that without my intervention" in this same ongoing task. This explicitly
supersedes per-handoff confirmation and authorizes narrow handoff-tool settings.

Cause: The local Codex configuration auto-approved `send_message_to_thread`
but had no explicit approval entry for `create_thread`. General
`approval_policy = "never"` suppresses prompts; it does not itself approve
an MCP tool that would otherwise need permission.

Change: Added plugin-scoped `create_thread` `approval_mode = "approve"` in
`/Users/mac/.codex/config.toml`, retaining the existing handoff-message setting.
Backed up the original beside the config. Parsed TOML comparison proves only
that entry was added. No global approval, sandbox, reviewer, model, or unrelated
tool setting changed. The exact backup and runtime verification thread identity
are in [automatic-startup.json](handoff-reliability-2026-10-21/automatic-startup.json).
The [official plugin configuration documentation](https://developers.openai.com/plugins/build/plugins#configure-installed-server-policy)
and [schema](https://developers.openai.com/codex/config-schema.json) define this
per-tool policy. Local settings do not override higher-priority restrictions.

Rule 18 now explicitly carries standing owner authorization for required
successor creation/startup, launch/recovery messages, verification and completion
notices. The recorded sender may recover a handoff that never started without
asking again. Prompts carry the source authorization forward. Rule 13 links to
this home. Existing duplicate checks, uncertain-outcome checks, source/feature
scope, install/release/push requirements and actual policy denials remain.

Runtime check: Created one bounded independent read-only review thread using
the native `create_thread` tool without owner intervention. Startup was verified in receiving turn
`01a15a15-e4b3-78c1-9c1e-18bac181ad3c`: assistant acknowledgment, rules/handoff
reads and the assigned diff inspection. The independent review found no
actionable issues and delivered its completion message automatically via
`send_message_to_thread`. Both creation/startup and messaging worked without
owner intervention. Saved configuration does not by itself prove policy reload
causation or universal availability. The current preference-pointer check passed
20 lane entries, changed rule links resolve and historical preferences are
unchanged. Workflow reading grows from 1097 to 1178 words and the message-rule
pointer grows from 166 to 173. Startup core/router remain unchanged.
See [independent review](handoff-reliability-2026-10-21/automatic-independent-review.txt).
This supporting rules review is not a firmware stage and has no successor.
