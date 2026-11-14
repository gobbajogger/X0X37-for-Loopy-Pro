# ROADMAP #120 observation adapter qualification

This stage supplies a runnable read-only GitHub observation adapter and replay
qualification to the existing request coordinator. The coordinator retains ledger,
checkpoint, claim acceptance and scheduler writes. This is an isolated source and
qualification stage, not operation of the automatic suggestion inbox.

Inputs are ROADMAP at `12e72b586c31e7ed412b6f4315bdf408c6f712b0`, feature registry
at `94197fc839b00aff47b98ca02f3c6396e4a6eabf`, historical PR #184 at
`efa57425600a23d88efde6152c977542883da4a7`, and its later merged coordination
successor `4a35a6cdaa0f112db6b3f577542138afd170c20d`. The latter provides the
unchanged helper, contract and 26 tests, including the later capacity handling.
The branch starts at verified development `nightly` commit
`66073c68dd80d51a7053432c2c161a8b720a1341`.

1. Recover current intake source, ledger/checkpoint and coordinator before writes.
   Both supported GitHub GET and native GET return 404 for the live
   `coordination/merge-queue` ref. The last verified merged coordination tree
   contains 38 requests and no accepted intake checkpoint. Current nightly,
   supplied ROADMAP tree and checkout contain no intake adapter/helper. Historical
   role `safe_merge_intake` (`6ada629c94c481918f86193a912d080d`) retains the single
   admission boundary; its current runtime and actual writer are unverified.
2. Preserve exact recovered helper/contract/test blobs. Delegate only
   `coordination/requests/github_observation.py` and its new replay tests to the
   implementation agent. This thread owns recovery, docs and evidence. No other
   source writer has these paths in this stage.
3. Collect repository identity, all open/closed issue pages, fresh issue details,
   complete timelines and required opening reactions with GET only. Fail unknown
   on incomplete reads, identity changes, conflicting duplicates and unsafe URLs.
   Keep trusted scope receipts, source assessment and explicit cancellation context
   separate from GitHub user content. Never infer human provenance or approval.
4. Prove pagination, identity, deduplication, cancellation and exact-scope approval
   behaviour with replay and actual helper integration. Reuse the unchanged 26-test
   successor suite. Run an authentic read and a dry-run without accepting state.
   Mark operational dry-run pending if the current coordinator inputs are absent;
   do not invent an empty accepted checkpoint.
5. Hold the candidate source fixed for independent adversarial review. Resolve
   reproduced technical findings and rerun only invalidated checks. Deliver exact
   source hashes, commands/results, replay/live-read evidence and a coordinator
   integration handoff.

Done means the adapter is runnable, scoped proof and independent review exist,
and the parent can integrate the fixed candidate without duplicating a writer.
Firmware builds, devices, owner votes, issues, `.github`, shared trackers,
scheduler wiring and coordinator state writes are outside this stage.
