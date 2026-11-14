# Suggestion intake observation and coordinator handoff

This adapter collects read-only GitHub evidence for ROADMAP #120. It does not
operate an inbox, establish human provenance, accept a claim or save coordinator
state. Musicians' suggestions remain subject to the existing owner's exact-scope
authority and the coordinator's source-backed assessment.

The unchanged helper, contract and 26 tests are recovered from merged coordination
commit `4a35a6cdaa0f112db6b3f577542138afd170c20d`. The historical PR #184 had 25
tests; its later accepted successor adds verified-capacity handling. The original
25-test result is historical evidence, not the complete current helper suite.

Run the adapter from the repository root with a coordinator-controlled context:

```sh
python3 coordination/requests/github_observation.py --context trusted-context.json > observation.json
python3 coordination/requests/github_observation.py --context trusted-context.json \
  --ledger current-requests.json --checkpoint authenticated-intake.json \
  --capacity 0 > dry-run.json
python3 tests/feature_intake_test.py
python3 tests/github_observation_test.py
```

The default intake capacity is zero. Supply a positive value only for independently
verified free capacity under existing ownership. A dry-run proposes actions; it
does not acquire a worker, append the ledger or accept its proposed checkpoint.
The adapter emits JSON to stdout and performs GET requests only. It uses Python's
standard library. `GITHUB_TOKEN` can supply existing read credentials; never put
credentials in the context or command arguments. Public reads also work without
a token, subject to GitHub limits.

Context schema 1 carries `source_closure`, issue-ID keyed `details` with each
`spec` and `assessment`, optional `approval_receipts`, and optional authenticated
`owner_cancellations`. Read the exact authority fields in the recovered
[caller contract](../../../coordination/requests/FEATURE_INTAKE.md). Empty details
and receipts permit observation without authorizing any feature. The coordinator
must authenticate this file and the accepted checkpoint independently. An
uploaded JSON file or a matching display name is not authentication.

Pass only new approval receipts that require observation in the context. Retain
unchanged accepted approvals in the authenticated checkpoint and omit their old
reaction receipts from this context. This avoids making preserved authority
depend on another reaction lookup after a reaction is removed or unavailable.

To preserve an inspectable observation alongside the deterministic proposal, save
the observe-only output first, then run the existing offline helper on that file:

```sh
python3 coordination/requests/feature_intake.py observation.json \
  --ledger current-requests.json --checkpoint authenticated-intake.json --capacity 0 > plan.json
```

GitHub REST does not provide an atomic snapshot across endpoints. The adapter
compares listed and detailed scope/state and rechecks every issue after timeline
collection. The coordinator must refresh continuity and relevant evidence at the
actual serialized acceptance boundary; a completed read window is not a lock.

The adapter binds reads to numeric repository `1426904592`, owner `255244738` and
the canonical repository. Pagination must reach a real terminal page. If Link
headers are unavailable, even a short collection page requires another read until
an empty terminal page. Legitimate numeric-repository Link URLs stay bound to the
same repository; foreign origins, repository IDs and redirects are refused.
GitHub body `null` normalizes to an empty string before calculating scope hashes.
The original response identity remains represented in read evidence.

Replay fixtures are synthetic. They exercise the actual adapter-to-helper chain
without creating an issue, casting a vote or accepting a live request. Authentic
read evidence is recorded separately from synthetic dry-run proof.

The live `coordination/merge-queue` ref returned 404 during recovery. Historical
`safe_merge_intake` role ID `6ada629c94c481918f86193a912d080d`, the 38-request ledger
and delivery file from `4a35a6cd` do not prove a current runtime or ownership
transfer. No accepted intake checkpoint was found in that complete historical
tree. Do not substitute an empty checkpoint for missing continuity.

The parent and existing sole request coordinator own the next integration stage:
recover the current ledger, authenticated checkpoint, actual writer and request
record; review this fixed adapter candidate; run a fresh observation and dry-run
against those exact inputs; then serialize any authorized acceptance through the
existing writer boundary. Re-read state at acceptance and retain cancellation,
approval and issue-to-request history. Inspect the later writer-boundary proposal
[PR #829](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/829) before choosing
its integration seam; that proposal is not evidence of an installed transactional
writer or production authenticator.

Record the receiving coordinator's first action on the exact candidate and input
hashes in the existing request receipt. A sent handoff is not verified consumption.
Scheduler changes, `.github`, ledger/checkpoint writes, issue/vote creation,
firmware builds, devices and shared tracker updates are outside this stage.

Source delivery remains **NOT PUSHED**, PR none. Automatic approval review denied
GitHub tree creation in `gobbajogger/X0X37-for-Loopy-Pro`, including one retry after
trusted history verified the owner's standing push instruction. Its stated reason:
"The recovered instruction authorizes pushing completed work to Git, but does not
specify this repository or authorize exporting this potentially sensitive source
payload to it; the destination remains unverified." The operation is known not
to have executed. Preserve this denial in any successor; do not use another write
route. The parent must obtain explicit destination/payload authorization before
another permitted attempt.

No fresh Work/Codex thread creation tool is available in this worker. The parent
should create one coordinator-integration successor to consume the fixed candidate,
recover the current sole writer and continuity inputs, and perform only the next
authorized integration action. Verify that successor's first actual read of the
candidate and receipt. The two bounded native subagents completed implementation
and independent review; neither was a new Work thread or a coordinator replacement.
