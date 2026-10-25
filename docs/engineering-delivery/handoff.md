# Small stage packet

Use the [canonical contract](SKILL.md). Fields are semantic, not a mandatory API
payload or metadata-fetch checklist. Omit inapplicable fields; optional unknown
facts and outputs not yet produced are not admission gates. Never publish private
paths, user notes, credentials, backups or approval-card content.

```yaml
workflow: {repository: gobbajogger/X0X37-for-Loopy-Pro, revision: <available commit>, sha256: <available file hash>}
task: <stable identity and evidence revision>
stage: <one bounded action>
owner: <one accountable identity or role>
purpose: <problem, design choice, expected benefit>
scope: {allowed: [], excluded: []}
inputs: [] # exact accessible source/target/artifact identities and hashes where relevant
authority: <current instruction and action limits; no private approval content>
execution: {route: <actual supported route>, progress_reader: <task reader or shared log>}
state: PLANNED # ACTIVE, DELIVERED, ACCEPTED, BLOCKED, CANCELLED
acceptance: [] # risk, check and observable result
checkpoint: <private accepted record; role/workflow/closures/policies/heads/pending/retry>
receipts: [] # start/end, input/checkpoint before/after, action/outcome/retries, verified outputs
metrics: {stage_elapsed_seconds: null, time_to_owner_installable_seconds: null, tokens: null} # actual telemetry only
next: {owner: <role>, action: <next operation>, condition: <dependency>}
stop: <completion or precise boundary>
```

For builds append per-output closures, model/package/controller hashes, CRC and
budgets. Hardware evidence attaches to exact model and bytes. Use accessible private
artifact retrieval locations in the authorized handoff, not public Git. For a fresh
external task, record real admission identity/input readback when available; pending
creation is not accepted delivery. A local shared log may suffice for local work.

Labels describe observations: BUILD_ONLY is compilation evidence; DEV CHECKED is
the actual development-validator result; QA_PENDING is not independent QA;
DEVICE NOT TESTED is not a hardware pass. Never promote one label into a later gate.
