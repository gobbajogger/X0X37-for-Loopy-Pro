# Scoped local proof

Select checks for the requested outcome. GitHub Actions is not a substitute for agent-run results. Missing/failed/NOT_RUN required checks stay negative; optional gaps do not stop independent work. See [RULES.md](../../RULES.md).

| Scope | Applicable proof |
| --- | --- |
| Documentation or skill rules | Diff, links; independent review when authority/delivery behavior changes. No firmware rebuild. |
| Workflow or validation helper | Helper tests and affected callers. No firmware build unless compiler inputs change. |
| Firmware implementation | Host regressions and meaningful negatives; native validation when the delivery stage requires it. |
| Evidence-only change | Authenticate stated inputs/exits/hashes; reuse unchanged valid proof. |
| Development package | Affected Elite/Pro validation, matching controller/model, CRC/SHA-256, memory/flash/pool budgets. |
| Integration | Fresh source/base, independent review, combined-tree checks, remote readback. |
| Publication | Qualification, preserved catalog/history, authorized publisher, served-byte verification. |
| Device/production | Separately authorized same-byte matching-model owner hardware result and production approval. |

A source-only task does not acquire unrelated future hardware/publication proof. No local pass overrides a denial, required review, or branch protection.
