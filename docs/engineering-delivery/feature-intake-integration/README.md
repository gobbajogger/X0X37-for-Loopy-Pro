# ROADMAP #120 historical reconciliation qualification

The missing continuity input is now an executable refusal and a tested historical replay. Current operational integration remains pending: no current sole writer, authenticated current ledger or accepted intake checkpoint was recovered. Nothing in this package authenticates supplied JSON or accepts coordinator state.

The consumed adapter commit is `7f27a97bed3d123a5e0b0bb02e9a5a9250737626`, on `chat/feature-intake-observer-20261114`. All eight producer files remain unchanged. Their earlier NOT PUSHED/denial text is historical: the specifically approved export subsequently succeeded at that commit. That export does not grant authority for unrelated actions.

The live `coordination/merge-queue` ref returned 404. Complete, untruncated trees for current `nightly` (`66073c68dd80d51a7053432c2c161a8b720a1341`) and `main` contain no coordinator ledger, intake checkpoint or writer-boundary implementation. [PR #829](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/829) is closed unmerged and its closing comment points to [#864](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/864). The latter is a merged firmware release/version PR; its source provides no coordinator adoption receipt. That closure must not be interpreted as installed transactional authentication.

Targeted current Git branch/PR searches, visible project-history receipts, the existing cloud handoff audit, local project filenames and privately visible task definitions disclosed no other owner for this integration qualification. A full live thread roster is unavailable here. The historical `safe_merge_intake` task is not accessible in this context; that does not establish inactivity, deletion or an ownership transfer. The parent and existing sole writer retain coordination ownership. No replacement writer is appointed.

The harness feeds the actual unchanged adapter into the actual unchanged helper. It uses the exact 38-request historical ledger at `4a35a6cdaa0f112db6b3f577542138afd170c20d` (SHA256 `ea7bae9470f4197d1d907975b877dfcb28005ddd5968b8d163248334647afdca`) and explicitly synthetic issue, authority and checkpoint fixtures. It does not save the ledger or a checkpoint. The additional tombstone case modifies only an isolated in-memory copy.

From the repository root, recover the historical input via an authorized GitHub GET and run:

```sh
curl -fsS https://raw.githubusercontent.com/gobbajogger/X0X37-for-Loopy-Pro/4a35a6cdaa0f112db6b3f577542138afd170c20d/coordination/requests/requests.json -o /tmp/intake-historical-requests.json
python3 docs/engineering-delivery/feature-intake-integration/reconcile.py --historical-replay --ledger /tmp/intake-historical-requests.json
python3 docs/engineering-delivery/feature-intake-integration/reconcile_test.py /tmp/intake-historical-requests.json
```

The historical CLI requires exact original ledger bytes and returns exit 2 for any byte change. Two new tests pass, with ten behaviour checks: zero claims at capacity zero; deterministic `LX-REQ-039`; retention of the three historical requests for synthetic issue #2; no reenqueuing of existing work; cancellation retention; refusal to reuse authority after reopening; explicit scope-bound reauthorization; tombstone reservation; refusal of a missing tracked issue; and unchanged inputs. The synthetic candidate IDs are not real claims or accepted requests. See [historical-replay.json](historical-replay.json).

Absent checkpoints return exit 2 without a proposed checkpoint; see [missing-continuity.json](missing-continuity.json). The comparison interface also supports an existing complete packet without writing it:

```sh
python3 docs/engineering-delivery/feature-intake-integration/reconcile.py --ledger current-requests.json --observation fresh-observation.json --checkpoint authenticated-intake.json
```

That interface always uses capacity zero, compares repeated results and checks that input file bytes remain unchanged. Its output explicitly keeps current acceptance false: the caller must independently authenticate provenance. An input filename, a matching digest or a `true` flag cannot establish authority.

Independent read-only review passed the fixed new Python/plan hashes and separately ran the two tests, the ten replay assertions and three CLI negatives. The unchanged producer evidence is reused: 28 adapter tests, 26 helper tests, eight extra adversarial probes and 34 authentic GETs. These suites were not repeated. The new stage performed read-only Git/receipt recovery; no fresh issue observation or current operational dry-run was run with invented continuity.

The exact remaining prerequisite belongs to the existing coordinator: supply its current sole-writer assignment and supported result route, authenticated complete ledger with revision/hash and retained mappings/history/tombstones, accepted intake checkpoint with revision/hash and approval/cancellation provenance, plus the current source-backed context. Missing continuity needs authenticated reconciliation by that owner. It cannot be replaced with `{ "schema": 1 }`.

Once recovered, collect a fresh GET-only adapter observation, retain its exact hash and read window, run the capacity-zero comparison, and re-read continuity at any eventual serialized acceptance boundary. The existing writer alone performs acceptance. This stage changes no scheduler, shared ledger/checkpoint, request, vote, `.github` file, firmware, protected ref or publication state.

No standard top-level thread creation or thread reader is exposed in this worker's tool inventory. The installed supervisor deployment pointer does not prove liveness. A final exact packet returns to the authorized creator; no successor admission or receiver consumption is claimed. Avoid another historical replay stage without new continuity evidence.
