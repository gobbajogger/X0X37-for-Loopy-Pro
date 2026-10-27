# Scoped local proof

Use this matrix to select meaningful checks for the requested outcome under
[AGENTS.md](../../AGENTS.md) and the [active workflow](workflow.md). Independent
review, current permissions, required remote checks and ownership remain separate.
GitHub Actions and billing status are not substitutes for agent-run test results.

| Scope | Applicable proof |
| --- | --- |
| Documentation or skill rules | Exact diff, syntax and reachable links; semantic/independent review when authority or delivery behavior changes. No firmware rebuild. |
| Workflow or validation helper | Relevant complete helper test closure, changed positive/negative behavior and affected callers. No firmware build unless consumed compiler inputs change. |
| Firmware implementation | Covering host regressions and meaningful negative/boundary tests; affected target native validation when required by the delivery stage. |
| Evidence-only change | Authenticate stated source/inputs, exits, records and stored byte hashes; reuse unchanged valid proof. |
| Development package | Actual affected Elite/Pro development validation, matching controller, model/config, CRC/SHA-256 and memory/flash/pool budgets. |
| Integration | Fresh exact source/base, independent review and applicable combined-tree checks, normal authorized update and remote readback. |
| Publication | Applicable qualification, preserved catalog/history, authorized publisher and served page/catalog/package byte verification. |
| Device/production | Separately authorized same-byte, matching-model owner hardware result and applicable production approval. |

`tools/local_proof_plan.py` may suggest selectors from the actual diff. Verify its
output against the requested behavior and current scripts; a planner is not proof.
Historical numbered stage or successor validators check old records only. They do
not require a new thread or block current delivery because no successor ID exists.

Rebuild only when consumed source/configuration/tool/dependency inputs changed or
required stored artifacts/proof cannot be verified. A new commit ID alone does
not invalidate identical input evidence. Missing, failed or NOT_RUN required checks
stay negative/pending; optional gaps do not stop independent ready work.

Preserve applicable DEV CHECKED records, package/controller hashes, model guards,
flash/static-RAM limits, `pool_free >= 8192`, data-preserving installer negatives,
independent QA and actual live byte verification. A source-only task needs none of
the unrelated future hardware/publication proof. No local pass overrides an actual
denial, required review or branch protection. Report, then continue authorized work.
