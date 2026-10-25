# Safety-gate carry-forward for F12 identical package bytes

**Applies to:** `backup-restore-readback`, `interruption-recovery`, and `ota-target-binding` for Elite/Pro page packages of `nightly-de11b6d7-dev`.

F12 stage-3/4 receipts cover BF-008 host/package checks only and must not alone substantiate those three safety gates. Nightly synthetic qualification tests are not candidate QA. This receipt uses option (1): carry forward the F11 qualified safety-gate evidence for the **same immutable package digests**, with an explicit byte-identity chain.

## Byte-identity chain (blank → controller → page)

| Identity | Digest | Provenance |
| --- | --- | --- |
| Blank Elite | `829bb6da6eb0042af0731a236d6238e50cc13581ce059ebbe02882534a94b056` | F12 stage-4 DEV CHECKED on `de11b6d7…` / content-tree `43e97f34…` — MATCH F11 blank |
| Blank Pro | `d2186cb0f3fe74376651cb2804973c8255780532a8cd54c33d2bac09aeda60ec` | same — MATCH F11 blank |
| Controller | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | unchanged vs F11 |
| Page Elite (bundled) | `4841110a274e2212d56abf9234cf0d921b45d915d1ff25fad0727e52edb7d18a` | same immutable host path as F11; HTTP re-fetch MATCH |
| Page Pro (bundled) | `b35d587557d9f1d4c72b2c81bc944a1dd129bee625e98ae35f06fe7f653fc720` | same immutable host path as F11; HTTP re-fetch MATCH |

**Why blanks match across source commits:** BF-008 is a one-line defined unsigned cast in `fm_exp2_nf` (`b.u += (uint32_t)n << 23`). Stage-4 recorded identical object code under Clang 4.0.1 for that cast; firmware tree `5b1a21f0…` → `9195dce5…` and content tree `f27c89e1…` → `43e97f34…` still differ as required. Controller bytes unchanged. Bundled page digests therefore equal F11.

Immutable host: `artifacts/nightly-dev` @ `02f4f4f48408db96e01b143d5410b7fb00a5a70a` (`dev-20261025-4f96b0d9/<sha256>/…`, content-addressed). Stage-7 stripped blank packages from those hosted bundled bytes via `tools/add_ctl.py --remove` and confirmed blank SHA-256 MATCH stage-4 records (`http-refetch.log`).

## Prior qualification of these exact page digests

F11 Stage 7 published `nightly-4f96b0d9-dev` with the **same** Elite/Pro page SHA-256 values above and marked the three safety gates PASS in `evidence/f11-stage7/safety-gates-note.md` / `safety-gates-candidate.log` (tip `d9276cc7260d200e27bebd41b028d357fb15ceba`). Supporting F11 receipts:

| Gate | F11 evidence_url (carried) |
| --- | --- |
| `backup-restore-readback` | https://raw.githubusercontent.com/gobbajogger/X0X37-for-Loopy-Pro/d9276cc7260d200e27bebd41b028d357fb15ceba/evidence/f11-stage7/safety-gates-note.md |
| `interruption-recovery` | https://raw.githubusercontent.com/gobbajogger/X0X37-for-Loopy-Pro/d9276cc7260d200e27bebd41b028d357fb15ceba/evidence/f11-stage7/safety-gates-note.md |
| `ota-target-binding` | https://raw.githubusercontent.com/gobbajogger/X0X37-for-Loopy-Pro/d9276cc7260d200e27bebd41b028d357fb15ceba/evidence/f11-stage7/safety-gates-note.md |
| F11 Stage 7 CHECKS / independent-qa | https://raw.githubusercontent.com/gobbajogger/X0X37-for-Loopy-Pro/d9276cc7260d200e27bebd41b028d357fb15ceba/evidence/f11-stage7/CHECKS.md · https://raw.githubusercontent.com/gobbajogger/X0X37-for-Loopy-Pro/d9276cc7260d200e27bebd41b028d357fb15ceba/evidence/f11-stage7/independent-qa-local-review.md |

## Applicability justification

Because the installable Elite/Pro **page package bytes** (and embedded controller) are byte-identical to the F11 qualified candidate, the backup/readback, interruption-recovery, and OTA-target-binding observations that qualified those digests remain applicable to F12’s `de11b6d7` catalog entry. F12 does **not** re-attribute those gates to F12 stage-3/4 BF-008-only receipts. F12 stage-3 remains the host-regression receipt for BF-008; F12 stage-4 remains package-integrity / model-controller / memory-flash for the de11b6d7 DEV CHECKED records. Independent-qa is F12 PR #272 local review plus HTTP re-fetch MATCH.

**Not claimed:** hardware test, production promotion, or that nightly `npm test` synthetic suites are candidate QA for these gates.

Policy: local-env review only — not GitHub Actions, not Codex-on-GitHub as a merge gate.
