# Incremental checkpoint procedure

The [contract](SKILL.md) governs. `tools/workflow_checkpoint.py` is a small local
JSON decision aid, not a scheduler, trust verifier, lock service or gate waiver.
No existing workflow checkpoint runner was found; install-ledger and historical
build checkpoints retain their own meaning. A maintained ledger may carry this
record without introducing a second framework.

## Cheap observation and exact identity

Read one prior accepted checkpoint and one fresh observation, using exact scoped
ref/status queries and known artifact/QA/deploy/hardware/retry ledger entries.
Do not fetch full PR histories or reread authenticated immutable proof merely to
rediscover unchanged work. Fetch details only for a changed relevant observation.
Adapters must enumerate every required observation. For scheduler progress, read
actual run identity/status/updated timestamp separately from any older
`latest_result` summary; hash relevant run state into `input_identity`. A stale
summary cannot establish a fresh unchanged result. Mark `read_status` failed
if any read failed, is stale, incomplete or ambiguous. They must derive complete
closures and authenticate stored bytes/evidence before recording ACCEPTED; this
helper trusts that validated input and cannot discover missing dependencies.

Keep observation timestamps in run receipts, outside the stable identity record.
Both records use this shape (all displayed hashes stand for actual measured values):

```json
{
  "schema": 1,
  "role": "builder",
  "workflow_revision": "<contract Git revision + file SHA-256>",
  "input_identity": {"source_tree_and_task": "<sha256>"},
  "last_accepted_stage": "BUILD_ONLY",
  "evidence_identity": {"accepted_ledger": "<sha256>"},
  "observed_remote_heads": {"nightly": "<exact Git SHA>"},
  "read_status": "success",
  "pending_work": [],
  "retry_conditions": [],
  "outputs": {
    "controller": {
      "closure": {"complete_controller_inputs": "<sha256>"},
      "policy": {"controller_build_acceptance": "<sha256>"},
      "sha256": "<actual output sha256>",
      "accepted_evidence": {
        "status": "ACCEPTED", "identity": {"stored_receipt": "<sha256>"}
      }
    }
  }
}
```

Closure manifests cover source/shared ABI, linker, generators/build/package
scripts and modes, configuration, SDK/toolchain identity and relevant flags.
Represent app, controller, package and checks as separate named outputs when
reusing them separately. Include dependency output hashes in each downstream
closure. A policy digest covers only that output's applicable acceptance rules;
a changed packaging rule invalidates packaging and dependents, not unchanged
controller compilation. Any contract revision change still requires a work
assessment; only scoped policies decide output invalidation. Evidence identity
must refer to authentic accepted output proof, including coverage and environment.
Never label unseen or changed bytes accepted to obtain reuse.

`pending_work` includes required review, missing artifact, deploy, hardware,
protection or policy assessment, even when source SHA matches. `retry_conditions`
records actionable retry state or its externally observed trigger. A run need
not perform pending work without authority; it must report the exact next action
or blocker. Failed reads and missing/malformed checkpoints produce WORK_REQUIRED,
never a no-change success. A removed/added output, changed proof, role, stage,
head or workflow revision also prevents a no-change exit.
When scheduler tasks or roles change, record the authorized old-to-new identity,
scope and owner migration. Reassess applicable acceptance before saving a new
checkpoint; do not relabel a new role as the old accepted one. Reuse unchanged
per-output proof only under the complete-closure and policy rules above.

```sh
python3 tools/workflow_checkpoint.py observation.json --checkpoint accepted.json
# 0: NO_CHANGE; 10: WORK_REQUIRED (inspect reason/reuse/affected).
# Optional --receipt /private/run.json writes a receipt, never accepted.json.
# Input aliases (including existing symlink/hardlink aliases) fail with exit 2,
# an argparse error on stderr and no stdout receipt or write.
python3 tests/workflow_checkpoint_test.py
```

`reuse` means identical complete output record, subject to its authentic evidence;
it does not certify current model pairing, protected merges or hardware. Recompose
and check current packages when affected app/package inputs change. A decision
never advances acceptance or overwrites the checkpoint. Only the stage owner may
save a new accepted record after applicable independent validation and gates.
Use guarded/atomic writes or actual supported leases where available; otherwise
serialize the known owner. Do not invent an account writer scan or lease API.

## Receipt and measurement

Append a compact local receipt to the existing evidence channel, or retain the
optional JSON file; no-change logging requires no Git commit or repeated PR comment.
The helper records actual UTC start/end, input and checkpoint before/after,
action/outcome, retry count, produced verified outputs (empty for a decision),
stage elapsed seconds, owner-installable elapsed time and tokens. Decision runs
leave checkpoint before/after identical. Later stages append their actual outputs
and first verified owner-installable timestamp; do not relabel a plan BUILD_ONLY
or a private artifact as owner-installable firmware. Start that elapsed clock at
the first admitted source task and record exclusions/coverage for comparisons.
Tokens and owner-installable time are null/unavailable when actual telemetry or
the corresponding stage is absent; never infer tokens from words or output size.

Host tests execute a bounded fixture: a broad baseline reads two decision files
plus 20 immutable proof files, while the unchanged decision reads only its two
inputs. Report these measured file reads as a fixture I/O proxy, not production
tokens, live query savings, throughput or NIGHTLY latency. Production comparisons
need equivalent tasks, real exposed usage, retries, accepted work and timing.
