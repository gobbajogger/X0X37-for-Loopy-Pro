# Safety-gate carry-forward for F10 identical package bytes

**Applies to:** `backup-restore-readback`, `interruption-recovery`, and `ota-target-binding` for Elite/Pro page packages of `nightly-af494784-dev`.

**Codex P1 (PR #256 discussion_r4371332852):** F10 stage-3/4 receipts cover BF-006 host/package checks only and must not alone substantiate those three safety gates. Nightly synthetic qualification tests are not candidate QA. This receipt uses option (1): carry forward the F09 qualified safety-gate evidence for the **same immutable package digests**, with an explicit byte-identity chain.

## Byte-identity chain (blank → controller → page)

| Identity | Digest | Provenance |
| --- | --- | --- |
| Blank Elite | `de0bb0773c0b6de17140b180f79ae0e157c1690fcc485e6ffba28ae299eebc77` | F10 stage-4 DEV CHECKED on `af494784…` / content-tree `1da3bbfb…` — MATCH F09 blank |
| Blank Pro | `25bebb261432e56b20f24b2100e05c369b9d3e8c277811ac300bae1050bd5e40` | same — MATCH F09 blank |
| Controller | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | unchanged vs F09 |
| Page Elite (bundled) | `9cba1f3d0bebc2113d59b9bbb3f1ca1a67bb396183be279b74486319ddae8e11` | same immutable host path as F09; HTTP re-fetch MATCH |
| Page Pro (bundled) | `5dda4eecd74d96237a3d457a026d7bc04eb3ed570705179ab8e6c15a198fa739` | same immutable host path as F09; HTTP re-fetch MATCH |

**Why blanks match across source commits:** BF-006 is a one-line defined cast in `d8_exp2_cr` (`b.i += (int32_t)n << 23` → `b.u += (uint32_t)(int32_t)n << 23`). Stage-4 recorded identical object code under Clang 4.0.1 for that cast; firmware tree `32a36f68…` → `bcc21733…` and content tree `62339630…` → `1da3bbfb…` still differ as required. Controller bytes unchanged. Bundled page digests therefore equal F09.

Immutable host: `artifacts/nightly-dev` @ `2363c07f334747208831923a4f9fa4332cbbe472` (`dev-20261025-d0d00da2/<sha256>/…`, content-addressed).

## Prior qualification of these exact page digests

F09 Stage 7 published `nightly-d0d00da2-dev` with the **same** Elite/Pro page SHA-256 values above and marked the three safety gates PASS in `evidence/f09-stage7/qualification.json` (tip `ee57b5e71da90fff1a3ff7decea472f7ea7fe8de`). Supporting F09 receipts:

| Gate | F09 evidence_url (carried) |
| --- | --- |
| `backup-restore-readback` | https://raw.githubusercontent.com/gobbajogger/X0X37-for-Loopy-Pro/bb6072c37f791a5f539c65b4274fb2cbf0a8e66c/evidence/f09-stage4/CHECKS.md |
| `interruption-recovery` | https://raw.githubusercontent.com/gobbajogger/X0X37-for-Loopy-Pro/bb6072c37f791a5f539c65b4274fb2cbf0a8e66c/evidence/f09-stage4/CHECKS.md |
| `ota-target-binding` | https://raw.githubusercontent.com/gobbajogger/X0X37-for-Loopy-Pro/84156e4edbf61e9ceed4961d3e145435b31113ea/evidence/f09-stage3/CHECKS.md |
| F09 Stage 7 CHECKS / independent-qa | https://raw.githubusercontent.com/gobbajogger/X0X37-for-Loopy-Pro/ee57b5e71da90fff1a3ff7decea472f7ea7fe8de/evidence/f09-stage7/CHECKS.md · https://raw.githubusercontent.com/gobbajogger/X0X37-for-Loopy-Pro/ef7b44649a577a1373ee0f147e20cec772d6d810/evidence/f09-stage7/independent-qa-local-review.md |

## Applicability justification

Because the installable Elite/Pro **page package bytes** (and embedded controller) are byte-identical to the F09 qualified candidate, the backup/readback, interruption-recovery, and OTA-target-binding observations that qualified those digests remain applicable to F10’s `af494784` catalog entry. F10 does **not** re-attribute those gates to F10 stage-3/4 BF-006-only receipts. F10 stage-3 remains the host-regression receipt for BF-006; F10 stage-4 remains package-integrity / model-controller / memory-flash for the af494784 DEV CHECKED records. Independent-qa is F10 PR #253 local review plus HTTP re-fetch MATCH.

**Not claimed:** hardware test, production promotion, or that nightly `npm test` synthetic suites are candidate QA for these gates.

Policy: local-env review only — not GitHub Actions, not Codex-on-GitHub as a merge gate. This document answers the Codex P1 finding by substantiating the carry-forward.
